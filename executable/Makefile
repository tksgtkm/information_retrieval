CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2
UTILS_DIR := ../utils

SRC := $(UTILS_DIR)/utils.cc \
       $(UTILS_DIR)/configurator.cc \
       $(UTILS_DIR)/stringtokenizer.cc \
       $(UTILS_DIR)/logging.cc

HEADERS := $(UTILS_DIR)/utils.h \
           $(UTILS_DIR)/configurator.h \
           $(UTILS_DIR)/stringtokenizer.h \
           $(UTILS_DIR)/logging.h \
           $(UTILS_DIR)/compression.h \
           $(UTILS_DIR)/all.h

TARGETS := ir

all: $(TARGETS)

ir: ir.cc $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ ir.cc $(SRC)

clean:
	rm -f $(TARGETS)