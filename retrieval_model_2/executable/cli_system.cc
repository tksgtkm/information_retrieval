#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>

#include "../misc/backend.h"

#define TOTAL_BUFFER_SIZE (256 * 1024 * 1024)

#define MERGE_BUFFER_SIZE TOTAL_BUFFER_SIZE

static void createEmptyIndex(int argc, char **argv) {
  if (argc != 1) {
    fprintf(stderr, "Error: You have to specify exactly one output file\n");
    exit(1);
  }
  char *fileName = argv[0];
  // struct stat buf;
  // if (stat(fileName, &buf) == 0) {
  //   fprintf(stderr, "Error: Output file already exists\n");
  //   exit(1);
  // }

}

int main(int argc, char **argv) {
  std::cout << "Hi" << std::endl;
  return 0;
}
