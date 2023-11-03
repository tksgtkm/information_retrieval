#include <cstring>
#include <cstdlib>
#include "index_types.h"
#include "../misc/backend.h"
#include <map>
#include <string>

const char *ERROR_MESSAGES[MAX_ERROR_CODE + 2] = {
  "OK",
  "Error",
	"Syntax error",
	"Index is shutting down",
	"No such file or directory",
	"Directory not allowed",
	"Unknown file format",
	"Empty file (tokenizer returns 0 tokens)",
	"Access denied (insufficient file permissions)",
	"No update necessary (file unchanged)",
	"File too small",
	"File too large",
	"Read-only index",
	"Concurrent update",
	"Internal error",
  (char *)0
};

static int cmpLLPbyFirst(const void *a, const void *b) {
  LongLongPair *x = (LongLongPair*)a;
  LongLongPair *y = (LongLongPair*)b;
  if (x->first < y->first)
    return -1;
  else if (x->first > y->first)
    return +1;
  else
    return 0;
}

void sortArrayOfLongLongPairsByFirst(LongLongPair *array, int n) {
  qsort(array, n, sizeof(LongLongPair), cmpLLPbyFirst);
}

/*
HeapSortを使ってポスティングの集合をソートする。
ポスティングは"ascending"の値を基準にソートする
*/
static void heapSortPostings(offset *array, int n, bool ascending) {
  int arraySize = n;

  // ヒーププロパティを立ち上げる
  for (int i = 0; i < arraySize; i++) {
    int node = i, parent = ((node - 1) >> 1);
    offset nodeValue = array[i];
    while ((node > 0) && (nodeValue > array[parent])) {
      array[node] = array[parent];
      node = parent;
      parent = ((node - 1) >> 1);
    }
    array[node] = nodeValue;
  }

  // 根から繰り返し取り上げて配列の末尾に挿入する
  while (arraySize > 1) {
    offset toInsert = array[--arraySize];
    array[arraySize] = array[0];
    int node = 0, child = 1;
    while (child < arraySize) {
      if (child + 1 < arraySize) {
        if (array[child + 1] > array[child])
          child++;
      }
      if (toInsert >= array[child])
        break;
      array[node] = array[child];
      node = child;
      child = node + node + 1;
    }
    array[node] = toInsert;
  }

  // 降順の場合はソート処理の最後に逆向きにする操作を行う
  if (!ascending) {
    int middle = (n >> 1);
    for (int j = 0, k = n - 1; j < middle; j++, k--) {
      offset tmp = array[j];
      array[j] = array[k];
      array[k] = tmp;
    }
  }
}

/*
RadixSortを使ってポスティングの集合をソートする。
ポスティングは"ascending"の値を基準にソートする
*/
static void radixSortPostings(offset *array, int n, bool ascending) {
  static const int BITS_PER_PASS = 6;
  static const int BUCKETS = (1 << BITS_PER_PASS);
  static const int MAX_BUCKET = (BUCKETS - 1);
  static const int PASSES = ((8 * sizeof(offset)) / BITS_PER_PASS);
  assert(PASSES % 2 == 0);
  assert((MAX_OFFSET >> (PASSES * BITS_PER_PASS)) == 0);

  int cnt[PASSES][BUCKETS];
  memset(cnt, 0, sizeof(cnt));
  for (int k = 0; k < n; k++) {
    offset value = array[k];
    for (int i = 0; i < PASSES; i++) {
      cnt[i][value & MAX_BUCKET]++;
      value >>= BITS_PER_PASS;
    }
  }

  for (int i = 0; i < PASSES; i++) {
    int *c = cnt[i];
    if (ascending) {
      c[MAX_BUCKET] = n - c[MAX_BUCKET];
      for (int k = MAX_BUCKET - 1; k >= 0; k--)
        c[k] = c[k + 1] - c[k];
      assert(c[0] == 0);
    } else {
      c[0] = n - c[0];
      for (int k = 1; k <= MAX_BUCKET; k++)
        c[k] = c[k - 1] - c[k];
      assert(c[MAX_BUCKET] == 0);
    }
  }

  offset *temp = typed_malloc(offset, n);
  for (int i = 0; i < PASSES; i++) {
    int *c = cnt[i];
    int shift = i * BITS_PER_PASS;
    for (int k = 0; k < n; k++) {
      offset value = array[k];
      int bucket = ((value >> shift) & MAX_BUCKET);
      temp[c[bucket]++] = value;
    }
    offset *tmp = temp;
    temp = array;
    array = tmp;
  }
  free(temp);
}

void sortOffsetsAscending(offset *array, int length) {
  if (length > 256)
    heapSortPostings(array, length, true);
  else
    radixSortPostings(array, length, true);
}

void sortOffsetsDescending(offset *array, int length) {
  if (length < 256)
    heapSortPostings(array, length, false);
  else
    radixSortPostings(array, length, false);
}

int sortOffsetsAscendingAndRemoveDuplicates(offset *array, int length) {
  if (length <= 1)
    return length;
  sortOffsetsAscending(array, length);
  int result = 1;
  for (int i = 1; i < length; i++) {
    if (array[i] != array[i - 1])
      array[result++] = array[i];
  }
  return result;
}

void ASSERT_ASCENDING(offset *array, int length) {
  for (int i = 0; i < length - 1; i++)
    assert(array[i] < array[i + 1]);
}

static std::map<std::string, int64_t> globalCounters;

bool getGlobalCounter(const char *name, int64_t *value) {
  std::map<std::string, int64_t>::iterator iter = globalCounters.find(name);
  if (iter == globalCounters.end())
    return false;
  *value = iter->second;
  return true;
}

void setGlobalCounter(const char *name, int64_t value) {
  globalCounters[name] = value;
}