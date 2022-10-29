#include "wraplibpmem.h"
#include "wraplibpmemobj.h"

PMEMobjpool *(*orig_pmemobj_create)(const char *path, const char *layout, size_t poolsize, mode_t mode);
PMEMobjpool *(*orig_pmemobj_open)(const char *path, const char *layout);
void (*orig_pmemobj_persist)(PMEMobjpool *pop, const void *addr, size_t len);
int (*orig_pmemobj_tx_add_range)(PMEMoid oid, uint64_t hoff, size_t size);
void (*orig_pmemobj_tx_process)();

__attribute__ ((constructor))
static void constructor_obj () {
    void *dlopen_val = dlopen("/home/satoshi/testlib/lib/libpmemobj.so.1", RTLD_NOW);

    if((orig_pmemobj_create = dlsym(dlopen_val, "pmemobj_create")) == NULL){
        printf("orig_pmemobj_create: %p\n%s\n", orig_pmemobj_create, dlerror());
        exit(1);
    }

    if((orig_pmemobj_open = dlsym(dlopen_val, "pmemobj_open")) == NULL){
        printf("orig_pmemobj_open: %p\n%s\n", orig_pmemobj_open, dlerror());
        exit(1);
    }

    if((orig_pmemobj_persist = dlsym(dlopen_val, "pmemobj_persist")) == NULL){
        printf("orig_pmemobj_persist: %p\n%s\n", orig_pmemobj_persist, dlerror());
        exit(1);
    }

    if((orig_pmemobj_tx_add_range = dlsym(dlopen_val, "pmemobj_tx_add_range")) == NULL){
        printf("orig_pmemobj_tx_add_range: %p\n%s\n", orig_pmemobj_tx_add_range, dlerror());
        exit(1);
    }

    if((orig_pmemobj_tx_process = dlsym(dlopen_val, "pmemobj_tx_process")) == NULL){
        printf("orig_pmemobj_tx_process: %p\n%s\n", orig_pmemobj_tx_process, dlerror());
        exit(1);
    }
}

void *addr;
size_t size;

PMEMobjpool *pmemobj_create(const char *path, const char *layout, size_t poolsize, mode_t mode){
    printf("****wrap pmemobj_create****\n");
    addr = orig_pmemobj_create(path, layout, poolsize, mode);
    size = poolsize;
    printf("addr: %p, size: %lu\n", addr, size);
    if(addr != NULL){
        add_PMEMaddrset(addr, poolsize, PMEMOBJ_FILE);
    }
    printf("****end pmemobj_create****\n");
    return addr;
}

PMEMobjpool *pmemobj_open(const char *path, const char *layout){
    printf("****wrap pmemobj_open****\n");

    int fd = open(path, O_RDONLY);
    size = lseek(fd, 0, SEEK_END);
    close(fd);

    addr = orig_pmemobj_open(path, layout);
    printf("addr: %p, size: %lu\n", addr, size);

    if(addr != NULL){
        add_PMEMaddrset(addr, size, PMEMOBJ_FILE);
    }
    printf("****end pmemobj_open****\n");
    return addr;
}

void pmemobj_persist(PMEMobjpool *pop, const void *addr, size_t len){
    printf("****wrap pmemobj_persist****\n");
    orig_pmemobj_persist(pop, addr, len);
    printf("****end pmemobj_persist****\n");
}

int pmemobj_tx_add_range(PMEMoid oid, uint64_t hoff, size_t size){
    printf("****wrap pmemobj_tx_add_range****\n");
    int ret = orig_pmemobj_tx_add_range(oid, hoff, size);
    printf("****end pmemobj_tx_add_range****\n");
    return ret;
}

void pmemobj_tx_process(){
    printf("****wrap pmemobj_tx_process****\n");
    //abortflag = 1;
    orig_pmemobj_tx_process();
    printf("****end pmemobj_tx_process****\n");
}