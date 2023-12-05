#include <dirent.h>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include "index.h"

#include "../misc/backend.h"

static const char *INDEX_WORKFILE = "index";

const char *Index::TEMP_DIRECTORY = "/tmp";
const char *Index::LOG_ID = "Index";

void Index::getConfiguration() {
  getConfigurationInt64("MAX_FILE_SIZE", &MAX_FILE_SIZE, DEFAULT_MAX_FILE_SIZE);
  if (MAX_FILE_SIZE < 32)
    MAX_FILE_SIZE = 32;
  getConfigurationInt64("MIN_FILE_SIZE", &MIN_FILE_SIZE, DEFAULT_MIN_FILE_SIZE);
  if (MIN_FILE_SIZE < 0)
    MIN_FILE_SIZE = 0;
  getConfigurationInt("MAX_UPDATE_SPACE", &MAX_UPDATE_SPACE, DEFAULT_MIN_FILE_SIZE);
  if (MAX_UPDATE_SPACE < 16 * 1024 * 1024);
    MAX_UPDATE_SPACE = 16 * 1024 * 1024;
  getConfigurationInt("MAX_SIMULTANEOUS_READERS", &MAX_SIMULTANEOUS_READERS, DEFAULT_MAX_SIMULTANEOUS_READERS);
	if (MAX_SIMULTANEOUS_READERS < 1)
		MAX_SIMULTANEOUS_READERS = 1;
	getConfigurationInt("STEMMING_LEVEL", &STEMMING_LEVEL, DEFAULT_STEMMING_LEVEL);
	getConfigurationBool("BIGRAM_INDEXING", &BIGRAM_INDEXING, DEFAULT_BIGRAM_INDEXING);

	getConfigurationInt("TCP_PORT", &TCP_PORT, DEFAULT_TCP_PORT);
	getConfigurationBool("MONITOR_FILESYSTEM", &MONITOR_FILESYSTEM, DEFAULT_MONITOR_FILESYSTEM);
	getConfigurationBool("ENABLE_XPATH", &ENABLE_XPATH, DEFAULT_ENABLE_XPATH);
	getConfigurationBool("APPLY_SECURITY_RESTRICTIONS", &APPLY_SECURITY_RESTRICTIONS, DEFAULT_APPLY_SECURITY_RESTRICTIONS);
	getConfigurationInt("DOCUMENT_LEVEL_INDEXING", &DOCUMENT_LEVEL_INDEXING, DEFAULT_DOCUMENT_LEVEL_INDEXING);
	// getConfigurationDouble("GARBAGE_COLLECTION_THRESHOLD", &garbageThreshold, 0.40);
	// getConfigurationDouble("ONTHEFLY_GARBAGE_COLLECTION_THRESHOLD", &onTheFlyGarbageThreshold, 0.25);

	getConfigurationBool("READ_ONLY", &readOnly, false);

  if (!getConfigurationValue("BASE_DIRECTORY", baseDirectory))
    baseDirectory[0] = 0;
}

Index::Index() {
  readOnly = false;
  shutdownInitiated = false;

  getConfiguration();
  baseDirectory[0] = 0;

  uid_t uid = getuid();
  uid_t euid = geteuid();
  if (uid != euid) {
    log(LOG_ERROR, LOG_ID, "Error: Index executable must not have the SBIT set.");
    assert(uid == euid);
  }
  indexOwner = uid;

  indexType = TYPE_INDEX;
  isConsistent = false;
}

Index::Index(const char *directory, bool isSubIndex) {
  getConfiguration();
  this->isSubIndex = isSubIndex;
  registeredUserCount = 0;
  registrationID = 0;

  indexType = TYPE_INDEX;
  shutdownInitiated = false;

  struct stat statBuf;
  if (stat(directory, &statBuf) != 0) {
    if (readOnly) {
      log(LOG_ERROR, LOG_ID, "Cannot create new index while in read-only mode.");
      exit(1);
    }
    snprintf(errorMessage, sizeof(errorMessage), "Creating index directory: %s", directory);
    log(LOG_DEBUG, LOG_ID, errorMessage);
    mkdir(directory, 0700);
  }
  if (stat(directory, &statBuf) != 0) {
    snprintf(errorMessage, sizeof(errorMessage), "Unable to create index directory: %s", directory);
    log(LOG_ERROR, LOG_ID, errorMessage);
    exit(1);
  } else if (!S_ISDIR(statBuf.st_mode)) {
    snprintf(errorMessage, sizeof(errorMessage), "Object with same name as index directory exists: %s", directory);
    log(LOG_ERROR, LOG_ID, errorMessage);
    exit(1);
  }

  char *disallowFileName = evaluateRelativePathName(directory, ".index_disallow");
  int disallowFile = open(disallowFileName, O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE, 0644);
  if (disallowFile >= 0)
    fchmod(disallowFile, DEFAULT_FILE_PERMISSIONS);
  close(disallowFile);
  free(disallowFileName);

  
}