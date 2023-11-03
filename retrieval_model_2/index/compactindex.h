#ifndef COMPACT_INDEX_H
#define COMPACT_INDEX_H

/*
(term, postings)の組合せで用いる簡易的なインデックス
*/

#include "../config/config.h"
#include "../misc/backend.h"
#include "index_types.h"
#include "index.h"
#include "ondisk_index.h"

class Index;
class IndexIterator;

// ロングリストセグメントのヘッダー情報
struct PostingListSegmentHeader {

  int32_t postingCount;

  int32_t byteLength;

  offset firstElement;

  offset lastElement;
  
};

struct CompactIndex_BlockDescriptor {
  // インデクスブロック内の最初のターム
  char firstTerm[MAX_TOKEN_LENGTH + 1];

  // ブロックスタートのファイルポジション
  off_t blockStart;

  // ブロックのファイルポジションの終わり
  off_t blockEnd;
};

/*
ディスク上のコンパクトインデックスのヘッダー情報
ファイルの末端情報を見つけるためにヘッダーではない
*/
struct CompactIndex_Header {
  // インデックスのタームの数字
  uint32_t termCount;

  // リストセグメントの番号
  uint32_t listCount;

  // 2-lebel B-tree 内のノードの番号
  uint32_t descriptionCount;

  // インデックス内のポスティングの全番号
  offset postingCount;
};

class CompactIndex : public OnDiskIndex {

public:
  // インデックスの作成とマージのために出力するバッファーのサイズ
  static const int WRITE_CACHE_SIZE = 4 * 1024 * 1024;

  // メモリ中でインデックス構築中におけるセグメントの最大数
  static const int MAX_SEGMENTS_IN_MEMORY = WRITE_CACHE_SIZE / TARGET_SEGMENT_SIZE;

  // インデックスをマージするときのバッファーサイズ
  static const int DEFAULT_MERGE_BUFFER_PER_INDEX = 1024 * 1024;

  constexpr static const double DESCRIPTOR_GROWTH_RATE = 1.21;

protected:
  
  CompactIndex();

  // ディスク上のインデックスファイルを管理するコンパクトインデックスを作成する
  CompactIndex(Index *owner, const char *fileName, bool create, bool use_O_DIRECT);

  // RAM中にディスク上のインデックスファイルを読み込むための新しいインスタンスを作成する
  CompactIndex(Index *owner, const char *fileName);

  // クエリ処理のために必要なデータ高層を設定する
  virtual void initializeForQuerying();

  // ディスク上のインデックス全体をメモリ内のバッファーに読み込ませる
  virtual void loadIndexIntoMemory();

public:
  
  static CompactIndex *getIndex(Index *owner, const char *fileName, bool create, bool use_O_DIRECT = false);

  virtual ~CompactIndex();
};

#endif