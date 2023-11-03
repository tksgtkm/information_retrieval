#ifndef STRINGTOKENIZER_H
#define STRINGTOKENIZER_H

#include <string>
#include <vector>

class StringTokenizer {
private:
  char *string_data, *delim;

  int nextPosition, stringLength;

public:
  /*
  コンポーネント内でstring_dataを分割するために新しいStringTokenizerを作成する
  string_dataの内容は変更されない、delim内の文字列は文字列を分割するために用いる
  */
  StringTokenizer(const char *string_data, const char *delim);

  ~StringTokenizer();

  // tokenizerにtokenがある場合はtrueを返す
  bool hasNext();

  // next tokenにポインタを返す
  char *nextToken();

  // nextTokenと同じ
  char *getNext();
  
  // 文字列をトークンに分割し、ベクターに格納する
  static void split(const std::string &s, const std::string &delim, std::vector<std::string> *v);

  // トークンの反転操作：delimを使用してベクター内の文字列を結合させる
  static std::string join(const std::vector<std::string> &v, const std::string &delim);
  
};

#endif