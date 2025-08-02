#ifndef __INDEX_TYPES_H
#define __INDEX_TYPES_H

#include <inttypes.h>
#include <cmath>
#include <sys/stat.h>
#include <sys/types.h>

using byte = unsigned char;

using offset = int64_t;
static const offset MAX_OFFSET = (1LL << 47) - 1;
#define OFFSET_FORMAT "%" PRId64

static const int MAX_INT = 0x7FFFFFFF;
static const offset ONE = 1;
static const offset TWO = 2;

// 初期ファイルの権限(インデックスファイルが作られたときに使う)
static const mode_t DEFAULT_FILE_PERMISSIONS = S_IWUSR | S_IRUSR | S_IRGRP;

#endif