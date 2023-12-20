#ifndef PF_H
#define PF_H

#include "base.h"

// ファイル内のページ番号
typedef int PageNum;

const int PF_PAGE_SIZE = 4096 - sizeof(int);

struct PF_FileHdr {
  int firstFree;
  char *pPageData;
};

class PF_BufferMgr;

class PF_FileHandle {
  friend class PF_Manager;
public:
  PF_FileHandle();
  ~PF_FileHandle();
};

class PF_Manager {
public:
  PF_Manager();
  ~PF_Manager();
  RC CreateFile(const char *fileName);
  RC DestroyFile(const char *fileName);

  RC OpenFile(const char *fileName, PF_FileHandle &fileHandle);
  RC CloseFile(PF_FileHandle &PF_FileHandle);

  RC ClearBuffer();
  RC PrintBuffer();
  RC ResizeBuffer(int iNewSize);

  RC GetBlockSize(int &length) const;
  RC AllocateBlock(char *&buffer);
  RC DisposeBlock(char *buffer);
};

#endif