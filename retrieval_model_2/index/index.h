#ifndef INDEX_H
#define INDEX_H

#include "index_types.h"
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

  // メモリ内のUpdateListに割り当てるメモリ量はどのくらいか
  static const int DEFAULT_MAX_UPDATE_SPACE = 40 * 1024 * 1024;
  configurable int MAX_UPDATE_SPACE;

  // 同時に読み取りロックを保持するプロセスの最大数
  static const int DEFAULT_MAX_SIMULTANEOUS_READERS = 4;
  configurable int MAX_SIMULTANEOUS_READERS;

  // ステミングレベルは0 ~ 2
  static const int DEFAULT_STEMMING_LEVEL = 0;
  configurable int STEMMING_LEVEL;

  // クエリ処理でファイルのアクセス許可を尊重する必要があるか示す
  static const bool DEFAULT_APPLY_SECURITY_RESTRICTIONS = true;
  configurable bool APPLY_SECURITY_RESTRICTIONS;

  // 現在起動しているTCPサーバーのポートを確認する。-1ならばサーバーなし
  static const int DEFAULT_TCP_PORT = -1;
  configurable int TCP_PORT;

  // ファイルシステムデーモンを起動し、/proc/fschangeから読み込む
  static const bool DEFAULT_MONITOR_FILESYSTEM = false;
  configurable bool MONITOR_FILESYSTEM;

  /*
  ドキュメントごとに用語の出現を追跡する必要があるかどうか示す
  ０の場合、ドキュメントレベルの情報がない
  1の場合、ドキュメントレベルとその位置の両方がある
  2の場合、ドキュメントレベルの情報のみ
  */
  static const int DEFAULT_DOCUMENT_LEVEL_INDEXING = 0;
  configurable int DOCUMENT_LEVEL_INDEXING;

  /*
  もしXPathをサポートするなら、インデックスを作成するときにインデックス内に
  特定のタグを加える。
  ネストレベル(深さ)Nのタグ毎にインデックスに<level!N>をもたせる。
  対応するすべての終了タグには</level!N>をもたせる。
  これが必要なのは、親と子をすぐにみつけたいので、ネストレベルを知る必要がある。
  */
  static const bool DEFAULT_ENABLE_XPATH = false;
  configurable bool ENABLE_XPATH;

  // もしここがtrueなら、ここのトークンに加えてトークンバイグラムをインデックスに追加する
  static const bool DEFAULT_BIGRAM_INDEXING = false;
  configurable bool BIGRAM_INDEXING;

  // ファイル権限などを管理するために、スーパーユーザーはすべて読むことをできる。
  static const uid_t SUPERUSER = (uid_t)0;

  // GODはスーパーユーザーよりもさらに強い。削除されたファイルも見ることができる。
  static const uid_t GOD = (uid_t)-1;

  // ユーザーはだれもいない、ワールドワイドな読み取り権限を持つファイルのみアクセスできる
  static const uid_t NOBODY = (uid_t)-2;

  // 一時データはどこに保存するか？
  static const char *TEMP_DIRECTORY;

  // Index
  static const char *LOG_ID;

  /*
  複数のプロセスがインデックスを更新しようとしている場合、
  一つのプロセスは他のプロセスが完了するまで待機する必要がある。
  UPDATE_WAIT_INTERVALは、待機プロセスによる後続の２つの問い合わせ処理の間の間隔
  */
  static const int INDEX_WAIT_INTERVAL = 20;

  // 並列時のクエリ処理の最大数
  static const int MAX_REGISTERED_USERS = 4;

  // インデックス内のガベージポスティングの数がこの値より小さい場合、ガベージコレクションの実行は拒否される。
  static const int MIN_GARBAGE_COLLECTION_THRESHOLD = 64 * 1024;

  // これは実行中のディレクトリ
  char *directory;

  // これはファイルのインデックスを作成できるディレクトリ
  char baseDirectory[MAX_CONFIG_VALUE_LENGTH];

public:

  // trueならこのインデックスはマスターインデックスインスタンスの子供になる
  bool isSubIndex;

  // TYPE_INDEX or TYPE_MASTERINDEX
  int indexType;

  // このインデックスが読み取り専用かどうかを示す
  bool readOnly;

  bool shutdownInitiated;

  /*
  このインデックスはディスク上のイメージが一貫した状態にあるかどうか示す。
  これを使用してディスク上のインデックスをロードするか、
  ロードを拒否してコンストラクタ内で新しいインデックスを作成するか決定する。
  */
  bool isConsistent;

  /*
  プロセスを開始したユーザーのUID
  このユーザー(およびスーパーユーザー)がインデックスへの完全なアクセス権を持っていると想定する。
  他のユーザーごとに結果はセキュリティマネージャーによってフィルターされる
  */
  uid_t indexOwner;

  // インデックスでユーザーが登録したリスト
  int64_t registeredUsers[MAX_REGISTERED_USERS];

  // 登録されたユーザーの番号
  int registeredUserCount;

  //  Counter used to give unique user IDs to Query instances using us
  int64_t registrationID;

  // 更新処理(WRITE, UNLINK)を行った数をカウントする 
  unsigned int updateOperationsPerformed;

  // ガベージコレクションのトリガー
  offset usedAddressSpace, deletedAddressSpace;

  /*
  その時点での最大のオフセット値
  これはnotifyOfAddressSpaceChangeの操作が実際にポスティングを削除するか
  単にインデックス作成プロセス中にアドレス空間の過剰割当を再調整するだけなのかを
  判断するのに役立つ
  */
  offset biggestOffsetSeenSoFar;
  
public:
  // デフォルトコンストラクタ
  Index();

  /*
  "directory"パラメータで指定されたディレクトリ内で見つけたデータからインデックスのインスタンスを作成する。
  もしデータがなければ新しい空のインデックスを作成する。
  もしディレクトリがなければ作成する。
  "isSubIndex"がtrueならマスターインデックスが存在し、それの子インデックスということになる。
  その場合、デーモンに接続できる許可は降りない。
  */
  Index(const char *directory, bool isSubIndex);

  // デストラクタ
  virtual ~Index();

protected:
  // コンフィグマネージャーからコンフィグ情報を得る
  virtual void getConfiguration();

  // メインのインデックスファイルから情報を読み取る
  void loadDataFromDisk();

  // データファイルにインデックス情報を書き込む
  void saveDataToDisk();
};

#endif