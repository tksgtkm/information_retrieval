#ifndef __CONFIGRATOR_H
#define __CONFIGRATOR_H

#include <sys/types.h>
#include "backend.h"

#define MAX_CONFIG_KEY_LENGTH 128
#define MAX_CONFIG_VALUE_LENGTH 4096

#define configurable

// configratorを初期化し、cmd-lineパラメータより与えられたデータを使う
void initializeConfiguratorFromCommandLineParameters(int argc, const char **argv);

// 2つのファイルで、コンフィグデータがあれば、config管理を初期化する
void initializeConfigurator(const char *primaryFile, const char *secondaryFile);

//indexconf 内の設定をconfig 管理を初期化する
void initializeConfigurator();

/*
設定データベースから項目"key"を検索し、"value"で参照されるメモリに値を書き込む。
キーが見つからない場合はfalseを返す。
*/
bool getConfigurationValue(const char *key, char *value);

/*
設定ファイルから整数値を読み込む
2^10 2^20 2^30の乗算器K, M, Gがそれぞれサポートされている。
例えば設定ファイルの値2Mは2^21( = 2*2^20)と解釈される
*/
bool getConfigurationInt64(const char *key, int64_t *value, int64_t defaultValue);

bool getConfigurationInt(const char *key, int *value, int defaultValue);

bool getConfigurationBool(const char *key, bool *value, bool defaultValue);

bool getConfigurationDouble(const char *key, double *value, double defaultValue);

#endif
