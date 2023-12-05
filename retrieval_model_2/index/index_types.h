#ifndef INDEX_TYPES_H
#define INDEX_TYPES_H

// アドレスやエクステントなどの基本的なデータ構造と型の定義
// cf: https://en.wikipedia.org/wiki/Extent_(file_systems)

#include <cmath>
#include <cinttypes>
#include <cstdint>
#include <sys/stat.h>
#include <sys/types.h>
#include "../config/config.h"
#include "../misc/macros.h"

// 関数のステータスコード
static const int RESULT_SUCCESS = 0;
static const int RESULT_ERROR = 1;

// インデックスのエラーコード
// クエリシンタックスエラー
static const int ERROR_SYNTAX_ERROR = 2;
// シャットダウン状態でクエリ処理を行おうとするとき
static const int ERROR_SHUTTING_DOWN = 3;
// 通常のファイルでないものにインデックスをつけようとするとき
static const int ERROR_NO_SUCH_FILE = 4;
// 隠れディリクトリ内でインデクスをつけようとするとき
static const int ERROR_DIR_NOT_ALLOWED = 5;
// 適切な FilteredInputStream 実装が見つからない場合
static const int ERROR_UNKNOWN_FILE_FORMAT = 6;
// 入力tokenizerが0トークンであると出力する
static const int ERROR_EMPTY_FILE = 7;

static const int MAX_ERROR_CODE = 14;

extern const char *ERROR_MESSAGES[MAX_ERROR_CODE + 2];

#define printErrorMesssage(statusCode, buffer) { \
  if (statusCode < 0) strcpy(buffer, "Error"); \
  else if (statusCode > MAX_ERROR_CODE) strcpy(buffer, "Error"); \
  else strcpy(buffer, ERROR_MESSAGES[statusCode]); \
}

using byte = unsigned char;

#if INDEX_OFFSET_BITS == 64
  using offset = int64_t;
  static const offset MY_HELPER_123 = 999999999;
  static const offset MAX_OFFSET = MY_HELPER_123 * MY_HELPER_123;
  #define OFFSET_FORMAT "%lld"
#elif INDEX_OFFSET_BIST == 32
  using offset = int32_t;
  static const offset MAX_OFFSET = 2147000000;
  #define OFFSET_FORMAT "%d"
#else
  #error "Size of index offsets undefined, Choose either 32 or 64"
#endif

static const int MAX_INT = 0x7FFFFFFF;
static const offset ONE = 1;
static const offset TWO = 2;

/*
ドキュメントレベルのインデックスを用いる場合、
与えられた用語のTFを各ポスティングの最下位Kビットに符号化する。
この符号化の方法によリKの値はTFの最大値に対応する。
ここでのTFの最大値は0.31で定義される。
*/
static const int DOC_LEVEL_SHIFT = 5;
static const offset DOC_LEVEL_MAX_TF = 0x1F;
static const offset DOC_LEVEL_ENCODING_THRESHOLD = 0x10;
static const double DOC_LEVEL_ENCODING_THRESHOLD_DOUBLE = DOC_LEVEL_ENCODING_THRESHOLD;
static const double DOC_LEVEL_BASE = 1.15;

static inline offset encodeDocLevelTF(offset tf) {
  if (tf < DOC_LEVEL_ENCODING_THRESHOLD)
    return tf;
  long result = DOC_LEVEL_ENCODING_THRESHOLD + 
    LROUND(log(tf / DOC_LEVEL_ENCODING_THRESHOLD_DOUBLE) / log(DOC_LEVEL_BASE));
  if (result > DOC_LEVEL_MAX_TF)
    return DOC_LEVEL_MAX_TF;
  else
    return result;
}

static inline offset decodeDocLevelTF(offset encode) {
  if (encode < DOC_LEVEL_ENCODING_THRESHOLD)
    return encode;
  double resultDouble = pow(DOC_LEVEL_BASE, encode - DOC_LEVEL_ENCODING_THRESHOLD);
  return LROUND(DOC_LEVEL_ENCODING_THRESHOLD + resultDouble);
}

// デフォルトのファイル権限
static const mode_t DEFAULT_FILE_PERMISSIONS = S_IWUSR | S_IRUSR | S_IRGRP;

// デフォルトのディクトリ権限
static const mode_t DEFAULT_DIRECTORY_PERMISSIONS = S_IWUSR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP;

struct Extent{
  offset from;
  offset to;
};

/*
インデックスの範囲から、もとのテキストを再構築するときに用いる。
(statictical feedback, display of search resultsなどで用いる)
*/
struct TokenPositionPair {

  // 流れてくるトークンの番号
  uint32_t sequenceNumber;

  //　ファイル内のトークンの始まりはどこからか？
  off_t filePosition;
};

struct LongLongPair {
  long long first;
  long long second;
};

void sortArrayOfLongLongPairsByFirst(LongLongPair *array, int n);

void sortArrayOfLongLongPairsBySecond(LongLongPair *array, int n);

// 与えられたオフセットのリストを昇順にソートする
void sortOffsetsAscending(offset *array, int length);

// 与えられたオフセットのリストを降順にソートする
void sortOffsetsDescending(offset *array, int length);

/*
sortOffsetsAscendingと同様であるが、重複部分は削除する。
重複部分を削除後のポスティングリストの番号を出力する。
*/
int sortOffsetsAscendingAndRemoveDuplicates(offset *array, int length);

/*
2つのプログラム間の整数値を交換するのに用いる
*/
bool getGlobalCounter(const char *name, int64_t *value);
void setGlobalCounter(const char *name, int64_t value);

void ASSERT_ASCENDING(offset *array, int length);

#endif