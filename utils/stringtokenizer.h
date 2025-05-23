#ifndef __STRINGTOKENIZER_H
#define __STRINGTOKENIZER_H

#include <string>
#include <vector>

class StringTokenizer {

public:
    /*
    stringを構成要素に分割するために新しいStringTokenizerを作成する
    stringの内容は変更されない。文字列を分割するためにdelimで指定された
    文字リストが使用される。
    */
   StringTokenizer(const char *string, const char *delim);

   ~StringTokenizer();

   // トークナイザーにさらにトークンがある場合にtrueを返す
   bool hasNext();

   // 次のトークンへのポインタを返す。呼び出し元はこれを開放できない。
   char *nextToken();

   char *getNext();

   // 指定された文字列をトークンに分割し、ベクターに格納する
   static void split(const std::string &s, const std::string &delim, std::vector<std::string> *v);

   // ベクター内の文字列をdelimを使って結合する
   static std::string join(const std::vector<std::string> &v, const std::string &delim);

private:

    char *string, *delim;

    int nextPosition, stringLength;
};

#endif