#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include "configrator.h"
#include "backend.h"

static bool configratorInitialized = false;

static const char *LOG_ID = "Configrator";

#define HASHTABLE_SIZE 257

typedef struct {
  char *key;
  char *value;
  void *next;
} KeyValuePair;

static KeyValuePair *hashTable[HASHTABLE_SIZE];

static KeyValuePair configrationData[1024];
static int kvpCount = 0;

static const int CONFIG_VALUE_BUFFER_SIZE = MAX_CONFIG_KEY_LENGTH + 256;
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
    candidate = (KeyValuePair*)candidate->next;
  }
  return false;
}

static char *addToBuffer(char *string_data) {
  int len = strlen(string_data);
  if (currentBufferPos + len >= CONFIG_VALUE_BUFFER_SIZE) {
    currentBuffer = (char *)malloc(CONFIG_VALUE_BUFFER_SIZE);
    currentBufferPos = 0;
  }
  char *result = &currentBuffer[currentBufferPos];
  strcpy(result, string_data);
  currentBufferPos += (len + 1);
  return result;
}

static void addToHashTable(char *key, char *value) {
  char message[256];
  if (key == nullptr) {
    snprintf(message, 255, "Syntax error in configration file: %s\n", key);
    log(LOG_ERROR, LOG_ID, message);
    return;
  }
  if (strlen(key) >= MAX_CONFIG_KEY_LENGTH - 2) {
    snprintf(message, 255, "Key too long in configation file: %s\n", key);
    log(LOG_ERROR, LOG_ID, message);
    return;
  }
  if (strlen(key) >= MAX_CONFIG_VALUE_LENGTH - 2) {
    snprintf(message, 255, "Value too long in configration file: %s\n", value);
    log(LOG_ERROR, LOG_ID, message);
    return;
  }

  int32_t hashValue = simpleHashFunction(key) % HASHTABLE_SIZE;
  KeyValuePair *kvp = &configrationData[kvpCount++];
  kvp->key = addToBuffer(key);
  kvp->value = addToBuffer(value);
  kvp->next = hashTable[hashValue];
  hashTable[hashValue] = kvp;

  if (strcasecmp(key, "LOG_LEVEL") == 0) {
    int logLevel;
    if (sscanf(value, "%d", &logLevel) == 1)
      setLogLevel(logLevel);
  }

  if (strcasecmp(key, "LOF_FILE") == 0) {
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
  char line[65536];
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
      snprintf(message, 255, "Syntax error in configration file: %s\n", ptr);
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

void initializeConfiguratorFromCommandLineParameters(int argc, const char **argv) {
  bool configFileGivenAsParam = false;
  initializeConfigurator("/dev/null", "/dev/null");
  for (int i = 1; i < argc; i++) {
    StringTokenizer *tok = new StringTokenizer(argv[i], "=");
    char *key = chop(tok->getNext());
    char *value = chop(tok->getNext());
    if ((key != nullptr) && (value != nullptr)) {
      char *key2 = key;
      while (*key2 == '-')
        key2++;
      if ((strcasecmp(key2, "CONFIG") == 0) || (strcasecmp(key2, "CONFIGFILE") == 0)) {
        configFileGivenAsParam = true;
        initializeConfigurator(value, "/dev/null");
      } else {
        addToHashTable(key2, value);
      }
    }
    if (key != nullptr)
      free(key);
    if (value != nullptr)
      free(value);
    delete tok;
  }
  if (!configFileGivenAsParam) {
    char *configFile = std::getenv("DEFAULT_CONFIG_FILE");
    if (configFile != nullptr)
      initializeConfigurator();
  }
}

void initializeConfigurator(const char *primaryFile, const char *secondaryFile) {
  if (!configratorInitialized) {
    for (int i = 0; i < HASHTABLE_SIZE; i++)
      hashTable[i] = nullptr;
  }
  if (primaryFile != nullptr)
    processConfigFile(primaryFile);
  if (secondaryFile != nullptr)
    processConfigFile(secondaryFile);
  configratorInitialized = true;
}

void initializeConfigurator() {
  char *homeDir = std::getenv("HOME");
  char *primaryFile = nullptr;
  if (homeDir != nullptr) {
    primaryFile = (char *)malloc(strlen(homeDir) + 32);
    strcpy(primaryFile, homeDir);
    if (primaryFile[strlen(primaryFile) - 1] != '/')
      strcat(primaryFile, "/");
    strcat(primaryFile, ".defaultconf");
  }
  initializeConfigurator(primaryFile, "/etc/defaultconf");
  free(primaryFile);
}

bool getConfigurationValue(const char *key, char *value) {
  assert(configratorInitialized == true);
  if (value == nullptr)
    return false;
  int32_t hashValue = simpleHashFunction(key) % HASHTABLE_SIZE;
  KeyValuePair *result = hashTable[hashValue];
  while (result != nullptr) {
    if (strcmp(result->key, key) == 0) {
      strcpy(value, result->value);
      return true;
    }
    result = (KeyValuePair*)result->next;
  }
  return false;
}

bool getConfigurationInt(const char *key, int *value, int defaultValue) {
  char string[MAX_CONFIG_VALUE_LENGTH], string2[MAX_CONFIG_VALUE_LENGTH];
  *value = defaultValue;
  if (!getConfigurationValue(key, string2))
    return false;
  if (sscanf(string2, "%s", string) < 1)
    return false;
  int v = 0;
  for (int i = 0; string[i] != 0; i++) {
    if ((string[i] < '0') || (string[i] > '9')) {
      if (string[i + 1] != 0)
        return false;
      char c = (string[i] | 32);
      if ((c != 'm') && (c != 'k') && (c != 'g'))
        return false;
      if (c == 'k')
        *value = v * 1024;
      if (c == 'm')
        *value = v * 1024 * 1024;
      if (c == 'g')
        *value = v * 1024 * 1024 * 1024;
      return true;
    }
    v = v * 10 + (string[i] - '0');
  }
  *value = v;
  return true;
}

bool getConfigurationInt64(const char *key, int64_t *value, int64_t defaultValue) {
  char string[MAX_CONFIG_VALUE_LENGTH], string2[MAX_CONFIG_VALUE_LENGTH];
  *value = defaultValue;
  if (!getConfigurationValue(key, string2))
    return false;
  if (sscanf(string2, "%s", string) < 1)
    return false;
  int64_t v = 0;
  for (int i = 0; string[i] != 0; i++) {
    if ((string[i] < '0') || (string[i] > '9')) {
      if (string[i + 1] != 0)
        return false;
      char c = (string[i] | 32);
      if ((c != 'm') && (c != 'k') && (c != 'g'))
        return false;
      if (c == 'k')
        *value = v * 1024;
      if (c == 'm')
        *value = v * 1024 * 1024;
      if (c == 'g')
        *value = v * 1024 * 1024 * 1024;
      return true;
    }
    v = v * 10 + (string[i] - '0');
  }
  *value = v;
  return true;
}

bool getConfigurationBool(const char *key, bool *value, bool defaultValue) {
  char string[MAX_CONFIG_VALUE_LENGTH];
  *value = defaultValue;
  if (!getConfigurationValue(key, string))
    return false;
  if ((strcasecmp(string, "true") == 0) || (strcmp(string, "i") == 0)) {
    *value = true;
    return true;
  }
  if ((strcasecmp(string, "false") == 0) || (strcmp(string, "0") == 0)) {
    *value = false;
    return true;
  }
  return false;
}

bool getConfigurationDouble(const char *key, double *value, double defaultValue) {
  char string[MAX_CONFIG_VALUE_LENGTH];
  double v;
  *value = defaultValue;
  if (!getConfigurationValue(key, string))
    return false;
  if (sscanf(string, "%lf", &v) != 1)
    return false;
  *value = v;
  return true;
}