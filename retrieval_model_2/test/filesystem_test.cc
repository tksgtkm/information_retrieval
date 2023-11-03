#include "../filesystem/filesystem.h"
#include "../misc/backend.h"
#include <cstdlib>
#include <iostream>

#define TEST_FILES 13

int main(int argc, char **argv) {
  if (argc > 5) {
    fprintf(stderr, "Syntax:   filesystem DATA_FILE PAGE_SIZE PAGE_COUNT TEST_DATA\n\n");
    fprintf(stderr, "Creates a filesystem inside DATAFILE with given parameters and tests it using\n");
    fprintf(stderr, "Data taken from the file TEST_DATA\n\n");
    return 1;
  }

  argv[1] = "../../temp/data_file";
  argv[2] = "1024";
  argv[3] = "256";
  argv[4] = "../../temp/test_data";

  char *fileName = argv[1];
  int pageSize = atoi(argv[2]);
  int pageCount = atoi(argv[3]);
  char *testData = argv[4];
  FileSystem *fs = new FileSystem(fileName, pageSize, pageCount);
  if (!fs->isActive()) {
    fprintf(stderr, "Could not create filesystems.\n");
    return 1;
  }

  int inputFile = open(testData, O_RDONLY);

  printf("Open file: %d\n", inputFile);

  FileObject **f = new FileObject*[256];
  for (int i = 0; i < TEST_FILES; i++) {
    f[i] = new FileObject(fs);
    printf("new handle: %i fileCount: %i\n", f[i]->getHandle(), fs->getFileCount());
  }
  f[4]->deleteFile();
  delete f[4];
  f[4] = new FileObject(fs);
  printf("new handle: %i fileCount: %i\n", f[4]->getHandle(), fs->getFileCount());

  close(inputFile);
  delete fs;

  int cnt = 0;
  char buffer[373];
  int inputFileSize = 0;
  while (true) {
    int size = forced_read(inputFile, buffer, sizeof(buffer));
    if (size == 0)
      break;
    for (int i = 0; i < TEST_FILES; i++)
      f[i]->write(size, buffer);
    inputFileSize += size;
  }

  char *inputData = (char *)malloc(inputFileSize);
  lseek(inputFile, 0, SEEK_SET);
  forced_read(inputFile, inputData, inputFileSize);

  printf("Total pages: %i. Used: %i.\n", fs->getPageCount(), fs->getUsedPageCount());

  int64_t a, b, c, d;
  fs->getCacheEfficiency(&a, &b, &c, &d);
  printf("Cache efficiency: %i/%i reads, %i/%i writes.\n", (int)b, (int)a, (int)d, (int)c);

  for (int i = 0; i < TEST_FILES; i++) {
    lseek(inputFileSize, 0, SEEK_SET);
    f[i]->seek(0);
    printf("f[%i]->getSize() = %i\n", i, f[i]->getSize());
    for (int j = 0; j < f[i]->getSize(); j++) {
      char c;
      f[i]->read(1, &c);
      if (c != inputData[j])
        fprintf(stderr, "ERROR!\n");
    }
    printf(" File %i ok.\n", i);
  }

  printf("Before closing %i %i %i.\n", fs->getPageSize(), fs->getPageCount(), fs->getFileCount());
  // close file close filesystem
  for (int i = 0; i < TEST_FILES; i++)
    delete f[i];
  delete fs;

  for (int loop = 0; loop < 2; loop++) {
    fs = new FileSystem(fileName);
    printf("After reopening %i %i %i.\n", fs->getPageSize(), fs->getPageCount(), fs->getFileCount());
    for (int i = 0; i < TEST_FILES; i++) {
      lseek(inputFile, 0, SEEK_SET);
      f[i] = new FileObject(fs, i, false);
      f[i]->seek(0);
      for (int j = 0; j < f[i]->getSize(); j++) {
        char c;
        f[i]->read(1, &c);
        if (c != inputData[j])
          fprintf(stderr, "ERROR!\n");
      }
      delete f[i];
    }
    delete fs;
  }
  return 0;
}