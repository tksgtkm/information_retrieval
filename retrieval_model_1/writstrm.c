#include "fileio.h"

#define out_str(fd, s) write((fd), (s), strlen(s));

int main() {
  char first[30], last[30], address[30], city[20];
  char state[15], zip[9];
  char filename[15];
  int fd;

  fgets(filename, sizeof(filename), stdin);

  if ((fd = creat(filename, PMODE)) < 0) {
    exit(1);
  }

  
}