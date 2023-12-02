#ifndef INDEX_H
#define INDEX_H

#include "../misc/backend.h"

class Index : public Lockable {
public:
  /*
  IndexとMasterIndexを区別するために以下の定数を使用する。
  indexTypeには適切な値が設定される。(コンストラクタで設定)
  */
  static const int TYPE_INDEX = 1;
  static const int TYPE_MASTERINDEX = 2;
  static const int TYPE_FAKEINDEX = 3;

  // ここでの設定値以上のファイルはインデックスにしない
  static const int64_t DEFAULT_MAX_FILE_SIZE = 20000000000LL;
  configurable int64_t MAX_FILE_SIZE;

  // ここでの設定値以下のファイルはインデックスにしない
  static const int64_t DEFAULT_MIN_FILE_SIZE = 8;
  configurable int64_t MIN_FILE_SIZE;

  

};

#endif