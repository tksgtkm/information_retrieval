CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2

# テスト対象ソースとヘッダ
SRC := utils.cc logging.cc configurator.cc stringtokenizer.cc
HEADERS := utils.h logging.h configurator.h compression.h stringtokenizer.h

# テストファイル
TESTS := utils_test configurator_test stringtokenizer_test

# デフォルトターゲット
all: $(TESTS)

# 各テストのビルドルール
utils_test: utils_test.cc $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ utils_test.cc $(SRC)

configurator_test: configurator_test.cc $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ configurator_test.cc $(SRC)

stringtokenizer_test: stringtokenizer_test.cc $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ stringtokenizer_test.cc $(SRC)

# クリーン
clean:
	rm -f $(TESTS)
