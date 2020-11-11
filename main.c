#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include "fs/operations.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100
#define BUFFER_SIZE 10
#define END "EOF"

pthread_mutex_t mutexfile;
pthread_cond_t canInsert, canRemove;
struct timeval t1,t2;

char buffer[BUFFER_SIZE][MAX_INPUT_SIZE];
int counter = 0;            //number of commands inside buffer
int insertPointer = 0;      //pointer to next free location to insert command
int removePointer = 0;      //pointer to next command to be read

int insertCommand(char* data){

    pthread_mutex_lock(&mutexfile);

    printf("(In)entrou\n");
    while (counter == BUFFER_SIZE) 
        pthread_cond_wait(&canInsert,&mutexfile); //if full waits for a command to be removed

    printf("(In)esperou\n");

    strcpy(buffer[insertPointer],data);
    insertPointer++;
    if (insertPointer == BUFFER_SIZE) 
        insertPointer = 0;
    counter++;

    pthread_cond_signal(&canRemove);
    pthread_mutex_unlock(&mutexfile);

    return 1;
}

int removeCommand(char* array){

    pthread_mutex_lock(&mutexfile);

    printf("(O)entrou\n");
    while(counter == 0)
        pthread_cond_wait(&canRemove,&mutexfile); //if empty waits for command to be inserted
    printf("(O)esperou\n");

    strcpy(array,buffer[removePointer]);

    if(strcmp(array,END) == 0){               //if finds command END, ends thread
        pthread_cond_signal(&canRemove);     //doesnt remove command END
        pthread_mutex_unlock(&mutexfile);   //so that other threads can end too.
        printf("(O)terminei\n");
        return -1;
    }
    printf("vou executar:%s\n",array);


    removePointer++;
    if (removePointer == BUFFER_SIZE) 
        removePointer = 0;
    counter--;

    printf("(O)acabou\n");
    pthread_cond_signal(&canInsert);
    pthread_mutex_unlock(&mutexfile);
    printf("(O)acabou\n");
    return 1;
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
    insertCommand(END);   //After reading everyting creates a command END to signal the end of the file
                         //Allows threads to end so that the program can continue running
}

void applyCommands(){

   char input[MAX_INPUT_SIZE];

    while (1){
        
        if(removeCommand(input) == -1)
           break;

        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(input, "%c %s %c", &token, name, &type);
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

/**
 * Function called during thread create.
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
 * Init mutex and cond used to process file.
*/
void init_locks_file(){
    pthread_mutex_init(&mutexfile,NULL);
    pthread_cond_init(&canInsert,NULL);
    pthread_cond_init(&canRemove,NULL);  //é preciso passar algo ou basta NULL?
}

/**
 * Destroys mutex and cond used to process file.
*/
void destroy_locks_file(){
    pthread_mutex_destroy(&mutexfile);
    pthread_cond_destroy(&canInsert);
    pthread_cond_destroy(&canRemove);
}
/**
 * Tracks time.
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
 * Calculates and prints timer.
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

    /* Release allocated memory and destroys locks*/
    free(tid);
    destroy_locks_file();
    destroy_fs();

    exit(EXIT_SUCCESS);
}
