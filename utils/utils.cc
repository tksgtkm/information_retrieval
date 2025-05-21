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

void collapsePath(char *path) {
	char *slashSlash = strstr(path, "//");
	while (slashSlash != NULL) {
		for (int i = 1; slashSlash[i] != 0; i++)
			slashSlash[i] = slashSlash[i + 1];
		slashSlash = strstr(path, "//");
	}
	char *slashDotSlash = strstr(path, "/./");
	while (slashDotSlash != NULL) {
		for (int i = 2; slashDotSlash[i] != 0; i++)
			slashDotSlash[i - 1] = slashDotSlash[i + 1];
		slashDotSlash = strstr(path, "/./");
	}
	char *slashDotDotSlash = strstr(path, "/../");
	while (slashDotDotSlash != NULL) {
		if (slashDotDotSlash == path) {
			memmove(path, &path[3], strlen(path) - 3);
			slashDotDotSlash = strstr(path, "/../");
		}
		int pos = ((int)(slashDotSlash - path)) + 3;
		int found = -1;
		for (int i = pos - 4; i >= 0; i--)
			if (path[i] == '/') {
				found = i;
				break;
			}
		if (found < 0)
			break;
		strcpy(&path[found], &path[pos]);
		slashDotDotSlash = strstr(path, "/../");
	}
	int len = strlen(path);
	if (strcmp(&path[len - 3], "/..") == 0) {
		len = len - 3;
		path[len] = 0;
		if (len > 1) {
			while ((len > 1) && (path[len] != '/'))
				len--;
			path[len] = 0;
		}
	} else if (strcmp(&path[len - 2], "/.") == 0) {
        path[len - 2] = 0;
    }
	else if ((len > 2) && (path[len - 1] == '/')) {
        path[len - 1] = 0;
    }
	if (path[0] == 0)
		strcpy(path, "/");
} 

char *duplicateString3(const char *s, const char *file, int line) {
    if (s == nullptr)
        return nullptr;
    char *result = (char *)malloc(strlen(s) + 1);
    
    strcpy(result, s);
    return result;
}

