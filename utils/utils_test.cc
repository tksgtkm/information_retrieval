#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <iomanip>

#include "utils.h"

int main() {
    char *s;

    s = " aaa b c ";
    char *result = chop(s);

    std::cout << result << std::endl;
    
    // collapsePath test
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

    // simpleHashFunction test
    std::vector<std::string> test_cases = {
        "",
        "a",
        "abc",
        "ABC",
        "The quick brown fox jumps over the lazy dog",
        "the quick brown fox jumps over the lazy dog",
        "1234567890",
        "!@#$%^&*()_+",
        "同じ文字列",
        "違う文字列"
    };

    std::cout << std::hex << std::setfill('0');
    for (const auto& test_str : test_cases) {
        unsigned int hash = simpleHashFunction(test_str.c_str());
        std::cout << "hash(\"" << test_str << "\") = 0x" 
        << std::setw(8) << hash << std::endl;
    }
}