#ifndef ONDISK_INDEX_H
#define ONDISK_INDEX_H

#include "index_types.h"
#include "../misc/lockable.h"

class OnDiskIndex : public Lockable {
public:
  OnDiskIndex() {}

  virtual ~OnDiskIndex() {}
};

#endif