#include "operations.h"
#include "sync.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */
void split_parent_child_from_path(char * path, char ** parent, char ** child) {

	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);

	// deal with trailing slash ( a/x vs a/x/ )
	if (path[len-1] == '/') {
		path[len-1] = '\0';
	}

	for (int i=0; i < len; ++i) {
		if (path[i] == '/' && path[i+1] != '\0') {
			last_slash_location = i;
			n_slashes++;
		}
	}

	if (n_slashes == 0) { // root directory
		*parent = "";
		*child = path;
		return;
	}

	path[last_slash_location] = '\0';
	*parent = path;
	*child = path + last_slash_location + 1;

}


/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs() {
	inode_table_init();
	
	/* create root inode */
	int root = inode_create(T_DIRECTORY);
	
	if (root != FS_ROOT) {
		printf("failed to create node for tecnicofs root\n");
		exit(EXIT_FAILURE);
	}
}


/*
 * Destroy tecnicofs and inode table.
 */
void destroy_fs() {
	inode_table_destroy();
}


/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */

int is_dir_empty(DirEntry *dirEntries) {
	if (dirEntries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (dirEntries[i].inumber != FREE_INODE) {
			return FAIL;
		}
	}
	return SUCCESS;
}


/*
 * Looks for node in directory entry from name.
 * Input:
 *  - name: path of node
 *  - entries: entries of directory
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */
int lookup_sub_node(char *name, DirEntry *entries) {
	if (entries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].inumber != FREE_INODE && strcmp(entries[i].name, name) == 0) {
            return entries[i].inumber;
        }
    }
	return FAIL;
}


/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType;
	union Data pdata;

	int size;
	char path[MAX_FILE_NAME];
	int locked_inodes[INODE_TABLE_SIZE];

	strcpy(path, name);
	size = lockup(path,locked_inodes,'w');


	strcpy(name_copy, name);

	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name);

	if (parent_inumber == FAIL) {
		printf("failed to create %s, invalid parent dir %s\n",
		        name, parent_name);
		unlock(locked_inodes,size);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to create %s, parent %s is not a dir\n",
		        name, parent_name);
		unlock(locked_inodes,size);
		return FAIL;
	}

	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
		       child_name, parent_name);
		unlock(locked_inodes,size);
		return FAIL;
	}

	/* create node and add entry to folder that contains new node */
	child_inumber = inode_create(nodeType);
	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n",
		        child_name, parent_name);
		unlock(locked_inodes,size);
		return FAIL;
	}

	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       child_name, parent_name);
		unlock(locked_inodes,size);
		return FAIL;
	}

	unlock(locked_inodes,size);
	return SUCCESS;
}


/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int delete(char *name){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	int size;
	char path[MAX_FILE_NAME];
	int locked_inodes[INODE_TABLE_SIZE];

	strcpy(path, name);
	size = lockup(path,locked_inodes,'w');

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name);

	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n",
		        child_name, parent_name);
		unlock(locked_inodes,size);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n",
		        child_name, parent_name);
		unlock(locked_inodes,size);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);

	if (child_inumber == FAIL) {
		printf("could not delete %s, does not exist in dir %s\n",
		       name, parent_name);
		unlock(locked_inodes,size);
		return FAIL;
	}

	inode_get(child_inumber, &cType, &cdata);

	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n",
		       name);
		unlock(locked_inodes,size);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n",
		       child_name, parent_name);
		unlock(locked_inodes,size);
		return FAIL;
	}

	if (inode_delete(child_inumber) == FAIL) {
		printf("could not delete inode number %d from dir %s\n",
		       child_inumber, parent_name);
		unlock(locked_inodes,size);
		return FAIL;
	}

	unlock(locked_inodes,size);
	return SUCCESS;
}


/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name) {
	
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";
	char *saveptr;

	int size;
	char original_path[MAX_FILE_NAME];  /*the name 'path' is already declared */
	int locked_inodes[INODE_TABLE_SIZE];

	strcpy(original_path, name);
	size = lockup(original_path,locked_inodes,'r');

	strcpy(full_path, name);

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;

	/* get root inode data */
	inode_get(current_inumber, &nType, &data);

	char *path = strtok_r(full_path, delim,&saveptr);

	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		inode_get(current_inumber, &nType, &data);
		path = strtok_r(NULL, delim, &saveptr);
	}

	unlock(locked_inodes,size);
	return current_inumber;
}

/*     c a/b/c/e  f */
/*     c a/b/c/d  f */
/*     c a/b/c/   d */
/*     c a/b/     d */
/*     c a/b        */
/*     c a          */

/**
 * Count number of char c in string path.
*/
int countChar(char* path,char c){
	int count = 0;

	for(int i=0; path[i]!='\0'; i++){
		if(path[i] == c)
			count++;
	}
	return count;
}

/**
 * Locks inodes involved in the operation.
 * @param name is the path where file will be created/destroyed/found
 * @param array is the array of inode_number that will be locked
 * @param arg is the type of operation. If 'r' will rwlock all inode, if 'w' will wrlock directory 
 *                                      where changes will be made and the inode being created/destroyed.
*/
int lockup(char* name, int* array, char arg){

    char full_path[MAX_FILE_NAME];
    char delim[] = "/";
	char *saveptr;

    int counter = 0;

    int current_inumber = FS_ROOT;

    array[counter++] = current_inumber;

    type nType;
    union Data data;
    pthread_rwlock_t current_lock;

	strcpy(full_path, name);
	int nslash = countChar(full_path,'/'); 

	if(arg == 'r' || nslash > 0){                 /*If 'r' all files are rdlock. If number of slashs > 0 no changes will be made inside root directory*/
		inode_get_lock(current_inumber, &current_lock);
		if(pthread_rwlock_rdlock(&current_lock) != 0){
			fprintf(stderr, "Error: rdlock lock error\n");
			exit(EXIT_FAILURE);
		}

		printf("ROOT NOT LOCKED\n"); //debug
	}
	else if (nslash == 0){                         /*If number of slash == 0 means changes will be made inside root directory*/       
		inode_get_lock(current_inumber, &current_lock);
		if(pthread_rwlock_wrlock(&current_lock) != 0){
			fprintf(stderr, "Error: wrlock lock error\n");
			exit(EXIT_FAILURE);
		}

		printf("ROOT LOCKED\n"); //debug
	}
	
	inode_get(current_inumber, &nType, &data);
    char* path = strtok_r(full_path, delim,&saveptr);     

    while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {  

		if(nslash >= 2 || arg == 'r'){          
			inode_get_lock(current_inumber, &current_lock);
			if(pthread_rwlock_rdlock(&current_lock) != 0){
				fprintf(stderr, "Error: rdlock lock error\n");
				exit(EXIT_FAILURE);
			}
			printf("READ LOCKED %s\n",path); //debug

		}
		else{                                     /* wr_locks diretory where file/directory will be created*/
			inode_get_lock(current_inumber, &current_lock);
			if(pthread_rwlock_wrlock(&current_lock) != 0){
				fprintf(stderr, "Error: wrlock lock error\n");
				exit(EXIT_FAILURE);
			}
			printf("WRITE LOCKED %s\n",path); //debug
		}

		array[counter++] = current_inumber;

        inode_get(current_inumber, &nType, &data);
        path = strtok_r(NULL, delim,&saveptr);

		nslash--;
    }
	return counter;
}

void unlock(int* array, int counter){

    pthread_rwlock_t current_lock;

	printf("Unlocking..\n");
	counter--;
	for(;counter>=0;counter--){
		inode_get_lock(array[counter],&current_lock);
		if(pthread_rwlock_unlock(&current_lock) != 0){
            fprintf(stderr, "Error: rwlock unlock error\n");
            exit(EXIT_FAILURE);
        }
	}
}

/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
void print_tecnicofs_tree(FILE *fp){
	inode_print_tree(fp, FS_ROOT, "");
	fclose(fp);
}
