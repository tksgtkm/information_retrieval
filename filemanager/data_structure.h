#ifndef __FILEMANAGER_DATA_STRUCTURE_H
#define __FILEMANAGER_DATA_STRUCTURE_H

#include "../index/index_type.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// このマジックナンバーは、DirectoryContent内のソートされた配列で空スロットを示すために使用される
#define DC_TYPE_SLOT 984732861

typedef struct {
    /* スロットのハッシュ値　ソートのために使用される */
    int32_t hashValue;

    /*
    参照するオブジェクト(ファイルまたはディレクトリ)のID
    スロットが何も含んでいない(子が削除された)場合はDC_EMPTY_SLOTになる
    */
    int32_t id;

} DC_ChildSlot;

typedef struct DicrectoryContent {

    /*
    ディレクトリ内のファイルおよびディレクトリの数
    最近ファイルが削除された場合、この数値は(longAllocated + shortCount)と異なることがある
    */
    int32_t count;

    /* ロングリスト内のIDの数 */
    int32_t longAllocated;

    /* ロングリスト本体 */
    DC_ChildSlot *longList;

    /* ソート済みリストにまだ統合されていない追加済みの子の数 */
    int16_t shortCount, shortSlotsAllocated;

    /* ディレクトリに追加された子のリスト */
    DC_ChildSlot *shortList;

} DicrectoryContent;

#define MAX_DIRECTORY_NAME_LENGTH (64 - 2 * sizeof(int32_t) - sizeof(void*) - 1)

/*
IndexDirectoryデータ構造はファイルシステムのディレクトリ構造を表すために使用される
各ディレクトリには一意のIDと親ディレクトリが割当てられる
各ディレクトリについて、その子(ディレクトリとファイル)すべてを含む
平衡探索木(バランスドツリー)を保持する
木のノードは名前のハッシュ値によってソートされる
*/
typedef struct {

    /* このディレクトリの一意なID */
    int32_t id;

    /*
    親ディレクトリのID
    parent == id の場合は親が存在せず、この
    */
    int32_t parent;

    /* このディレクトリの所有者 */
    uid_t owner;

    /* このディレクトリに関連付けられたユーザーグループ */
    gid_t group;

    /* Unixスタイルのディレクトリのパーミッション */
    mode_t permissions;

    /*
    ディレクトリ名
    約50文字以上の名前を持つディレクトリ内でのファイル検索はサポートされない
    ディレクトリがマウントポイントである場合、名前は[/dev/]で始める
    */
    char name[MAX_DIRECTORY_NAME_LENGTH + 1];

    /* 高速アクセスのために、名前のハッシュ値をここに保存する */
    int32_t hashValue;

    /* このディレクトリのすべての子(ファイルとディレクトリ)を格納する構造体 */
    DicrectoryContent children;

} IndexDirectory;

#define MAX_FILE_NAME_LENGTH (64 - 2 * sizeof(int32_t) - 1)

typedef struct {
    int32_t iNode;

    int32_t parent;

    int32_t hashValue;
} IndexedFile;

#endif