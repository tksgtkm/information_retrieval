#ifndef CONFIGURATOR_H
#define CONDIFURATOR_H

#include <sys/types.h>

#define MAX_CONFIG_KEY_LENGTH 128
#define MAX_CONFIG_VALUE_LENGTH 4096

#define configurable

void initializeConfiguratorFromCommandLineParameters(int argc, const char **argv);

void initializeConfigurator(const char *primaryFile, const char *secondaryFile);

void initializeConfigurator();

bool getConfigurationValue(const char *key, char *value);

bool getConfigurationInt(const char *key, int *value, int defaultValue);

bool getConfigurationInt64(const char *key, int64_t *value, int64_t defaultValue);

bool getConfigurationBool(const char *key, bool *value, bool defaultValue);

bool getConfigurationDouble(const char *key, double *value, double defaultValue);

char **getConfigurationArray(const char *key);

#endif