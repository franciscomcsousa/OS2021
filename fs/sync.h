#ifndef SYNC_H
#define SYNC_H

void executionTime(struct timeval t1,struct timeval t2);
//void threadCreate(int numthreads, void* function, FILE* fp_input);
void commandLock();
void commandUnlock();

#endif /* SYNC_H */