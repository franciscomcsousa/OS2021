#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include "fs/operations.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_INPUT_SIZE 100

int sockfd; //server socket

pthread_mutex_t mutex;
pthread_mutex_t mutexEnd;
pthread_cond_t canPrint, canContinue;
int flag = 0;
int count = 0;

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

// counter = numthreads;

void applyCommands(){

    int c;
    socklen_t addrlen;
    char input[MAX_INPUT_SIZE];
    struct sockaddr_un client_addr;
    addrlen=sizeof(struct sockaddr_un);

    while (1){

        /**
         * pthread_mutex_lock(&mutex);
         * if(flag == 1)
         *  counter--;
         *  if(counter == 0)
         *      pthread_cond_signal(&podeExecutar);
         *  while(flag == 1)
         *      pthread_cond_wait(&podeContinuar,&mutex);
         * flag = 0;
         * pthread_mutex_unlock(&mutex);
        */

        c = recvfrom(sockfd, input, sizeof(input)-1, 0,(struct sockaddr *)&client_addr, &addrlen);
        
        //printf("From the function, the thread id = %ld\n", pthread_self());
        //for(long i =0;i<100000000;i++);  delay

        if (c<=0)
            break;
        input[c]='\0';

        int numTokens;
        char token, type;
        char name[MAX_INPUT_SIZE];
        char path[MAX_INPUT_SIZE];
        char pathdest[MAX_INPUT_SIZE];


        pthread_mutex_lock(&mutex);
        while (flag == 1)
            pthread_cond_wait(&canContinue, &mutex);
        count++;
        pthread_mutex_unlock(&mutex);

        if(input[0] == 'm')
            numTokens = sscanf(input, "%c %s %s", &token, path, pathdest); // different sscanf for move command
        else
            numTokens = sscanf(input, "%c %s %c", &token, name, &type);

        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int Result;
        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        printf("Create file: %s\n", name);
                        Result = create(name, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        Result = create(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                Result = lookup(name,'u');
                if (Result >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;
            case 'd':
                printf("Delete: %s\n", name);
                Result = delete(name);
                break;

            case 'm':
                printf("Move: %s to %s\n",path,pathdest);
                Result = move(path,pathdest);
                break;

            case 'p':
                printf("Print tecnicofs tree\n");
                //Result = print_tecnicofs_tree(name);
                /**
                 * pthread_mutex_lock(&mutex)
                 * flag = 1
                 * while(counter != 0)
                 *  pthread_cond_wait(&podeComecar,&mutex);
                 * pthread_mutex_unlock(&mutex)
                 * print()
                 * pthread_cond_broadcast(&podeContinuar);
                */
                pthread_mutex_lock(&mutex);
                flag = 1;
                while (count != 0)
                    pthread_cond_wait(&canPrint, &mutex);
                Result = print_tecnicofs_tree(name);
                flag = 0;
                pthread_cond_signal(&canContinue);
                pthread_mutex_unlock(&mutex);
                break;

            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
        sendto(sockfd, &Result, sizeof(Result), 0, (struct sockaddr *)&client_addr, addrlen);
        pthread_mutex_lock(&mutex);
        count--;
        if (count == 0)
            pthread_cond_signal(&canPrint);
        pthread_mutex_unlock(&mutex);
    }
}

/**
 * Auxiliar function called during thread create.
*/
void *applyCommands_aux(){
    applyCommands();
    return NULL;
}

/**
 * Verifies the validity of the arguments given as input.
 * @param argc: number of arguments given by user
 * @param argv: array from stdin given by user
*/
void verifyInput(int argc, char* argv[]){
    if (argc != 3){
        fprintf(stderr, "Error: invalid number of arguments\n");
        exit(EXIT_FAILURE);
    }
    /*argv[3] refers to the number of threads*/
    if (atoi(argv[1]) <= 0){
        fprintf(stderr, "Error: invalid number of threads\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * Creates pool of threads.
 * @param tid: array of thread id's
 * @param numthreads: number of threads
 * @param function: function to execute
*/
void threadCreate(pthread_t* tid, int numthreads, void* function){
    for(int i = 0; i < numthreads; i++){
        if(pthread_create(&tid[i], NULL, function, NULL) != 0){
            fprintf(stderr, "Error: creating threads\n");
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * Joins pool of threads.
 * @param tid: array of thread id's
 * @param numthreads: number of threads
*/
void threadJoin(pthread_t* tid, int numthreads){
    for (int i = 0; i < numthreads; i++){
        if(pthread_join(tid[i], NULL) != 0){
            fprintf(stderr, "Error: joining threads\n");
            exit(EXIT_FAILURE);
        }
    }
}

int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

void initSocket(char* argv2){  

    char *path;
    socklen_t addrlen;
    struct sockaddr_un server_addr;

    if ((sockfd = socket(AF_UNIX,SOCK_DGRAM,0)) < 0){
        perror("server:can't open socket");
        exit(EXIT_FAILURE);
    }

    path = argv2;

    unlink(path);

    addrlen = setSockAddrUn(argv2,&server_addr);
    if (bind(sockfd, (struct sockaddr *) &server_addr, addrlen) < 0) {
        perror("server: bind error");
        exit(EXIT_FAILURE);
    }
}

/**
 * Inicializes mutex and conditional variables used to process file.
*/
void init_locks_file(){
    pthread_mutex_init(&mutex,NULL);
    pthread_mutex_init(&mutexEnd,NULL);
    pthread_cond_init(&canPrint,NULL);
    pthread_cond_init(&canContinue,NULL);
}

/**
 * Destroys mutex and conditional variables used to process file.
*/
void destroy_locks_file(){
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&mutexEnd);
    pthread_cond_destroy(&canPrint);
    pthread_cond_destroy(&canContinue);
}

int main(int argc, char* argv[]) {

    int numthreads = atoi(argv[1]);

    /* Verifies given input */
    verifyInput(argc, argv);

    init_locks_file();

    /* Init filesystem and locks */
    init_fs();

    /* Init server socket */
    initSocket(argv[2]);

    /* Creates array of thread id's */
    pthread_t* tid = (pthread_t*) malloc(sizeof(pthread_t) * numthreads);

    /* Creates pool of threads to execute the commands inside the buffer */
    threadCreate(tid,numthreads,applyCommands_aux);



    /*-------------------------Never reaches this part-----------------------------*/
    /* Joins threads */
    threadJoin(tid,numthreads);

    /* Release allocated memory and destroys locks */
    free(tid);
    destroy_fs();

    /* server never leaves the loop and therefore this is never executed */
    destroy_locks_file();
    close(sockfd);
    unlink(argv[1]);
    exit(EXIT_SUCCESS);
}
