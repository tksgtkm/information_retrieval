#ifndef __LOGGING_H
#define __LOGGING_H

#include <cstdio>
#include <string>
#include <cstring>

#define LOG_DEBUG  1
#define LOG_OUTPUT 2
#define LOG_ERROR  3

void log(int logLevel, const char *logID, const char *message);

void log(int logLevel, const std::string& logID, const std::string& message);

void setLogLevel(int logLevel);

void setLogOutputStream(FILE *outputStream);

#endif