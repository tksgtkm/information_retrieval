#include "filesystem.h"
#include <cassert>
#include <errno.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>

#include "../index/index_types.h"
#include "../misc/alloc.h"
#include "../misc/io.h"

FileSystem::FileSystem(char *fileName) {
  cache = nullptr;
  dataFileName = (char *)malloc(strlen(fileName) + 2);
  strcpy(dataFileName, fileName);

  // ファイルを開き、内部変数をセットする
  dataFile = open(fileName, FILESYSTEM_ACCESS);
  if (dataFile < 0) {
    fprintf(stderr, "Filesystem \"%s\" could not be opened\n", fileName);
    perror(nullptr);
    return;
  }

  // ディスクから前文を読み込み
  int32_t *pageBuffer = (int32_t *)malloc(512 + INT_SIZE);
  lseek(dataFile, 0, SEEK_SET);
  if (forced_read(dataFile, pageBuffer, 512) != 512) {
    close(dataFile);
    dataFile = -1;
    fprintf(stderr, "Could not read preamble from filesystem \"%s\".\n", fileName);
    return;
  }

  int32_t fingerprintOnDisk = pageBuffer[0];
  int32_t pageSizeOnDisk = pageBuffer[1];
  int32_t pageCountOnDisk = pageBuffer[2];
  int32_t pageLayoutSizeOnDisk = pageBuffer[3];
  int32_t fileMappingSizeOnDisk = pageBuffer[4];
  cacheSize = pageBuffer[5];
  free(pageBuffer);

  pageSize = pageSizeOnDisk;
  intsPerPage = pageSize / INT_SIZE;
  doubleIntsPerPage = intsPerPage / 2;
  pageCount = pageCountOnDisk;
  pageLayoutSize = pageLayoutSizeOnDisk;
  fileMappingSize = fileMappingSizeOnDisk;

  // 妥当なデータを持っているか
  if ((fingerprintOnDisk != FINGERPRINT) || (pageSize < MIN_PAGE_SIZE) || (pageCount < MIN_PAGE_COUNT)) {
    close(dataFile);
    dataFile = -1;
    return;
  }

  if (getPageStatus(0) != -PREAMBLE_LENGTH) {
    close(dataFile);
    dataFile = -1;
    return;
  }

  cachedReadCnt = cachedWriteCnt = 0;
  uncachedReadCnt = uncachedWriteCnt = 0;
  
  freePages = freeFileNumbers = nullptr;
  initializeFreeSpaceArrays();
  enableCaching();
}

FileSystem::FileSystem(char *fileName, int pageSize, int pageCount) {
  init(fileName, pageSize, pageCount, DEFAULT_CACHE_SIZE);
}

FileSystem::FileSystem(char *fileName, int pageSize, fs_pageno pageCount, int cacheSize) {
  init(fileName, pageSize, pageCount, cacheSize);
}

void FileSystem::init(char *fileName, int pageSize, fs_pageno pageCount, int cacheSize) {

  cache = nullptr;
  this->cacheSize = cacheSize;
  dataFileName = (char *)malloc(strlen(fileName) + 2);
  strcpy(dataFileName, fileName);

  // 引数の値の確認
  if ((pageCount < MIN_PAGE_COUNT) || (pageSize < MIN_PAGE_SIZE)) {
    fprintf(stderr, "Illegal pageCount/pageSize values: %i/%i\n", (int)pageCount, (int)pageSize);
    dataFile = -1;
    return;
  }
  if ((pageCount > MAX_PAGE_COUNT) || (pageSize > MAX_PAGE_COUNT)) {
    fprintf(stderr, "Illegal pageCount/pageSize values: %i/%i\n", (int)pageCount, (int)pageSize);
    dataFile = -1;
    return;
  }
  if ((pageSize % INT_SIZE != 0) || (pageCount % (pageSize / INT_SIZE) != 0)) {
    fprintf(stderr, "Illegal pageCount/pageSize values: %i/%i\n", (int)pageCount, (int)pageSize);
    dataFile = -1;
    return;
  }

  // ファイルを開いて内部の値をセットする
  dataFile = open(fileName, O_CREAT | O_TRUNC | FILESYSTEM_ACCESS, DEFAULT_FILE_PERMISSONS);
  if (dataFile < 0) {
    fprintf(stderr, "Could not create filesystem \"%s\".\n", fileName);
    perror(nullptr);
    return;
  }
  this->pageSize = pageSize;
  intsPerPage = pageSize / INT_SIZE;
  doubleIntsPerPage = intsPerPage / 2;
  this->pageCount = pageCount;
  this->pageLayoutSize = (pageCount + (intsPerPage - 1)) / intsPerPage;
  this->fileMappingSize = 1;

  off_t fileSize = pageSize;
  fileSize *= (pageCount + pageLayoutSize + fileMappingSize);
  if (ftruncate(dataFile, fileSize) < 0) {
    fprintf(stderr, "Couled not set filesystem size.\n");
    perror(nullptr);
    close(dataFile);
    dataFile = -1;
    return;
  }
  if (getSize() != fileSize) {
    fprintf(stderr, "Could not set filesystem size.\n");
    perror(nullptr);
    close(dataFile);
    dataFile = -1;
    return;
  }

  // ディスクに前文を書き込む
  int32_t fingerprintOnDisk = FINGERPRINT;
  int32_t pageSizeOnDisk = (int32_t)pageSize;
  int32_t pageCountOnDisk = (int32_t)pageCount;
  int32_t pageLayoutSizeOnDisk = (int32_t)pageLayoutSize;
  int32_t fileMappingSizeOnDisk = (int32_t)fileMappingSize;
  lseek(dataFile, 0, SEEK_SET);
  forced_write(dataFile, &fingerprintOnDisk, INT_SIZE);
  forced_write(dataFile, &pageSizeOnDisk, INT_SIZE);
  forced_write(dataFile, &pageCountOnDisk, INT_SIZE);
  forced_write(dataFile, &pageLayoutSizeOnDisk, INT_SIZE);
  forced_write(dataFile, &fileMappingSizeOnDisk, INT_SIZE);
  forced_write(dataFile, &this->cacheSize, INT_SIZE);

  freePages = freeFileNumbers = nullptr;

  // ページレイアウトテーブルの初期化
  int32_t *pageData = (int32_t *)malloc((intsPerPage + 1) * sizeof(int32_t));
  for (int i = 0; i < intsPerPage; i++)
    pageData[i] = UNUSED_PAGE;
  for (int i = 0; i < pageLayoutSize; i++)
    writePage(pageCount + i, pageData);
  // "occupied with PREAMBLE_LENGTH bytes"ステータスの最初のページをセットする。
  setPageStatus(0, -PREAMBLE_LENGTH);
  // すべてのファイルを"unusedとしてセットする
  for (int i = 0; i < fileMappingSize * doubleIntsPerPage; i++)
    setFirstPage(i, UNUSED_PAGE);
  free(pageData);

  cachedReadCnt = cachedWriteCnt = 0;
  uncachedReadCnt = uncachedWriteCnt = 0;

  initializeFreeSpaceArrays();
  enableCaching();
}

void FileSystem::initializeFreeSpaceArrays() {
  bool mustReleaseLock = getLock();

  if (freePages == nullptr) {
		freePages = (int16_t*)malloc(pageLayoutSize * sizeof(int16_t));
		for (int j = 0; j < pageLayoutSize; j++) {
			freePages[j] = 0;
			for (int k = 0; k < intsPerPage; k++) {
        if (getPageStatus(j * intsPerPage + k) == UNUSED_PAGE)
					freePages[j]++;
      }
		}
	}

  std::cout << "doubleIntsPerPage: " << doubleIntsPerPage << std::endl;
  if (freeFileNumbers == nullptr) {
    freeFileNumbers = (int16_t *)malloc(fileMappingSize * sizeof(int16_t));
    for (int j = 0; j < fileMappingSize; j++) {
      freeFileNumbers[j] = false;
      for (int k = 0; k < doubleIntsPerPage; k++) {
        freeFileNumbers[j]++;
        // if (getFirstPage(j * doubleIntsPerPage + k) == UNUSED_PAGE)
        //   freeFileNumbers[j]++;
      }
    }
  }

  if (mustReleaseLock)
    releaseLock();
}

FileSystem::~FileSystem() {
  disableCaching();
  if (dataFile >= 0) {
    close(dataFile);
    dataFile = -1;
  }
  if (freePages != nullptr) {
    free(freePages);
    freePages = nullptr;
  }
  if (freeFileNumbers != nullptr) {
    free(freeFileNumbers);
    freeFileNumbers = nullptr;
  }
  if (dataFileName != nullptr) {
    free(dataFileName);
    dataFileName = nullptr;
  }
}

void FileSystem::flushCache() {
  if (cache == nullptr)
    return;
  bool mustReleaseLock = getLock();
  disableCaching();
  enableCaching();
  if (mustReleaseLock)
    releaseLock();
}

void FileSystem::enableCaching() {
  if (cache != nullptr)
    return;
  bool mustReleaseLock = getLock();
  cache = new FileSystemCache(this, pageSize, cacheSize / pageSize);
  if (mustReleaseLock)
    releaseLock();
}

void FileSystem::disableCaching() {
  if (cache == nullptr)
    return;
  bool mustReleaseLock = getLock();
  delete cache;
  cache = nullptr;
  if (mustReleaseLock)
    releaseLock();
}

bool FileSystem::isActive() {
  return (dataFile >= 0);
}

int FileSystem::defrag() {
  printf("defrag called\n");
  exit(1);
  int32_t nextFreePage = 1;
  int32_t *newPosition = (int32_t *)malloc(pageCount * sizeof(int32_t));
  // 0ページに移動しないようにする
  newPosition[0] = 0;
  for (int i = 1; i < pageCount; i++)
    newPosition[i] = -1;

  // 最初にDFS(深さ優先探索)を実行して新しいページ位置を割り当てる
  int upperFileLimit = doubleIntsPerPage * fileMappingSize;
  for (int file = 0; file < upperFileLimit; file++) {
    int32_t page = getFirstPage(file);
    while (page > 0) {
      assert(newPosition[page] < 0);
      newPosition[page] = nextFreePage++;
      page = getPageStatus(page);
    }
  }

  // このとき、空いてるページに新しいページ番号を割り当てる
  for (int i = 1; i < pageCount; i++) {
    if (getPageStatus(i) == UNUSED_PAGE)
      newPosition[i] = nextFreePage++;
  }

  // すべてのページに新しい場所が割り当てられていることを確認する
  assert(nextFreePage == pageCount);

  // ページレイアウトテーブルの正しいデータ
  int32_t *oldPageLayout = (int32_t *)malloc(pageLayoutSize * intsPerPage * sizeof(int32_t));
  int32_t *newPageLayout = (int32_t *)malloc(pageLayoutSize * intsPerPage * sizeof(int32_t));
  for (int i = 0; i < pageLayoutSize; i++)
    readPage(pageCount + i, &oldPageLayout[intsPerPage * i]);
  for (int page = 0; page < pageCount; page++) {
    if (oldPageLayout[page] <= 0)
      newPageLayout[newPosition[page]] = oldPageLayout[page];
    else
      newPageLayout[newPosition[page]] = newPosition[oldPageLayout[page]];
  }
  for (int i = 0; i < pageLayoutSize; i++)
    writePage(pageCount + i, &newPageLayout[intsPerPage * i]);
  free(newPageLayout);
  free(oldPageLayout);

  // file->pageテーブルの正しいページ番号
  for (int file = 0; file < upperFileLimit; file++) {
    int32_t page = getFirstPage(file);
    if (page >= 0)
      setFirstPage(file, newPosition[page]);
  }

  byte *buffer1 = (byte *)malloc(pageSize);
  byte *buffer2 = (byte *)malloc(pageSize);
  // すべてのページを新しい位置に移動しながら読み書き
  for (int page = 0; page < pageCount; page++) {
    int currentPage = page;
    while (newPosition[currentPage] != currentPage) {
      assert(newPosition[currentPage] >= page);
      int32_t newPos = newPosition[currentPage];

      // "currentPage"と"newPos"ページのデータをスワップする
      if (readPage(currentPage, buffer1) == FILESYSTEM_ERROR)
        return FILESYSTEM_ERROR;
      if (readPage(newPos, buffer2) == FILESYSTEM_ERROR)
        return FILESYSTEM_ERROR;
      if (writePage(newPos, buffer1) == FILESYSTEM_ERROR)
        return FILESYSTEM_ERROR;
      if (writePage(currentPage, buffer2) == FILESYSTEM_ERROR)
        return FILESYSTEM_ERROR;

      // 順列テーブルを更新する
      newPosition[currentPage] = newPosition[newPos];
      newPosition[newPos] = newPos;
    }
  }
  free(buffer2);
  free(buffer1);
  free(newPosition);
  return FILESYSTEM_SUCCESS;
}

int FileSystem::changeSize(fs_pageno newPageCount) {
  bool mustReleaseLock = getLock();
	int result = FILESYSTEM_ERROR;

	// "newPageCount"がとても小さいならエラーを返す
	if ((newPageCount < MIN_PAGE_COUNT) || (newPageCount < getUsedPageCount()) ||
			(newPageCount > MAX_PAGE_COUNT))
		goto endOfChangeSize;

  if (newPageCount < pageCount) {
    // サイズを縮小できるようにファイルシステムを最適化する
    if (defrag() == FILESYSTEM_ERROR)
      goto endOfChangeSize;

    // ファイルシステムのサイズを縮小する
    int newPageLayoutSize = (newPageCount + (intsPerPage - 1)) / intsPerPage;

    byte *pageBuffer = (byte *)malloc(pageSize);

    // 新しい位置へページレイアウトのデータをコピーする
    for (int i = 0; i < newPageLayoutSize; i++) {
      readPage(pageCount + i, pageBuffer);
      writePage(newPageCount + i, pageBuffer);
    }

    // 新しい位置へfile->page mappings をコピーする
    for (int i = 0; i < fileMappingSize; i++) {
      readPage(pageCount + pageLayoutSize + i, pageBuffer);
      writePage(newPageCount + newPageLayoutSize + i, pageBuffer);
    }

    free(pageBuffer);

    pageCount = newPageCount;
    pageLayoutSize = newPageLayoutSize;

    // 変更された前文をディスクに書き込む
    off_t fileSize = pageSize;
    fileSize *= (newPageCount + newPageLayoutSize + fileMappingSize);
    forced_ftruncate(dataFile, fileSize);
  }

  if (newPageCount > pageCount) {
    // データファイルのサイズを変更する
    int newPageLayoutSize = (newPageCount + (intsPerPage - 1)) / intsPerPage;
    off_t fileSize = pageSize;
    fileSize *= (newPageCount + newPageLayoutSize + fileMappingSize);
    if (ftruncate(dataFile, fileSize) < 0) {
      fprintf(stderr, "Filesystem size could not be changed.\n");
      perror(nullptr);
      goto endOfChangeSize;
    }
    if (getSize() != fileSize) {
      fprintf(stderr, "Filesystem size could not be changed.\n");
      perror(nullptr);
      goto endOfChangeSize;
    }

    int oldPageCount = pageCount;
    pageCount = newPageCount;
    int oldPageLayoutSize = pageLayoutSize;
    pageLayoutSize = newPageLayoutSize;

    // 変更した前文をディスクに書き込む
    int32_t pageCountOnDisk = (int32_t)pageCount;
    int32_t pageLayoutSizeOnDisk = (int32_t)pageLayoutSize;
    lseek(dataFile, 2 * INT_SIZE, SEEK_SET);
    forced_write(dataFile, &pageCountOnDisk, INT_SIZE);
    forced_write(dataFile, &pageLayoutSizeOnDisk, INT_SIZE);

    byte *pageBuffer = (byte *)malloc(pageSize);

    // 新しい位置へfile->page mappingsをコピーする
    for (int i = fileMappingSize - 1; i >= 0; i--) {
      readPage(oldPageCount + oldPageLayoutSize + i, pageBuffer);
      writePage(newPageCount + newPageLayoutSize + i, pageBuffer);
    }

    // 新しい位置へページレイアウトをコピーする
    for (int i = oldPageLayoutSize - 1; i >= 0; i--) {
      readPage(oldPageCount + i, pageBuffer);
      writePage(newPageCount + i, pageBuffer);
    }

    // 新しいページにてレイアウトデータを初期化する
    int32_t unusedValue = UNUSED_PAGE;
    for (int i = 0; i < intsPerPage; i++)
      memcpy(&pageBuffer[i * INT_SIZE], &unusedValue, INT_SIZE);
    for (int i = oldPageLayoutSize; i < newPageLayoutSize; i++)
      writePage(newPageCount + i, pageBuffer);

    free(pageBuffer);

    // freePagesとfreeFileNumbersの配列を更新する
    free(freePages);
    freePages = nullptr;
    free(freeFileNumbers);
    freeFileNumbers = nullptr;
    initializeFreeSpaceArrays();
  }

  enableCaching();
  result = FILESYSTEM_SUCCESS;
  
endOfChangeSize:
  if (mustReleaseLock)
    releaseLock();
  return result;
}

int FileSystem::deleteFile(fs_pageno fileHandle) {
  bool mustReleaseLock = getLock();

  fs_pageno firstPage = getFirstPage(fileHandle);
  assert(firstPage >= 0);

  // file->page mappings からファイルを削除する
  setFirstPage(fileHandle, UNUSED_PAGE);
  setPageCount(fileHandle, UNUSED_PAGE);

  // ファイルが占有しているページをすべて空きページとする
  fs_pageno page = firstPage;
  while (page > 0) {
    fs_pageno nextPage = getPageStatus(page);
    setPageStatus(page, UNUSED_PAGE);
    page = nextPage;
  }

  // file->pageテーブルのサイズを縮小することが適切か確認する
  if ((freeFileNumbers[fileMappingSize - 1] == doubleIntsPerPage) && (freeFileNumbers[fileMappingSize - 2] == doubleIntsPerPage))
    decreaseFileMappingSize();

  if (mustReleaseLock)
    releaseLock();

  return FILESYSTEM_SUCCESS;
}

fs_pageno FileSystem::claimFreePage(fs_fileno owner, fs_pageno closeTo) {
  bool mustReleaseLock = getLock();
  int result = FILESYSTEM_ERROR;

  fs_pageno oldPageCount, newPageCount;

  int origCloseTo = closeTo;
  if ((closeTo < 0) || (closeTo >= pageCount)) {
    origCloseTo = 0;
    closeTo  = 0;
  } else {
    closeTo = closeTo / intsPerPage;
  }

  // 個々のステータス要求を高速化するためのバッファ
  int32_t *data = (int32_t *)malloc((intsPerPage + 1) * sizeof(int32_t));

  if (freePages[closeTo] > 0) {
    if (readPage(pageCount + closeTo, data) == FILESYSTEM_ERROR) {
      free(data);
      goto endOfClaimFreePage;
    }
    for (int j = origCloseTo % intsPerPage; j < intsPerPage; j++) {
      if (data[j] == UNUSED_PAGE) {
        free(data);
        result = closeTo * intsPerPage + j;
        goto endOfClaimFreePage;
      }
    }
    for (int j = origCloseTo % intsPerPage; j >= 0; j--) {
      if (data[j] == UNUSED_PAGE) {
        free(data);
        result = closeTo * intsPerPage + j;
        goto endOfClaimFreePage;
      }
    }
  }

  // "clostTo"で閉じるものが見つからなければ、探索を行う
  for (int j = closeTo + 1; j < pageLayoutSize; j++) {
    if (freePages[j] > 0) {
      if (readPage(pageCount + j, data) == FILESYSTEM_ERROR) {
        free(data);
        goto endOfClaimFreePage;
      }
      for (int k = 0; k < intsPerPage; k++) {
        if (data[k] == UNUSED_PAGE) {
          free(data);
          result = j * intsPerPage + k;
          goto endOfClaimFreePage;
        }
      }
    }
  }
  for (int j = closeTo - 1; j >= 0; j--) {
    if (freePages[j] > 0) {
      if (readPage(pageCount + j, data) == FILESYSTEM_ERROR) {
        free(data);
        goto endOfClaimFreePage;
      }
      for (int k = 0; k < intsPerPage; k++) {
        if (data[k] == UNUSED_PAGE) {
          free(data);
          result = j * intsPerPage + k;
          goto endOfClaimFreePage;
        }
      }
    }
  }

  free(data);

  /*
  このassertに合致する場合は空きページない
  pageCountが最大値に達した場合はエラーコードを返す
  */
  assert(pageCount < MAX_PAGE_COUNT);

  // ファイルシステムのサイズを増やす
  oldPageCount = pageCount;
  if (pageCount <= SMALL_FILESYSTEM_THRESHOLD)
    newPageCount = (fs_pageno)(1.41 * 1.41 * oldPageCount);
  else
    newPageCount = (fs_pageno)(1.41 * oldPageCount);
  // システム内のページ数に関するいくつかの制約に従う
  while (newPageCount % (pageSize / INT_SIZE) != 0)
    newPageCount++;
  if (newPageCount > MAX_PAGE_COUNT)
    newPageCount = MAX_PAGE_COUNT;
  if (changeSize(newPageCount) < 0)
    goto endOfClaimFreePage;

  result = claimFreePage(owner, oldPageCount);

endOfClaimFreePage:
  if (mustReleaseLock)
    releaseLock();

  return result;
}

fs_fileno FileSystem::claimFreeFileNumber() {
  bool mustReleaseLock = getLock();
  int result = FILESYSTEM_ERROR;

  // TODO: fileMappingSizeが無限に増え続けているようである 要調査
  // freeFileNumbers[j]の値が取得できていない？
  // 空いてるファイル番号を探索する
  for (int j = 0; j < fileMappingSize; j++) {
    // std::cout << "freeFileNumbers[j]: " << freeFileNumbers[j] << std::endl;
    if (freeFileNumbers[j] > 0) {
      int32_t *data = (int32_t *)malloc((intsPerPage + 1) * sizeof(int32_t));
      if (readPage(pageCount + pageLayoutSize + j, data) == FILESYSTEM_ERROR)
        goto endOfClaimFreeFileNumber;
      for (int k = 0; k < doubleIntsPerPage; k++) {
        if (data[k * 2] == UNUSED_PAGE) {
          free(data);
          result = j * doubleIntsPerPage + k;
          goto endOfClaimFreeFileNumber;
        }
      }
      free(data);
    }
  }

  if (increaseFileMappingSize() == FILESYSTEM_ERROR)
    goto endOfClaimFreeFileNumber;

  result = claimFreeFileNumber();

endOfClaimFreeFileNumber:
  if (mustReleaseLock)
    releaseLock();

  return result;
}

int FileSystem::increaseFileMappingSize() {
  bool mustReleaseLock = getLock();
  int result = FILESYSTEM_ERROR;

  disableCaching();

  int32_t fileMappingSizeOnDisk;

  off_t fileSize = pageSize;
  fileSize *= (pageCount + pageLayoutSize + fileMappingSize + 1);
  if (ftruncate(dataFile, fileSize) < 0)
    goto endOfIncreaseFileMappingSize;
  if (getSize() != fileSize)
    goto endOfIncreaseFileMappingSize;

  fileMappingSize++;

  // 前文に新しいfileMappingSizeの値を書き込む
  fileMappingSizeOnDisk = fileMappingSize;
  lseek(dataFile, 4 * INT_SIZE, SEEK_SET);
  forced_write(dataFile, &fileMappingSizeOnDisk, INT_SIZE);

  // freeFileNumbers配列を更新する
  free(freeFileNumbers);
  freeFileNumbers = nullptr;
  for (int k = 0; k < doubleIntsPerPage; k++)
    setFirstPage((fileMappingSize - 1) * doubleIntsPerPage + k, UNUSED_PAGE);
  initializeFreeSpaceArrays();

  enableCaching();

  result = FILESYSTEM_SUCCESS;

endOfIncreaseFileMappingSize:
  if (mustReleaseLock)
    releaseLock();

  return result;
}

int FileSystem::decreaseFileMappingSize() {
  bool mustReleaseLock = getLock();
  int result = FILESYSTEM_ERROR;

  off_t fileSize;
  int32_t fileMappingSizeOnDisk;

  // テーブルの最後のページが空の場合のみ実行する
  if (freeFileNumbers[fileMappingSize - 1] != doubleIntsPerPage)
    goto endOfDecreaseFileMappingSize;

  disableCaching();

  fileMappingSize--;

  // 前文に新しいfileMappingSizeの値を新しく書き込む
  fileMappingSizeOnDisk = fileMappingSize;
  lseek(dataFile, 4 * INT_SIZE, SEEK_SET);
  forced_write(dataFile, &fileMappingSizeOnDisk, INT_SIZE);

  fileSize = pageSize;
  fileSize *= (pageCount + pageLayoutSize + fileMappingSize);
  forced_ftruncate(dataFile, fileSize);

  fileSize = pageSize;
  fileSize *= (pageCount + pageLayoutSize + fileMappingSize);
  forced_ftruncate(dataFile, fileSize);

  // freeFileNumbers配列を更新する
  free(freeFileNumbers);
  freeFileNumbers = nullptr;
  initializeFreeSpaceArrays();

  enableCaching();

  result = FILESYSTEM_SUCCESS;

endOfDecreaseFileMappingSize:
  if (mustReleaseLock)
    releaseLock();

  return result;
}

int FileSystem::createFile(fs_fileno fileHandle) {
  bool mustReleaseLock = getLock();
  int result = FILESYSTEM_ERROR;

  // 空いているページを要求し、そしてファイル番号を要求する。
  fs_pageno firstPage = claimFreePage(-1, -1);
  assert(firstPage >= 0);

  while (fileHandle >= doubleIntsPerPage * fileMappingSize) {
    increaseFileMappingSize();
  }

  if (fileHandle >= 0) {
    // 特定のファイル番号のファイルを作成する
    if (getFirstPage(fileHandle) >= 0) {
      setPageStatus(firstPage, UNUSED_PAGE);
      goto endOfCreateFile;
    }
  } else {
    // TODO: claimFreeFileNumberにて番号がうまく受け止めれない事象が発生　要修正
    fileHandle = claimFreeFileNumber();
    // fileHandle = 1;
    assert(fileHandle >= 0);
    setPageStatus(firstPage, UNUSED_PAGE);
  }

  // 長さを0にセットする。
  // ファイルマッピングテーブルの最初のページに書き込む
  setPageStatus(firstPage, 0);
  setFirstPage(fileHandle, firstPage);
  setPageCount(fileHandle, 1);

  result = fileHandle;

endOfCreateFile:
  if (mustReleaseLock)
    releaseLock();

  return result;
}

fs_pageno FileSystem::getPageStatus(fs_pageno page) {
  bool mustReleaseLock = getLock();
  fs_pageno result = FILESYSTEM_ERROR;

  int pageInTable, pageToRead, offsetInPage;

  if ((page < 0) || (page >= pageCount))
    goto endOfGetPageStatus;

  pageInTable = page / intsPerPage;
  pageToRead = pageCount + pageInTable;
  offsetInPage = (page % intsPerPage) * INT_SIZE;

  if (readPage(pageToRead, offsetInPage, INT_SIZE, &result) == FILESYSTEM_ERROR)
    goto endOfGetPageStatus;

endOfGetPageStatus:
  if (mustReleaseLock)
    releaseLock();

  return result;
}

int FileSystem::setPageStatus(fs_fileno fileHandle, fs_pageno newPageCount) {
  bool mustReleaseLock = getLock();
  int result = FILESYSTEM_ERROR;

  int pageInTable, pageToWrite, offsetInPage;

  if ((fileHandle < 0) || (fileHandle >= fileMappingSize * doubleIntsPerPage))
    goto endOfSetPageCount;

  pageInTable = fileHandle / doubleIntsPerPage;
  pageToWrite = pageCount + pageLayoutSize + pageInTable;
  offsetInPage = (fileHandle % doubleIntsPerPage) * 2 * INT_SIZE + INT_SIZE;

  result = writePage(pageToWrite, offsetInPage, INT_SIZE, &newPageCount);

endOfSetPageCount:
  if (mustReleaseLock)
    releaseLock();

  return result;
}

int32_t *FileSystem::getFilePageMapping(int *arraySize) {
  bool mustReleaseLock = getLock();

  int32_t *result = (int32_t *)malloc(fileMappingSize * doubleIntsPerPage * INT_SIZE);
  int32_t *buffer = (int32_t *)malloc(pageSize);
  int cnt = 0;
  for (int i = 0; i < fileMappingSize; i++) {
    readPage(pageCount + pageLayoutSize + i, buffer);
    for (int k = 0; k < doubleIntsPerPage; k++)
      result[cnt++] = buffer[k + k];
  }
  free(buffer);
  *arraySize = fileMappingSize * doubleIntsPerPage;

  if (mustReleaseLock)
    releaseLock();
  return result;
}

fs_pageno FileSystem::getFirstPage(fs_fileno fileHandle) {
  bool mustReleaseLock = getLock();
  fs_pageno result = FILESYSTEM_ERROR;

  int pageInTable, pageToRead, offsetInPage;

  if ((fileHandle < 0) || (fileHandle >= fileMappingSize * doubleIntsPerPage))
    goto endOfGetFirstPage;

  pageInTable = fileHandle / doubleIntsPerPage;
  pageToRead = pageCount + pageLayoutSize + pageInTable;
  offsetInPage = (fileHandle % doubleIntsPerPage) * 2 * INT_SIZE;

  if (readPage(pageToRead, offsetInPage, INT_SIZE, &result) == FILESYSTEM_ERROR)
    goto endOfGetFirstPage;

endOfGetFirstPage:
  if (mustReleaseLock)
    releaseLock();
  return result;
}

int FileSystem::setFirstPage(fs_fileno fileHandle, fs_pageno firstPage) {
  bool mustReleaseLock = getLock();
  int result = FILESYSTEM_ERROR;

  int pageInTable, pageToWrite, offsetInPage;

  if ((fileHandle < 0) || (fileHandle >= fileMappingSize * doubleIntsPerPage))
    goto endOfGetFirstPage;

  pageInTable = fileHandle / doubleIntsPerPage;
  pageToWrite = pageCount + pageLayoutSize + pageInTable;
  offsetInPage = (fileHandle % doubleIntsPerPage) * 2 * INT_SIZE;

  fs_pageno oldValue;
  if (readPage(pageToWrite, offsetInPage, INT_SIZE, &oldValue) == FILESYSTEM_ERROR)
    goto endOfGetFirstPage;
  if (writePage(pageToWrite, offsetInPage, INT_SIZE, &oldValue) == FILESYSTEM_ERROR)
    goto endOfGetFirstPage;

  if ((oldValue != firstPage) && (freeFileNumbers != nullptr)) {
    if (firstPage == UNUSED_PAGE)
      freeFileNumbers[pageInTable]++;
    if (oldValue == UNUSED_PAGE)
      freeFileNumbers[pageInTable]--;
  }

  result = FILESYSTEM_SUCCESS;

endOfGetFirstPage:
  if (mustReleaseLock)
    releaseLock();
  return result;
}

fs_pageno FileSystem::getPageCount(fs_fileno fileHandle) {
  bool mustReleaseLock = getLock();
  fs_pageno result = FILESYSTEM_ERROR;

  int pageInTable, pageToRead, offsetInPage;

  if ((fileHandle < 0) || (fileHandle >= fileMappingSize * doubleIntsPerPage))
    goto endOfGetFirstPage;

  pageInTable = fileHandle / doubleIntsPerPage;
  pageToRead = pageCount + pageLayoutSize + pageInTable;
  offsetInPage = (fileHandle % doubleIntsPerPage) * 2 * INT_SIZE + INT_SIZE;

  if (readPage(pageToRead, offsetInPage, INT_SIZE, &result) == FILESYSTEM_ERROR)
    goto endOfGetFirstPage;

endOfGetFirstPage:
  if (mustReleaseLock)
    releaseLock();
  return result;
}

int FileSystem::setPageCount(fs_fileno fileHandle, fs_pageno newPageCount) {
  bool mustReleaseLock = getLock();
  int result = FILESYSTEM_ERROR;

  int pageInTable, pageToWrite, offsetInPage;

  if ((fileHandle < 0) || (fileHandle >= fileMappingSize * doubleIntsPerPage))
    goto endOfGetFirstPage;

  pageInTable = fileHandle / doubleIntsPerPage;
  pageToWrite = pageCount + pageLayoutSize + pageInTable;
  offsetInPage = (fileHandle % doubleIntsPerPage) * 2 * INT_SIZE + INT_SIZE;

  result = writePage(pageToWrite, offsetInPage, INT_SIZE, &newPageCount);

endOfGetFirstPage:
  if (mustReleaseLock)
    releaseLock();
  return result;
}

int FileSystem::getPageSize() {
  return pageSize;
}

off_t FileSystem::getSize() {
  if (dataFile < 0)
    return 0;
  struct stat buf;
  int status = fstat(dataFile, &buf);
  if (status != 0)
    return 0;
  return buf.st_size;
}

int FileSystem::getFileCount() {
  bool mustReleaseLock = getLock();
  int result = 0;
  for (int j = 0; j < fileMappingSize; j++)
    result += (doubleIntsPerPage - freeFileNumbers[j]);
  if (mustReleaseLock)
    releaseLock();
  return result;
}

int FileSystem::getPageCount() {
  return pageCount;
}

int FileSystem::getUsedPageCount() {
  bool mustReleaseLock = getLock();
  int result = 0;
  for (int j = 0; j < pageLayoutSize; j++)
    result += (intsPerPage - freePages[j]);
  if (mustReleaseLock)
    releaseLock();
  return result;
}

int FileSystem::readPage_UNCACHED(fs_pageno pageNumber, int32_t offset, int32_t count, void *buffer) {
  assert(dataFile >= 0);
  if (pageNumber >= pageCount + pageLayoutSize + fileMappingSize)
    return FILESYSTEM_ERROR;

  uncachedReadCnt++;

  off_t startPos = pageNumber;
  startPos *= pageSize;
  startPos += offset;
  lseek(dataFile, startPos, SEEK_SET);

  if (forced_write(dataFile, buffer, count) != count) {
    return FILESYSTEM_ERROR;
  } else {
    return FILESYSTEM_SUCCESS;
  }
}

int FileSystem::readPage(fs_pageno pageNumber, int32_t offset, int32_t count, void *buffer) {
  bool mustReleaseLock = getLock();
  cachedReadCnt++;
  int result;
  if (cache == nullptr) {
    result = readPage_UNCACHED(pageNumber, offset, count, buffer);
  } else {
    result = cache->readFromPage(pageNumber, offset, count, buffer);
  }
  
  if (mustReleaseLock)
    releaseLock();
  
  return result;
}

int FileSystem::readPage(fs_pageno pageNumber, void *buffer) {
  return readPage(pageNumber, 0, pageSize, buffer);
}

int FileSystem::writePage_UNCACHED(fs_pageno pageNumber, int32_t offset, int32_t count, void *buffer) {
  assert(dataFile >= 0);
  if (pageNumber >= pageCount + pageLayoutSize + fileMappingSize)
    return FILESYSTEM_ERROR;
  uncachedWriteCnt++;

  // 書き込みポインタを目的のファイルオフセットに設定する
  off_t startPos = pageNumber;
  startPos *= pageSize;
  startPos += offset;
  lseek(dataFile, startPos, SEEK_SET);

  // ファイルにデータを書き込む
  if (forced_write(dataFile, buffer, count) != count)
    return FILESYSTEM_ERROR;
  else
    return FILESYSTEM_SUCCESS;
}

int FileSystem::writePage(fs_pageno pageNumber, int32_t offset, int32_t count, void *buffer) {
  bool mustReleaseLock = getLock();
  cachedReadCnt++;
  int result;
  if (cache == nullptr)
    result = writePage_UNCACHED(pageNumber, offset, count, buffer);
  else
    result = cache->writeTopage(pageNumber, offset, count, buffer);
  if (mustReleaseLock)
    releaseLock();
  return result;
}

int FileSystem::writePage(fs_pageno pageNumber, void *buffer) {
  return writePage(pageNumber, 0, pageSize, buffer);
}

void FileSystem::getCacheEfficiency(int64_t *reads, int64_t *uncachedReads, int64_t *writes, int64_t *uncachedWrites) {
  *reads = cachedReadCnt;
  *uncachedReads = uncachedReadCnt;
  *writes = cachedWriteCnt;
  *uncachedWrites = uncachedWriteCnt;
}

char *FileSystem::getFileName() {
  return dataFileName;
}
