#ifndef __FILE_MANAGER_H
#define __FILE_MANAGER_H

#include "data_structure.h"
#include "../index/index_type.h"

/*
FileManagerクラスはファイルシステムの構造(リンク、inode、　ディレクトリ)
を管理するために使用される
*/

class Index;

typedef struct {
    offset startOffset;
    uint32_t tokenCount;
    int32_t delta;
} AddressSpaceChange;

class FileManager {
    friend class Index;

public:

    static const int INITIAL_TRANSACTION_LOG_SPACE = 8;

    /*
    デフォルトではディレクトリ、ファイル、INodeに対して
    少なくともこの数のスロットが割当てられる
    */
    static const int MINIMUM_SLOT_COUNT = 1024;

    /*
    スロットが不足した場合は、スロット用メモリを再割当てを行う
    このときに使用される成長率
    */
    static const double SLOT_GROWTH_RATE = 1.23;

    /*
    あるタイプ(ディレクトリ、ファイル、INode)のスロット使用率がこの値より
    小さくなった場合、メモリを節約するために該当する配列を再配置(リパック)する
    */
    static const double SLOT_REPACK_THRESHOLD = 0.78;

    static const off_t INODE_FILE_HEADER_SIZE = 2 * sizeof(int32_t) + sizeof(offset);

    static const char *LOG_ID;

private:

    Index *owner;

    char *directoryDataFile, *fileDataFile, *iNodeDataFile;

    int directoryData, fileData, iNodeData;

    int cachedFileID;
    char cachedDirName[256];

    int cacheDirID;
    char caccheDirName[256];

    char mountPoint[256];

    int32_t directoryCount;

    int32_t directorySlotsAllocated;

    IndexDirectory *directories;

    int32_t freeDirectoryCount;

    int32_t *freeDirectoryIDs;

    int32_t fileCount;

    int32_t fileSlotsAllocated;

    IndexedFile files;

    int32_t freeFileIDs;

    int32_t iNodeCount;

    int32_t iNodeSlotsAllocated;

    int32_t biggestINodeID;

    offset biggestOffset;

    offset addressSpaceCovered;

    AddressSpaceChange *transactionLog;

    int transactionLogSize, transactionLogAllocated;

public:

    FileManager(Index *owner, const char *wordDirectory, bool create);

    ~FileManager();


};

#endif