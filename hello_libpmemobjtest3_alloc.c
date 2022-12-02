/*
 * hello_libpmemobj.c -- an example for libpmemobj library
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "libpmemobj.h"

// Name of our layout in the pool
#define LAYOUT "hello_layout"

// Maximum length of our buffer
#define MAX_BUF_LEN 48



struct substruct {
	char buf[MAX_BUF_LEN];
	size_t a;
	size_t b;
};
// Root structure
struct my_root {
	struct substruct data[3];
};

POBJ_LAYOUT_BEGIN(string_store);
POBJ_LAYOUT_ROOT(string_store, struct my_root);
POBJ_LAYOUT_TOID(string_store, struct substruct);
POBJ_LAYOUT_END(string_store);

/****************************
 * This function writes the "Hello..." string to persistent-memory.
 *****************************/
void write_hello_string (char *buf, char *path, char *set_chars)
{
	PMEMobjpool *pop;
	
	// Create the pmemobj pool or open it if it already exists
	pop = pmemobj_create(path, LAYOUT, PMEMOBJ_MIN_POOL, 0666);

	// Check if create failed		
	if (pop == NULL) 
	{
		pop = pmemobj_open(path, LAYOUT);
		if (pop == NULL) 
		{
			perror(path);
			exit(1);
		}
	}
					
	// Get the PMEMObj root
	TOID(struct my_root) root = POBJ_ROOT(pop, struct my_root);

	// Pointer for structure at the root

	// Write the string to persistent memory

	// Copy string and persist it

	TX_BEGIN(pop){
		TX_ADD(root);
		// pmemobj_tx_add_range(root, 0, 32);
		//PMEMWRAP_SET_ABORTFLAG();
		POBJ_ALLOC(pop, NULL, struct substruct, sizeof(struct substruct), NULL, NULL);
		// memset(&rootp[0], set_chars[0], 64);
		// memset(&rootp[1], set_chars[1], 64);
		// memset(&rootp[2], set_chars[2], 64);
	}TX_END


	
	//pmemobj_memcpy_persist(pop, rootp->buf, buf, MAX_BUF_LEN * 32);

	// Write the string from persistent memory 	to console
	// printf("1st 64B %p :%.64s\n", &rootp[0], (char *)&rootp[0]);
	// printf("2nd 64B %p :%.64s\n", &rootp[1], (char *)&rootp[1]);
	// printf("3rd 64B %p :%.64s\n", &rootp[2], (char *)&rootp[2]);
	
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