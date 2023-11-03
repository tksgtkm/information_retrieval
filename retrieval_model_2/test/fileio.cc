#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdio>
#include <unistd.h>

int main() {
  int dataFile;
  char *fileName = "../temp/data_file";
  dataFile = open(fileName, O_CREAT | O_TRUNC);

  if (dataFile == -1) {
    fprintf(stderr, "Could not open file:%s\n", fileName);
  }

  printf("Open file: %d\n", dataFile);

  close(dataFile);

  return 0;
}