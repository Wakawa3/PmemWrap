// #include <string.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include "libpmem.h"
// #include <stdint.h>

#include "libpmem.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libpmem.h>

#define PMEM_LEN 1024

// struct teststruct{
//     long int addr;
//     char padding[60];
//     long int addr2;
// };

int main(int argc, char *argv[]){
    char buf[] = "/mnt/pmem0/p";
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

    printf("is_pmem: %d, mapped_len: %ld\n", is_pmem, mapped_len);

	/* store a string to the persistent memory */
	//strcpy(pmemaddr, buf);

	/* flush above strcpy to persistence */


	/* output a string from the persistent memory to console */
	//printf("\nWrite the (%s) string to persistent memory.\n",pmemaddr);	

    // printf("%ld\n", mapped_len);

    // //printf("addr: %d, addr2: %d", pmemaddr->addr, pmemaddr->addr2);

    if(strcmp (argv[1], "-w") == 0){
        memset(pmemaddr, 0, mapped_len);
        *(long int *)pmemaddr = 100000400;
        *(long int *)(pmemaddr + 64) = 100000000;
        //char *string = (char *)((void *)pmemaddr + pmemaddr->addr + pmemaddr->addr2);
        // printf("addr: %ld, addr2: %ld\n",pmemaddr->addr, pmemaddr->addr2);
        // printf("addr: %ld, addr2: %ld, string*: %p, pmemaddr*: %p\n", pmemaddr->addr, pmemaddr->addr2, string, pmemaddr);

        void *string = (void *)(pmemaddr + *(long int *)pmemaddr - *(long int *)(pmemaddr + 64));
        
        strcpy(string, "teststring");
        if (is_pmem)
            pmem_persist(pmemaddr, mapped_len);
        else
            pmem_msync(pmemaddr, mapped_len);
    }
    else{
        char *pos = (char *)(pmemaddr + *(long int *)pmemaddr - *(long int *)(pmemaddr + 64));
        //strcpy(pos, "asdfghjkl");
        printf("addr: %ld, addr2: %ld, string*: %p, pmemaddr*: %p\n", *(long int *)pmemaddr, *(long int *)(pmemaddr + 64), pos, pmemaddr);
        printf("read string: %s\n", (char *)(pmemaddr + *(long int *)pmemaddr - *(long int *)(pmemaddr + 64)));

    }

    

}