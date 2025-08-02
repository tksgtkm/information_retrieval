#include <cstdlib>

#define typed_malloc(type, num) (type*)malloc((num) * sizeof(type))
#define typed_realloc(type, ptr, num) ptr = (type*)realloc(ptr, (num) * sizeof(type))