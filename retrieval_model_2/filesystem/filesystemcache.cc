#include "filesystem.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include "../misc/alloc.h"

#define FILESYSTEMCACHE_HASH_SIZE 1847

// #define FILESYSTEMCACHE_DEBUG

FileSystemCache::FileSystemCache(FileSystem* fs, int pageSize, int pageCount) {
  this->fileSystem = fs;
  this->pageSize = pageSize;
  this->cacheSize = pageCount;

  workMode = FILESYSTEMCACHE_LRU;
  currentPageCount = 0;

  firstSlot = (FileSystemCacheSlot *)malloc(sizeof(FileSystemCacheSlot));
  lastSlot = (FileSystemCacheSlot *)malloc(sizeof(FileSystemCacheSlot));
  firstSlot->pageNumber = -1;
  firstSlot->data = nullptr;
  firstSlot->prev = nullptr;
  firstSlot->next = lastSlot;
  lastSlot->pageNumber = -2;
  lastSlot->data = nullptr;
  lastSlot->prev = firstSlot;
  lastSlot->next = nullptr;

  whereIsPage = (FileSystemCacheHashElement **)malloc(FILESYSTEMCACHE_HASH_SIZE * sizeof(FileSystemCacheHashElement*));
  for (int i = 0; i < FILESYSTEMCACHE_HASH_SIZE; i++)
    whereIsPage[i] = nullptr;
  readWriteBuffer = (byte *)malloc(pageSize);
}

FileSystemCache::~FileSystemCache() {
  // flush cache first
  flush();

  // pageNumber hashTableによる占有されたメモリを開放する
  for (int i = 0; i < FILESYSTEMCACHE_HASH_SIZE; i++) {
    FileSystemCacheHashElement *hashRunner = whereIsPage[i];
    while (hashRunner != nullptr) {
      FileSystemCacheHashElement *next = (FileSystemCacheHashElement *)hashRunner->next;
      free(hashRunner);
      hashRunner = next;
    }
  }
  free(whereIsPage);

  // キャッシュデータを占有したメモリを開放する
  FileSystemCacheSlot *slotRunner = firstSlot;
  while (slotRunner != nullptr) {
    FileSystemCacheSlot *next = (FileSystemCacheSlot*)slotRunner->next;
    if (slotRunner->data != nullptr)
      free(slotRunner->data);
    free(slotRunner);
    slotRunner = next;
  }

  free(readWriteBuffer);
}

FileSystemCacheSlot* FileSystemCache::findPage(int pageNumber) {
  int hashValue = pageNumber % FILESYSTEMCACHE_HASH_SIZE;
  FileSystemCacheHashElement *hashElement = whereIsPage[hashValue];
  while (hashElement != nullptr) {
    if (hashElement->data->pageNumber == pageNumber)
      return hashElement->data;
    hashElement = (FileSystemCacheHashElement*)hashElement->next;
  }
  return nullptr;
}

void FileSystemCache::printCacheQueue() {
  FileSystemCacheSlot *slot = firstSlot;
  fprintf(stderr, "[Forward:");
  while (slot != nullptr) {
    fprintf(stderr, " %i", slot->pageNumber);
    slot = (FileSystemCacheSlot *)slot->next;
  }
  fprintf(stderr, "] ");
  slot = lastSlot;
  fprintf(stderr, "[Backward:]");
  while (slot != nullptr) {
    fprintf(stderr, " %i", slot->pageNumber);
    slot = (FileSystemCacheSlot*)slot->prev;
  }
  fprintf(stderr, "]\n");
}

bool FileSystemCache::isInCache(int pageNumber) {
  FileSystemCacheSlot *slot = findPage(pageNumber);
#ifdef FILESYSTEMCACHE_DEBUG
  fprintf(stderr, "FileSystemCache::isInCache(%i) returns %s.\n", pageNumber, (slot == nullptr ? "false" : "true"));
#endif
  if (slot == nullptr)
    return false;
  else
    return true;
}

void FileSystemCache::setWorkMode(int newWorkMode) {
  if ((newWorkMode == FILESYSTEMCACHE_LRU) || (newWorkMode == FILESYSTEMCACHE_FIFO))
    workMode = newWorkMode;
}

void FileSystemCache::touchSlot(FileSystemCacheSlot *slot) {
  // 古い位置からスロットを開放する
  ((FileSystemCacheSlot *)slot->prev)->next = slot->next;
  ((FileSystemCacheSlot *)slot->next)->prev = slot->prev;

#ifdef FILESYSTEMCACHE_DEBUG
  printCacheQueue();
#endif
  
  // 最初の位置にスロットを挿入する
  slot->next = firstSlot->next;
  ((FileSystemCacheSlot *)slot->next)->prev = slot;
  slot->prev = firstSlot;
  firstSlot->next = slot;
}

int FileSystemCache::getPage(int pageNumber, void *buffer) {
#ifdef FILESYSTEMCACHE_DEBUG
  fprintf(stderr, "FileSystemCache:getPage(%i, ...) called.\n", pageNumber);
#endif
  FileSystemCacheSlot *slot = findPage(pageNumber);
  if (slot == nullptr)
    return FILESYSTEM_ERROR;
  memcpy(buffer, slot->data, pageSize);
  if (workMode == FILESYSTEMCACHE_LRU)
    touchSlot(slot);
#ifdef FILESYSTEMCACHE_DEBUG
  printCacheQueue();
#endif
  return FILESYSTEM_SUCCESS;
}

void FileSystemCache::removeHashEntry(int pageNumber) {
  int hashValue = pageNumber % FILESYSTEMCACHE_HASH_SIZE;
  FileSystemCacheHashElement *hashElement = whereIsPage[hashValue];
  if (hashElement == nullptr)
    return;

  if (hashElement->data->pageNumber == pageNumber) {
    whereIsPage[hashValue] = (FileSystemCacheHashElement*)hashElement->next;
    free(hashElement);
    return;
  }
  while (hashElement->next != nullptr) {
    FileSystemCacheHashElement *next = (FileSystemCacheHashElement *)hashElement->next;
    if (next->data->pageNumber == pageNumber) {
      hashElement->next = next->next;
      free(next);
      return;
    }
    hashElement = next;
  }
}

void FileSystemCache::evict(FileSystemCacheSlot *toEvict) {
  int pageNumber = toEvict->pageNumber;
#ifdef FILESYSTEMCACHE_DEBUG
  fprintf(stderr, "Evicting page %i from cache.\n", pageNumber);
  if (toEvict->hasBeenChanged)
    fprintf(stderr, " Page has been modified. Writing content to disk.\n");
#endif
  if (toEvict->hasBeenChanged)
    fileSystem->writePage_UNCACHED(pageNumber, 0, pageSize, toEvict->data);
  free(toEvict->data);
  ((FileSystemCacheSlot*)toEvict->prev)->next = toEvict->next;
  ((FileSystemCacheSlot*)toEvict->next)->prev = toEvict->prev;
  removeHashEntry(pageNumber);
  free(toEvict);
  currentPageCount--;
}

FileSystemCacheSlot *FileSystemCache::loadPage(int pageNumber, void *buffer, bool copyData) {
#ifdef FILESYSTEMCACHE_DEBUG
  fprintf(stderr, "FilesystemCache::loadPage(%i, ...) called.\n", pageNumber);
#endif
  
  // ページが現在キャッシュにある場合の特別な処理
  FileSystemCacheSlot *slot = findPage(pageNumber);
  if (slot != nullptr) {
    if (copyData) {
      memcpy(slot->data, buffer, pageSize);
    } else {
      free(slot->data);
      slot->data = (char *)buffer;
    }
    touchSlot(slot);
    return slot;
  }

  // キャッシュが満杯ならページを削除
  while (currentPageCount >= cacheSize) {
    FileSystemCacheSlot *toEvict = (FileSystemCacheSlot *)lastSlot->prev;
    bool hasBeenChanged = toEvict->pageNumber;
    evict(toEvict);
    if (hasBeenChanged) {
      for (int i = 1; i <= 3; i++) {
        toEvict = findPage(pageNumber + i);
        if (toEvict == nullptr)
          break;
        if (!toEvict->hasBeenChanged)
          break;
        evict(toEvict);
      }
    }
  }

  // キャッシュ内に新しいページをロードする
  FileSystemCacheSlot *newSlot = (FileSystemCacheSlot *)malloc(sizeof(FileSystemCacheSlot));
  newSlot->pageNumber = pageNumber;
  if (copyData) {
    newSlot->data = (char *)malloc(pageSize);
    memcpy(newSlot->data, buffer, pageSize);
  } else {
    newSlot->data = (char *)buffer;
  }
  newSlot->hasBeenChanged = false;
  int hashValue = pageNumber % FILESYSTEMCACHE_HASH_SIZE;
  FileSystemCacheHashElement *cacheElement = (FileSystemCacheHashElement *)malloc(sizeof(FileSystemCacheHashElement));
  cacheElement->data = newSlot;
  cacheElement->next = whereIsPage[hashValue];
  whereIsPage[hashValue] = cacheElement;

  // 最初の要素として挿入する
  newSlot->next = firstSlot->next;
  ((FileSystemCacheSlot *)newSlot->next)->prev = newSlot;
  newSlot->prev = firstSlot;
  firstSlot->next = newSlot;
  currentPageCount++;
#ifdef FILESYSTEMCACGE_DEBUG
  printCacheQueue();
#endif
  return newSlot;
}

int FileSystemCache::touchPage(int pageNumber) {
#ifdef FILESYSTEMCACHE_DEBUG
  fprintf(stderr, "FilesystemCache::touchPage(%i) called.\n", pageNumber);
  printCacheQueue();
#endif
  FileSystemCacheSlot *slot = findPage(pageNumber);
  if (slot == nullptr)
    return FILESYSTEM_ERROR;
  touchSlot(slot);
#ifdef FILESYSTEMDCACHE_DEBUG
  fprintf(stderr, "After touching:\n");
  printCacheQueue();
#endif
  return FILESYSTEM_SUCCESS;
}

int FileSystemCache::writeTopage(int pageNumber, int offset, int length, void *buffer) {
  if (pageNumber <= 0)
    return fileSystem->writePage_UNCACHED(pageNumber, offset, length, buffer);
  FileSystemCacheSlot *slot = findPage(pageNumber);

  if (slot != nullptr) {
    // ページが見つかった：データをコピーし"modified"フラッグをセットする
    memcpy(&slot->data[offset], buffer, length);
    slot->hasBeenChanged = true;
    touchSlot(slot);
    return FILESYSTEM_SUCCESS;
  }

  // ページが見つからない場合、以下２つのケースがある
  /*
  (a) ページ全体が書かれている
  　　この場合、直近ではページにアクセスする読み込み/下記も見の処理を
  　　行う可能性は低い
  (b) ページの一部のみが書き込まれる。
  　　ページをキャッシュし、ページが操作されるまで待機する
  */
  if (length == pageSize) {
    return fileSystem->writePage_UNCACHED(pageNumber, offset, length, buffer);
  } else {
    if (fileSystem->readPage_UNCACHED(pageNumber, 0, pageSize, readWriteBuffer) == FILESYSTEM_ERROR)
      return FILESYSTEM_ERROR;
    slot = loadPage(pageNumber, readWriteBuffer, false);
    readWriteBuffer = (byte *)malloc(pageSize);
    memcpy(&slot->data[offset], buffer, length);
    slot->hasBeenChanged = true;
    return FILESYSTEM_SUCCESS;
  }
}

int FileSystemCache::readFromPage(int pageNumber, int offset, int length, void *buffer) {
  if (pageNumber <= 0)
    return fileSystem->readPage_UNCACHED(pageNumber, offset, length, buffer);

  FileSystemCacheSlot *slot = findPage(pageNumber);

  if (slot != nullptr) {
    // ページが見つかった：データをコピーし"modified"フラッグをセットする
    memcpy(buffer, &slot->data[offset], length);
    touchSlot(slot);
    return FILESYSTEM_SUCCESS;
  }

  if (fileSystem->readPage_UNCACHED(pageNumber, 0, pageSize, readWriteBuffer) == FILESYSTEM_ERROR)
    return FILESYSTEM_ERROR;
  slot = loadPage(pageNumber, readWriteBuffer, false);
  readWriteBuffer = (byte *)malloc(pageSize);
  memcpy(buffer, &slot->data[offset], length);

  for (int i = 1; i <= 3; i++) {
    if (pageNumber + i < fileSystem->getPageCount()) {
      slot = findPage(pageNumber + i);
      if (slot == nullptr) {
        if (fileSystem->readPage_UNCACHED(pageNumber + i, 0, pageSize, readWriteBuffer) == FILESYSTEM_SUCCESS) {
          loadPage(pageNumber + i, readWriteBuffer, false);
          readWriteBuffer = (byte *)malloc(pageSize);
        }
      }
    }
  }

  return FILESYSTEM_SUCCESS;
}

void FileSystemCache::flush() {
  while (firstSlot->next != lastSlot) {
    FileSystemCacheSlot *toEvict = (FileSystemCacheSlot *)firstSlot->next;
    evict(toEvict);
  }
}