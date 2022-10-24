#include "libpmem.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libpmem.h>

/* using 1k of pmem for this example */
#define PMEM_LEN 1024

// Maximum length of our buffer
#define MAX_BUF_LEN 30

extern void testfunc(void *addr, int len);

void write_hello_string (char *buf, char *path)
{
	char *pmemaddr, *pmemaddr2, *pmemaddr3;
	size_t mapped_len, mapped_len2, mapped_len3;
	int is_pmem, is_pmem2, is_pmem3;
	
	/* create a pmem file and memory map it */
	if ((pmemaddr = (char *)pmem_map_file(path, PMEM_LEN, PMEM_FILE_CREATE,
				0666, &mapped_len, &is_pmem)) == NULL) {
		perror("pmem_map_file");
		exit(1);
	}

	if ((pmemaddr2 = (char *)pmem_map_file("/mnt/pmem0/test2", PMEM_LEN, PMEM_FILE_CREATE,
				0666, &mapped_len2, &is_pmem2)) == NULL) {
		perror("pmem_map_file");
		exit(1);
	}

	if ((pmemaddr3 = (char *)pmem_map_file("/mnt/pmem0/test3", PMEM_LEN, PMEM_FILE_CREATE,
				0666, &mapped_len3, &is_pmem3)) == NULL) {
		perror("pmem_map_file");
		exit(1);
	}


	/* store a string to the persistent memory */
	char *buf2 = buf;
	char *buf3 = buf;

	strcpy(pmemaddr, buf);
	strcpy(pmemaddr2, buf2);
	strcpy(pmemaddr3, buf3);

	/* flush above strcpy to persistence */
	printf("is_pmem:%d\n", is_pmem);
	printf("pmemaddr: %p, pmemaddr2: %p, pmemaddr3: %p\n",pmemaddr, pmemaddr2, pmemaddr3);
	if (is_pmem){
		pmem_flush(pmemaddr + 42, 10);
		pmem_flush(pmemaddr + 66, 9);
		//pmem_persist(pmemaddr, mapped_len);
		pmem_persist(pmemaddr + 13, 14);
		//pmem_persist(pmemaddr2, mapped_len2);
		testfunc(pmemaddr2, mapped_len2);
		pmem_persist(pmemaddr3, mapped_len3);
		for(int i = 0; i<4; i++){
			pmem_persist(pmemaddr, mapped_len);
		}
	}
	else
		pmem_msync(pmemaddr, mapped_len);

	/* output a string from the persistent memory to console */
	printf("\nWrite the (%s) string to persistent memory.\n",pmemaddr);	

	pmem_unmap(pmemaddr, mapped_len);
	pmem_unmap(pmemaddr2, mapped_len2);
	pmem_unmap(pmemaddr3, mapped_len3);
			
	return;	
}

/****************************
 * This function reads the "Hello..." string from persistent-memory.
 *****************************/
void read_hello_string(char *path)
{
	char *pmemaddr;
	size_t mapped_len;
	int is_pmem;

		/* open the pmem file to read back the data */
		if ((pmemaddr = (char *)pmem_map_file(path, PMEM_LEN, PMEM_FILE_CREATE,
					0666, &mapped_len, &is_pmem)) == NULL) {
			perror("pmem_map_file");
			exit(1);
		}  	
		/* Reading the string from persistent-memory and write to console */
		printf("\nRead the (%s) string from persistent memory.\n",pmemaddr);

	pmem_unmap(pmemaddr, mapped_len);
	
	return;
}

/****************************
 * This main function gather from the command line and call the appropriate
 * functions to perform read and write persistently to memory.
 *****************************/
int main(int argc, char *argv[])
{
	char *path = argv[2];
	
	// Create the string to save to persistent memory
	char *buf = argv[3];
	
	if (strcmp (argv[1], "-w") == 0) {
	
		write_hello_string (buf, path);
		
	}   else if (strcmp (argv[1], "-r") == 0) {

		read_hello_string(path);
	}	
	else { 
		fprintf(stderr, "Usage: %s <-w/-r> <filename>\n", argv[0]);
		exit(1);
	}

	return 0;
}