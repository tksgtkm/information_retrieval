#ifndef UTIL_H
#define UTIL_H

#include <cstdio>
#include <string>
#include <sys/types.h>
#include <semaphore.h>
#include "macros.h"

#ifndef MAX
  #define MAX(a, b) (a > b ? a : b)
#endif
#ifndef MIN
  #define MIN(a, b) (a < b ? a : b)
#endif

// sem_init: 名前無しセマフォを初期化する
#define SEM_INIT(SEM, CNT) if (sem_init(&SEM, 0, CNT) < 0) { \
  log(LOG_ERROR, LOG_ID, "Failed to initialize semaphore: " __STRING(SEM) " (" __FILE__ ")"); \
  assert(false); \
}

#define log2(x) (log(x) / lg(2))

static const int SECONDS_PER_DAY = 24 * 3600;

static const int MILLISECONDS_PER_DAY = 24 * 3600 * 1000;

/*
完全修飾パス名("path") を受け取り、正規のパス名でない部分パス('//', '/./', '/.../')
をすべて削除する
*/
void collapsePath(char *path);

/*
XXXXXX...という形式のファイル名パターンを受け取り、
Xガ出現する部分をすべてランダムに変換する。
*/
void randomTempFileName(char *pattern);

/*
ミリ秒感スレッその実行を一時停止する
*/
void waitMilliSeconds(int ms);

// 現在の時刻をミリ秒単位で返す
int currentTimeMillis();

// currentTimeMillisと同じ、ただし基本単位は秒になる
double getCurrentTime();

// 文字列のハッシュ値として、符号なし整数値を返す
unsigned int simpleHashFunction(const char *string_data);

// 階乗n!を出力する
double stirling(double n);

// 入力した文字列のコピーを作成して出力し、ポインターも一緒に出力する
#define duplicateString(s) duplicateString3(s, __FILE__, __LINE__)
char *duplicateString3(const char *s, const char *file, int line);

void toLowerCase(char *s);

// 文字列のトリミングを行う
void trimString(char *s);

// "s"の複製を作成し、ヘッダーと末尾の空白を削除する
char *duplicateAndTrim(const char *s);

// ２つの文字列を結合させて3つめの文字列を生成する
char *concatenateStrings(const char *s1, const char *s2);

// 3つの文字列を結合させる
char *concatenateStrings(const char *s1, const char *s2, const char *s3);

// concatenateStringsと同じ、ただしs1とs2の文字列を開放させる
char *concatenateStringsAndFree(char *s1, char *s2);

/*
与えられたファイルパスの最後のコンポーネントを返す。
"copy"がtrueならば、返されるポインタは新しく割り当てられたメモリを参照し、
呼び出し元によって開放される必要がある。
そうでなければ、ファイルパスそのものを指し示して
"s"の部分文字列を"start"(0から数える)から始まり、"end"の直前で終わるようにして返す。
メモリは呼び出し側で開放する必要がある。
*/
char *getSubstring(const char *s, int start, int end);

// "longString"の接頭辞が"shortString"ならtrue
bool startsWith(const char *longString, const char *shortString,
                bool caseSensitive = true);

bool endsWith(const char *longString, int longLength,
              const char *shortString, int shortLength,
							bool caseSensitive = true);
/*
"file"を相対パスとして捉えて、"dir"に適用して新しいパスを作成する。
メモリは呼び出し側で開放する必要がある。
*/
char *evaluateRelativePathName(const char *dir, const char *file);

// evaluateRelativePathNameと同じだがディリクトリであう必要はない
char *evaluateRelativeURL(const char *base, const char *link);

/*
与えられたファイルパスの最後のコンポーネントを返す。
"copy"がtrueならば、返されるポインタは新しく割り当てられたメモリを参照し、
呼び出し元によって開放される必要がある。
そうでなければ、ファイルパスそのものを指し示しており、改変できない
*/
char *extractLastComponent(const char *filePath, bool copy);

// URLを取得し、比較しやすいように正規化された形式に変換する。
void normalizeURL(char *url);

/*
文字列を受け取り、すべての句読点を空白文字に置き換え、残りを小文字にする。
連続した複数の空白文字を一つの空白文字に置き換える。
*/
char *normalizeString(char *s);
void normalizeString(std::string *s);

/*
"o"で指定されたオフセットをwhereで参照されるメモリにプリントし、whereへのポインを返す。
where == nullの場合、新しいメモリが確保され、そのメモリへのポインタが返される。
*/
char *printOffset(int64_t o, char *where);

void printOffset(int64_t o, FILE *stream);

// 先頭と末尾の空白を除去した文字列を返す。
// メモリは呼び出し側で開放させる
char *chop(char *s);

/*
入力した文字列が"pattern"のマッチする場合、trueを返す。
patternは任意のワイルドカード文字列であり、"?"と"*"を含んでいる。
*/
bool mathesPattern(const char *string_data, const char *pattern);

// qsortとdouble配列の比較を行う。qsortは降順でソートする
int doubleComparator(const void *a, const void *b);

// 文字列"s"が整数値ならばtrueを返す
bool isNumber(const char *s);

// string_data内のoldCharを一つかより万遍なくnewCharに置き換える
void replaceChar(char *string_data, char oldChar, char newChar, bool replaceAll);

bool compareNumbers(double a, double b, const char *comparator);

// 与えられた整数値の中の1bitsの値を返す
int getHammingWeight(unsigned int n);

// n_choose_kで使用する
double logFactorial(int n);

double n_choose_k(int n, int k);

// ファイルが存在するか
bool fileExists(const char *fileName);

// ファイルサイズを出力する。存在しないファイルなら-1を返す
int64_t getFileSize(const char *fileName);

void getNextNonCommentLine(FILE *f, char *buffer, int bufferSize);

#endif
