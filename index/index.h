#ifndef __INDEX_H
#define __INDEX_H

#include "../utils/all.h"

class Index {

public:
    static const int TYPE_INDEX = 1;
    static const int TYPE_MASTERINDEX = 2;
    static const int TYPE_FAKEINDEX = 3;

    static const int64_t DEFAULT_MAX_FILE_SIZE = 20000000000LL;
    configurable int64_t MAX_FILE_SIZE;

    static const int64_t DEFAULT_MIN_FILE_SIZE = 8;
    configurable int64_t MIN_FILE_SIZE;

    static const int DEFAULT_MAX_SIMULTANEOUS_READERS = 4;
    configurable int MAX_SIMULTANEOUS_READERS;

    static const int DEFAULT_STEMMING_LEVEL = 0;
    configurable int STEMING_LEVEL;

    static const bool DEFAULT_APPLY_SECURITY_RESTRICTIONS = true;
    configurable bool APPLY_SECURITY_RESTRICTIONS;

    static const int DEFAULT_TCP_PORT = -1;
    configurable int TCP_PORT;

    static const bool DEFAULT_MONITOR_FILESYSTEM = false;
    configurable bool MONITOR_FILESYSTEM;

    static const int DEFAULT_DOCUMENT_LEVEL_INDEXING = 0;
    configurable int DOCUMENT_LEVEL_INDEXING;

    static const bool DEFAULT_ENABLE_XPATH = false;
    configurable bool ENABLE_XPATH;

    static const bool DEFAULT_BIGRAM_INDEXING = false;
    configurable bool BIGRAM_INDEXING;

    static const uid_t SUPERUSER = (uid_t)0;

    static const uid_t GOD = (uid_t)-1;

    static const uid_t NOBODY = (uid_t)-2;

    static const char *TEMP_DIRECTORY;

    static const char *LOG_ID;

public:

    Index();

protected:

    virtual void getConfiguration();
};

#endif