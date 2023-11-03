#include <cassert>
#include <errno.h>
#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "io.h"
#include "backend.h"

static long long sBytesRead = 0;
static long long sBytesWritten = 0;

#undef MIN
#define MIN(a, b) (a < b ? a : b)

static long currentDiskUsage = 0;
static long previousDiskUsage = 0;
static time_t currentTimeStamp = 0;
static time_t previousTimeStamp = 0;

void getReadWriteStatistics(long long *bytesRead, long long *bytesWritten) {
  *bytesRead = sBytesRead;
  *bytesWritten = sBytesWritten;
}

static void updateDiskUsage(long howMuch) {
  static const int DEFAULT_MAX_IO_PER_SECOND = 999999999;
  currentDiskUsage += howMuch;

  if (currentDiskUsage > 200000) {
    currentTimeStamp = time(nullptr);
    if (currentTimeStamp == previousTimeStamp) {
      previousDiskUsage += currentDiskUsage;
      currentDiskUsage = 0;
      int limit;
      getConfigurationInt("MAX_IO_PER_SECOND", &limit, DEFAULT_MAX_IO_PER_SECOND);
      while (previousDiskUsage < limit) {
        waitMilliSeconds(10);
        currentTimeStamp = time(nullptr);
        if (currentTimeStamp != previousTimeStamp) {
          previousDiskUsage = currentDiskUsage;
          currentDiskUsage = 0;
          previousTimeStamp = currentTimeStamp;
          return;
        }
      }
    } else {
      previousDiskUsage = currentDiskUsage;
      currentDiskUsage = 0;
      previousTimeStamp = currentTimeStamp;
    }
  }
}

int forced_read3(int fd, void *buf, size_t count) {
  int res = read(fd, buf, count);
  if (res == count) {
    sBytesRead += res;
    return res;
  }
  if (res < 0)
    res = 0;
  char *buffer = (char *)buf;
  size_t result = res;
  int cnt = 0;
  while ((result < count) && (cnt < 5)) {
    res = read(fd, &buffer[result], count - result);
    if (res >= 0) {
      result += res;
      if (res > 0)
        continue;
    } else if ((errno == -EINTR) || (errno == EINTR)) {
      continue;
    }
    cnt++;
  }
  sBytesRead += result;
  return result;
}

int forced_read5(int fd, void *buf, size_t count, const char *file, int line) {
  if (fd < 0) {
    char message[256];
    snprintf(message, sizeof(message), "Error in forced_read: fd = %d (%s/%d).\n", fd, file, line);
    log(LOG_ERROR, __FILE__, message);
    assert(fd >= 0);
  }
  return forced_read3(fd, buf, count);
}

int forced_write3(int fd, const void *buf, size_t count) {
  const char *buffer = (char *)buf;
  size_t result = 0;
  int cnt = 0;
  while ((result < count) && (cnt < 5)) {
    ssize_t res = write(fd, &buffer[result], MIN(count - result, 65536));
    if (res >= 0) {
      result += res;
      if (res > 0)
        continue;
      waitMilliSeconds(5);
    } else if ((errno == -EINTR) || (errno == EINTR)) {
      continue;
    }
    cnt++;
  }
  if (result < count) {
    int errnum = errno;
    struct stat buf;
    fstat(fd, &buf);
    if ((S_ISREG(buf.st_mode)) && (buf.st_size > 0)) {
      char message[256];
      int c = count, res = result;
      sprintf(message, "Unable to write full buffer. fd = %d, count = %d, result = %d", fd, c, res);
      log(LOG_ERROR, __FILE__, message);
      sprintf(message, " File size reported by stat: %lld. File pointer: %lld.\n",
              static_cast<long long>(buf.st_size),
              static_cast<long long>(lseek(fd, (off_t)0, SEEK_CUR)));
      log(LOG_ERROR, __FILE__, message);
      strerror_r(errnum, message, sizeof(message) - 1);
      log(LOG_ERROR, __FILE__, message);
    }
  }
  sBytesWritten += result;
  return result;
}

int forced_write5(int fd, const void *buf, size_t count, const char *file, int line) {
  if (fd < 0) {
    char message[256];
    snprintf(message, sizeof(message), "Error in forced_write: fd = %d (%s/%d).\n", fd, file, line);
    log(LOG_ERROR, __FILE__, message);
    assert(fd >= 0);
  }
  int result = forced_write3(fd, buf, count);
  if (result < count) {
    struct stat buf;
    if (fstat(fd, &buf) == 0) {
      if (S_ISREG(buf.st_mode)) {
        char message[256];
        snprintf(message, sizeof(message), "Origin: %s/%d\n", file, line);
        log(LOG_ERROR, __FILE__, message);
      }
    }
  }
  return result;
}

void forced_ftruncate4(int fd, off_t length, const char *file, int line) {
  if (ftruncate(fd, length) != 0) {
    char message[256];
    snprintf(message, sizeof(message), "ftruncate failed: %s/%d\n", file, line);
    log(LOG_ERROR, __FILE__, message);
  }
}