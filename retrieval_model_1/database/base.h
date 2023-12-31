#ifndef BASE_H
#define BASE_H

#include <iostream>
#include <cmath>
#include <cassert>

// リレーション名または属性名の最大長
#define MAXNAME 24
// 文字型の属性の最大長
#define MAXSTRINGLEN 255
// 関係する属性の最大数
#define MAXATTRS 40
// INSERTコマンド単体の属性の最大数
#define MAXINSERTATTRS 1024

// #define yywrap() 1
inline static int yywrap() {
  return 1;
}
void yyerror(const char *);

// 結果のコード
typedef int RC;

// 正常終了時のコード
#define OK_RC 0

#define START_PF_ERR -1

const int ALL_PAGES = -1;

#endif