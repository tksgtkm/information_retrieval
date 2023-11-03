#include <cstdio>
#include <cstring>
#include "stringtokenizer.h"
#include "alloc.h"

StringTokenizer::StringTokenizer(const char *string_data, const char *delim) {
  this->string_data = (char *)malloc(strlen(string_data) + 2);
  strcpy(this->string_data, string_data);
  this->delim = (char *)malloc(strlen(delim) + 2);
  strcpy(this->delim, delim);
  nextPosition = 0;
  stringLength = strlen(string_data);
}

StringTokenizer::~StringTokenizer() {
  free(string_data);
  free(delim);
}

bool StringTokenizer::hasNext() {
  if (string_data[nextPosition] == 0)
    return false;
  else
    return true;
}

char *StringTokenizer::nextToken() {
  return getNext();
}

char *StringTokenizer::getNext() {
  if (nextPosition >= stringLength)
    return nullptr;
  int pos = nextPosition;
  while (string_data[pos] != 0) {
    bool found = false;
    for (int i = 0; delim[i] != 0; i++) {
      if (string_data[pos] == delim[i])
        found = true;
    }
    if (found)
      break;
    pos++;
  }
  if (string_data[pos] == 0)
    string_data[pos + 1] = 0;
  else
    string_data[pos] = 0;
  char *result = &string_data[nextPosition];
  nextPosition = pos + 1;
  return result;
}

void StringTokenizer::split(const std::string &s, const std::string &delim, std::vector<std::string> *v) {
  v->clear();
  StringTokenizer tokenizer(s.c_str(), delim.c_str());
  for (char *token = tokenizer.getNext(); token != nullptr; token = tokenizer.getNext())
    v->push_back(token);
}

std::string StringTokenizer::join(const std::vector<std::string> &v, const std::string &delim) {
  if (v.size() == 0)
    return "";
  int size = 0;
  for (int i = 0; i < v.size(); ++i)
    size += v[i].size() + delim.size();
  std::string result = v[0];
  result.reserve(size);
  for (int i = 1; i < v.size(); ++i)
    result += delim + v[i];
  return result;
}