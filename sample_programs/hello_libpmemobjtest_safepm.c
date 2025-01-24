/*
 * hello_libpmemobj.c -- an example for libpmemobj library
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libpmemobj.h>

// Name of our layout in the pool
#define LAYOUT "hello_layout"

// Maximum length of our buffer
#define MAX_BUF_LEN 30


// Root structure
struct my_root {
	size_t len;
	char buf[MAX_BUF_LEN];
};

/****************************
 * This function writes the "Hello..." string to persistent-memory.
 *****************************/
void write_hello_string (char *buf, char *path)
{
	PMEMobjpool *pop;
	
	// Create the pmemobj pool or open it if it already exists
	pop = pmemobj_create(path, LAYOUT, PMEMOBJ_MIN_POOL * 4, 0666);

	// Check if create failed		
	if (pop == NULL) 
	{
		perror(path);
		exit(1);
	}
					
	// Get the PMEMObj root
	PMEMoid root = pmemobj_root(pop, sizeof (struct my_root));

	// Pointer for structure at the root
	struct my_root *rootp = pmemobj_direct(root);

	// Write the string to persistent memory
	// Assign the string length and persist it
	rootp->len = strlen(buf);
	pmemobj_persist(pop, &rootp->len, sizeof (rootp->len));
	
	// Copy string and persist it
	pmemobj_memcpy_persist(pop, rootp->buf, buf, rootp->len);

	// Write the string from persistent memory 	to console
	printf("\nWrite the (%s) string to persistent-memory.\n", rootp->buf);
	
	// Close PMEM object pool
	pmemobj_close(pop);	
		
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
	printf("\nRead the (%s) string from persistent-memory.\n", rootp->buf);
	
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

		write_hello_string(buf, path);
		
	} else if (strcmp (argv[1], "-r") == 0) {

		read_hello_string(path);
	} else { 
		fprintf(stderr, "Usage: %s <-w/-r> <filename>\n", argv[0]);
		exit(1);
	}

}