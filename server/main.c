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
#define BUFFER_SIZE 9
#define END "EOF"
#define SUCCESS 0
#define ENDOFFILE 1

#define INDIM 30
#define OUTDIM 512

struct timeval t1,t2;
pthread_mutex_t mutexfile;
pthread_cond_t canInsert, canRemove;

char buffer[BUFFER_SIZE][MAX_INPUT_SIZE];  //circular buffer
int counter = 0;                           //number of commands inside buffer
int insertPointer = 0;                     //pointer to next free location to insert command
int removePointer = 0;                     //pointer to next command to be read

/**
 * Inserts a command in the buffer.
 * @param data: to insert in the buffer
*/
int insertCommand(char* data){

    if(pthread_mutex_lock(&mutexfile) != 0){
        fprintf(stderr, "Error: mutex lock error\n");
        exit(EXIT_FAILURE);
    }
  
    while (counter == BUFFER_SIZE) 
        pthread_cond_wait(&canInsert,&mutexfile); //if full waits for a command to be removed

    strcpy(buffer[insertPointer],data);
    insertPointer++;
    if (insertPointer == BUFFER_SIZE) 
        insertPointer = 0;
    counter++;

    pthread_cond_signal(&canRemove);
    if(pthread_mutex_unlock(&mutexfile) != 0){
        fprintf(stderr, "Error: mutex unlock error\n");
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}

/**
 * Removes a command from the buffer.
 * @param array: to store a command
*/
int removeCommand(char* array){

    if(pthread_mutex_lock(&mutexfile) != 0){
        fprintf(stderr, "Error: mutex lock error\n");
        exit(EXIT_FAILURE);
    }

    while(counter == 0)
        pthread_cond_wait(&canRemove,&mutexfile); //if empty waits for command to be inserted

    strcpy(array,buffer[removePointer]);

    if(strcmp(array,END) == 0){               //if finds command END, ends thread
        pthread_cond_signal(&canRemove);     //doesnt remove command END
        pthread_mutex_unlock(&mutexfile);   //so that other threads can end too.
        return ENDOFFILE;
    }

    removePointer++;
    if (removePointer == BUFFER_SIZE) 
        removePointer = 0;
    counter--;

    pthread_cond_signal(&canInsert);
    if(pthread_mutex_unlock(&mutexfile) != 0){
        fprintf(stderr, "Error: mutex unlock error\n");
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void processInput(FILE* fp_input){
    char line[MAX_INPUT_SIZE];

    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), fp_input)) {  
        char token, type;
        char name[MAX_INPUT_SIZE];
        char path[MAX_INPUT_SIZE];
        char pathdest[MAX_INPUT_SIZE];

        int numTokens;

        if(line[0] == 'm')
            numTokens = sscanf(line, "%c %s %s", &token, path, pathdest); // different sscanf for move command
        else
            numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line) == EXIT_SUCCESS)
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line) == EXIT_SUCCESS)
                    break;
                return;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line) == EXIT_SUCCESS)
                    break;
                return;
            
            case 'm':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line) == EXIT_SUCCESS)
                    break;
                return;

            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
    }
    insertCommand(END);   //After reading everyting creates a command END to signal the end of the file.
                         //Allows threads to end so that the program can finish.
}

void applyCommands(){

   char input[MAX_INPUT_SIZE];

    while (1){
        
        if(removeCommand(input) == ENDOFFILE)
           break;

        char token, type;
        char name[MAX_INPUT_SIZE];
        char path[MAX_INPUT_SIZE];
        char pathdest[MAX_INPUT_SIZE];
        int numTokens;

        if(input[0] == 'm')
            numTokens = sscanf(input, "%c %s %s", &token, path, pathdest); // different sscanf for move command
        else
            numTokens = sscanf(input, "%c %s %c", &token, name, &type);

        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        printf("Create file: %s\n", name);
                        create(name, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                searchResult = lookup(name,'u');
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;
            case 'd':
                printf("Delete: %s\n", name);
                delete(name);
                break;

            case 'm':
                printf("Move: %s to %s\n",path,pathdest);
                move(path,pathdest);
                break;

            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
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
 * Inicializes mutex and conditional variables used to process file.
*/
void init_locks_file(){
    pthread_mutex_init(&mutexfile,NULL);
    pthread_cond_init(&canInsert,NULL);
    pthread_cond_init(&canRemove,NULL);
}

/**
 * Destroys mutex and conditional variables used to process file.
*/
void destroy_locks_file(){
    pthread_mutex_destroy(&mutexfile);
    pthread_cond_destroy(&canInsert);
    pthread_cond_destroy(&canRemove);
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

int main(int argc, char* argv[]) {

    //int numthreads = atoi(argv[3]);

    /* Verifies given input */
    verifyInput(argc, argv);

    /* Init mutex and cond */
    //init_locks_file();

    /* Init filesystem and locks */
    //init_fs();

    /* Creates array of thread id's */
    //pthread_t* tid = (pthread_t*) malloc(sizeof(pthread_t) * numthreads);

    /* Creates pool of threads to execute the commands inside the buffer */
    //threadCreate(tid,numthreads,applyCommands_aux);

    /* Reads input file to buffer */
    //processInput(fp_input);

    /* Joins threads */
    //threadJoin(tid,numthreads);

// SERVER

    int sockfd;
    struct sockaddr_un server_addr;
    socklen_t addrlen;
    char *path;

    if ((sockfd = socket(AF_UNIX,SOCK_DGRAM,0)) < 0){
        perror("server:can't open socket");
        exit(EXIT_FAILURE);
    }

    path = argv[2];

    unlink(path);

    addrlen = setSockAddrUn(argv[2],&server_addr);
    if (bind(sockfd, (struct sockaddr *) &server_addr, addrlen) < 0) {
        perror("server: bind error");
        exit(EXIT_FAILURE);
    }

    while (1) {
        struct sockaddr_un client_addr;
        char in_buffer[INDIM], out_buffer[OUTDIM];
        int c;

        addrlen=sizeof(struct sockaddr_un);
        c = recvfrom(sockfd, in_buffer, sizeof(in_buffer)-1, 0,(struct sockaddr *)&client_addr, &addrlen);
        if (c <= 0) 
            continue;
        //Preventivo, caso o cliente nao tenha terminado a mensagem em '\0', 
        in_buffer[c]='\0';
        
        printf("Recebeu mensagem de %s\n", client_addr.sun_path);

        c = sprintf(out_buffer, "Ola' %s, que tal vai isso?", in_buffer);
        
        sendto(sockfd, out_buffer, c+1, 0, (struct sockaddr *)&client_addr, addrlen);
  }

/* server never leaves the loop and therefore this is never executed */
    close(sockfd);
    unlink(argv[1]);
    exit(EXIT_SUCCESS);

    /* Release allocated memory and destroys locks */
    //free(tid);
    //destroy_locks_file();
    //destroy_fs();
}
