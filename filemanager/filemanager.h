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

    // データファイルのファイル名
    char *directoryDataFile, *fileDataFile, *iNodeDataFile;

    // データファイルのファイルハンドル
    int directoryData, fileData, iNodeData;

    // 最も最近にアクセスされたファイルのキャッシュ
    int cachedFileID;
    char cachedDirName[256];

    // 最も最近にサクセスされたディレクトリのキャッシュ
    int cacheDirID;
    char caccheDirName[256];

    /*
    このディレクトリツリーが存在するマウントポイント
    初期値は"/"　setMountPointを呼び出すことで変更できる
    */
    char mountPoint[256];

    // FileManagerが管理しているディレクトリの数
    int32_t directoryCount;

    // 確保されているスロット数(IndexedDirectoryインスタンスの数)
    int32_t directorySlotsAllocated;

    // 把握しているすべてのディレクトリのリスト
    IndexDirectory *directories;

    // freeDirectoryIDs配列に含まれる空きディレクトリの数
    int32_t freeDirectoryCount;

    /*
    空きディレクトリIDのリストを含む配列
    これですべての割り当て済みディレクトリスロットを線形にスキャンせずに
    新しいディレクトリにIDを割り当てることができる
    */
    int32_t *freeDirectoryIDs;

    // FileManagerが管理しているファイルの数
    int32_t fileCount;

    // 確保されているスロット数(indexFileインスタンスの数)
    int32_t fileSlotsAllocated;

    // 把握しているすべてのファイル
    IndexedFile files;

    // freeFileIDs配列に含まれる空きファイルの数
    int32_t freeFileIDs;

    // システム内のINodeの数
    int32_t iNodeCount;

    // 確保されているINodeスロット数(IndexedINodeインスタンスの数)
    int32_t iNodeSlotsAllocated;

    // FileManagerに最も最近追加されたINodeのID
    int32_t biggestINodeID;

    /*
    これまでに観測された最大のオフセット値
    INodesが常に昇順に並ぶことを保証するために使用される
    */
    offset biggestOffset;

    /*
    ファイルによって占有されているインデックス全体のアドレス空間の割合を示す
    ファイルが追加・変更・削除されるたびに更新する必要がある
    */
    offset addressSpaceCovered;

    /*
    現在のトランザクション中に何が起こったか要約する
    AddressSpaceChangeインスタンスのリスト
    */
    AddressSpaceChange *transactionLog;

    // トランザクションログ内の要素数
    int transactionLogSize, transactionLogAllocated;

public:

    /*
    "create"の値に応じて新しいFileManagerインスタンスを作成する
    FileManagerは空になるか、指定されたworkDirectoryにあるデータが格納される
    */
    FileManager(Index *owner, const char *wordDirectory, bool create);

    // データをディスクに保存し、メモリを開放する
    ~FileManager();


};

#endif