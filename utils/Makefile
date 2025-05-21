CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2

TARGET = utils_test

SRCS = utils.cc utils_test.cc

OBJS = $(SRCS:.cc=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)