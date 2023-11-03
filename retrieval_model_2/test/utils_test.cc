#include "../misc/backend.h"
#include <cstring>
#include <cstdlib>
#include <iostream>

static const int INITIAL_HASHTABLE_SIZE = 1024;

int main() {
  char *string_data = "hello world!!";
  char *chop_data = chop(string_data);
  unsigned int hash_data = simpleHashFunction(string_data) % INITIAL_HASHTABLE_SIZE;

  char *string_data2 = " aaaaa   b     ";
  char *dupString = duplicateAndTrim(string_data2);

  char *s1 = "this is ";
  char *s2 = "a pen.";
  char *concatenate = concatenateStrings(s1, s2);
  s1 = duplicateString(s1);
  s2 = duplicateString(s2);
  char *concatenateFree = concatenateStringsAndFree(s1, s2);
  
  char *s3 = "japanese";
  char *subString = getSubstring(s3, 1, 3);

  char *file = "test.txt";
  char *dir = "/usr/local/ham//";
  char *newPath = evaluateRelativePathName(dir, file);

  // char *url = "http://hoge.com/www/var/index.html";
  char *text = "aaa bbb  cCc   DDD";
  char *norm = normalizeString(text);

  std::cout << chop_data << std::endl;
  std::cout << hash_data << std::endl;
  std::cout << dupString << std::endl;
  std::cout << concatenate << std::endl;
  std::cout << concatenateFree << std::endl;
  std::cout << subString << std::endl;
  std::cout << newPath << std::endl;
  std::cout << norm << std::endl;
}