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

#define START_PF_ERR  (-1)
#define END_PF_ERR    (-100)
#define START_RM_ERR  (-101)
#define END_RM_ERR    (-200)
#define START_IX_ERR  (-201)
#define END_IX_ERR    (-300)
#define START_SM_ERR  (-301)
#define END_SM_ERR    (-400)
#define START_QL_ERR  (-401)
#define END_QL_ERR    (-500)

#define START_PF_WARN  1
#define END_PF_WARN    100
#define START_RM_WARN  101
#define END_RM_WARN    200
#define START_IX_WARN  201
#define END_IX_WARN    300
#define START_SM_WARN  301
#define END_SM_WARN    400
#define START_QL_WARN  401
#define END_QL_WARN    500

#define START_PF_ERR -1

const int ALL_PAGES = -1;

// TRUE, FALSE and BOOLEAN
#ifndef BOOLEAN
typedef char Boolean;
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL 0
#endif

#endif