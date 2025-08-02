#ifndef __UTILS_H
#define __UTILS_H

#include <cstring>
#include <cstdio>
#include <sys/types.h>

// セマフォの初期化
#define SEM_INIT(SEM, CNT) if (sem_init(&SEM, 0, CNT) < 0) { \
    log(LOG_ERROR, LOG_ID, "Failed to initialize semaphore: " __STRING(SEM) " (" __FILE__ ")"); \
    assert(false); \
}

/*
先頭と末尾の空白を削除した文字列を返す
メモリは呼び出し元で開放しなければいけない
*/
char *chop(char *s);

/*
受け取ったパス名から("//", "/./", "/../")をすべて削除する
*/
void collapsePath(char *path);

// 文字列sのコピーを作り、新しい文字列としてポインターを返す
#define duplicateString(s) duplicateString3(s, __FILE__, __LINE__)
char *duplicateString3(const char *s, const char *file, int line);

// 文字列のハッシュ値を返す簡単なハッシュ関数
unsigned int simpleHashFunction(const char *string);

/*
２つの文字列を結合して一つの文字列を作成する
メモリは呼び出し元で開放しなければいけない
*/
char *concatenateStrings(const char *s1, const char *s2);

// 「shortString」が「longString」の接頭辞である場合にtrueを返す
bool startsWith(const char *longString, const char *shortString, bool caseSensitive = true);

// fileを相対パスと解釈してdirに適用することで新しいパスを作成する
// メモリは呼び出し元で開放しなければいけない
char *evaluateRelativePathName(const char *dir, const char *file);

#endif