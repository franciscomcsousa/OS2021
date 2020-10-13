#ifndef SYNC_H
#define SYNC_H

void threadCreate(int numthreads, void* function);
void initLock(char* syncstrat);
void lock(char rw);
void unlock();
void destroyLock();

#endif