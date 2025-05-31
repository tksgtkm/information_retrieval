#ifndef __INDEX_H
#define __INDEX_H

#include "../utils/all.h"
#include "index_type.h"
#include <semaphore.h>

class Index {

public:
    /*
    以下の定数は、IndexとMasterIndexを区別するために使う。
    indexTypeはコンストラクトで設定される適切な値を持つ
    */
    static const int TYPE_INDEX = 1;
    static const int TYPE_MASTERINDEX = 2;
    static const int TYPE_FAKEINDEX = 3;

    // これより大きいファイルはインデックス化しない
    static const int64_t DEFAULT_MAX_FILE_SIZE = 20000000000LL;
    configurable int64_t MAX_FILE_SIZE;

    // これより小さいファイルはインデックス化しない
    static const int64_t DEFAULT_MIN_FILE_SIZE = 8;
    configurable int64_t MIN_FILE_SIZE;

    // メモリ内のUpdateListに割り当てるメモリ量
    static const int DEFAULT_MAX_UPDATE_SPACE = 40 * 1024 * 1024;
    configurable int MAX_UPDATE_SPACE;
    
    // 同時に読み取りロックを保持できるプロセスの最大数
    static const int DEFAULT_MAX_SIMULTANEOUS_READERS = 4;
    configurable int MAX_SIMULTANEOUS_READERS;

    // ステミングレベルは０から2の間で設定可能
    static const int DEFAULT_STEMMING_LEVEL = 0;
    configurable int STEMMING_LEVEL;

    // クエリ処理時にファイルのパーミッションを考慮すべきかどうか示す
    static const bool DEFAULT_APPLY_SECURITY_RESTRICTIONS = true;
    configurable bool APPLY_SECURITY_RESTRICTIONS;

    // TCPサーバが動作している場合の待受サポート　-1はサーバなし
    static const int DEFAULT_TCP_PORT = -1;
    configurable int TCP_PORT;

    // FileSysDaemonが起動していて/proc/fschangeを監視しているかどうか
    static const bool DEFAULT_MONITOR_FILESYSTEM = false;
    configurable bool MONITOR_FILESYSTEM;

    /*
    ドキュメントごとの単語出現頻度を追跡する必要があるかどうかを示す
    0: 追跡しない
    1: 位置情報とドキュメント情報の療法を取得
    2: ドキュメント情報のみ保持
    */
    static const int DEFAULT_DOCUMENT_LEVEL_INDEXING = 0;
    configurable int DOCUMENT_LEVEL_INDEXING;

    /*
    XPathサポートを有効にする場合、インデックス作成時に特殊なタグを追加する必要がある。
    ネストレベルNの開始タグに対しては<level!N>、終了タグに対しては</level!N>をインデックス
    に追加する。親子関係の判別のためにネストの深さを把握する。
    */
    static const bool DEFAULT_ENABLE_XPATH = false;
    configurable bool ENABLE_XPATH;

    // trueに設定すると、単語単位に加えてバイグラム(2語連続)もインデックスに追加する
    static const bool DEFAULT_BIGRAM_INDEXING = false;
    configurable bool BIGRAM_INDEXING;

    // ファイルのパーミッション管理のために使用。スーパーユーザーはすべてのファイルを読み取れる
    static const uid_t SUPERUSER = (uid_t)0;

    // GODはスーパーユーザーよりも強力で、削除されたファイルも見ることができる
    static const uid_t GOD = (uid_t)-1;

    // ユーザーnobody。全世界に読み取り権限のあるファイルのみアクセス可能
    static const uid_t NOBODY = (uid_t)-2;

    // 一時データの保存場所
    static const char *TEMP_DIRECTORY;

    // "Index" (ログ識別子として使われる可能性がある)
    static const char *LOG_ID;

    /*
    複数のプロセスが同時にインデックスを更新しようとする場合、一方が終了するまで
    他方は待機する。UPDATE_WAIT_INTERVALはその待機中にポーリングを行う間隔
    */
    static const int INDEX_WAIT_INTERVAL = 20;

    // 同時に処理可能なクエリの最大数
    static const int MAX_REGISTERED_USERS = 4;

    // インデックス内の不要ポスティング数がこの値より少ない場合、
    // ガベージコレクションは実行されない
    static const int MIN_GARBAGE_COLLECTION_THRESHOLD = 64 * 1024;

    // 作業ディレクトリ
    char *directory;
   
    /*
    このディレクトリ以下のファイルがインデックス化対象になる。
    空文字列の場合、すべてが対象になる
    */
    char baseDirectory[MAX_CONFIG_VALUE_LENGTH];

public:

    // このインデックスがMsterIndexのサブインデックスである場合にtrue
    bool isSubIndex;

    // TYPE_INDEX or TYPE_MASTERINDEX
    int indexType;

    bool readOnly;

    bool shutDownInitiated;

    bool isConsistent;

    uid_t indexOwner;

    int64_t registerdUsers[MAX_REGISTERED_USERS];

    int registerdUserCount;

    int64_t registrationID;

    unsigned int updateOperationsPerformed;

    sem_t registeredUserSemaphore;

    sem_t updateSemaphore;

    bool indexIsBeingUpdated;

    offset usedAddressSpace, deletedAddressSpace;

    double garbageThreshold, onTheFlyGarbageThreshold;

    offset biggestOffsetSeenSoFar;

public:

    // デフォルトコンストラクタ
    Index();

    /*
    指定されたdirectory内のデータをもとにIndexインスタンスを作成する。
    データが存在しない場合は、新しい空のインデックスが作成される。
    ディレクトリが存在しない場合は自動的に作成される。
    isSubIndexがtrueの場合、それはこのインデックスがシステム全体のMasterIndexの子であることを意味する
    サブインデックスである場合、自身で接続デーモンを持つことはできない
    */
    Index(const char *directory, bool isSubIndex);

    virtual ~Index();

protected:

    // 設定マネージャから構成情報を取得する
    virtual void getConfiguration();
};

#endif