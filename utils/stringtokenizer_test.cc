#include <iostream>
#include <vector>
#include <cassert>
#include "stringtokenizer.h"

void test_basic_tokenization() {
    StringTokenizer tokenizer("apple,banana,cat", ",");
    std::vector<std::string> tokens;
    while (tokenizer.hasNext()) {
        char *token = tokenizer.getNext();
        assert(token != nullptr);
        tokens.push_back(token);
    }

    assert(tokens.size() == 3);
    assert(tokens[0] == "apple");
    assert(tokens[1] == "banana");
    assert(tokens[2] == "cat");

    std::cout << "test_basic_tokenization passed.\n";
}

void test_split_function() {
    std::vector<std::string> result;
    StringTokenizer::split("dog|elephant|fox", "|", &result);
    assert(result.size() == 3);
    assert(result[0] == "dog");
    assert(result[1] == "elephant");
    assert(result[2] == "fox");

    std::cout << "test_split_function passed.\n";
}

void test_join_function() {
    std::vector<std::string> input = {"one", "two", "three"};
    std::string joined = StringTokenizer::join(input, "-");
    assert(joined == "one-two-three");

    std::cout << "test_join_function passed.\n";
}

void test_empty_input() {
    StringTokenizer tokenizer("", ",");
    assert(!tokenizer.hasNext());

    std::vector<std::string> result;
    StringTokenizer::split("", ",", &result);
    assert(result.empty());

    std::cout << "test_empty_input passed.\n";
}

int main() {
    test_basic_tokenization();
    test_split_function();
    test_join_function();
    test_empty_input();

    std::cout << "All tests passed.\n";
}