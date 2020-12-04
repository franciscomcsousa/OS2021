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

int sockfd; //server file descriptor

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
} 

void applyCommands(){

    int c;
    socklen_t addrlen;
    char input[MAX_INPUT_SIZE];
    struct sockaddr_un client_addr;
    addrlen=sizeof(struct sockaddr_un);

    while (1){

        /* reads bytes into input through sockfd and returns the number of bytes read */
        if ((c = recvfrom(sockfd, input, sizeof(input)-1, 0,(struct sockaddr *)&client_addr, &addrlen)) <= 0){
            perror("server: recvfrom error");
            break;
        }

        /* always sets last char of input to '\0' */
        input[c]='\0';

        int numTokens;
        char token, type;
        char name[MAX_INPUT_SIZE];
        char path[MAX_INPUT_SIZE];
        char pathdest[MAX_INPUT_SIZE];

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
                printf("Print tree\n");
                Result = print_tecnicofs_tree(name);
                break;

            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
        /* sends bytes of Result on sockfd to client_addr */
        if (sendto(sockfd, &Result, sizeof(Result), 0, (struct sockaddr *)&client_addr, addrlen) < 0){
            perror("server: sendto error");
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
    /* argv[1] refers to the number of threads */
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

/**
 * Resets and set socket address.
 * @param path: socket name
 * @param addr: socket address
*/
int setSockAddrUn(char *path, struct sockaddr_un *addr) {

    if (addr == NULL)
        return 0;

    /* set addr bytes to zero */
    bzero((char *)addr, sizeof(struct sockaddr_un));

    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    return SUN_LEN(addr);
}

/**
 * Inicializes server socket.
 * @param socket_name: socket name
*/
void initSocket(char* socket_name){  

    char *path;
    socklen_t addrlen;
    struct sockaddr_un server_addr;

    /* creates a socket of domain UNIX and type DATAGRAM */
    if ((sockfd = socket(AF_UNIX,SOCK_DGRAM,0)) < 0){
        perror("server:can't open socket");
        exit(EXIT_FAILURE);
    }

    path = socket_name;

    unlink(path);

    /* clears and set the socket address */
    addrlen = setSockAddrUn(socket_name,&server_addr);

    /* binds a name to the socket, this name is specified by the server_addr */
    if (bind(sockfd, (struct sockaddr *) &server_addr, addrlen) < 0) {
        perror("server: bind error");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) {

    int numthreads = atoi(argv[1]);

    /* Verifies given input */
    verifyInput(argc, argv);

    /* Init filesystem and locks */
    init_fs();

    /* Init server socket */
    initSocket(argv[2]);

    /* Creates array of thread id's */
    pthread_t* tid = (pthread_t*) malloc(sizeof(pthread_t) * numthreads);

    /* Creates pool of threads to execute the commands inside the buffer */
    threadCreate(tid,numthreads,applyCommands_aux);

    /* Joins threads */
    threadJoin(tid,numthreads);

    /* Release allocated memory and destroys locks */
    free(tid);
    destroy_fs();

    /* Closes and unlinks socket */
    close(sockfd);
    unlink(argv[1]);

    exit(EXIT_SUCCESS);
}
