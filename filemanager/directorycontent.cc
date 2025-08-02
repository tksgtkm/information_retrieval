#include <cmath>
#include <cstdio>
#include <cstring>
#include "directorycontent.h"
#include "data_structure.h"
#include "filemanager.h"
#include "../utils/all.h"

void initializeDirectoryContent(DicrectoryContent *dc) {
    dc->count = 0;
    dc->longAllocated = 0;
    dc->longList = nullptr;
    dc->shortCount = 0;
    dc->shortSlotsAllocated = 4;
    dc->shortList = typed_malloc(DC_ChildSlot, dc->shortSlotsAllocated);
}