#include "operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

/**
 * Given a path, fills pointers with strings for the parent path and child file name.
 * @param path: the path to split. ATENTION: the function may alter this parameter
 * @param parent: reference to a char*, to store parent path
 * @param child: reference to a char*, to store child file name
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

/**
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

/**
 * Destroy tecnicofs and inode table.
*/
void destroy_fs() {
	inode_table_destroy();
}

/**
 * Checks if content of directory is not empty.
 * @param entries: entries of directory
 * @return SUCCESS or FAIL
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

/**
 * Looks for node in directory entry from name.
 * @param name: path of node
 * @param entries: entries of directory
 * @return inumber or FAIL
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

/**
 * Creates a new node given a path.
 * @param name: path of node
 * @param nodeType: type of node
 * @return SUCESS or FAIL
*/
int create(char *name, type nodeType){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType;
	union Data pdata;

	int size;
	int locked_inodes[INODE_TABLE_SIZE];

	size = lockPath(name,locked_inodes,"w");

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name,'l');

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

/**
 * Deletes a node given a path.
 * @param name: path of node
 * @return SUCCESS OR FAIL
*/
int delete(char *name){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	int size;
	int locked_inodes[INODE_TABLE_SIZE];
	pthread_rwlock_t* lock;

	size = lockPath(name,locked_inodes,"w");

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name,'l');

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

	if((lock = getlock(child_inumber)) == NULL){
		return FAIL;
	}

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
	if(pthread_rwlock_unlock(lock) != 0){
        fprintf(stderr, "Error: rwlock unlock error\n");
        exit(EXIT_FAILURE);
    }

	unlock(locked_inodes,size-1);  //size-1 because deleted inode was already unlocked
	return SUCCESS;
}

/**
 * Lookup for a given path.
 * @param name: path of node
 * @param flag: -l or -u 
 * @return inumber or FAIL
*/
int lookup(char *name, char flag) {
	
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";
	char *saveptr;

	int size;
	int locked_inodes[INODE_TABLE_SIZE];


	/* if path is unlocked */
	if(flag == 'u') 
		size = lockPath(name,locked_inodes,"r");

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

	if(flag == 'u')
		unlock(locked_inodes,size);

	return current_inumber;
}

/**
 * Verifies loops in move command.
 * If the destiny path is a subdirectory of the path, it causes a loop.
 * @param path: path of node
 * @param dest: destiny path
 * @return SUCCESS or FAIL
*/
int verifyLoop(char* path, char* dest){

	char copy[MAX_FILE_NAME], copy_dest[MAX_FILE_NAME];
	char delim[] = "/";

	char *saveptr;
	char *saveptr_dest;

	strcpy(copy, path);
	strcpy(copy_dest, dest);

	char *path_tok = strtok_r(copy, delim,&saveptr);
	char *dest_tok = strtok_r(copy_dest, delim,&saveptr_dest);

	while (path_tok != NULL && dest_tok != NULL) {
		if(strcmp(path_tok,dest_tok) == 0){
			path_tok = strtok_r(NULL, delim,&saveptr);
			dest_tok = strtok_r(NULL, delim,&saveptr_dest);
		}
		else
			return SUCCESS;
	}

	/* path is inside dest */
	if(path_tok == NULL)
		return FAIL;

	return SUCCESS;
}

/**
 * Moves file/dir from one path to another.
 * @param path: path of node
 * @param dest: destiny path
 * @return SUCCESS or FAIL
*/
int move(char* path, char* dest){

	int parent_inumber, child_inumber, parent_inumber_dest;
	char *parent_name, *child_name, *parent_name_dest, *child_name_dest;

	char name_copy[MAX_FILE_NAME];

	int size, size_dest, value;
	int locked_inodes[INODE_TABLE_SIZE], locked_inodes_dest[INODE_TABLE_SIZE];

	type ptype, ptype_dest;
	union Data pdata, pdata_dest;

	if(verifyLoop(path,dest) == FAIL){
		printf("failed to move, cannot move %s to a subdirectory of itself, %s\n", path, dest);
		return FAIL;
	}

    /* to prevent deadlocks, locks are made according to the size of path, if the same, alphabetically */
	value = strcmp(path,dest);
	size = strlen(path);
	size_dest = strlen(dest);

	if(size > size_dest){
		size_dest = lockPath(dest,locked_inodes_dest,"w");
		size = lockPath(path,locked_inodes,"m");
	}
	else if(size < size_dest){
		size = lockPath(path,locked_inodes,"w");
		size_dest = lockPath(dest,locked_inodes_dest,"m");
	}
	else{
		if(value > 0){
			size = lockPath(path,locked_inodes,"w");
			size_dest = lockPath(dest,locked_inodes_dest,"m");
		}
		else if (value < 0){
			size_dest = lockPath(dest,locked_inodes_dest,"w");
			size = lockPath(path,locked_inodes,"m");
		}
		else
			return SUCCESS;
	}

	strcpy(name_copy, path);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name,'l');

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

	parent_inumber_dest = lookup(parent_name_dest,'l');

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

	if(child_inumber == parent_inumber_dest){
		printf("failed to move %s, to a subdirectory of itself %s",child_name,parent_name_dest);
		return FAIL;
	}

	/* resets entry in path directory */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n",child_name, parent_name);
		unlock(locked_inodes,size);
		unlock(locked_inodes_dest,size_dest);
		return FAIL;
	}

	/* the new entry has the same inumber but a different name */
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
 * Calculates the number of inodes in a given path.
 * @param fullpath: path to node
 * @return number of nodes in path
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
 * The lockPath function is used to lock all of the nodes in a path.
 * If it's called within create 
 * 	Ex: c /a/b
 * 	it will rdlock 'root' and wrlock 'a'
 * If it's called within delete
 * 	Ex: d /a/b
 * 	it will rdlock 'root' and wrlock 'a' and 'b'
 * If it's called within lookup
 * 	Ex: l /a/b
 * 	it will rdlock every node
 * If it's called within move
 * 	Ex: m /a/b/c /a/z
 * 	it will FIRST rdlock 'root' and wrlock 'a' THEN rwlock 'b' anc 'c'
 * 
 * This works by calculating the number of nodes (nNodes) in a given path.
 * Its start from the root decreasing the value of nNodes for each node locked.
 * 
 * We know that when nNodes == 1 --> root will be rwlock
 *                   nNodes > 1  --> root will be rdlock
 *              when locking a node if nNodes > 2 --> node will be rdlock
 *                                     nNodes < 2 --> node will be wrlock
 * 
 * If the lockPath is called from the function move with "mr" or "mw" flag it will do 
 * trylock instead of lock to prevent deadlocks, if trylock is successful it will 
 * store the locked node otherwise we know that the other path already locked it and we 
 * dont need to store it.
*/

/**
 * Registers and locks node of a given path.
 * @param name is the path where file will be created/destroyed/found
 * @param array is the array of inode_number that will be locked
 * @param arg is the type of operation. 
 * "r" -> will rdlock all nodes
 * "w" -> will wrlock modifiable nodes and rdlock the rest
 * "mw" -> will trywrlock modifiable nodes and tryrdlock the rest
 * @return number of nodes locked
*/
int lockPath(char* name, int* array, char* arg){

    char full_path[MAX_FILE_NAME];
    char delim[] = "/";
	char *saveptr;

    int counter = 0;
    int current_inumber = FS_ROOT;

    type nType;
    union Data data;

	strcpy(full_path, name);
	int nNodes = countiNodes(full_path);

	if(nNodes > 1 || !strcmp(arg,"r")){ 
		if(!strcmp(arg,"m")){
			if(inode_lock(current_inumber,"mr") == 0) //only stores if successfull
				array[counter++] = current_inumber;
		}
		else{            
			inode_lock(current_inumber,"r");
			array[counter++] = current_inumber;  
		}
	}     
	else if (nNodes == 1){    
		if(!strcmp(arg,"m")){
			if(inode_lock(current_inumber,"mw") == 0)
				array[counter++] = current_inumber;
		}
		else{
			inode_lock(current_inumber,"w");
			array[counter++] = current_inumber;
		}
	}

	inode_get(current_inumber, &nType, &data);
    char* path = strtok_r(full_path, delim,&saveptr);     

	/** 
	 * Process the rest of path reducing in each iteration the value of nNodes. This way we will
	 * know when we have reached a node that needs to be wrlock instead of rdlock.
	*/
    while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {

		if(nNodes > 2 || !strcmp(arg,"r")){   
			if(!strcmp(arg,"m")){
				if(inode_lock(current_inumber,"mr") == 0)
					array[counter++] = current_inumber;
			}    
			else{   
				inode_lock(current_inumber,"r");
				array[counter++] = current_inumber;
			}
		}
		else{    
			if(!strcmp(arg,"m")){
				if(inode_lock(current_inumber,"mw") == 0)
					array[counter++] = current_inumber;
			}
			else{                                
				inode_lock(current_inumber, "w");
				array[counter++] = current_inumber;
			}
		}

        inode_get(current_inumber, &nType, &data);
        path = strtok_r(NULL, delim,&saveptr);

		nNodes--;
    }
	return counter; 
}

/**
 * Unlocks nodes.
 * @param array: array of inode_numbers to unlock
 * @param counter: number of node in array
*/
void unlock(int* array, int counter){
	for(counter--;counter>=0;counter--){
		inode_unlock(array[counter]);
	}
}

/**
 * Prints tecnicofs tree.
 * @param fp: pointer to file
*/
void print_tecnicofs_tree(FILE *fp){
	inode_print_tree(fp, FS_ROOT, "");
}
