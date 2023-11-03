#ifndef LOGGING_H
#define LOGGING_H

#include <cstdlib>
#include "backend.h"

#define LOG_DEBUG  1
#define LOG_OUTPUT 2
#define LOG_ERROR  3

void log(int logLevel, const char *logID, const char *message);

void setLogLevel(int logLevel);

void setLogOutputStream(FILE *outStream);

#endif
