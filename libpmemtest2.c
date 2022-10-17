#include "libpmem.h"

#include <stdio.h>
#include <libpmem.h>

void testfunc(void *addr, int len){
    printf("testfunc\n");
    pmem_persist(addr, len);
}