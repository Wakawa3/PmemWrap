/*
Copyright (c) 2018-2019 Intel Corporation

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
 
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 
SPDX-License-Identifier: BSD-3-Clause
*/

/*
 * hello_libpmem.c -- an example for libpmem library
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libpmem.h>

/* using 1k of pmem for this example */
#define PMEM_LEN 1024

// Maximum length of our buffer
#define MAX_BUF_LEN 30

/****************************
 * This function writes the "Hello..." string to persistent-memory.
 *****************************/
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
		pmem_persist(pmemaddr, mapped_len);
		pmem_persist(pmemaddr2, mapped_len2);
		pmem_persist(pmemaddr3, mapped_len3);
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