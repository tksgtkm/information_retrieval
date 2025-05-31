#include <dirent.h>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <ctime>
#include <unistd.h>
#include "index.h"
#include "../utils/all.h"

static const char *INDEX_WORKFILE = "index";

const char *Index::TEMP_DIRECTORY = "/tmp";
const char *Index::LOG_ID = "Index";

void Index::getConfiguration() {
    getConfigurationInt64("MAX_FILE_SIZE", &MAX_FILE_SIZE, DEFAULT_MAX_FILE_SIZE);
    if (MAX_FILE_SIZE < 32)
        MAX_FILE_SIZE = 32;
}

Index::Index() {
    readOnly = false;
    shutDownInitiated = false;

    getConfiguration();
    baseDirectory[0] = 0;

    
}