#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include <sys/time.h>
#include <pthread.h>

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

#define SYNCSTRAT1 "mutex"
#define SYNCSTRAT2 "rwlock"
#define SYNCSTRAT3 "nosync"

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

pthread_mutex_t mutex;
pthread_rwlock_t rwl;

FILE *fp_input, *fp_output;
struct timeval t1,t2;

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void processInput(){
    char line[MAX_INPUT_SIZE];

    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), fp_input)) {
        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
    }
    fclose(fp_input);
}

void applyCommands(){

    while (numberCommands > 0){
        pthread_mutex_lock(&mutex); 
        const char* command = removeCommand();
        pthread_mutex_unlock(&mutex); 
        if (command == NULL){
            continue;
        }

        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
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
                searchResult = lookup(name);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;
            case 'd':
                printf("Delete: %s\n", name);
                delete(name);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void *applyCommands_aux(){
    applyCommands();
    return NULL;
}

void processFiles(char* argv[]){

    fp_input = fopen(argv[1],"r");

    if (fp_input == NULL){                     
        fprintf(stderr,"Error: invalid input file\n");
        exit(EXIT_FAILURE);
    }

    fp_output = fopen(argv[2],"w");

    if (fp_output == NULL){
        fprintf(stderr,"Error: invalid output file\n");
        exit(EXIT_FAILURE);
    }
}

void executionTime(struct timeval t1,struct timeval t2){

    double time = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec)/1000000.0;
    printf("TecnicoFS completed in %.4f seconds.\n",time);
}

void verifyInput(int argc, char* argv[]){
    if (argc != 5){
        fprintf(stderr, "Error: invalid number of arguments\n");
        exit(EXIT_FAILURE);
    }
    /*argv[3] refers to the number of threads*/
    if (atoi(argv[3]) <= 0){
        fprintf(stderr, "Error: invalid number of threads\n");
        exit(EXIT_FAILURE);
    }
    /*argv[4] refers to the sync strategy*/
    if (strcmp(argv[4], "mutex") && strcmp(argv[4], "rwlock") && strcmp(argv[4], "nosync")){
        fprintf(stderr, "Error: invalid sync strategy\n");
        exit(EXIT_FAILURE);
    }

    if (!strcmp(argv[4],"nosync") && atoi(argv[3]) != 1){
        fprintf(stderr, "Error: invalid number of threads for nosync\n");
        exit(EXIT_FAILURE);
    }
}

void threadCreate(int numthreads){

    pthread_t tid[6];

    for(int i = 0; i < numberThreads; i++){
        if(pthread_create(&tid[i], NULL, applyCommands_aux, NULL) != 0){
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < numberThreads; i++){
        pthread_join(tid[i], NULL);
    }
}

int main(int argc, char* argv[]) {

    pthread_mutex_init(&mutex, NULL);
    pthread_rwlock_init(&rwl, NULL);

    /* verifies the validity of argc and argv */
    verifyInput(argc, argv);
    
    /* opens input and output file */
    processFiles(argv);

    /* init filesystem */
    init_fs();

    /* process input */
    processInput();

    /* starts timer */
    if (gettimeofday(&t1,NULL)){
        fprintf(stderr, "Error: system time\n");
        exit(EXIT_FAILURE);
    }

    /* creates pool of threads */
    threadCreate(atoi(argv[3]));

    /* ends timer */
    if (gettimeofday(&t2,NULL)){
        fprintf(stderr, "Error: system time\n");
        exit(EXIT_FAILURE);
    }

    executionTime(t1,t2);
    print_tecnicofs_tree(fp_output);

    /* release allocated memory */
    destroy_fs();
    exit(EXIT_SUCCESS);
}
