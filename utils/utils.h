#ifndef __UTILS_H
#define __UTILS_H

#include <cstring>
#include <cstdio>
#include <sys/types.h>

#define SEM_INIT(SEM, CNT) if (sem_init(&SEM, 0, CNT) < 0) { \
    log(LOG_ERROR, LOG_ID, "Failed to initialize semaphore: " __STRING(SEM) " (" __FILE__ ")"); \
    assert(false); \
}

char *chop(char *s);

void collapsePath(char *path);

#define duplicateString(s) duplicateString3(s, __FILE__, __LINE__)
char *duplicateString3(const char *s, const char *file, int line);

unsigned int simpleHashFunction(const char *string);

char *concatenateStrings(const char *s1, const char *s2);

bool startsWith(const char *longString, const char *shortString, bool caseSensitive = true);

char *evaluateRelativePathName(const char *dir, const char *file);

#endif