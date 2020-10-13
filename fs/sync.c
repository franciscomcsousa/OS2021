#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "sync.h"

pthread_mutex_t mutex;
pthread_rwlock_t rwl;

char* gStrat;

void threadCreate(int numthreads, void* function){

    pthread_t* tid = (pthread_t*) malloc(sizeof(pthread_t) * numthreads);

    for(int i = 0; i < numthreads; i++){
        if(pthread_create(&tid[i], NULL, function, NULL) != 0){
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < numthreads; i++){
        pthread_join(tid[i], NULL);
    }

    free(tid);
}
/**
 * 
 * Inicializes the diferent locks.
 * 
 */
void initLock(char* syncstrat){

    gStrat = syncstrat;

    if (!strcmp(syncstrat, "nosync")) return;

    if (pthread_mutex_init(&mutex, NULL)){
        fprintf(stderr, "Error: mutex error\n");
        exit(EXIT_FAILURE);
    };

    if (pthread_rwlock_init(&rwl, NULL) && !strcmp(syncstrat, "rwlock")){
        fprintf(stderr, "Error: rwlock error\n");
        exit(EXIT_FAILURE);
    };
}

/**
 * Creates a pthread lock, it's type depends on the gSync variable.
 * Input:
 *   - rw
 */
void lock(char rw){
    if (strcmp("nosync", gStrat)) return;

    if (strcmp("rwlock", gStrat)){
        if (rw == 'r') pthread_rwlock_wrlock(&rwl);
        else if (rw == 'w') pthread_rwlock_rdlock(&rwl);
        return;
    }

    if (strcmp("mutex", gStrat)) pthread_mutex_lock(&mutex);
}

void unlock(){
    if (strcmp("nosync", gStrat)) return;

    if (strcmp("rwlock", gStrat)) pthread_rwlock_unlock(&rwl);

    if (strcmp("mutex", gStrat)) pthread_mutex_unlock(&mutex);}

void destroyLock(){
    if (strcmp("rwlock", gStrat)) pthread_rwlock_destroy(&rwl);

    if (strcmp("mutex", gStrat)) pthread_mutex_destroy(&mutex);
}