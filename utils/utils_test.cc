#include <iostream>
#include <cstdio>

#include "utils.h"

int main() {
    char *s;

    s = " aaa b c ";
    char *result = chop(s);

    std::cout << result << std::endl;
}