#ifndef __MASTER_INDEX_H
#define __MASTER_INDEX_H

class MasterIndex {

public:

    // MasterIndex(const char *directory);

    MasterIndex(int subIndexCount, char **subIndexDirs);

    ~MasterIndex();
};

#endif