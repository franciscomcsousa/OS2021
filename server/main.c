#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include "fs/operations.h"

#define MAX_INPUT_SIZE 100
#define BUFFER_SIZE 9
#define END "EOF"
#define SUCCESS 0
#define ENDOFFILE 1

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
 * Opens file.
 * @param argv: array from stdin given by user
 * @param c: file mode
*/
FILE* openFile(char* argv[],char c){
    FILE* fp;
    if (c == 'r'){
        fp = fopen(argv[1],"r");

        if (fp == NULL){                     
            fprintf(stderr,"Error: invalid input file\n");
            exit(EXIT_FAILURE);
        }
        return fp;
    }
    else if (c == 'w'){
        fp = fopen(argv[2],"w");

        if (fp == NULL){
            fprintf(stderr,"Error: invalid output file\n");
            exit(EXIT_FAILURE);
        }
        return fp;
    }
    else
        exit(EXIT_FAILURE);
}

/**
 * Verifies the validity of the arguments given as input.
 * @param argc: number of arguments given by user
 * @param argv: array from stdin given by user
*/
void verifyInput(int argc, char* argv[]){
    if (argc != 4){
        fprintf(stderr, "Error: invalid number of arguments\n");
        exit(EXIT_FAILURE);
    }
    /*argv[3] refers to the number of threads*/
    if (atoi(argv[3]) <= 0){
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
 * Registers Time.
 * @param mode: 's' to start timer, 'e' to end
*/
void Timer(char mode){

    if('s' == mode){
        if (gettimeofday(&t1,NULL) != 0){
            fprintf(stderr, "Error: system time\n");
            exit(EXIT_FAILURE);
        }
    }
    else if('e' == mode){
        if (gettimeofday(&t2,NULL) != 0){
            fprintf(stderr, "Error: system time\n");
            exit(EXIT_FAILURE);
        }
    }
    else 
        exit(EXIT_FAILURE);
}

/**
 * Calculates and prints execution time.
*/
void executionTime(){
    double time = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec)/1000000.0;
    printf("TecnicoFS completed in %.4f seconds.\n",time);
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

int main(int argc, char* argv[]) {

    FILE* fp_input;
    FILE* fp_output;
    int numthreads = atoi(argv[3]);

    /* Verifies given input */
    verifyInput(argc, argv);

    /* Opens input and output files */
    fp_input = openFile(argv,'r');
    fp_output = openFile(argv,'w');

    /* Init mutex and cond */
    init_locks_file();

    /* Init filesystem and locks */
    init_fs();

    /* Creates array of thread id's */
    pthread_t* tid = (pthread_t*) malloc(sizeof(pthread_t) * numthreads);

    /* Starts Timer */
    Timer('s');

    /* Creates pool of threads to execute the commands inside the buffer */
    threadCreate(tid,numthreads,applyCommands_aux);

    /* Reads input file to buffer */
    processInput(fp_input);

    /* Joins threads */
    threadJoin(tid,numthreads);

    /* Stops Timer */
    Timer('e');

    /* Prints time and tree */
    executionTime();
    print_tecnicofs_tree(fp_output);

    /* Close input and output files */
    fclose(fp_output);
    fclose(fp_input);

    /* Release allocated memory and destroys locks */
    free(tid);
    destroy_locks_file();
    destroy_fs();

    exit(EXIT_SUCCESS);
}
