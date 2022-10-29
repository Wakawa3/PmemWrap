#include "wraplibpmem.h"
#include "wraplibpmemobj.h"

__attribute__ ((constructor))
static void constructor_obj () {
    void *dlopen_val = dlopen("/home/satoshi/testlib/lib/libpmemobj.so.1", RTLD_NOW);

    if((orig_pmemobj_create = dlsym(dlopen_val, "pmemobj_create")) == NULL){
        printf("orig_pmemobj_create: %p\n%s\n", orig_pmemobj_create, dlerror());
        exit(1);
    }

    if((orig_pmemobj_persist = dlsym(dlopen_val, "pmemobj_persist")) == NULL){
        printf("orig_pmemobj_persist: %p\n%s\n", orig_pmemobj_persist, dlerror());
        exit(1);
    }
}

void *addr;
size_t size;

PMEMobjpool *pmemobj_create(const char *path, const char *layout, size_t poolsize, mode_t mode){
    addr = orig_pmemobj_create(path, layout, poolsize, mode);
    size = poolsize;
    printf("addr: %p, size: %lu\n", addr, size);
    return addr;
}

void pmemobj_persist(PMEMobjpool *pop, const void *addr, size_t len){
    printf("wrap pmemobj_persist\n");
    return orig_pmemobj_persist(pop, addr, len);
    printf("end pmemobj_persist\n");
}