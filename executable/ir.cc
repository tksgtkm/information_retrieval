#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include "../utils/all.h"

#define PRINT_DEBUG_INFORMATION 1

static char workDir[MAX_CONFIG_VALUE_LENGTH + 32];

static int statusCode;

static void printHelp() {
	printf("Syntax: ir [--KEY=VALUE]\n\n");
	printf("KEY and VALUE can be arbitrary index configuration pairs. Give \"CONFIGURATION\"\n");
	printf("as KEY in order to process the configuration file given by VALUE.\n");
	printf("The index directory is specified using --DIRECTORY=...\n\n");
	exit(0);
}

static void processParameter(char *p) {
    if ((strcasecmp(p, "--help") == 0) || (strcasecmp(p, "-h") == 0))
        printHelp();
}

int main(int argc, char **argv) {
    initializeConfiguratorFromCommandLineParameters(argc, (const char**)argv);
    log(LOG_DEBUG, "Index", "Starting application");
    for (int i = 1; i < argc; i++)
        processParameter(argv[i]);

    // インデックスディレクトリがしていされているか確認(設定ファイルかコマンドライン)
    if (!getConfigurationValue("DIRECTORY", workDir)) {
        fprintf(stderr, "ERROR: No directory specified. Check .irconf file or give directory as command-line parameter.\n\n");
        exit(1);
    }

    // 複数のインデックスがあるか確認する
    // 複数のインデックスディレクトリがある場合は通常にIndexインスタンスではなく、
    // MasterIndexを作成する
    if (strchr(workDir, ',') != nullptr) {
        char *dirs[100];
        int indexCount = 0;
        StringTokenizer *tok = new StringTokenizer(workDir, ",");
        while (tok->hasNext()) {
            char *token = tok->getNext();
            if (token[strlen(token) - 1] == '/')
                dirs[indexCount++] = duplicateString(token);
            else
                dirs[indexCount++] = concatenateStrings(token, "/");
            if (indexCount >= 100)
                break;

            /*
            デバッグ用
            ./ir --DIRECTORY=dir1,dir2,dir3/
            dirs[0] = [dir1/]
            dirs[1] = [dir2/]
            dirs[2] = [dir3/]
            */
            printf("dirs[%d] = [%s]\n", indexCount - 1, dirs[indexCount - 1]);
        }
        delete tok;
    }
}