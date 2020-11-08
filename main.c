#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include "fs/operations.h"
#include "fs/sync.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100
#define BUFFER_SIZE 10

pthread_mutex_t mutexfile;
pthread_cond_t canInsert, canRemove;

char buffer[BUFFER_SIZE][MAX_INPUT_SIZE];
int counter = 0;        //quantos comandos estao no buffer
int insertPointer = 0; //onde colocar o comando lido
int removePointer = 0;    //qual o proximo comando a ser executado
int eof = 0;            //indica se ja chegamos ao fim do ficheiro "0"->ainda n "1"->ja

int insertCommand(char* data){

    pthread_mutex_lock(&mutexfile);

    while (counter == BUFFER_SIZE) 
        pthread_cond_wait(&canInsert,&mutexfile); //if full waits for a command to be removed

    strcpy(buffer[insertPointer],data);
    insertPointer++;
    if (insertPointer == BUFFER_SIZE) 
        insertPointer = 0;
    counter++;

    pthread_cond_signal(&canRemove);
    pthread_mutex_unlock(&mutexfile);

    return 1;
}

void removeCommand(char* array){

    pthread_mutex_lock(&mutexfile);
    char* data;

    while(counter == 0) 
        pthread_cond_wait(&canRemove,&mutexfile); //if empty waits for command to be inserted
    
    strcpy(array,buffer[removePointer]);
    removePointer++;
    if (removePointer == BUFFER_SIZE) 
        removePointer = 0;
    counter--;

    pthread_cond_signal(&canInsert);
    pthread_mutex_unlock(&mutexfile);
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
}

void applyCommands(){

   char input[MAX_INPUT_SIZE];

    while (1){
        removeCommand(input);
        if (strcmp(input,NULL) == 0){
            break;
        }

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

void *applyCommands_aux(){
    applyCommands();
    return NULL;
}

/**
 * Opens input and output file.
 * @param argv[]: array from stdin given by user
*/
void openFiles(char* argv[], FILE* fp_input, FILE* fp_output){

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

/**
 * Verifies the validity of the arguments given as input.
 * @param argc: number of arguments given by user
 * @param argv[]: array from stdin given by user
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

void init_locks_file(){

    pthread_mutex_init(&mutexfile,NULL);
    pthread_cond_init(&canInsert,NULL);
    pthread_cond_init(&canRemove,NULL);  //é preciso passar algo ou basta NULL?

}

void startTimer(struct timeval t1){
    if (gettimeofday(&t1,NULL) != 0){
        fprintf(stderr, "Error: system time\n");
        exit(EXIT_FAILURE);
    }
}

void endTimer(struct timeval t2){
    if (gettimeofday(&t2,NULL) != 0){
        fprintf(stderr, "Error: system time\n");
        exit(EXIT_FAILURE);
    }
}

void executionTime(struct timeval t1,struct timeval t2){
    double time = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec)/1000000.0;
    printf("TecnicoFS completed in %.4f seconds.\n",time);
}

void threadCreate(pthread_t* tid, int numthreads, void* function){
    for(int i = 0; i < numthreads; i++){
        if(pthread_create(&tid[i], NULL, function, NULL) != 0){
            fprintf(stderr, "Error: creating threads\n");
            exit(EXIT_FAILURE);
        }
    }
}

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
    struct timeval t1,t2;
    int numthreads = atoi(argv[3]);

    /* verifies given input */
    verifyInput(argc, argv);

    /* open given files */
    openFiles(argv,fp_input,fp_output);

    /* init mutex and cond */
    init_locks_file();

    /* init filesystem and locks */
    init_fs();

    pthread_t* tid = (pthread_t*) malloc(sizeof(pthread_t) * numthreads);

    startTimer(t1);
    
    threadCreate(tid,numthreads,applyCommands_aux);
    processInput(fp_input);
    threadJoin(tid,numthreads);
    free(tid);

    endTimer(t2);
    executionTime(t1,t2);

    /* prints tree */
    print_tecnicofs_tree(fp_output);
    fclose(fp_output);
    fclose(fp_input);

    /* release allocated memory and destroys locks*/
    destroy_fs();

    exit(EXIT_SUCCESS);
}
