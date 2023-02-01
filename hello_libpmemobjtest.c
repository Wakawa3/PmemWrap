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
#define MAX_BUF_LEN 64


// Root structure
struct my_root {
	char buf[MAX_BUF_LEN];
	char buf2[MAX_BUF_LEN];
	char buf3[MAX_BUF_LEN];
};

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
	PMEMoid root = pmemobj_root(pop, sizeof (struct my_root));

	// Pointer for structure at the root
	struct my_root *rootp = pmemobj_direct(root);

	// Write the string to persistent memory
	printf("rootp: %p\n", rootp);

	// Copy string and persist it

	TX_BEGIN(pop){
		printf("rootp: %p\n", rootp);
		//pmemobj_tx_add_range_direct(rootp, 32);
		memset(rootp, set_chars[0], 64);
		memset(rootp->buf2, set_chars[1], 64);
		memset(rootp->buf3, set_chars[2], 64);
		//PMEMWRAP_FORCE_ABORT();
	}TX_END

	//pmemobj_memcpy_persist(pop, rootp->buf, buf, MAX_BUF_LEN * 32);

	// Write the string from persistent memory 	to console
	printf("1st 64B :%.64s\n", rootp->buf);
	printf("2nd 64B :%.64s\n", rootp->buf2);
	printf("3rd 64B :%.64s\n", rootp->buf3);
	
	// Close PMEM object pool
	//pmemobj_close(pop);	
		
	return;
}

/****************************
 * This function reads the "Hello..." string from persistent-memory.
 *****************************/
void read_hello_string(char *path)
{
	PMEMobjpool *pop;
	
	//Attempt open instead
	pop = pmemobj_open(path, LAYOUT);
	
	// Check if open failed
	if (pop == NULL) {
		perror(path);
		exit(1);
	} 
	
	// Get the PMEMObj root
	PMEMoid root = pmemobj_root(pop, sizeof (struct my_root));
	
	// Pointer for structure at the root
	struct my_root *rootp = pmemobj_direct(root);
	
	// Read the string from persistent memory and write to the console
	printf("1st 64B :%.64s\n", rootp->buf);
	printf("2nd 64B :%.64s\n", rootp->buf2);
	printf("3rd 64B :%.64s\n", rootp->buf3);
	
	// Close PMEM object pool
	pmemobj_close(pop);

	return;
}

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

		read_hello_string(path);
	} else { 
		fprintf(stderr, "Usage: %s <-w/-r> <filename>\n", argv[0]);
		exit(1);
	}

}