#include "operations.h"
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
	int locked_inodes[INODE_TABLE_SIZE];

	size = lockPath(name,locked_inodes,'w');

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
	int locked_inodes[INODE_TABLE_SIZE];
	pthread_rwlock_t rwl;

	size = lockPath(name,locked_inodes,'w');

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
	inode_get_lock(child_inumber,&rwl);

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
	
	/* unlocks deleted inode */
	if(pthread_rwlock_unlock(&rwl) != 0){
        fprintf(stderr, "Error: rwlock unlock error\n");
        exit(EXIT_FAILURE);
    }

	unlock(locked_inodes,size-1);  //size-1 because deleted inode was already unlocked
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
	int locked_inodes[INODE_TABLE_SIZE];

	size = lockPath(name,locked_inodes,'r');

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

int countChar(char* path,char c){
    int count = 0;

    for(int i=0; path[i]!='\0'; i++){
        if(path[i] == c)
            count++;
    }
    return count;
}

/**
 * Moves file/dir from path to destiny path.
 * @param path
 * @param dest
*/
int move(char* path, char* dest){

	int parent_inumber, child_inumber, parent_inumber_dest;

	char *parent_name, *child_name, *parent_name_dest, *child_name_dest;

	char name_copy[MAX_FILE_NAME];

	int size,size_dest;
	int locked_inodes[INODE_TABLE_SIZE], locked_inodes_dest[INODE_TABLE_SIZE];

	type ptype, ptype_dest;
	union Data pdata, pdata_dest;

    /* to prevent deadlocks, locks are made in alphabetical order*/
	int value = strcmp(path,dest);
	if(value > 0){
		size = lockPath(path,locked_inodes,'w');
		size_dest = lockPath(dest,locked_inodes_dest,'w');
	}
	else if (value < 0){
		size_dest = lockPath(dest,locked_inodes_dest,'w');
		size = lockPath(path,locked_inodes,'w');
	}
	else
		return SUCCESS;

	strcpy(name_copy, path);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name);
	if (parent_inumber == FAIL) {
		printf("failed to move %s, invalid parent dir %s\n",path, parent_name);
		unlock(locked_inodes,size);
		unlock(locked_inodes_dest,size_dest);
		return FAIL;
	}

	inode_get(parent_inumber, &ptype, &pdata);
	if(ptype != T_DIRECTORY) {
		printf("failed to move %s, parent %s is not a dir\n",path, parent_name);
		unlock(locked_inodes,size);
		unlock(locked_inodes_dest,size_dest);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);

	if (child_inumber == FAIL) {
		printf("failed to move %s, doesnt exists in dir %s\n",
		       child_name, parent_name);
		unlock(locked_inodes,size);
		unlock(locked_inodes_dest,size_dest);
		return FAIL;
	}

	strcpy(name_copy, dest);
	split_parent_child_from_path(name_copy, &parent_name_dest, &child_name_dest);

	parent_inumber_dest = lookup(parent_name_dest);
	if (parent_inumber_dest == FAIL) {
		printf("failed to move %s, invalid parent dir %s\n",dest, parent_name_dest);
		unlock(locked_inodes,size);
		unlock(locked_inodes_dest,size_dest);
		return FAIL;
	}

	inode_get(parent_inumber_dest, &ptype_dest, &pdata_dest);
	if(ptype_dest != T_DIRECTORY) {
		printf("failed to move %s, parent %s is not a dir\n",dest, parent_name_dest);
		unlock(locked_inodes,size);
		unlock(locked_inodes_dest,size_dest);
		return FAIL;
	}

	if (lookup_sub_node(child_name_dest, pdata_dest.dirEntries) != FAIL) {
		printf("failed to move %s, exists in dir %s\n",child_name_dest, parent_name_dest);
		unlock(locked_inodes,size);
		unlock(locked_inodes_dest,size_dest);
		return FAIL;
	}

	/* resets entry in path directory and adds entry in dest directory */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n",child_name, parent_name);
		unlock(locked_inodes,size);
		unlock(locked_inodes_dest,size_dest);
		return FAIL;
	}

	if (dir_add_entry(parent_inumber_dest, child_inumber, child_name_dest) == FAIL) {
		printf("could not add entry %s in dir %s\n",child_name_dest, parent_name_dest);
		unlock(locked_inodes,size);
		unlock(locked_inodes_dest,size_dest);
		return FAIL;
	}

	unlock(locked_inodes,size);
	unlock(locked_inodes_dest,size_dest);
	return SUCCESS;
}

/**
 * Calculates the number of inodes of a given path that will be locked.
 * @param fullpath
*/
int countiNodes(char* fullpath){
	int counter = 0;
	char fullpath_copy[MAX_FILE_NAME];
	char *saveptr;
	char delim[] = "/";

	strcpy(fullpath_copy,fullpath); 
	char* path = strtok_r(fullpath_copy,delim,&saveptr);

	while(path != NULL){
		counter++;
		path = strtok_r(NULL,delim,&saveptr);
	}
	return counter;
}

/**
	* For a given path we have to wrlock the last two elements and rdlock the rest.
	* To achieve that we calculate the amount of nodes (nNodes) that need to be locked,
	* from there we decrease the value of nNodes in each iteration of the while cycle knowing that
	* when this value reaches 2 we will start wrlock nodes instead of rdlock.
	* The exception to this rule is when we are calling the function Lookup which only needs rdlocks.
	* Ex: "c a d"      -> 'root' and 'a' will be wrlocked
	*     "c a/b d"    -> 'a' and 'b' will be wrlock and 'root' rdlock
	*     "c a/b/c d"  -> 'b' and 'c' will be rwlock and 'root' and 'a' rdlock
*/

/**
 * Registers and locks inode of a given path.
 * @param name is the path where file will be created/destroyed/found
 * @param array is the array of inode_number that will be locked
 * @param arg is the type of operation. If 'r' will rwlock all inode, if 'w' will wrlock directory 
 *                                      where changes will be made and the inode being created/destroyed.
*/
int lockPath(char* name, int* array, char arg){

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
	int nNodes = countiNodes(full_path);

	if (nNodes == 1 && arg != 'r'){                       
		inode_get_lock(current_inumber, &current_lock);
		if(pthread_rwlock_wrlock(&current_lock) != 0){
			fprintf(stderr, "Error: wrlock lock error\n");
			exit(EXIT_FAILURE);
		}
	}
	else{                 
		inode_get_lock(current_inumber, &current_lock);
		if(pthread_rwlock_rdlock(&current_lock) != 0){
			fprintf(stderr, "Error: rdlock lock error\n");
			exit(EXIT_FAILURE);
		}
	}   
	
	inode_get(current_inumber, &nType, &data);
    char* path = strtok_r(full_path, delim,&saveptr);     

	/** 
	 * Process the rest of path reducing in each iteration the value of nNodes. This way we will know 
	 * when we have reached an inode_number that needs to be wrlock instead of rdlock.
	*/
    while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {  

		if(nNodes >= 3 || arg == 'r'){          
			inode_get_lock(current_inumber, &current_lock);
			if(pthread_rwlock_rdlock(&current_lock) != 0){
				fprintf(stderr, "Error: rdlock lock error\n");
				exit(EXIT_FAILURE);
			}
		}
		else{                                    
			inode_get_lock(current_inumber, &current_lock);
			if(pthread_rwlock_wrlock(&current_lock) != 0){
				fprintf(stderr, "Error: wrlock lock error\n");
				exit(EXIT_FAILURE);
			}
		}

		array[counter++] = current_inumber;

        inode_get(current_inumber, &nType, &data);
        path = strtok_r(NULL, delim,&saveptr);

		nNodes--;
    }
	return counter; 
}

/**
 * Unlocks inodes inside given array.
 * @param array: array of inode_numbers to unlock
 * @param counter: number of inode_numbers in array
*/
void unlock(int* array, int counter){

    pthread_rwlock_t current_lock;
	for(counter--;counter>=0;counter--){
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
}
