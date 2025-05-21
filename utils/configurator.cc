#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "configurator.h"

#define HASHTABLE_SIZE 257

typedef struct {
    char *key;
    char *value;
    void *next;
} KeyValuePair;

static KeyValuePair *hashTable[HASHTABLE_SIZE];

static KeyValuePair configurationData[1024];
static int kvpCount = 0;

static const int CONFIG_VALUE_BUFFER_SIZE = MAX_CONFIG_VALUE_LENGTH + 256;
static char *currentBuffer = nullptr;
static int currentBufferPos = CONFIG_VALUE_BUFFER_SIZE;

static char *addToBuffer(char *string) {
    int len = strlen(string);
    if (currentBufferPos + len >= CONFIG_VALUE_BUFFER_SIZE) {
        currentBuffer = (char *)malloc(CONFIG_VALUE_BUFFER_SIZE);
        currentBufferPos = 0;
    }
    char *result = &currentBuffer[currentBufferPos];
    strcpy(result, string);
    currentBufferPos += (len + 1);
    return result;
}