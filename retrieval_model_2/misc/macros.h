#ifndef __MACROS_H
#define __MACROS_H

#include <cmath>

#define isNAN(x) isnan(x)

#define isINF(x) isinf(x)

#define LROUND(x) ((long)(x + 0.5))

#define __STRING(x) #x

// 不要かもしれない
#define INDEX_MUST_BE_WORD_ALIGNED 0

#endif