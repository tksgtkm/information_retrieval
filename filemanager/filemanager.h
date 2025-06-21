#ifndef __FILE_MANAGER_H
#define __FILE_MANAGER_H

#include "../index/index_type.h"

class Index;

typedef struct {
    offset startOffset;
    uint32_t tokenCount;
    int32_t delta;
} AddressSpaceChange;

class FileManager {
    friend class Index;

public:

    static const int INITIAL_TRANSACTION_LOG_SPACE = 8;

    static const int MINIMUM_SLOT_COUNT = 1024;

    static const double SLOT_GROWTH_RATE = 1.23;
};

#endif