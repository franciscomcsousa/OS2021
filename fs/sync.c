#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "sync.h"

struct timeval t1,t2;

pthread_mutex_t mutex;
pthread_rwlock_t rwl;

char* gStrat; /* Strategy sync chosen by user */

/**
 * Prints execution time.
*/
void executionTime(struct timeval t1,struct timeval t2){

    double time = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec)/1000000.0;
    printf("TecnicoFS completed in %.4f seconds.\n",time);
}

/**
 * Creates pool of threads and after they finish, join them.
 * Also keeps track of execution time, printint it at the end.
 * Input:
 * - numthreads: number of threads to create
 * - function:   function to be executed by threads
*/
void threadCreate(int numthreads, void* function){

    pthread_t* tid = (pthread_t*) malloc(sizeof(pthread_t) * numthreads);

    /* starts timer */
    if (gettimeofday(&t1,NULL)){
        fprintf(stderr, "Error: system time\n");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < numthreads; i++){
        if(pthread_create(&tid[i], NULL, function, NULL) != 0){
            fprintf(stderr, "Error: creating threads\n");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < numthreads; i++){
        if(pthread_join(tid[i], NULL) != 0){
            fprintf(stderr, "Error: joining threads\n");
            exit(EXIT_FAILURE);
        }
    }

    free(tid);

    /* ends timer */
    if (gettimeofday(&t2,NULL)){
        fprintf(stderr, "Error: system time\n");
        exit(EXIT_FAILURE);
    }

    executionTime(t1,t2);
}

/**
 * Inicializes the necessary locks.
 * Input:
 * - syncstrat: sync strategy given by user
 */
void initLock(char* syncstrat){

    gStrat = syncstrat;

    if (pthread_mutex_init(&mutex, NULL)){
        fprintf(stderr, "Error: mutex error\n");
        exit(EXIT_FAILURE);
    };

    if (!strcmp(syncstrat, "nosync")) 
        return;

    if (pthread_rwlock_init(&rwl, NULL) && !strcmp(syncstrat, "rwlock")){
        fprintf(stderr, "Error: rwlock error\n");
        exit(EXIT_FAILURE);
    };
}

/**
 * Locks a thread lock.
 * Input:
 *   - rw: type of rwlock, either read or write
 */
void lock(char rw){
    if (strcmp("nosync", gStrat)) 
        return;

    if (strcmp("rwlock", gStrat)){
        if (rw == 'w')
            pthread_rwlock_wrlock(&rwl);
        else if (rw == 'r') 
            pthread_rwlock_rdlock(&rwl);

        return;
    }

    if (strcmp("mutex", gStrat)) 
        pthread_mutex_lock(&mutex);
}

/**
 * Unlocks a thread lock.
*/
void unlock(){
    if (strcmp("nosync", gStrat)) 
        return;

    if (strcmp("rwlock", gStrat)) 
        pthread_rwlock_unlock(&rwl);

    if (strcmp("mutex", gStrat)) 
        pthread_mutex_unlock(&mutex);
}

/**
 * Destroys created locks
*/
void destroyLock(){
    if (strcmp("rwlock", gStrat))
        pthread_rwlock_destroy(&rwl);

    /* always created for removeCommand() and therefore always destroyed */
    pthread_mutex_destroy(&mutex);
}