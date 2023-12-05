#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctime>
#include "../config/config.h"
#include "../index/index.h"
#include "../misc/backend.h"

#define RUNMODE_INDEX 1
#define RUNMODE_QUERY 2

#define CONFIG_FILE "default.config"

#undef byte
#define byte unsigned char

static const char *LOG_ID = "TREC-Frontend";

static const char *RUN_ID = "IR=TREC";

static int runMode;
static FILE *inputFile;
static FILE *outputFile;
static FILE *logFile;

static unsigned const char WHITESPACES[40] = {
	',', ';', '.', ':', '-', '_', '#', '\'', '+', '*', '~',
	'^', '!', '"', '$', '%', '&', '/', '(', ')',
	'[', ']', '{', '}', '=', '?', '\\', '<', '>', '|', 0
};

static char isWhiteSpace[256];

static const char *STOPWORDS[256] = {
	"",
	"a", "about", "are", "also", "and", "any", "as",
	"be", "been", "but", "by",
	"did", "does",
	"for",
	"had", "has", "have", "how",
	"etc",
	"if", "in", "is", "it", "its",
	"not",
	"of", "on", "or",
	"s", "so", "such",
	"than", "that", "the", "their", "there", "this", "then", "to",
	"was", "were", "what", "which", "who", "will", "with", "would",
	NULL
};

static const int HASHTABLE_SIZE = 7951;

static const char *STOPWORD_HASHTABLE[HASHTABLE_SIZE];

static Index *myIndex;
static char queryID[1024];

// 標準エラーとして使用法方を出力してプログラムを終了する
static void usage() {
	fprintf(stderr, "Usage:  trec (INDEX|QUERY) INPUT_FILE OUTPUT_FILE LOG_FILE\n\n");
	fprintf(stderr, "In INDEX mode, the INPUT_FILE contains a list of input files for which an\n");
	fprintf(stderr, "index should be created (one file per line).\n");
	fprintf(stderr, "In QUERY mode, the INPUT_FILE contains a list of flat search queries, one\n");
	fprintf(stderr, "per line, of the form: \"TOPIC_ID TERM_1 TERM_2 ... TERM_N\"\n\n");
	fprintf(stderr, "OUTPUT_FILE and LOG_FILE will contain the output data produced by Wumpus.\n\n");
	exit(1);
}

// 標準エラーにメッセージを出力し、プログラムを終了する
static void dieWithMessage(const char *s1, const char *s2) {
  fprintf(stderr, s1, s2);
  exit(1);
}

// コマンドラインパラメータを処理し、すべてのファイルが存在することを確認する
static void processParametes(int argc, char **argv) {
  if (strcasecmp(argv[1], "index") == 0)
    runMode = RUNMODE_INDEX;
  else if (strcasecmp(argv[1], "query") == 0)
    runMode = RUNMODE_QUERY;
  else
    dieWithMessage("Illegal run mode: %s\n", argv[1]);
  inputFile = fopen(argv[2], "r");
  if (inputFile == nullptr)
    dieWithMessage("Input run does not exist: %s\n", argv[2]);
  outputFile = fopen(argv[3], "a");
  if (outputFile == nullptr)
    dieWithMessage("Unable to create output file: %s\n", argv[3]);
  fclose(outputFile);
  logFile = fopen(argv[4], "a");
  if (logFile == nullptr)
    dieWithMessage("Unable to create log file: %s\n", argv[4]);
  fclose(logFile);

  fprintf(stderr, "Starting execution. Everything will be logged to \"%s\" and \"%s\".\n", argv[3], argv[4]);
  fprintf(stderr, "All data will be appended at the end of the respective file.\n\n");

  // freopen: 新たにファイルをオープンし、ストリームと結びつける
  // cf: https://www.ibm.com/docs/ja/i/7.1?topic=ssw_ibm_i_71/rtref/freopen.html
  stdout = freopen(argv[3], "a", stdout);
  assert(stdout != nullptr);
  stderr = freopen(argv[4], "a", stderr);
  assert(stderr != nullptr);
}

// コンフィグレーションファイルを読み込み、コンフィグレーションサービスを初期化する
static void initConfig() {
  static const char *CONFIG[20] = {
	"LOG_LEVEL=2",
	"LOG_FILE=stderr",
	"STEMMING_LEVEL=3",
	"MERGE_AT_EXIT=true",
	"MAX_FILE_SIZE=3000M",
	"MAX_UPDATE_SPACE=240M",
	"UPDATE_STRATEGY=NO_MERGE",
	"DOCUMENT_LEVEL_INDEXING=2",
	"COMPRESSED_INDEXCACHE=true",
	"POSITIONLESS_INDEXING=true",
	"LEXICON_TYPE=TERABYTE_LEXICON",
	"HYBRID_INDEX_MAINTENANCE=false",
	"APPLY_SECURITY_RESTRICTIONS=false",
	"CACHED_EXPRESSIONS=\"<doc>\"..\"</doc>\"",
	nullptr
  };
  int configCnt = 0;
  while (CONFIG[configCnt] != nullptr)
    configCnt++;
  initializeConfiguratorFromCommandLineParameters(configCnt, CONFIG);
  struct stat buf;
  if (stat(CONFIG_FILE, &buf) != 0)
    dieWithMessage("Unable to open configuration file: %s\n", CONFIG_FILE);
  initializeConfigurator(CONFIG_FILE, nullptr);

  memset(isWhiteSpace, 0, sizeof(isWhiteSpace));
  for (int i = 1; i <= 32; i++)
    isWhiteSpace[i] = 1;
  for (int i = 0; WHITESPACES[i] != 0; i++)
    isWhiteSpace[(byte)WHITESPACES[i]] = 1;

  memset(STOPWORD_HASHTABLE, 0, sizeof(STOPWORD_HASHTABLE));
  for (int i = 0; STOPWORDS[i] != nullptr; i++) {
	int slot = simpleHashFunction(STOPWORDS[i]) % HASHTABLE_SIZE;
	if (STOPWORD_HASHTABLE[slot] != nullptr)
	  fprintf(stderr, "%s <-> %s\n", STOPWORDS[i], STOPWORD_HASHTABLE[slot]);
	assert(STOPWORD_HASHTABLE[slot] == nullptr);
	STOPWORD_HASHTABLE[slot] = STOPWORDS[i];
  }
}

/*
STOPWORDS配列で定義されている指定された用語がストップワード
のリスト内で見つかった場合はtrueを返す。
STOPWORD_HASHTABLEは、検索を高速化するために使用する
*/
static bool isStopWord(char *term) {
  int hashSlot = simpleHashFunction(term) % HASHTABLE_SIZE;
  if (STOPWORD_HASHTABLE[hashSlot] == nullptr)
    return false;
  else
    return (strcmp(term, STOPWORD_HASHTABLE[hashSlot]) == 0);
}

// ログファイル(LOG_OUTPUT)に現在のタイムスタンプ表示する。
static void logTimeStamp() {
  char msg[64];
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  sprintf(msg, "Timestamp: %d.%03d", (int)tv.tv_sec, (int)(tv.tv_usec / 1000));
  log(LOG_OUTPUT, LOG_ID, msg);
}

// inputFileからファイル名を読み込む。ファイル名ごとにファイルをインデックスに追加される
static void buildIndex() {
  char line[8192];
  log(LOG_OUTPUT, LOG_ID, "Building index");
  logTimeStamp();

  // myIndex = new Index("./database", false);
  // int cnt = 0;
  // while (fgets(line, sizeof(line), inputFile) != nullptr) {
  //   char *fileName = chop(line);
  //   if (strlen(fileName) > 0) {
      
  //   }
  //   free(fileName);
  // }
  // delete myIndex;

  // sprintf(line, "%d files indexed Done.", cnt);
  // log(LOG_OUTPUT, LOG_ID, line);
  // logTimeStamp();
}

int main(int argc, char **argv) {
  if (argc != 5)
    usage();
  processParametes(argc, argv);
  initConfig();

}