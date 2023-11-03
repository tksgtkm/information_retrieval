#ifndef ALLOC_H
#define ALLOC_H

#include <stdlib.h>
#include "../config/config.h"

#if ALLOC_DEBUG
  #define malloc(size) debugMalloc(size, __FILE__, __LINE__)
  #define free(ptr) debugFree(ptr, __FILE__, __LINE__)
  #define realloc(ptr, size) debugRealloc(ptr, size, __FILE__, _LINE__)
  #define typed_malloc(type, num) \
      (type*)debugMalloc((num) * sizeof(type), __FILE__, __LINE__)
  #define tyed_realloc(type, ptr, num) \
      ptr = (type *)debugRealloc(ptr, (num) * sizeof(type), __FILE__, __LINE__)
#else
  #define typed_malloc(type, num) (type*)malloc((num) * sizeof(type))
  #define typed_realloc(type, ptr, num) ptr = (type*)realloc(ptr, (num) * sizeof(type))
#endif

// 新しいアロケーションインスタンンスをハッシュテーブル内に追加する
void *debugMalloc(int size, const char *file, int line);

/*
解放しようとするメモリを記述するAllocationインスタンスを探す。
見つからなければstderrにエラーメッセージを書き込み、終了コードは-1を返す。
この機能はnullポインターを保存する。
*/
void debugFree(void *ptr, const char *file, int line);

void realFree(void *ptr);

void *debugRealloc(void *ptr, int size, const char *file, int line);

// 現在のタイムスタンプを出力する
int getAllocTimeStamp();

void printAllocationAfter(int timeStamp);

void printAllocations();

void printAllocationsBefore(int timeStamp);

int getAllocationCount();

int getAllocationSize();

long getMaxAllocated();

void setMaxAllocated(long newMax);

#endif
