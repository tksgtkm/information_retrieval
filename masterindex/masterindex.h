#ifndef __MASTER_INDEX_H
#define __MASTER_INDEX_H

#include <cstdint>

/*
MasterIndexはデーモンプロセスであり、システム内でファイル変更やイベントの監視を行う。
管理者からファイルシステムのマウント時に新しいIndexインスタンスを作成を要求したり、
UNMOUNT時にIndexインスタンスを削除するなどを行う。
*/

class MasterIndex {

public:

    // 同時にマウント可能なファイルシステムの最大数
    static const int MAX_MOUNT_COUNT = 100;

    // サブインデックスあたりの最大ファイル数
    static const int MAX_FILES_PER_INDEX = 20000000;

    // サブインデックスあたりの最大ディレクトリ数
    static const int MAX_DIRECTORIES_PER_INDEX = 20000000;

    /*
    * すべてのサブインデックスにはそれぞれ独自のインデックス範囲がある。
    * 競合を避けるため、各サブインデックスに対して10^12の大きな範囲を割り当てる
    */
    static const int64_t MAX_INDEX_RANGE_PER_INDEX = 10000000000000LL;

    // MasterIndexが正常に起動したか示す
    bool startupOk;

protected:

    int activeMountCount;

    char *mountPoints[MAX_MOUNT_COUNT];

    int indexCount;



public:

    // MasterIndex(const char *directory);

    MasterIndex(int subIndexCount, char **subIndexDirs);

    ~MasterIndex();
};

#endif