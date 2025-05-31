#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include "../index/index.h"
#include "../utils/all.h"

int main() {
    initializeConfigurator();
    
    const char *testDir = "/tmp/test_index_main";

    // 初期状態確認
    std::cout << "[Test] Cleaning test directory if exists..." << std::endl;
    std::string cleanup = "rm -rf " + std::string(testDir);
    system(cleanup.c_str());

    // 存在していないこと確認
    if (access(testDir, F_OK) == 0) {
        std::cerr << "Directory should not exist!" << std::endl;
        return 1;
    }

    std::cout << "[Test] Creating Index object..." << std::endl;
    Index index(testDir, false);

    struct stat st;
    if (stat(testDir, &st) == 0 && S_ISDIR(st.st_mode)) {
        std::cout << "[OK] Directory successfully created: " << testDir << std::endl;
    } else {
        std::cerr << "[FAIL] Directory was not created!" << std::endl;
        return 1;
    }

    // .index_disallow ファイルが存在するか
    std::string disallowFile = std::string(testDir) + "/.index_disallow";
    if (access(disallowFile.c_str(), F_OK) == 0) {
        std::cout << "[OK] .index_disallow file created." << std::endl;
    } else {
        std::cerr << "[FAIL] .index_disallow file not found!" << std::endl;
    }

    // クリーンアップ
    std::cout << "[Test] Cleaning up..." << std::endl;
    system(cleanup.c_str());

    return 0;
}