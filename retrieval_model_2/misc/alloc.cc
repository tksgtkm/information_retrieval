#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include "logging.h"
#include "lockable.h"

/*
すべての malloc 呼び出しに対して、Allocation インスタンスがハッシュテーブルに格納する。
衝突はリンクリストによって解決される
*/
typedef struct {
  const char *file; // どのファイルでmallocを呼び出すか
  int line; // どの行か
  long ptr, size; // メモリサイズ
  int timeStamp;
  void *next; // ハッシュテーブルの成功の位置
} Allocation;

static const char *LOG_ID = "Allocator";

static const int HASHTABLE_SIZE = 1234567;

/*
メモリブロックの前後にセーフティゾーンを設けて、'\0'で埋める
free(...)を呼ばれたときにその値が変わっていたらエラーを出す
*/
static const int SAFETY_ZONE_SIZE = 16;

// ハッシュテーブル内のメモリ配分の情報を保存する
static Allocation *hashtable[HASHTABLE_SIZE];

/*
メモリ解放で競合が発生した場合、解放しようとしているメモリを特定するために、
次のハッシュテーブルを用いる。
*/
typedef struct {
  const char *file;
  int line;
} Deallocation;

static const int SMALL_HASHTABLE_SIZE = 12347;

static Deallocation deallocHashtable[SMALL_HASHTABLE_SIZE];

static bool initialized = false;

static int timeStamp = 0;

// 同時に配分を火星化させる数
static int allocationCnt = 0;

// バイトの配分数
static long bytesAllocated = 0;

// これまでに同時に割り当てられた最大バイト数
static long maxAllocated = 0;

static Lockable *lock;

/*
もしalloc.hでデバッグが有効ならば、mallocを利用するたびに呼び出される
この関数は新しいAllocationのインスタンスを*hashtableにプッシュする。
*/
void *debugMalloc(int size, const char *file, int line) {
  char message[256];
  if (size <= 0) {
    sprintf(message, "Trying to allocate 0 bytes at %s/%i", file, line);
    log(LOG_ERROR, LOG_ID, message);
  }
  assert(size > 0);
  if (!initialized) {
    memset(hashtable, 0, sizeof(hashtable));
    memset(deallocHashtable, 0, sizeof(deallocHashtable));
    initialized = true;
    lock = new Lockable();
  }

  lock->getLock();

  char *realPtr = new char[size + 2 * SAFETY_ZONE_SIZE];
  if (realPtr == nullptr) {
    sprintf(message, "Trying to allocate %i bytes at %s/%i", size, file, line);
    log(LOG_ERROR, LOG_ID, message);
    sprintf(message, "Number of active allocations: %i", allocationCnt);
    log(LOG_ERROR, LOG_ID, message);
    assert(realPtr != nullptr);
  }
  memset(realPtr, 0, size + 2 * SAFETY_ZONE_SIZE);
  void *result = &realPtr[SAFETY_ZONE_SIZE];

  int hashValue = ((long)result) % HASHTABLE_SIZE;
  if (hashValue < 0)
    hashValue = -hashValue;
  Allocation *a = new Allocation;
  assert(a != nullptr);
  a->timeStamp = timeStamp;
  a->file = file;
  a->line = line;
  a->ptr = (long)result;
  a->size = size;
  a->next = hashtable[hashValue];
  hashtable[hashValue] = a;
  allocationCnt++;
  timeStamp++;

  bytesAllocated += size;
  if (bytesAllocated > maxAllocated)
    maxAllocated = bytesAllocated;

  lock->releaseLock();
  return result;
}

void debugFree(void *ptr, const char *file, int line) {
  lock->getLock();

  assert(ptr != nullptr);
  long address = (long)ptr;
  int hashValue = address % HASHTABLE_SIZE;
  if (hashValue < 0)
    hashValue = -hashValue;
  Allocation *runner = hashtable[hashValue];
  Allocation *prev = nullptr;
  while (runner != nullptr) {
    if (runner->ptr == address) {
      if (prev == nullptr)
        hashtable[hashValue] = (Allocation *)runner->next;
      else
        prev->next = (Allocation *)runner->next;
      char *charPtr = (char *)ptr;
      bool ok = true;
      for (int i = 0; i < SAFETY_ZONE_SIZE; i++) {
        if ((charPtr[i] != 0) || (charPtr[runner->size + i + SAFETY_ZONE_SIZE] != 0))
          ok = false;
      }
      if (!ok) {
        char message[256];
        sprintf(message, "Memory allocated at %ld: Write beyond array boundaries", (long)ptr);
        log(LOG_ERROR, LOG_ID, message);
        sprintf(message, "Allocated by %s/%i", runner->file, runner->line);
        log(LOG_ERROR, LOG_ID, message);
        sprintf(message, "Being freed by %s/%i", file, line);
        log(LOG_ERROR, LOG_ID, message);
        assert(false);
      }
      long thisSize = runner->size;
      delete runner;
      delete[] charPtr;
      allocationCnt++;
      
      hashValue = address % SMALL_HASHTABLE_SIZE;
      if (hashValue < 0)
        hashValue = -hashValue;
      deallocHashtable[hashValue].file = file;
      deallocHashtable[hashValue].line = line;

      bytesAllocated -= thisSize;
      lock->releaseLock();
      return;
    }
    prev = runner;
    runner = (Allocation *)runner->next;
  }

  char message[256];
  sprintf(message, "%s/%i is trying to free data at %ld, which is not in the allocation table",
          file, line, address);
  log(LOG_ERROR, LOG_ID, message);
  hashValue = address % SMALL_HASHTABLE_SIZE;
  if (hashValue < 0)
    hashValue = -hashValue;
  if (deallocHashtable[hashValue].file != nullptr) {
    sprintf(message, "It has property been freed by %s/%i",
            deallocHashtable[hashValue].file, deallocHashtable[hashValue].line);
    log(LOG_ERROR, LOG_ID, message);   
  }
  lock->releaseLock();
  assert(false);
}

void *debugRealloc(void *ptr, int size, const char *file, int line) {
  if (ptr == nullptr)
    return debugMalloc(size, file, line);
  lock->getLock();

  long address = (long)ptr;
  int hashValue = address % HASHTABLE_SIZE;
  if (hashValue < 0)
    hashValue = -hashValue;
  Allocation *runner = hashtable[hashValue];
  int oldSize = -1;
  while (runner != nullptr) {
    if (runner->ptr == address) {
      oldSize = runner->size;
      break;
    }
    runner = (Allocation *)runner->next;
  }

  if (oldSize < 0) {
    char message[256];
    sprintf(message, "Problem reallocating data (already freed?): %s/%i", file, line);
    log(LOG_ERROR, LOG_ID, message);
    assert(false);
  }

  lock->releaseLock();

  void *result = debugMalloc(size, file, line);
  memcpy(result, ptr, (oldSize < size ? oldSize : size));
  debugFree(ptr, file, line);
  return result;
}

int getAllocTimeStamp() {
  return timeStamp;
}

static int printAllocation(Allocation *a, int p1, int p2, int value) {
  return value + 1;
}

static int countAllocationSize(Allocation *a, int p1, int p2, int value) {
  return value + a->size;
}

static int forAllocations(int (*visitor)(Allocation *a, int param1, int param2, int value), int p1, int p2) {
  int result = 0;
  for (int i = 0; i < HASHTABLE_SIZE; i++) {
    Allocation *runner = hashtable[i];
    while (runner != nullptr) {
      result = visitor(runner, p1, p2, result);
      runner = (Allocation *)runner->next;
    }
  }
  return result;
}

int getAllocationSize() {
  return forAllocations(countAllocationSize, 0, 0);
}

int getAllocationCount() {
  return allocationCnt;
}

void printAllocationAfter(int timeStamp) {
  forAllocations(printAllocation, timeStamp, 2000000000);
}

void printAllocations() {
  forAllocations(printAllocation, -1, 2000000000);
}

void printAllocationsBefore(int timeStamp) {
  forAllocations(printAllocation, -1, timeStamp);
}

long getMaxAllocated() {
  return maxAllocated;
}

void setMaxAllocated(long newMax) {
  maxAllocated = newMax;
}

#undef free

void realFree(void *ptr) {
  free(ptr);
}