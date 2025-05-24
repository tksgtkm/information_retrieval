#ifndef __UTILS_H
#define __UTILS_H

char *chop(char *s);

void collapsePath(char *path);

#define duplicateString(s) duplicateString3(s, __FILE__, __LINE__)
char *duplicateString3(const char *s, const char *file, int line);

unsigned int simpleHashFunction(const char *string);

char *concatenateStrings(const char *s1, const char *s2);

#endif