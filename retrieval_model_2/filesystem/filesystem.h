/*
ファイルシステムクラスはディスク上のファイル保存を簡易的に行うために用いる。
ファイルからファイルへの階層的な構造にポスティングリストを保存する。
OSのファイルシステムを用いる場合、1000000語以上を保存できる。
*/

#ifndef __FILESYSTEM_H
#define __FILESYSTEM_H

#include "../misc/lockable.h"
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>

// ステータスコード付きの関数の値を返すようにする
static const int FILESYSTEM_SUCCESS = 0;
static const int FILESYSTEM_ERROR = -1;

// ファイルシステムキャッシュオブジェクトのモード
static const int FILESYSTEMCACHE_LRU = 1;
static const int FILESYSTEMCACHE_FIFO = 2;

class FileObject;
class FileSystem;
class FileSystemCache;

using fs_pageno = int32_t;
using fs_fileno = int32_t;
using byte = unsigned char; 

class FileSystem : public Lockable {

  friend class FileObject;
  friend class FileSystemCache;

public:
  /*
  ファイルを開くときにopen(2)に渡されるパラメータ
  O_RDWRのみを持つ(O_RDWR | O_SYNCを持つほうがいいかも)
  */
  static const int FILESYSTEM_ACCESS = O_RDWR;

  // ファイルシステムのオブジェクトの初期値
  static const int DEFAULT_PAGE_COUNT = 1024;
  static const int DEFAULT_PAGE_SIZE = 1024;

  // 最小値及び最大値の設定
  static const int MIN_PAGE_COUNT = 32;
  static const int MIN_PAGE_SIZE = 128;
  static const int MAX_PAGE_SIZE = 8192;
  static const int MAX_PAGE_COUNT = 1073741824;

  // ページ数が下記の値より小さいファイルシステムは小さいとみなす
  static const int SMALL_FILESYSTEM_THRESHOLD = 1024;

  // これがパージレイアウトファイル内にある場合、そのページが空であるとする
  static const int32_t UNUSED_PAGE = -123456789;
  static const int32_t USED_PAGE = -987654321;

  // ファイルシステムの1MBあたりのキャッシュサイズの初期値
  static const int DEFAULT_CACHE_SIZE = 1 * 1024 * 1024;

private:

  /*
  ファイルシステムをロードする場合、それが本当にファイルシステムであるか確認したい。
  ファイルの最初の32bitは常にFINGERPRINTの値でなければならない
  */
  static const int32_t FINGERPRINT = 912837123;

  // ファイルシステムの前文の長さ(フィンガープリント、ページサイズ、ページカウント)
  static const int PREAMBLE_LENGTH = 5 * sizeof(int32_t);

  // ディスク上の整数値ポインターの一つの長さ
  static const int INT_SIZE = sizeof(int32_t);

  // アイルシステムのページの数
  int32_t pageCount;

  // ファイルシステム内の各ページのサイズ(in bytes)
  int32_t pageSize;
  int32_t intsPerPage, doubleIntsPerPage;

  // ページレイアウトテーブルによって占有されるページ数
  int32_t pageLayoutSize;

  // file->page mappingsによって占有されるページ数
  int32_t fileMappingSize;

  // ページレイアウトテーブルの各ページについて、ファイルシステムで
  // 参照する空きページの数を知らせる
  int16_t *freePages;

  // file->page mappingsの各ページについて、新しいファイルによって占有される
  // 空きファイルのスロットの数を知らせる
  int16_t *freeFileNumbers;

protected:

  /*
  ファイルシステムが置かれているファイルのハンドルまたは
  クラスのコンストラクタでなにか問題があれば-1を返す。
  */
  int dataFile;

  // データファイルの名前
  char *dataFileName;

  // パフォーマンスを上げるためにキャッシングする
  FileSystemCache *cache;

  // コンストラクタを呼び出したときのキャッシュのサイズ
  int32_t cacheSize;

  // キャッシュの効率性を評価する。
  int64_t cachedReadCnt, uncachedReadCnt;
  int64_t cachedWriteCnt, uncachedWriteCnt;

public:
 
  // "fileName"パラメータによって与えられたファイルから仮想ファイルシステムをロードする
  FileSystem(char *fileName);

  /*
  特定のパラメータで仮想ファイルシステムを作成する。
  ファイルシステムはページ数を含む。
  個々のページのサイズは"pageSize"パラメータで与えられる。
  ページの初期値は"pageCount"で与えられる。
  */
  FileSystem(char *fileName, int pageSize, fs_pageno pageCount);

  //キャッシュのサイズを固定したコンストラクタとして扱う
  FileSystem(char *fileName, int pageSize, fs_pageno pageCount, int cacheSize);

  // ファイルシステムオブジェクトを開放する
  ~FileSystem();

  /*
  ファイルシステムのメモリを呼び出したときのどれくらい空いているかを
  すべてfile->page マッピングで含めたはいれつとして返す
  */
  int32_t *getFilePageMapping(int *arraySize);

private:

  void init(char *fileName, int pageSize, fs_pageno pageCount, int cacheSize);

public:

  // データファイルの名前を返す
  char *getFileName();

  // ファイルシステムのインスタンスがアクティブなファイルシステムを表すならtrueを返す。
  bool isActive();

  /*
  ファイルシステム上でdefragmentation algorithmが走る
  出力はFILESYSTEM_SUCCESS または FILESYSTEM_ERRORを返す。
  */
  int defrag();

  /*
  パラメータ"newPageCount"の値でファイルシステムのサイズを変更する。
  変更できたらFILESYSTEM_SUCCESSの値を返す。
  */
  int changeSize(fs_pageno newPageCount);

  /*
  "fileHandle"で指定したファイルが存在すれば削除する。
  出力はFILESYSTEM_SUCCESS または FILESYSTEM_ERRORを返す。
  */
  int deleteFile(fs_fileno fileHandle);

  /*
  新しいファイルを作成する。
  新しいファイルのファイルハンドルかエラーが発生した場合はFILESYSTEM_ERRORを返す。
  パラメータ"fileno"を使用して、作成するファイルのファイル番号を指定することができる。
  もし-1を指定すると任意の番号が選ばれる。
  "fileno"の番号を持つファイルがすでに存在する場合はFILESYSTEM_ERRORを返す。
  */
  fs_fileno createFile(fs_fileno fileno);

  /*
  "fileHandle"で指定したファイルの最初のページを出力する。
  なければFILESYSTEM_ERRORを返す。
  */
  fs_pageno getFirstPage(fs_fileno fileHandle);

  // ファイルシステムのページサイズを返す
  int getPageSize();

  // ファイルシステムのページ番号を返す
  int getPageCount();

  // ファイルシステムで使ったページ番号を返す
  int getUsedPageCount();

  // ファイルシステムのファイルの番号を返す
  int getFileCount();

  // ファイルシステムのサイズを返す
  off_t getSize();

  /*
  "buffer"で参照するバッファー内で"pageNumber"で指定したパージを読み込む
   出力はFILESYSTEM_SUCCESS または FILESYSTEM_ERRORを返す。
  */
  int readPage(fs_pageno pageNumber, int32_t offset, int32_t count, void *buffer);
  int readPage(fs_pageno pageNumber, void *buffer);

  // readPage(int)のカウンターパート
  int writePage(fs_pageno pageNumber, int32_t offset, int32_t count, void *buffer);
  int writePage(fs_pageno pageNumber, void *buffer);

  void getCacheEfficiency(int64_t *reads, int64_t *uncachedReads, 
                          int64_t *writes, int64_t *uncachedWrites);

  // データファイルからディリクトリに読み込む
  // int readPage_UNCACHED(fs_pageno pageNumber, int32_t offset, int32_t count, void *buffer);

  // データファイルからディリクトリに書き込む
  // int writePage_UNCACHED(fs_pageno pageNumber, int32_t offset, int32_t count, void *buffer);


protected:

  // データファイルからディリクトリに読み込む
  int readPage_UNCACHED(fs_pageno pageNumber, int32_t offset, int32_t count, void *buffer);

  // データファイルからディリクトリに書き込む
  int writePage_UNCACHED(fs_pageno pageNumber, int32_t offset, int32_t count, void *buffer);

  // "fileHandle"で指定したファイルが占有するページ数を返す
  fs_pageno getPageCount(fs_fileno fileHandle);

  // getPageCount(fs_fileno)のカウンターパート
  int setPageCount(fs_fileno fileHandle, fs_fileno newPageCount);

  fs_pageno getPageStatus(fs_pageno page);

  int setPageStatus(fs_pageno page, fs_pageno newStatus);

  /*
  ファイルシステム内の空きページのオフセット数を返す
  "closeTo"で指定したページに近いページを探そうとする。
  もし"closeTo"が < 0 ならすべての空きページを返す。
  エラーが発生すればFILESYSTEM_ERRORを返す。
  */
  fs_pageno claimFreePage(fs_fileno owner, fs_pageno closeTo);

public:

  // 保留中の更新状をすべてディスクに書き込み、キャッシュの内容を削除する
  void flushCache();

private:

  void initializeFreeSpaceArrays();

  // もし空いてるストレージ空間を含む"pageNumber"を与えたらtrueを返す
  bool isPageFree(fs_pageno pageNumber);

  // file->page mappingsにデータを書き込む。
  int setFirstPage(fs_fileno fileHandle, fs_pageno firstPage);

  // 現在未使用のファイル番号を予約して返す。
  fs_fileno claimFreeFileNumber();

  /*
  ファイルマッピングテーブルのサイズを1ページ分増やす。
  この操作はcreateFileによって空きファイルスロットが見つからないときに実行される。
  */
  int increaseFileMappingSize();

  /*
  ファイルマッピングテーブルのサイズを減らす
  */
  int decreaseFileMappingSize();

  // ディスクキャッシュ活性化
  void enableCaching();

  // ディスクキャッシュ非活性化
  void disableCaching();
};

// 仮想ファイルシステム内のファイルのクラス
class FileObject : public Lockable {

friend class FileSystem;

private:
  
  // ファイルシステム内のファイルの一意の識別子
  int32_t handle;

  // バイト表記によるファイルサイズ
  int32_t size;

  // 書き込みと読み込みを行う位置
  int32_t seekPos;

  // バイト表記による各ページのサイズ
  int32_t pageSize;

  // ページ番号、"pages"にスロットを配分する
  int32_t pageCount, allocateCount;

  // そのファイルに属する全ページのリスト
  int32_t *pages;

  // そのファイルが所属するファイルシステム
  FileSystem *fileSystem;

  // このファイルの最初のページのインデックス番号
  fs_pageno firstPage;

private:

  void init(FileSystem *fileSystem, fs_fileno fileHandle, bool create);

public:

  FileObject();

  /*
  新しいファイルを作成し、サイズ0で初期化する。
  ファイルを作成できなかったら、"handle"に負の数が含まれる
  */
  FileObject(FileSystem *fileSystem);

  /*
  "fileHandle"パラメータで指定されたハンドルに対応するファイルを開く
  If "create" == true かつファイルが存在するならファイルは作成できない。
  If "create" == false かつファイルが存在しないなら開くことができない。
  もしエラーが発生したら、"handle"は負の数が含まれるようになる
  */
  FileObject(FileSystem *fileSystem, fs_fileno fileHandle, bool create);

  // オブジェクトとして専有していたリソースをすべて開放する
  virtual ~FileObject();

  // ファイルシステムからファイルを削除する
  virtual void deleteFile();

  // このファイルのファイルハンドルを返す。
  virtual fs_fileno getHandle();

  // ファイルのサイズを返すか存在しないなら-1を返す
  virtual off_t getSize();

  // 専有しているページ番号を返す
  virtual int32_t getPageCount();

  // 現在のシーク位置を返す
  virtual off_t getSeekPos();

  // "newSeekPos"パラメータで与えられた値のポジションへシークをセットする
  virtual int seek(off_t newSeekPos);

  /*
  現在のシークの位置からファイルから"bufferSize"バイトを読み込む。
  ファイルから読み込んだバイト数を返す
  */
  virtual int read(int bufferSize, void *buffer);

  virtual int seekAndRead(off_t position, int bufferSize, void *buffer);

  /*
  ファイルに"bufferSize"バイトを書き込む。
  現在のシーク位置から始める。
  ファイルに書き込んだバイト数を返すか、間違いがあれば-1を返す。
  */
  virtual int write(int bufferSize, void *buffer);

  /*
  ファイルから*bufferSizeバイトを読み込む。
  実際に読み取られたバイト数を反映するように*bufferSizeの値を調整する。
  *mustBufferは返されたバッファーを開放するかどうかをy呼び出し元に伝える
  */
  virtual void *read(int *bufferSize, bool *mustFreeBuffer);
};

/*
FileSystemCacheSlotインスタンスはキャッシュに保存されたファイルシステムのページを表す。
スロットはキューのような構造になる。(FIFO)
*/
typedef struct {
  // ストットにキャッシュされたファイルシステムのページ数
  int pageNumber;
  // このページに対する書き込み操作の有無を示す
  bool hasBeenChanged;

  char *data;

  void *prev;

  void *next;
} FileSystemCacheSlot;

// ハッシュ衝突を解決するための連結リスト(linked list)
typedef struct {
  // キャッシュに保存したページを参照する
  FileSystemCacheSlot *data;
  // 同じハッシュコードの次のデータ要素
  void *next;
} FileSystemCacheHashElement;

class FileSystemCache {
  
  friend class FileSystem;

public:

  static const int DEFAULT_CACHE_SIZE = 128;

private:

  // 所有するファイルシステム
  FileSystem *fileSystem;
  
  // 各ページのサイズ
  int pageSize;

  // キャッシュ内のページ数
  int currentPageCount;

  // キャッシュ内のページの最大数
  int cacheSize;

  // FILESYSTEMCACHE_LRU or FILESYSTEMCACHE_FIFO
  int workMode;

  // キャッシュキュー内の最初のページスロット
  FileSystemCacheSlot *firstSlot;

  // キャッシュキュー内の最後のページスロット
  FileSystemCacheSlot *lastSlot;

  FileSystemCacheHashElement **whereIsPage;

  // I/O処理のための多目的バッファー
  byte *readWriteBuffer;

private:

  /*
  "pageNumber"で指定したページを含むスロットの参照を返し、
  なければNULLを返す。
  */
  FileSystemCacheSlot *findPage(int pageNumber);

  // キュー内の最初のポジションに"slot"で参照したスロットを移動させる
  void touchSlot(FileSystemCacheSlot *slot);

  // ハッシュテーブルから"pageNumber"のページを削除する
  void removeHashEntry(int pageNumber);

  // デバッグで使う
  void printCacheQueue();

public:

  /*
  新しいファイルシステムキャッシュのインスタンスを作成する。
  "pageCount"はページの、"pageSize"は各ページのキャッシュを持ち続ける。
  "dataFile"で指定されたファイルに書き込みまたは読み込みを行う。
  */
  FileSystemCache(FileSystem *fs, int pageSize, int pageCount);

  // オブジェクト削除
  ~FileSystemCache();

  // "pageNumber"で指定したページがキャッシュならtrueを返す
  bool isInCache(int pageNumber);

  // "newWorkMode"によってLRUまたはFIFOのキャッシュのワークモードを設定する
  void setWorkMode(int newWorkMode);

  /*
  キャッシュ内のページをリクエストしたら、"buffer"へコピーして、
  FILESYSTEM_SUCCESSを返す。
  失敗したらFILESYSTEM_ERRORを返す。
  */
  int getPage(int pageNumber, void *buffer);

  /*
  "pageNumber"で指定したページをキャッシュ内に読み込む。
  ページデータは"buffer"から読み込む。
  もし"copyData"がtrueなら"buffer"内のデータを見つけたとき、
  ポインターはコピーされ、データはスロットに保管される。
  */
  FileSystemCacheSlot *loadPage(int pageNumber, void *buffer, bool copyData);

  // "pageNumber"で指定したページのタイムスタンプを更新する
  int touchPage(int pageNumber);

  // キャッシュの書き込み処理　FILESYSTEM_SUCCESSかFILESYSTEM_ERRORを返す
  int writeTopage(int pageNumber, int offset, int length, void *buffer);

  // キャッシュの読み込み処理　FILESYSTEM_SUCCESSかFILESYSTEM_ERRORを返す
  int readFromPage(int pageNumber, int offset, int length, void *buffer);

  // キャッシュの書き込み処理
  void flush();

private:

  /*
  キャッシュから"slot"で指定したスロット内のページを削除する。
  もしページが変更されているなら、データは削除される前に
  ディスクに書き込まれる。
  */
  void evict(FileSystemCacheSlot *toEvict);
};

#endif