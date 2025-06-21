CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g

SRC_DIR := ../../index
UTILS_DIR := ../../utils

SRCS := $(SRC_DIR)/index.cc
TEST_SRC := index_test.cc
UTILS_SRCS := \
    $(UTILS_DIR)/configurator.cc \
    $(UTILS_DIR)/logging.cc \
    $(UTILS_DIR)/stringtokenizer.cc \
    $(UTILS_DIR)/utils.cc

# BUILD_DIR := ../build
# BIN := $(BUILD_DIR)/test_index
BIN := test_index

all: $(BIN)

$(BIN): $(SRCS) $(TEST_SRC) $(UTILS_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

run: all
	@echo "[Run] Starting test..."
	./$(BIN)

clean:
	rm -rf $(BIN)

.PHONY: all clean run