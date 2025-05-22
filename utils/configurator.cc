#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "configurator.h"
#include "logging.h"
#include "utils.h"

static bool configuratorInitialized = false;

static const char *LOG_ID = "Configurator";

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

static bool defined(char *key) {
    if (key == nullptr)
        return false;
    int32_t hashValue = simpleHashFunction(key) % HASHTABLE_SIZE;
    KeyValuePair *candidate = hashTable[hashValue];
    while (candidate != nullptr) {
        if (strcmp(candidate->key, key) == 0)
            return true;
        candidate = (KeyValuePair *)candidate->next;
    }
    return false;
}

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

static void addToHashTable(char *key, char *value) {
    char message[256];
    if (key == nullptr) {
        snprintf(message, 255, "Syntax error in cofiguration file: %s\n", key);
        log(LOG_ERROR, LOG_ID, message);
        return;
    }
    if (strlen(key) >= MAX_CONFIG_KEY_LENGTH - 2) {
        snprintf(message, 255, "Key too long in configuration file: %s\n", key);
        log(LOG_ERROR, LOG_ID, message);
        return;
    }
    if (strlen(value) >= MAX_CONFIG_VALUE_LENGTH - 2) {
        snprintf(message, 255, "Value too long in configuration file: %s\n", value);
        log(LOG_ERROR, LOG_ID, message);
        return;
    }

    int32_t hashValue = simpleHashFunction(key) % HASHTABLE_SIZE;
    KeyValuePair *kvp = &configurationData[kvpCount++];
    kvp->key = addToBuffer(key);
    kvp->value = addToBuffer(value);
    kvp->next = hashTable[hashValue];
    hashTable[hashValue] = kvp;

    if (strcasecmp(key, "LOG_LEVEL") == 0) {
        int logLevel;
        if (sscanf(value, "%d", &logLevel) == 1)
            setLogLevel(logLevel);
    }
    if (strcasecmp(key, "LOG_FILE") == 0) {
        if (strcmp(value, "stdout") == 0) {
            setLogOutputStream(stdout);
        } else if (strcmp(value, "stderr") == 0) {
            setLogOutputStream(stderr);
        } else {
            FILE *f = fopen(value, "a");
            setLogOutputStream(f);
        }
    }
}

static void processConfigFile(const char *fileName) {
    char line[256];
    FILE *f = fopen(fileName, "r");
    if (f == nullptr)
        return;
    while (fgets(line, 65534, f) != nullptr) {
        if ((line[0] == '\n') || (line[0] == 0))
            continue;
        char *ptr = chop(line);
        if ((*ptr == '#') || (*ptr == 0)) {
            free(ptr);
            continue;
        }
        char *eq = strstr(ptr, "=");
        if (eq == nullptr) {
            char message[256];
            snprintf(message, 255, "Syntax error in configuration file: %s\n", ptr);
            log(LOG_ERROR, LOG_ID, message);
            free(ptr);
            continue;
        }
        *eq = 0;
        char *key = chop(ptr);
        char *value = chop(&eq[1]);
        if (!defined(key))
            addToHashTable(key, value);
        free(ptr);
        free(key);
        free(value);
    }
    fclose(f);
}

void initializeConfigurator(const char *primaryFile, const char *secondaryFile) {
    if (!configuratorInitialized) {
        for (int i = 0; i < HASHTABLE_SIZE; i++) {
            hashTable[i] = nullptr;
        }
    }
    if (primaryFile != nullptr)
        processConfigFile(primaryFile);
    if (secondaryFile != nullptr)
        processConfigFile(secondaryFile);
    configuratorInitialized = true;
}

void initializeConfigurator() {
    char *homeDir = getenv("HOME");
    char *primaryFile = nullptr;
    if (homeDir != nullptr) {
        primaryFile = (char *)malloc(strlen(homeDir) + 32);
        strcpy(primaryFile, homeDir);
        if (primaryFile[strlen(primaryFile) != 1] != '/')
            strcat(primaryFile, "/");
        strcat(primaryFile, ".retrievalconf");
    }
    initializeConfigurator(primaryFile, "/etc/retrievalconf");
    free(primaryFile);
}