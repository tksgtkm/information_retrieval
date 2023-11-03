#include <cstdio>
#include <cstring>
#include <time.h>
#include <iostream>
#include "logging.h"
#include "alloc.h"

static int sLogLevel = LOG_OUTPUT;

// for debug
// static int sLogLevel = 0;


static FILE *sOutputStream = stderr;

void log(int logLevel, const char *logID, const char *message) {
  if (logLevel < sLogLevel)
    return;
  char buffer[256];
  time_t now = time(nullptr);
  ctime_r(&now, buffer);
  if (buffer[strlen(buffer) - 1] == '\n')
    buffer[strlen(buffer) - 1] = 0;
  switch(logLevel) {
    case LOG_DEBUG:
      fprintf(sOutputStream, "(DEBUG) [%s] [%s] %s\n", logID, buffer, message);
      break;
    case LOG_OUTPUT:
      fprintf(sOutputStream, "(OUTPUT) [%s] [%s] %s\n", logID, buffer, message);
      break;
    case LOG_ERROR:
      fprintf(sOutputStream, "(ERROR) [%s] [%s] %s\n", logID, buffer, message);
      break;
  }
}

void setLogLevel(int logLevel) {
  sLogLevel = logLevel;
}

void setLogOutputStream(FILE *outputStream) {
  sOutputStream = outputStream;
}