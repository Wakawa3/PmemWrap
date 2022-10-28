#include "libpmem.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libpmem.h>

#define PMEM_LEN 1024

int main(int argc, char *argv[]){
    //char buf[] = "/mnt/pmem0/p";
    char *path = argv[2];
    // path = buf;
    // size_t mapped_len;
    // int is_pmem;

    // //struct teststruct *pmemaddr;
    // char *pmemaddr;

    // if ((pmemaddr = (char *)pmem_map_file(path, PMEM_LEN, PMEM_FILE_CREATE,
	// 			0666, &mapped_len, &is_pmem)) == NULL) {
    //     perror("pmem_map_file");
	// 	exit(1);
    // }

    void *pmemaddr;
	size_t mapped_len;
	int is_pmem;
	
	/* create a pmem file and memory map it */
	if ((pmemaddr = (void *)pmem_map_file(path, PMEM_LEN, PMEM_FILE_CREATE,
				0666, &mapped_len, &is_pmem)) == NULL) {
		perror("pmem_map_file");
		exit(1);
	}

    //printf("is_pmem: %d, mapped_len: %ld\n", is_pmem, mapped_len);

    if(strcmp (argv[1], "-w") == 0){
        memset(pmemaddr, 0, mapped_len);
        *(long int *)pmemaddr = 100000400;
        *(long int *)(pmemaddr + 64) = 100000000;

        void *string = (void *)(pmemaddr + *(long int *)pmemaddr - *(long int *)(pmemaddr + 64));
        
        strcpy(string, "teststring");
        // printf("%s\n", pmemaddr - 10000000000000);
        // abort();
        if (is_pmem)
            pmem_persist(pmemaddr, mapped_len);
        else
            pmem_msync(pmemaddr, mapped_len);
    }
    else{
        if((*(long int *)pmemaddr == 0) && *(long int *)(pmemaddr + 64) == 0){
            printf("not written\n");
            return 0;
        }
        char *pos = (char *)(pmemaddr + *(long int *)pmemaddr - *(long int *)(pmemaddr + 64));

        //printf("addr: %ld, addr2: %ld, string*: %p, pmemaddr*: %p\n", *(long int *)pmemaddr, *(long int *)(pmemaddr + 64), pos, pmemaddr);
        printf("read string: %s\n", (char *)(pmemaddr + *(long int *)pmemaddr - *(long int *)(pmemaddr + 64)));
    }
}