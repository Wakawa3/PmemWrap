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

struct myobject;
// Root structure
struct my_root;

POBJ_LAYOUT_BEGIN(string_store);
POBJ_LAYOUT_ROOT(string_store, struct my_root);
POBJ_LAYOUT_TOID(string_store, struct myobject);
POBJ_LAYOUT_END(string_store);

struct myobject{
	int num;
	char buf[64];
	TOID(struct myobject) next;
	TOID(struct myobject) prev;
};

struct my_root {
	TOID(struct myobject) object1;
	TOID(struct myobject) object2;
};

int main(int argc, char *argv[])
{
	PMEMobjpool *pop;

	char *path = argv[1];
	
	TOID(struct my_root) root;
	// Create the pmemobj pool or open it if it already exists
	if(access(path, F_OK) != 0){
		pop = pmemobj_create(path, LAYOUT, PMEMOBJ_MIN_POOL * 3, 0666);
		if (pop == NULL) 
		{
			perror(path);
			exit(1);
		}
		root = POBJ_ROOT(pop, struct my_root);
		TOID(struct myobject) object1;
		TOID(struct myobject) object2;
		POBJ_ALLOC(pop, &object1, struct myobject, sizeof(struct myobject), NULL, NULL);
		POBJ_ALLOC(pop, &object2, struct myobject, sizeof(struct myobject), NULL, NULL);
		D_RW(root)->object1 = object1;
		D_RW(root)->object2 = object2;
		D_RW(object1)->num = 1;
		D_RW(object1)->next = object2;
		D_RW(object1)->prev = object2;
		D_RW(object2)->num = 2;
		D_RW(object2)->next = object1;
		D_RW(object2)->prev = object2;
		
		pmemobj_persist(pop, D_RW(root), sizeof(struct my_root));
		pmemobj_persist(pop, D_RW(object1), sizeof(struct myobject));
		pmemobj_persist(pop, D_RW(object2), sizeof(struct myobject));

		TOID(struct myobject) object3;
		POBJ_ALLOC(pop, &object3, struct myobject, sizeof(struct myobject), NULL, NULL);
		D_RW(object3)->num = 3;
		pmemobj_persist(pop, D_RW(object3), sizeof(struct myobject));

		D_RW(object3)->next = object2;
		D_RW(object3)->prev = object1;
		D_RW(D_RW(object3)->next)->prev = object3;
		D_RW(D_RW(object3)->prev)->next = object3;

		PMEMWRAP_FORCE_ABORT();
		pmemobj_persist(pop, D_RW(object3), sizeof(struct myobject));
		pmemobj_persist(pop, D_RW(object1), sizeof(struct myobject));
		pmemobj_persist(pop, D_RW(object2), sizeof(struct myobject));
		

		// POBJ_FREE(&object1);
		
		// PMEMWRAP_FORCE_ABORT();
		// abort();
	}
	
	else{
		pop = pmemobj_open(path, LAYOUT);
		if (pop == NULL) 
		{
			perror(path);
			exit(1);
		}
		root = POBJ_ROOT(pop, struct my_root);
		TOID(struct myobject) object1;
		TOID(struct myobject) object2;
		object1 = D_RO(root)->object1;
		object2 = D_RO(root)->object2;

		printf("object1->next->num: %d\n", D_RO(D_RO(object1)->next)->num);
		printf("object2->prev->num: %d\n", D_RO(D_RO(object2)->prev)->num);
		TOID(struct myobject) object3 = D_RW(object1)->next;
		printf("object3->next->num: %d\n", D_RO(D_RO(object3)->next)->num);
		printf("object3->prev->num: %d\n", D_RO(D_RO(object3)->prev)->num);
	}

	pmemobj_close(pop);	
		
	return 0;
}