#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include "filemanager.h"
#include "../utils/all.h"
#include "../index/index.h"

#define FILE_DATA_FILE "index.file"
#define INODE_DATA_FILE "index.inodes"
#define DIRECTORY_DATA_FILE "index.directories"

const char *FileManager::LOG_ID = "FileManager";
const int FileManager::MINIMUM_SLOT_COUNT;
const double FileManager::SLOT_GROWTH_RATE;
const double FileManager::SLOT_REPACK_THRESHOLD;
const off_t FileManager::INODE_FILE_HEADER_SIZE;

FileManager::FileManager(Index *owner, const char *workDirectory, bool create) {
    this->owner = owner;
    biggestINodeID = -1;
    cachedFileID = -1;
    cacheDirID = -1;
    addressSpaceCovered = 0;
    memset(mountPoint, 0, sizeof(mountPoint));

    transactionLog = typed_malloc(AddressSpaceChange, INITIAL_TRANSACTION_LOG_SPACE);
    transactionLogSize = 0;
    transactionLogAllocated = INITIAL_TRANSACTION_LOG_SPACE;

    fileDataFile = evaluateRelativePathName(workDirectory, FILE_DATA_FILE);
    iNodeDataFile = evaluateRelativePathName(workDirectory, INODE_DATA_FILE);
    directoryDataFile = evaluateRelativePathName(workDirectory, DIRECTORY_DATA_FILE);
}