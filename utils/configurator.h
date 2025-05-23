#ifndef CONFIGURATOR_H
#define CONDIFURATOR_H

#include <sys/types.h>

#define MAX_CONFIG_KEY_LENGTH 128
#define MAX_CONFIG_VALUE_LENGTH 4096

void initializeConfiguratorFromCommandLineParameters(int argc, const char **argv);

void initializeConfigurator(const char *primaryFile, const char *secondaryFile);

void initializeConfigurator();

bool getConfigurationValue(const char *key, char *value);

#endif