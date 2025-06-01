#include <dirent.h>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <ctime>
#include <unistd.h>
#include <cassert>
#include <fcntl.h>
#include "index.h"
#include "../utils/all.h"

static const char *INDEX_WORKFILE = "index";

const char *Index::TEMP_DIRECTORY = "/tmp";
const char *Index::LOG_ID = "Index";

char errorMessage[256];

void Index::getConfiguration() {
    getConfigurationInt64("MAX_FILE_SIZE", &MAX_FILE_SIZE, DEFAULT_MAX_FILE_SIZE);
    if (MAX_FILE_SIZE < 32)
        MAX_FILE_SIZE = 32;
    getConfigurationInt64("MIN_FILE_SIZE", &MIN_FILE_SIZE, DEFAULT_MIN_FILE_SIZE);
	if (MIN_FILE_SIZE < 0)
		MIN_FILE_SIZE = 0;
	getConfigurationInt("MAX_UPDATE_SPACE", &MAX_UPDATE_SPACE, DEFAULT_MAX_UPDATE_SPACE);
	if (MAX_UPDATE_SPACE < 16 * 1024 * 1024)
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
	getConfigurationDouble("GARBAGE_COLLECTION_THRESHOLD", &garbageThreshold, 0.40);
	getConfigurationDouble("ONTHEFLY_GARBAGE_COLLECTION_THRESHOLD", &onTheFlyGarbageThreshold, 0.25);

	getConfigurationBool("READ_ONLY", &readOnly, false);

	if (!getConfigurationValue("BASE_DIRECTORY", baseDirectory))
		baseDirectory[0] = 0;
}

Index::Index() {
    readOnly = false;
    shutDownInitiated = false;

    getConfiguration();
    baseDirectory[0] = 0;

    
}

Index::Index(const char *directory, bool isSubIndex) {
    getConfiguration();
    this->isSubIndex = isSubIndex;
    registerdUserCount = 0;
    registrationID = 0;
    SEM_INIT(registeredUserSemaphore, MAX_REGISTERED_USERS);
    indexType = TYPE_INDEX;
    indexIsBeingUpdated = false;
    shutDownInitiated = false;

    struct stat statBuf;
    if (stat(directory, &statBuf) != 0) {
        if (readOnly) {
            log(LOG_ERROR, LOG_ID, "Cannot create new index while in read-only mode.");
            exit(1);
        }
        snprintf(errorMessage, sizeof(errorMessage), "Creating index directory: %s", directory);
        log(LOG_ERROR, LOG_ID, errorMessage);
        mkdir(directory, 0700);
    }
    if (stat(directory, &statBuf) != 0) {
        snprintf(errorMessage, sizeof(errorMessage), "Unable to create index directory: %s", directory);
        log (LOG_ERROR, LOG_ID, errorMessage);
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

    usedAddressSpace = 0;
    deletedAddressSpace = 0;
    biggestOffsetSeenSoFar = 0;

    uid_t uid = getuid();
    uid_t euid = getuid();
    if (uid != euid) {
        log(LOG_ERROR, LOG_ID, "Index executable must not have the SBIT set.");
        exit(1);
    }
    indexOwner = uid;

    this->directory = duplicateString(directory);
    char *fileName = evaluateRelativePathName(directory, INDEX_WORKFILE);

    struct stat fileInfo;
    if (lstat(fileName, &fileInfo) == 0) {
        loadDataFromDisk();
        if (!isConsistent) {
            snprintf(errorMessage, sizeof(errorMessage),
                    "On-disk index found in inconsistent state: %s. Creating new index.", directory);
            log(LOG_ERROR, LOG_ID, errorMessage);
            DIR *dir = opendir(directory);
            struct dirent *child;
            while ((child = readdir(dir)) != nullptr) {
                if (child->d_name[0] == '.')
                    continue;
                char *fn = evaluateRelativePathName(directory, child->d_name);
                struct stat buf;
                if (lstat(fn, &buf) == 0) {
                    if ((buf.st_mode & 0777) == DEFAULT_FILE_PERMISSIONS) {
                        if ((S_ISLNK(buf.st_mode)) || (S_ISREG(buf.st_mode)) || (S_ISFIFO(buf.st_mode)))
                            unlink(fn);
                    }
                }
                free(fn);
            }
            closedir(dir);
        }
    }

    bool createFromScrach;
    if (lstat(fileName, &fileInfo) == 0) {
        loadDataFromDisk();
        createFromScrach = false;
    } else {
        if (readOnly) {
            log(LOG_ERROR, LOG_ID, "Cannot create new index while in read-only mode.");
            exit(1);
        }

        updateOperationsPerformed = 0;
        isConsistent = true;
        int fd = open(fileName, O_RDWR | O_CREAT | O_TRUNC | O_LARGEFILE, DEFAULT_FILE_PERMISSIONS);
        if (fd < 0) {
            snprintf(errorMessage, sizeof(errorMessage), "Unable to create index: %s", fileName);
            log(LOG_ERROR, LOG_ID, errorMessage);
            assert(fd >= 0);
        }
        close(fd);
        createFromScrach = true;
    }
    
}

Index::~Index() {
    bool mustReleaseLock;

    shutDownInitiated = true;


}

void Index::loadDataFromDisk() {
    char *fileName = evaluateRelativePathName(directory, INDEX_WORKFILE);
    FILE *f = fopen(fileName, "r");
    free(fileName);
    if (f == nullptr) {
        snprintf(errorMessage, sizeof(errorMessage), "Unable to open index: %s", fileName);
        log(LOG_ERROR, LOG_ID, errorMessage);
        exit(1);
    }
    char line[1024];
    STEMMING_LEVEL = -1;
    while (fgets(line, 1022, f) != nullptr) {
        if (strlen(line) > 1) {
            while (line[strlen(line) - 1] == '\n')
                line[strlen(line) - 1] = 0;
        }
        if (startsWith(line, "STEMMING_LEVEL = "))
            sscanf(&line[strlen("STEMMING_LEVEL = ")], "%d", &STEMMING_LEVEL);
        if (startsWith(line, "BIGRAM_INDEXING = "))
            BIGRAM_INDEXING = (strcasecmp(&line[strlen("BIGRAM_INDEXING = ")], "true") == 0);
        if (startsWith(line, "UPDATE_OPERATORS = "))
            sscanf(&line[strlen("UPDATE_OPERATIONS = ")], "%d", &updateOperationsPerformed);
        if (startsWith(line, "IS_CONSISTENT = ")) {
            if (strcasecmp(&line[strlen("IS_CONSISTENT = ")], "true") == 0)
                isConsistent = true;
            else if (strcasecmp(&line[strlen("IS_CONSISTENT = ")], "false") == 0)
                isConsistent = false;
        }
        if (startsWith(line, "DOCUMENT_LEVEL_INDEXING = "))
            sscanf(&line[strlen("DOCUMENT_LEVEL_INDEXING = ")], "%d", &DOCUMENT_LEVEL_INDEXING);
        if (startsWith(line, "USED_ADDRESS_SPACE = "))
            sscanf(&line[strlen("USED_ADDRESS_SPACE = ")], OFFSET_FORMAT, &usedAddressSpace);
        if (startsWith(line, "DELETED_ADDRESS_SPACE = "))
            sscanf(&line[strlen("DELETED_ADDRESS_SPACE = ")], OFFSET_FORMAT, &usedAddressSpace);
        if (startsWith(line, "BIGGEST_OFFSET = "))
            sscanf(&line[strlen("BIGGEST_OFFSET = ")], OFFSET_FORMAT, &usedAddressSpace);
    }
    if ((STEMMING_LEVEL < 0) || (STEMMING_LEVEL > 3)) {
        snprintf(errorMessage, sizeof(errorMessage),
                "Illegal configurate values in index file: %s", directory);
        log(LOG_ERROR, LOG_ID, errorMessage);
        exit(1);
    }
    fclose(f);
}