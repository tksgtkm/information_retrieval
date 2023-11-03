#include <cassert>
#include <signal.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include "../misc/backend.h"

static char *searchengineDir;
static char *homeDir;

static pid_t inotifyPID = 0;
static pid_t transformPID = 0;
static pid_t searchenginePID = 0;
static pid_t httpdPID = 0;

int main(int argc, char **argv) {
  searchengineDir = duplicateString(argv[0]);
  int len = strlen(searchengineDir);
  if (len > 0) {
    while (searchengineDir[len - 1] != '/')
      searchengineDir[--len] = 0;
    
  }
}