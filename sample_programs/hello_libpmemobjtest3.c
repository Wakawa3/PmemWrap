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
	char buf[3][64];
};
// Root structure
struct my_root {
	PMEMoid data_oid;
};

POBJ_LAYOUT_BEGIN(string_store);
POBJ_LAYOUT_ROOT(string_store, struct my_root);
POBJ_LAYOUT_TOID(string_store, struct datastruct);
POBJ_LAYOUT_END(string_store);

int main(int argc, char *argv[])
{
	PMEMobjpool *pop;

	char *path = argv[1];
	
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
		D_RW(root)->data_oid = data.oid;
		pmemobj_persist(pop, D_RW(root), sizeof(struct my_root));
		memset(D_RW(data)->buf[0], 'a', 64);
		memset(D_RW(data)->buf[1], 'b', 64);
		memset(D_RW(data)->buf[2], 'c', 64);
		pmemobj_persist(pop, D_RW(data), sizeof(struct datastruct));

		TX_BEGIN(pop){
			TX_ADD(data);
			memset(D_RW(data)->buf[0], 'd', 64);
		    memset(D_RW(data)->buf[1], 'e', 64);
		    memset(D_RW(data)->buf[2], 'f', 64);
			//PMEMWRAP_FORCE_ABORT();
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
		TOID_ASSIGN(data, D_RW(root)->data_oid);

		// printf("1st 64B %p :%.64s\n", D_RW(data)->buf[0], (char *)D_RW(data)->buf[0]);
		// printf("2nd 64B %p :%.64s\n", D_RW(data)->buf[1], (char *)D_RW(data)->buf[1]);
		// printf("3rd 64B %p :%.64s\n", D_RW(data)->buf[2], (char *)D_RW(data)->buf[2]);
		printf("1st 64B :%.64s\n", (char *)D_RW(data)->buf[0]);
		printf("2nd 64B :%.64s\n", (char *)D_RW(data)->buf[1]);
		printf("3rd 64B :%.64s\n", (char *)D_RW(data)->buf[2]);
	}


	// Close PMEM object pool
	pmemobj_close(pop);	
		
	return 0;
}