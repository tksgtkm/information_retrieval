#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <cstring>
#include <cassert>
#include <sys/stat.h>
#include <sys/types.h>
#include "filemanager.h"
#include "directorycontent.h"
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

    if (create) {
        if (owner->readOnly) {
            log(LOG_ERROR, LOG_ID, "Cannot create fresh fileManager instance in read-only mode.");
            exit(1);
        }

        // 与えられたディレクトリ内で新しいファイルマネージャーインスタンスを作成する
        int flags = O_RDWR | O_CREAT | O_TRUNC | O_LARGEFILE;
        mode_t mode = DEFAULT_FILE_PERMISSIONS;
        fileData = open(fileDataFile, flags, mode);
        if (fileData < 0)
            assert("Unable to open " FILE_DATA_FILE == nullptr);
        iNodeData = open(iNodeDataFile, flags, mode);
        if (iNodeData < 0)
            assert("Unable to open " INODE_DATA_FILE == nullptr);
        directoryData = open(directoryDataFile, flags, mode);
        if (directoryData < 0)
            assert("Unable to open " DIRECTORY_DATA_FILE == nullptr);

        // マウント場所を"/"に初期化
        strcpy(mountPoint, "/");

        // ディレクトリデータ内部を初期化
        directoryCount = 0;
        directorySlotsAllocated = MINIMUM_SLOT_COUNT;
        directories = typed_malloc(IndexDirectory, directorySlotsAllocated);
        for (int i = 0; i < directorySlotsAllocated; i++)
            directories[i].id = -1;

        // rootディレクトリを作成
        directories[0].id = 0;
        directories[0].parent = 0;
        directories[0].name[0] = 0;
        directories[0].hashValue = 0;
        directoryCount++;
        initializeDirectoryContent(&directories[0].children);
    }
}
