#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include <sys/time.h>

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

#define SYNCSTRAT1 "mutex"
#define SYNCSTRAT2 "rwlock"
#define SYNCSTRAT3 "nosync"

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

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
    
    if (gettimeofday(&t1,NULL)){
        fprintf(stderr, "Error: system time\n");
        exit(EXIT_FAILURE);
    }

    while (numberCommands > 0){
        const char* command = removeCommand();
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
    if (gettimeofday(&t2,NULL)){
        fprintf(stderr, "Error: system time\n");
        exit(EXIT_FAILURE);
    }
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
    if (strcmp(argv[4], SYNCSTRAT1) && strcmp(argv[4], SYNCSTRAT2) && strcmp(argv[4], SYNCSTRAT3)){
        fprintf(stderr, "Error: invalid sync strategy\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) {

    /* verifies the validity of argc and argv */
    verifyInput(argc, argv);
    
    /* opens input and output file */
    processFiles(argv);

    /* init filesystem */
    init_fs();

    /* process input and print tree and execu */
    processInput();
    applyCommands();
    executionTime(t1,t2);
    print_tecnicofs_tree(fp_output);

    /*fechar input no fim do processInput() e fechar output no fim do print_tree()
      boa ideia ? */
    /* (temporario) tempo comeca a contar no inicio do applycommands e 
       para de contar no fim */

    /* release allocated memory */
    destroy_fs();
    exit(EXIT_SUCCESS);
}
