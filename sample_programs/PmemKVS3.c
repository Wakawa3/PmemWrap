/*
 * hello_libpmemobj.c -- an example for libpmemobj library
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libpmemobj.h>

// Name of our layout in the pool
#define LAYOUT "KVS_layout"
//#define NUM_LAYOUT "number"

// Maximum length of our buffer
#define MAX_BUF_LEN 30
#define MAX_STORE 100

#define PATH "/mnt/pmem0/data"
//#define NUM_PATH "num"


// Root structure
struct subKVstruct {
	//int id;
    int keylen;
    int value;
	char key[MAX_BUF_LEN];
};

struct KVstruct{
    struct subKVstruct data[MAX_STORE];
    int number;
};

/****************************
 * This function writes the "Hello..." string to persistent-memory.
 *****************************/
void write_KVS (char *key, int value)
{
    PMEMobjpool *pop;
    int id;
    struct KVstruct *rootp;
    PMEMoid root;

    //char *num_path = NUM_PATH;
    
    pop = pmemobj_create(PATH, LAYOUT, PMEMOBJ_MIN_POOL, 0666);
		
	if (pop == NULL) 
	{
        pop = pmemobj_open(PATH, LAYOUT);
        if(pop == NULL){
            perror(PATH);
            exit(1);
        }

        root = pmemobj_root(pop, sizeof (struct KVstruct));
        rootp = pmemobj_direct(root);
        id = rootp->number;
	}
    else{
        root = pmemobj_root(pop, sizeof (struct KVstruct));
        rootp = pmemobj_direct(root);
        id = 0;
    }

    TX_BEGIN(pop){
        pmemobj_tx_add_range(root, 0, sizeof(int) * 2);
	    rootp->data[id].keylen = strlen(key);
        rootp->data[id].value = value;
        memcpy(rootp->data[id].key, key, rootp->data[id].keylen);

        rootp->number = id + 1;

    } TX_END

	printf("Write [key: %s, value: %d]\n", rootp->data[id].key, rootp->data[id].value);
    printf("number : %d\n", rootp->number);

	pmemobj_close(pop);	
		
	return;
}

/****************************
 * This function reads the "Hello..." string from persistent-memory.
 *****************************/
void read_KVS(char *key, int read_all, int id)
{
    PMEMobjpool *pop;
    int number;
    struct KVstruct *rootp;
    PMEMoid root;
    
    pop = pmemobj_open(PATH, LAYOUT);
		
	if (pop == NULL) 
	{
        perror(PATH);
        exit(1);
    }

    root = pmemobj_root(pop, sizeof (struct KVstruct));
    rootp = pmemobj_direct(root);
    number = rootp->number;
	
    if(id != -1){
        printf("Read [key: %s, value: %d, keylen: %d]\n", rootp->data[id].key, rootp->data[id].value, rootp->data[id].keylen);
        pmemobj_close(pop);
        return;
    }

    int flag = 0;
    int i;    
    if(read_all == 0){
        for(i = 0; i < number; i++){
            printf("loop: %d\n", i);
            if(strncmp(rootp->data[i].key, key, rootp->data[i].keylen) == 0){
                printf("Read [key: %s, value: %d]\n", rootp->data[i].key, rootp->data[i].value);
                flag = 1;
                break;
            }
        }
        if(flag == 0)
            printf("%s is not registered.\n", key);
    }
    else{
        for(i = 0; i < number; i++){
            printf("Read [key: %s, value: %d]\n", rootp->data[i].key, rootp->data[i].value);
        }
    }
    
	pmemobj_close(pop);

	return;
}

void delete_KVS(char *key){
    PMEMobjpool *pop;
    int number;
    struct KVstruct *rootp;
    PMEMoid root;
    
    pop = pmemobj_open(PATH, LAYOUT);
		
	if (pop == NULL) 
	{
        perror(PATH);
        exit(1);
    }

    root = pmemobj_root(pop, sizeof (struct KVstruct));
    rootp = pmemobj_direct(root);
    number = rootp->number;
	

    int flag = 0;
    int i;
    for(i = 0; i < number; i++){
        printf("loop: %d\n", i);
        if(strncmp(rootp->data[i].key, key, rootp->data[i].keylen) == 0){
            printf("Read [key: %s, value: %d]\n", rootp->data[i].key, rootp->data[i].value);
            TX_BEGIN(pop){
                if(i != number - 1){     
                    pmemobj_tx_add_range(root, 0, sizeof(struct KVstruct));
                    memcpy(&rootp->data[i], &rootp->data[number-1], sizeof(struct subKVstruct));
                    printf("overwrite %s\n", key);
                }
                rootp->number = number - 1;
            }TX_END
            flag = 1;
            break;
        }
    }
    if(flag == 0)
        printf("%s is not registered.\n", key);
    
	pmemobj_close(pop);

	return;
}

/****************************
 * This main function gather from the command line and call the appropriate
 * function to perform read and write persistently to memory.
 *****************************/
int main(int argc, char *argv[])
{
    char *key = argv[2];
	
	if (strcmp (argv[1], "-w") == 0 && argc == 4) {
        int value = atoi(argv[3]);
		write_KVS(key, value);
	} else if (strcmp (argv[1], "-r") == 0 && argc == 3) {
		read_KVS(key, 0, -1);
	} else if (strcmp (argv[1], "-ra") == 0 && argc == 2) {
		read_KVS("", 1, -1);
    }else if (strcmp (argv[1], "-ri") == 0 && argc == 3) {
        read_KVS("", 0, atoi(argv[2]));
	} else if (strcmp (argv[1], "-d") == 0 && argc == 3){
        delete_KVS(key);
    } else { 
		fprintf(stderr, "Usage error: %s\n", argv[0]);
		exit(1);
	}

}