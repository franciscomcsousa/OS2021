#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

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

void processInput(FILE *fp){
    char line[MAX_INPUT_SIZE];

    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), fp)) {
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

void applyCommands(FILE *fp_output){
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
                        fprintf(fp_output,"Create file: %s\n", name);
                        create(name, T_FILE);
                        break;
                    case 'd':
                        fprintf(fp_output,"Create directory: %s\n", name);
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
                    fprintf(fp_output,"Search: %s found\n", name);
                else
                    fprintf(fp_output,"Search: %s not found\n", name);
                break;
            case 'd':
                fprintf(fp_output,"Delete: %s\n", name);
                delete(name);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

FILE* processFile(char* path,char* param){
    FILE *fp;

    fp = fopen(path,param);

    if (fp == NULL){                     
        printf("teste.txt: No such file or directory\n");   
        exit(EXIT_FAILURE);
    }

    return fp;
}

int main(int argc, char* argv[]) {

    FILE *fp_input, *fp_output;

    /* opens input and output file */
    fp_input = processFile(argv[1],"r");
    fp_output = processFile(argv[2],"w");

    /* init filesystem */
    init_fs();

    /* process input and print tree */
    processInput(fp_input);
    fclose(fp_input);
    applyCommands(fp_output);
    print_tecnicofs_tree(fp_output);
    fclose(fp_output);

    /* release allocated memory */
    destroy_fs();
    exit(EXIT_SUCCESS);
}
