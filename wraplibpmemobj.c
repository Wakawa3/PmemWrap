#include "wraplibpmem.h"
#include "wraplibpmemobj.h"

PMEMobjpool *(*orig_pmemobj_create)(const char *path, const char *layout, size_t poolsize, mode_t mode);
PMEMobjpool *(*orig_pmemobj_open)(const char *path, const char *layout);
void (*orig_pmemobj_persist)(PMEMobjpool *pop, const void *addr, size_t len);
int (*orig_pmemobj_tx_add_range)(PMEMoid oid, uint64_t hoff, size_t size);
void (*orig_pmemobj_tx_process)();
void (*orig_pmemobj_close)(PMEMobjpool *pop);

__attribute__ ((constructor))
static void constructor_obj () {
    void *dlopen_val = dlopen("/home/satoshi/testlib/lib/libpmemobj.so.1", RTLD_NOW);

    if((orig_pmemobj_create = dlsym(dlopen_val, "pmemobj_create")) == NULL){
        fprintf(stderr, "orig_pmemobj_create: %p\n%s\n", orig_pmemobj_create, dlerror());
        exit(1);
    }

    if((orig_pmemobj_open = dlsym(dlopen_val, "pmemobj_open")) == NULL){
        fprintf(stderr, "orig_pmemobj_open: %p\n%s\n", orig_pmemobj_open, dlerror());
        exit(1);
    }

    if((orig_pmemobj_persist = dlsym(dlopen_val, "pmemobj_persist")) == NULL){
        fprintf(stderr, "orig_pmemobj_persist: %p\n%s\n", orig_pmemobj_persist, dlerror());
        exit(1);
    }

    if((orig_pmemobj_tx_add_range = dlsym(dlopen_val, "pmemobj_tx_add_range")) == NULL){
        fprintf(stderr, "orig_pmemobj_tx_add_range: %p\n%s\n", orig_pmemobj_tx_add_range, dlerror());
        exit(1);
    }

    if((orig_pmemobj_tx_process = dlsym(dlopen_val, "pmemobj_tx_process")) == NULL){
        fprintf(stderr, "orig_pmemobj_tx_process: %p\n%s\n", orig_pmemobj_tx_process, dlerror());
        exit(1);
    }

    if((orig_pmemobj_close = dlsym(dlopen_val, "pmemobj_close")) == NULL){
        fprintf(stderr, "orig_pmemobj_close: %p\n%s\n", orig_pmemobj_close, dlerror());
        exit(1);
    }

    
}

void *addr;
size_t size;

PMEMobjpool *pmemobj_create(const char *path, const char *layout, size_t poolsize, mode_t mode){
    // printf("****wrap pmemobj_create****\n");
    addr = orig_pmemobj_create(path, layout, poolsize, mode);
    size = poolsize;
    // printf("addr: %p, size: %lu\n", addr, size);
    if(addr != NULL){
        add_PMEMaddrset(addr, poolsize, PMEMOBJ_FILE);
    }
    // printf("****end pmemobj_create****\n");
    return addr;
}

PMEMobjpool *pmemobj_open(const char *path, const char *layout){
    // printf("****wrap pmemobj_open****\n");

    int fd = open(path, O_RDONLY);
    size = lseek(fd, 0, SEEK_END);
    close(fd);

    addr = orig_pmemobj_open(path, layout);
    // printf("addr: %p, size: %lu\n", addr, size);

    if(addr != NULL){
        add_PMEMaddrset(addr, size, PMEMOBJ_FILE);
    }
    // printf("****end pmemobj_open****\n");
    return addr;
}

void pmemobj_wrappersist(PMEMobjpool *pop, const void *addr, size_t len, char *file, int line){
    // printf("****wrap pmemobj_persist****\n");
    plus_persistcount(file, line);
    orig_pmemobj_persist(pop, addr, len);
    // printf("****end pmemobj_persist****\n");
}

int pmemobj_wraptx_add_range(PMEMoid oid, uint64_t hoff, size_t size, char* file, int line){
    // printf("****wrap pmemobj_tx_add_range****\n");
    plus_persistcount(file, line);
    abortflag = rand_set_abortflag(file, line);
    int ret = orig_pmemobj_tx_add_range(oid, hoff, size);
    // printf("****end pmemobj_tx_add_range****\n");
    return ret;
}

void pmemobj_close(PMEMobjpool *pop){
    delete_PMEMaddrset(pop);
    
    orig_pmemobj_close(pop);
}

char *tx_process_file = NULL;
int tx_process_line = 0;
int c_flag = 0;

void pmemobj_wraptx_process(char *file, int line){
    printf("****wrap pmemobj_wraptx_process**** file: %s, line: %d\n", file, line);

    if((tx_process_file == NULL) || (strcmp(tx_process_file, file) != 0) || (tx_process_line != line)){
        plus_persistcount(file, line);
        c_flag = 0;
        abortflag = rand_set_abortflag(file, line);
    }
    else if(c_flag <= 1){
        c_flag++;
    }
    else{
        plus_persistcount(file, line);
        c_flag = 0;
        abortflag = rand_set_abortflag(file, line);
    }

    // plus_persistcount(file, line);

    tx_process_file = file;
    tx_process_line = line;

    orig_pmemobj_tx_process();
    // printf("****end pmemobj_wraptx_process****\n");
}

void pmemobj_tx_process(){
    // printf("****wrap pmemobj_tx_process****\n");
    //abortflag = 1;
    orig_pmemobj_tx_process();
    // printf("****end pmemobj_tx_process****\n");
}

void *pmemobj_wrapmemcpy_persist(PMEMobjpool *pop, void *dest, const void *src, size_t len, char* file, int line){
    // printf("wrap pmemobj_wrapmemcpy_persist\n");
    void *ret = memcpy(dest, src, len);
    pmemobj_wrappersist(pop, dest, len, file, line);
    return ret;
}

void *pmemobj_wrapmemset_persist(PMEMobjpool *pop, void *dest, int c, size_t len, char* file, int line){
    // printf("wrap pmemobj_wrapmemset_persist\n");
    void *ret = memset(dest, c, len);
    pmemobj_wrappersist(pop, dest, len, file, line);
    return ret;
}