#ifndef LOCKABLE_H
#define LOCKABLE_H

#include <cassert>
#include <pthread.h>
#include <semaphore.h>

/*
Lockableクラスは、機密データの同時変更に対処する方法を提供する。
クラスでローカルデータの保護が必要な場合は、スーパークラスのリストに Lockable を追加するだけで、
Lockable クラスが提供するメソッドを使用できる。
*/
class Lockable {
public:

  // 読み込み処理の最大同時実行数
  constexpr static const int MAX_SIMULTANEOUS_READERS = 4;

  /*
  shutdown()が呼ばれたとき、レソースを使用している処理がすべて
  実行を終了するまで待つ必要がある。
  SHUTDOWN_WAIT_INTERNALは、後続の2つのポーリングオペレーション間隔になる
  */
   constexpr static const int SHUTDOWN_WAIT_INTERNAL = 10;

  Lockable();

  virtual ~Lockable();

  // 呼び出し元のプロセスがリードロックを保持している場合、trueを返す
  bool hasReadLock();

  /*
  読み取りロックを取得する。
  少なくとも一つのプロセスがリードロックを保持している場合、
  他のプロセスはインデックスのデータを変更することはできない。
  書き込みロックを持っている場合、書き込みロックが最初に開放される。
  このメソッドはそのプロセスが呼び出し前に読み取りロックを持っていない場合はtrueを返し、
  そうでない場合はfalseを返す。
  */
  bool getReadLock();

  /*
  直前に取得した読み取りロックを開放する。
  プロセスが読み取りロックを保持していない場合、何もしない
  */
  void releaseReadLock();

  // 呼び出し元のプロセスが書き込みロックを保持している場合にtrueを返す
  bool hasWriteLock();

  /*
  書き込みロックを取得する。
  あるプロセスが書き込みロックを保持している場合、
  他のプロセスはインデックスデータを読むことはできない。
  読み取りロックを持っている場合、読み取りロックが先に開放される。
  このメソッドは呼び出す前にスレッドがロックを持っていなかった場合はtrueを返す。
  そうでない場合はfalseを返す。
  */
  bool getWriteLock();

  // 先に取得した書き込みロックを解除する。
  // プロセスが書き込みロックを保持していない場合何もしない
  void releaseWriteLock();

  /*
  現在保持されているロックを、読み取りロックか書き込みロックに関係なく開放する。
  プロセスがロックを保持していない場合、何もしない。
  */
  void releaseAnyLock();

  void setReadLockSupported(bool value);

  // mutexを取得する。すでにある場合はfalseを返す
  bool getLock();

  // mutexを開放する
  void releaseLock();

  /*
  現在のプロセスのロック機構を無効にする。
  これはプロセスに属するすべてのスレッドに適用される
  */
  static void disableLocking();

  // "target"で参照したバッファー内のオブジェクトのクラス名を得る
  virtual void getClassName(char *target);

protected:

  // エラーロギングで使う
  char errorMessage[256];

  // オブジェクトの同時読み込み数の最大数を設定する
  void setMaxSimultaneousReaders(int value);

private:

  // 可能な同時読み込みの最大数
  int maxSimultaneousReaders;

  // 読み取り時のロックスロットが使用できるか
  bool readLockFree[64];

  // 各ロックスロットの所有者を知らせる
  pthread_t readLockHolders[64];

  // 書き込みロックが開放されているか
  bool writeLockFree;

  // 書き込みロックの所有者を教える
  pthread_t writeLockHolder;

  pthread_t lockHolder;

  /*
  このセマフォは読み取り/書き込みロックの使用が不適切な場合に用いる
  */
  sem_t internalDataSemaphore;

  /*
  読み書きのロックをシミュレートする。
  ロック自体は最大ユーザー数をサポートしていないので、
  ロックの代わりにセマフォを用いる。
  */
   sem_t readWriteSemaphore;
};

class LocalLock {

private:
  Lockable *lockable;

  /*
  実際にロックを保持しているかどうか、
  またデストラクタでロックを開放しなければならないかどうか示す
  */
  bool mustReleaseLock;

public:
  // 与えられたオブジェクトのローカルロックを取得する。
  LocalLock(Lockable *lockable);

  ~LocalLock();

};

#endif
