/*
 * hello_libpmemobj.c -- an example for libpmemobj library
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "libpmemobj.h"

// Name of our layout in the pool
#define LAYOUT "hello_layout"

// Maximum length of our buffer
#define MAX_BUF_LEN 64



struct datastruct {
	char buf[MAX_BUF_LEN * 3];
};
// Root structure
struct my_root {
	PMEMoid oid;
};

POBJ_LAYOUT_BEGIN(string_store);
POBJ_LAYOUT_ROOT(string_store, struct my_root);
POBJ_LAYOUT_TOID(string_store, struct datastruct);
POBJ_LAYOUT_END(string_store);

/****************************
 * This function writes the "Hello..." string to persistent-memory.
 *****************************/
void write_hello_string (char *buf, char *path, char *set_chars)
{
	PMEMobjpool *pop;
	
	TOID(struct my_root) root;
	// Create the pmemobj pool or open it if it already exists
	if(access(path, F_OK) != 0){
		pop = pmemobj_create(path, LAYOUT, PMEMOBJ_MIN_POOL, 0666);
		if (pop == NULL) 
		{
			perror(path);
			exit(1);
		}
		root = POBJ_ROOT(pop, struct my_root);
		TOID(struct datastruct) data;
		POBJ_ALLOC(pop, &data, struct datastruct, sizeof(struct datastruct), NULL, NULL);
		pmemobj_memset_persist(pop, D_RW(data)->buf, 'a', MAX_BUF_LEN);
		pmemobj_memset_persist(pop, D_RW(data)->buf + 64, 'b', MAX_BUF_LEN);
		pmemobj_memset_persist(pop, D_RW(data)->buf + 128, 'c', MAX_BUF_LEN);

		TX_BEGIN(pop){
			//TX_ADD(data);
			memset(D_RW(data)->buf, 'd', 64);
		    memset(D_RW(data)->buf + 64, 'e', 64);
		    memset(D_RW(data)->buf + 128, 'f', 64);
		}TX_END

		PMEMWRAP_FORCE_ABORT();
	}
	
	else{
		pop = pmemobj_open(path, LAYOUT);
		if (pop == NULL) 
		{
			perror(path);
			exit(1);
		}
		root = POBJ_ROOT(pop, struct my_root);
		TOID(struct datastruct) data;
		TOID_ASSIGN(data, D_RW(root)->oid);

		printf("1st 64B %p :%.64s\n", D_RO(data)->buf, (char *)D_RO(data)->buf);
		printf("2nd 64B %p :%.64s\n", D_RO(data)->buf + 64, (char *)(D_RO(data)->buf + 64));
		printf("3rd 64B %p :%.64s\n", D_RO(data)->buf + 128, (char *)(D_RO(data)->buf + 128));
	}


	// Close PMEM object pool
	pmemobj_close(pop);	
		
	return;
}

/****************************
 * This function reads the "Hello..." string from persistent-memory.
 *****************************/
// void read_hello_string(char *path)
// {
// 	PMEMobjpool *pop;
	
// 	//Attempt open instead
// 	pop = pmemobj_open(path, LAYOUT);
	
// 	// Check if open failed
// 	if (pop == NULL) {
// 		perror(path);
// 		exit(1);
// 	} 
	
// 	// Get the PMEMObj root
// 	PMEMoid root = pmemobj_root(pop, sizeof (struct my_root) * 3);
	
// 	// Pointer for structure at the root
// 	struct my_root *rootp = pmemobj_direct(root);
	
// 	// Read the string from persistent memory and write to the console
// 	printf("1st 64B :%.64s\n", (char *)&rootp[0]);
// 	printf("2nd 64B :%.64s\n", (char *)&rootp[1]);
// 	printf("3rd 64B :%.64s\n", (char *)&rootp[2]);
	
// 	// Close PMEM object pool
// 	pmemobj_close(pop);

// 	return;
// }

/****************************
 * This main function gather from the command line and call the appropriate
 * function to perform read and write persistently to memory.
 *****************************/
int main(int argc, char *argv[])
{
	char *path = argv[2];
	
	// Create the string to save to persistent memory
	char buf[MAX_BUF_LEN] = "Hello Persistent Memory!!!";
	
	if (strcmp (argv[1], "-w") == 0) {

		write_hello_string(buf, path, argv[3]);
		
	} else if (strcmp (argv[1], "-r") == 0) {

		//read_hello_string(path);
	} else { 
		fprintf(stderr, "Usage: %s <-w/-r> <filename>\n", argv[0]);
		exit(1);
	}

}