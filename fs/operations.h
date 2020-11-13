#ifndef FS_H
#define FS_H
#include "state.h"

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType);
int delete(char *name);
int lookup(char *name);
int move(char* path, char* dest);
int countChar(char* path,char c);
int countiNodes(char* fullpath);
int lockPath(char* name, int* array, char arg);
void unlock(int* array, int counter);
void print_tecnicofs_tree(FILE *fp);

#endif /* FS_H */
