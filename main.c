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

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

FILE *fp_input, *fp_output;
extern pthread_mutex_t mutex;

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {
    commandLock();
    if(numberCommands > 0){
        numberCommands--;
        char* output = inputCommands[headQueue++];
        commandUnlock();
        return output;  
    }
    commandUnlock();
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
    while (1){
        const char* command = removeCommand();
        if (command == NULL){
            break;
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

/**
 * Opens input and output file.
 * @param argv[]: array from stdin given by user
*/
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

int main(int argc, char* argv[]) {

    /* verifies given input */
    verifyInput(argc, argv);

    /* open given files */
    processFiles(argv);

    /* inicializes locks for sync */
    initLock();

    /* init filesystem */
    init_fs();

    /* process input */
    processInput();

    /* creates pool of threads */
    threadCreate(atoi(argv[3]), applyCommands_aux);

    /* prints tree */
    print_tecnicofs_tree(fp_output);

    /* release allocated memory */
    destroy_fs();

    /* destroy locks */
    destroyLock();

    exit(EXIT_SUCCESS);
}
