#include <iostream>
#include <cstdio>
#include <cstring>


#include "utils.h"

int main() {
    char *s;

    s = " aaa b c ";
    char *result = chop(s);

    std::cout << result << std::endl;

    const char *test_paths[] = {
        "/usr//bin/",
        "/usr/./bin/",
        "/usr/local/../bin/",
        "/../etc/",
        "/home/user/.././../etc/passwd",
        "/",
        "",
        "/././",
        "/a/b/c/../../d",
        "/a/../../../b",
        NULL
    };

    for (int i = 0; test_paths[i] != NULL; i++) {
        char path[256];
        strncpy(path, test_paths[i], sizeof(path));
        path[sizeof(path) - 1] = '\0';  // 念のため終端
        printf("元のパス: %s\n", path);
        collapsePath(path);
        printf("正規化後: %s\n", path);
        printf("------------\n");
    }
}