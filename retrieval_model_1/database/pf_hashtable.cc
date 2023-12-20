#include "pf_internal.h"
#include "pf_hashtable.h"

PF_HashTable::PF_HashTable(int _numBuckets) {
  this->numBuckets = _numBuckets;

  hashTable = new PF_HashEntry* [numBuckets];

  for (int i = 0; i < numBuckets; i++)
    hashTable[i] = nullptr;
}

PF_HashTable::~PF_HashTable() {
  for (int i = 0; i < numBuckets; i++) {
    PF_HashEntry *entry = hashTable[i];
    while (entry != nullptr) {
      PF_HashEntry *next = entry->next;
      delete entry;
      entry = next;
    }
  }

  delete[] hashTable;
}

RC PF_HashTable::Find(int fd, PageNum pageNum, int &slot) {
  int bucket = Hash(fd, pageNum);
  
  if (bucket < 0)
    return (PF_HASHNOTFOUND);

  for (PF_HashEntry *entry = hashTable[bucket]; entry != nullptr; entry = entry->next) {
    if (entry->fd == fd && entry->pageNum == pageNum) {
      slot = entry->slot;
      return 0;
    }
  }

  return PF_HASHNOTFOUND;
}

RC PF_HashTable::Insert(int fd, PageNum pageNum, int slot) {
  int bucket = Hash(fd, pageNum);

  PF_HashEntry *entry;
  for (entry = hashTable[bucket]; entry != nullptr; entry = entry->next) {
    if (entry->fd == fd && entry->pageNum == pageNum)
      return PF_HASHPAGEEXIST;
  }

  if ((entry = new PF_HashEntry) == nullptr)
    return PF_NOMEM;
}