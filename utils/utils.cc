#include <stdio.h>
#include <cstdlib>
#include <cstring>

#include "utils.h"

char *chop(char *s) {
    if (s == nullptr)
        return nullptr;
    while ((*s > 0) && (*s <= ' '))
        s = &s[1];
    s = duplicateString(s);
    int len = strlen(s);
    while ((len > 1) && (s[len - 1] > 0) && (s[len - 1] <= ' '))
        len = len - 1;
    s[len] = 0;
    return s;
}

char *duplicateString3(const char *s, const char *file, int line) {
    if (s == nullptr)
        return nullptr;
    char *result = (char *)malloc(strlen(s) + 1);
    
    strcpy(result, s);
    return result;
}