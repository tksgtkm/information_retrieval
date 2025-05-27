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
    
}