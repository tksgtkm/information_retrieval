#ifndef PF_H
#define PF_H

#include "base.h"

// ファイル内のページ番号
typedef int PageNum;

const int PF_PAGE_SIZE = 4096 - sizeof(int);

class PF_PageHandle {
  friend class PF_FileHandle;
public:
  PF_PageHandle();
  ~PF_PageHandle();
  PF_PageHandle(const PF_PageHandle &pageHandle);
  PF_PageHandle& operator=(const PF_PageHandle &paggeHandle);

  RC GetData(char *&pData) const;
  RC GetPageNum(PageNum &pageNum) const;

private:
  int pageNum;
  char *pPageData;
};

struct PF_FileHdr {
  int firstFree;
  char *numPages;
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

#define PF_PAGEPINNED      (START_PF_WARN + 0) // page pinned in buffer
#define PF_PAGENOTINBUF    (START_PF_WARN + 1) // page isn't pinned in buffer
#define PF_INVALIDPAGE     (START_PF_WARN + 2) // invalid page number
#define PF_FILEOPEN        (START_PF_WARN + 3) // file is open
#define PF_CLOSEDFILE      (START_PF_WARN + 4) // file is closed
#define PF_PAGEFREE        (START_PF_WARN + 5) // page already free
#define PF_PAGEUNPINNED    (START_PF_WARN + 6) // page already unpinned
#define PF_EOF             (START_PF_WARN + 7) // end of file
#define PF_TOOSMALL        (START_PF_WARN + 8) // Resize buffer too small
#define PF_LASTWARN        PF_TOOSMALL

#define PF_NOMEM           (START_PF_ERR - 0)  // no memory
#define PF_NOBUF           (START_PF_ERR - 1)  // no buffer space
#define PF_INCOMPLETEREAD  (START_PF_ERR - 2)  // incomplete read from file
#define PF_INCOMPLETEWRITE (START_PF_ERR - 3)  // incomplete write to file
#define PF_HDRREAD         (START_PF_ERR - 4)  // incomplete read of header
#define PF_HDRWRITE        (START_PF_ERR - 5)  // incomplete write to header

// Internal errors
#define PF_PAGEINBUF       (START_PF_ERR - 6) // new page already in buffer
#define PF_HASHNOTFOUND    (START_PF_ERR - 7) // hash table entry not found
#define PF_HASHPAGEEXIST   (START_PF_ERR - 8) // page already in hash table
#define PF_INVALIDNAME     (START_PF_ERR - 9) // invalid PC file name

// Error in UNIX system call or library routine
#define PF_UNIX            (START_PF_ERR - 10) // Unix error
#define PF_LASTERROR       PF_UNIX

#endif