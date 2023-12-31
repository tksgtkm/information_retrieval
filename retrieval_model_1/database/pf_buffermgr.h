#ifndef PF_BUFFERMGR_H
#define PF_BUFFERMGR_H

#include "pf_internal.h"
#include "pf_hashtable.h"

#define INVALID_SLOT -1

struct PF_BufPageDesc {
  char *pData;
  int next;
  int prev;
  int bDirty;
  short int pinCount;
  PageNum pageNum;
  int fd;
};



#endif