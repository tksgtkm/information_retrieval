#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>

#include "lockable.h"
#include "backend.h"

static bool lockingEnabled = true;

static char *LOG_ID = "Lockable";

Lockable::Lockable() {
  maxSimultaneousReaders = MAX_SIMULTANEOUS_READERS;
  SEM_INIT(internalDataSemaphore, 1);
  SEM_INIT(readWriteSemaphore, maxSimultaneousReaders);
  sem_wait(&internalDataSemaphore);
  for (int i = 0; i < maxSimultaneousReaders; i++)
    readLockFree[i] = true;
  writeLockFree = true;
  lockHolder = (pthread_t)0;
  sem_post(&internalDataSemaphore);
}

Lockable::~Lockable() {
  sem_wait(&internalDataSemaphore);
  sem_destroy(&readWriteSemaphore);
  sem_destroy(&internalDataSemaphore);
}

void Lockable::disableLocking() {
  lockingEnabled = false;
}

void Lockable::setMaxSimultaneousReaders(int value) {
  if (value < 1)
    value = 1;
  if (value > 64)
    value = 64;
  sem_wait(&internalDataSemaphore);
  for (int i = 0; i < maxSimultaneousReaders; i++) {
    if ((!readLockFree[i]) || (!writeLockFree)) {
      assert("Impossible to change the maximum number if simultaneous readers" == nullptr);
      sem_post(&internalDataSemaphore);
      return;
    }
  }
  maxSimultaneousReaders = value;
  for (int i = 0; i < maxSimultaneousReaders; i++)
    readLockFree[i] = true;
  writeLockFree = true;
  sem_post(&internalDataSemaphore);
}

bool Lockable::hasReadLock() {
  if (!lockingEnabled)
    return false;
  sem_wait(&internalDataSemaphore);
  pthread_t thisThread = pthread_self();
  for (int i = 0; i < maxSimultaneousReaders; i++) {
    if ((readLockFree[i] == false) && (pthread_equal(readLockHolders[i], thisThread))) {
      sem_post(&internalDataSemaphore);
      return true;
    }
  }
  sem_post(&internalDataSemaphore);
  return false;
}

bool Lockable::getReadLock() {
  if ((!lockingEnabled) || (hasReadLock()))
    return false;
  sem_wait(&readWriteSemaphore);
  sem_wait(&internalDataSemaphore);
  for (int i = 0; i < maxSimultaneousReaders; i++) {
    if (readLockFree[i]) {
      readLockFree[i] = false;
      readLockHolders[i] = pthread_self();
      break;
    }
  }
  sem_post(&internalDataSemaphore);
  return true;
}

void Lockable::releaseReadLock() {
  if (!lockingEnabled)
    return;
  pthread_t thisThread = pthread_self();
  sem_wait(&internalDataSemaphore);
  for (int i = 0; i < maxSimultaneousReaders; i++) {
    if (readLockFree[i] == false) {
      if (pthread_equal(readLockHolders[i], thisThread)) {
        readLockFree[i] = true;
        sem_post(&readWriteSemaphore);
      }
    }
  }
  sem_post(&internalDataSemaphore);
}

bool Lockable::hasWriteLock() {
  if (!lockingEnabled)
    return false;
  pthread_t thisThread = pthread_self();
  if ((!writeLockFree) && (pthread_equal(writeLockHolder, thisThread)))
    return true;
  else
    return false;
}

bool Lockable::getWriteLock() {
  if ((!lockingEnabled) || (hasWriteLock()))
    return false;
  for (int i = 0; i < maxSimultaneousReaders; i++)
    sem_wait(&readWriteSemaphore);
  sem_wait(&internalDataSemaphore);
  writeLockFree = false;
  writeLockHolder = pthread_self();
  sem_post(&internalDataSemaphore);
  return true;
}

void Lockable::releaseWriteLock() {
  if ((!lockingEnabled) || (!hasWriteLock()))
    return;
  sem_wait(&internalDataSemaphore);
  writeLockFree = true;
  writeLockHolder = (pthread_t)0;
  sem_post(&internalDataSemaphore);
  for (int i = 0; i < maxSimultaneousReaders; i++)
    sem_post(&readWriteSemaphore);
}

void Lockable::releaseAnyLock() {
  if (!lockingEnabled)
    return;
  releaseReadLock();
  releaseWriteLock();
  releaseLock();
}

bool Lockable::getLock() {
  if (!lockingEnabled)
    return false;
  pthread_t thisThread = pthread_self();
  if (lockHolder == thisThread)
    return false;
  sem_wait(&internalDataSemaphore);
  lockHolder = thisThread;
  return true;
}

void Lockable::releaseLock() {
  if (!lockingEnabled)
    return;
  if (lockHolder != pthread_self())
    return;
  lockHolder = (pthread_t)0;
  sem_post(&internalDataSemaphore);
}

void Lockable::getClassName(char *target) {
  strcpy(target, "Lockable");
}

LocalLock::LocalLock(Lockable *lockable) {
  this->lockable = lockable;
  mustReleaseLock = lockable->getLock();
}

LocalLock::~LocalLock() {
  if (mustReleaseLock)
    lockable->releaseLock();
}