#ifndef IO_H
#define IO_H

#include <unistd.h>
#include <sys/types.h>
#include "backend.h"

void getReadWriteStatistics(long long *bytesRead, long long *bytesWritten);

#define forced_read(fb, buf, count) forced_read5(fb, buf, count, __FILE__, __LINE__)

#define forced_write(fb, buf, count) forced_write5(fb, buf, count, __FILE__, __LINE__)

/*
"fd"から"count"バイトを"buf"に読み込む。
中断された場合、"read"の呼び出しを繰り返し、読み込んだバイト数を出力する。
*/
int forced_read3(int fb, void *buf, size_t count);

int forced_read5(int fd, void *buf, size_t count, const char *file, int line);

/*
"buf"から”count"バイトを"fd"に書き込む。
中断された場合、"write"の呼び出しを繰り返し、読み込んだバイト数を出力する。
*/
int forced_write3(int fd, const void *buf, size_t count);

int forced_write5(int fd, const void *buf, size_t count, const char *file, int line);

#define forced_ftruncate(fd, length) forced_ftruncate4(fd, length, __FILE__, __LINE__)

void forced_ftruncate4(int fd, off_t length, const char *file, int line);

#endif