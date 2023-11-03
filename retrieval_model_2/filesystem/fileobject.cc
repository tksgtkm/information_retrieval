#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "filesystem.h"
#include "../misc/backend.h"

// 全ファイルに対して、メモリ内の全ページ番号の配列をセットする
// この値は配列の最小のサイズになる
#define MINIMUM_PAGES_ARRAY_SIZE 16

void FileObject::init(FileSystem *fileSystem, fs_fileno fileHandle, bool create) {
  this->pages = nullptr;
  this->handle = -1;
  assert(fileSystem->isActive());
  this->fileSystem = fileSystem;
  if (create)
    fileHandle = fileSystem->createFile(fileHandle);
  if (fileHandle < 0) {
    fprintf(stderr, "Negative file handle.\n");
    return;
  }

  // ファイルの最初の入手
  firstPage = fileSystem->getFirstPage(fileHandle);
  if (firstPage < 0)
    printAllocations();
  assert(firstPage >= 0);

  handle = fileHandle;
  seekPos = 0;
  pageSize = fileSystem->getPageSize();
  pageCount = fileSystem->getPageCount(fileHandle);
  if (pageCount <= 1)
    allocateCount = 0;
  else if (pageCount <= MINIMUM_PAGES_ARRAY_SIZE / 2)
    allocateCount = MINIMUM_PAGES_ARRAY_SIZE;
  else
    allocateCount = pageCount * 2;
  
  if (allocateCount == 0)
    pages = &firstPage;
  else
    pages = typed_malloc(int32_t, allocateCount);

  // メモリ内の全ページ番号を読みこむ
  int32_t page = firstPage;
  int lc = 0;
  do {
    pages[lc++] = page;
    page = fileSystem->getPageStatus(page);
  } while (page > 0);
  assert(lc == pageCount);
  int32_t lengthOfLastPage = -page;
  size = (pageCount - 1) * pageSize + lengthOfLastPage;
}

FileObject::FileObject(FileSystem *fileSystem) {
  init(fileSystem, -1, true);
}

FileObject::FileObject(FileSystem *fileSystem, fs_fileno fileHandle, bool create) {
  init(fileSystem, fileHandle, create);
}

FileObject::FileObject() {
  pages = nullptr;
}

FileObject::~FileObject() {
  if (pages != &firstPage) {
    if (pages != nullptr)
      free(pages);
  }
  pages = nullptr;
}

void FileObject::deleteFile() {
  if (handle >= 0)
    fileSystem->deleteFile(handle);
  handle = -1;
}

fs_fileno FileObject::getHandle() {
  return handle;
}

off_t FileObject::getSize() {
  return size;
}

int32_t FileObject::getPageCount() {
  return pageCount;
}

off_t FileObject::getSeekPos() {
  return seekPos;
}

int FileObject::seek(off_t newSeekPos) {
  LocalLock lock(this);
  if ((newSeekPos < 0) || (newSeekPos > size))
    return FILESYSTEM_ERROR;
  seekPos = newSeekPos;
  return FILESYSTEM_SUCCESS;
}

int FileObject::read(int bufferSize, void *buffer) {
  LocalLock lock(this);
  char *data = (char*)buffer;
  int readCount = 0;
  if (seekPos >= size)
    return 0;
  while (bufferSize > 0) {
    int32_t page = pages[seekPos / pageSize];
    int32_t pageOffSet = seekPos % pageSize;
    int32_t toRead = pageSize - pageOffSet;
    if (toRead > bufferSize)
      toRead = bufferSize;
    if (toRead > size - seekPos)
      toRead = size - seekPos;
    if (toRead == 0)
      break;
    
    // ファイルシステムからでデータを読み込む
    int result = fileSystem->readPage(page, pageOffSet, toRead, data);
    if (result < 0)
      return FILESYSTEM_ERROR;

    // 変数を更新
    readCount += toRead;
    bufferSize -= toRead;
    data += toRead;
    seekPos += toRead;
  }
  return readCount;
}

int FileObject::seekAndRead(off_t position, int bufferSize, void *buffer) {
  LocalLock lock(this);
  seek(position);
  return read(bufferSize, buffer);
}

int FileObject::write(int bufferSize, void *buffer) {
  LocalLock lock(this);
  char *data = (char*)buffer;
  int writeCount = 0;

  while (bufferSize > 0) {
    int32_t pageNumber = seekPos / pageSize;
    int32_t mySeekPos = seekPos;

    // ファイル容量が埋まっている：ファイルに加える新しいページを用意する
    if (pageNumber >= pageCount) {
      // ファイルの最後のページに近い新しいページを取得しようとする
      if ((pages == &firstPage) || (pages == nullptr)) {
        pages = typed_malloc(int32_t, MINIMUM_PAGES_ARRAY_SIZE);
        allocateCount = MINIMUM_PAGES_ARRAY_SIZE;
        pages[0] = firstPage;
      }
      fs_pageno newPage = fileSystem->claimFreePage(handle, pages[pageCount - 1]);
      assert(newPage >= 0);
      if (newPage < 0)
        return FILESYSTEM_ERROR;
      pages[pageCount] = newPage;
      fileSystem->setPageStatus(pages[pageCount - 1], newPage);
      fileSystem->setPageStatus(newPage, 0);
      fileSystem->setPageCount(handle, ++pageCount);

      // "pages"配列が埋まっているなら、新しいメモリを配分する
      if (pageCount >= allocateCount) {
        allocateCount *= 2;
        pages = (int32_t *)realloc(pages, allocateCount * sizeof(int32_t));
        assert(pages != nullptr);
      }
    }

    seekPos = mySeekPos;

    int32_t page = pages[pageNumber];
    int32_t pageOffSet = seekPos % pageSize;
    int32_t toWrite = pageSize - pageOffSet;
    if (toWrite > bufferSize)
      toWrite = bufferSize;

    // ファイルシステムにデータを書き込む
    int result = fileSystem->writePage(page, pageOffSet, toWrite, data);
    if (result < 0)
      return FILESYSTEM_ERROR;

    // 変数を更新
    writeCount += toWrite;
    bufferSize -= toWrite;
    data += toWrite;
    seekPos += toWrite;

    // 必要ならファイルサイズを更新する
    if (seekPos > size) {
      size = seekPos;
      if ((size % pageSize) == 0)
        fileSystem->setPageStatus(pages[pageCount - 1], -pageSize);
      else
        fileSystem->setPageStatus(pages[pageCount - 1], -(size % pageSize));
    }
  }

  return writeCount;
}

void *FileObject::read(int *bufferSize, bool *mustFreeBuffer) {
  void *result = malloc(*bufferSize + 1);
  *bufferSize = read(*bufferSize, result);
  *mustFreeBuffer = true;
  return result;
}