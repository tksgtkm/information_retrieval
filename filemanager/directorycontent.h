#ifndef __DIRECTORYCONTENT_H
#define __DIRECTORYCONTENT_H

/*
DirectoryContentはディレクトリの内容(ファイルとサブディレクトリ)を管理するために
使用される。メモリ使用量を抑えるため、２分探索木ではなく長いソート済みリストと、
短いソートされていないIDリストを使用する。
短いリストの長さはsqrt(n)となり、nは長いリストの長さ
これにより、O(sqrt(n))で検索できる
これは、約10000個未満の子ディレクトリを含むディレクトリであれば許容できる時間
DirectoryContentオブジェクトには多数のファイルIDとディレクトリIDが含まれる
正のID値はファイルを参照し、負のID値はディレクトリを参照する
*/

#include <sys/types.h>
#include "data_structure.h"
#include "../utils/all.h"

class FileManager;

void initializeDirectoryContent(DicrectoryContent *dc);

#endif