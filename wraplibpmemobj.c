#include "wraplibpmem.h"
#include "wraplibpmemobj.h"

PMEMobjpool *(*orig_pmemobj_create)(const char *path, const char *layout, size_t poolsize, mode_t mode);
PMEMobjpool *(*orig_pmemobj_open)(const char *path, const char *layout);
void (*orig_pmemobj_persist)(PMEMobjpool *pop, const void *addr, size_t len);
void (*orig_pmemobj_drain)(PMEMobjpool *pop);
int (*orig_pmemobj_tx_add_range)(PMEMoid oid, uint64_t hoff, size_t size);
int (*orig_pmemobj_tx_add_range_direct)(const void *ptr, size_t size);
void (*orig_pmemobj_tx_process)();
void (*orig_pmemobj_close)(PMEMobjpool *pop);
int (*orig_pmemobj_alloc)(PMEMobjpool *pop, PMEMoid *oidp, size_t size, uint64_t type_num, pmemobj_constr constructor, void *arg);
int (*orig_pmemobj_zalloc)(PMEMobjpool *pop, PMEMoid *oidp, size_t size, uint64_t type_num);
int (*orig_pmemobj_realloc)(PMEMobjpool *pop, PMEMoid *oidp, size_t size, uint64_t type_num);
void *(*orig_pmemobj_direct)(PMEMoid oid);
PMEMoid (*orig_pmemobj_tx_alloc)(size_t size, uint64_t type_num);
PMEMoid (*orig_pmemobj_tx_zalloc)(size_t size, uint64_t type_num);

__attribute__ ((constructor))
static void constructor_obj () {
    char *solib_path = getenv("PMEMWRAP_SOLIB_PATH");
    char sofile_path[1024];
    sprintf(sofile_path, "%s%s", solib_path, "/libpmemobj.so.1");
    void *dlopen_val = dlopen(sofile_path, RTLD_NOW);

    // void *dlopen_val = dlopen("/home/satoshi/testlib/lib/libpmemobj.so.1", RTLD_NOW);
    // void *dlopen_val = dlopen("/home/satoshi/safepm/build/pmdk/install/lib/pmdk_debug/libpmemobj.so.1", RTLD_NOW);

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

    if((orig_pmemobj_drain = dlsym(dlopen_val, "pmemobj_drain")) == NULL){
        fprintf(stderr, "orig_pmemobj_drain: %p\n%s\n", orig_pmemobj_drain, dlerror());
        exit(1);
    }

    if((orig_pmemobj_tx_add_range = dlsym(dlopen_val, "pmemobj_tx_add_range")) == NULL){
        fprintf(stderr, "orig_pmemobj_tx_add_range: %p\n%s\n", orig_pmemobj_tx_add_range, dlerror());
        exit(1);
    }

    if((orig_pmemobj_tx_add_range_direct = dlsym(dlopen_val, "pmemobj_tx_add_range_direct")) == NULL){
        fprintf(stderr, "orig_pmemobj_tx_add_range_direct: %p\n%s\n", orig_pmemobj_tx_add_range_direct, dlerror());
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

    if((orig_pmemobj_alloc = dlsym(dlopen_val, "pmemobj_alloc")) == NULL){
        fprintf(stderr, "orig_pmemobj_alloc: %p\n%s\n", orig_pmemobj_alloc, dlerror());
        exit(1);
    }

    if((orig_pmemobj_zalloc = dlsym(dlopen_val, "pmemobj_zalloc")) == NULL){
        fprintf(stderr, "orig_pmemobj_zalloc: %p\n%s\n", orig_pmemobj_zalloc, dlerror());
        exit(1);
    }

    if((orig_pmemobj_realloc = dlsym(dlopen_val, "pmemobj_realloc")) == NULL){
        fprintf(stderr, "orig_pmemobj_realloc: %p\n%s\n", orig_pmemobj_realloc, dlerror());
        exit(1);
    }

    if((orig_pmemobj_direct = dlsym(dlopen_val, "pmemobj_direct")) == NULL){
        fprintf(stderr, "orig_pmemobj_direct: %p\n%s\n", orig_pmemobj_direct, dlerror());
        exit(1);
    }

    if((orig_pmemobj_tx_alloc = dlsym(dlopen_val, "pmemobj_tx_alloc")) == NULL){
        fprintf(stderr, "orig_pmemobj_tx_alloc: %p\n%s\n", orig_pmemobj_tx_alloc, dlerror());
        exit(1);
    }

    if((orig_pmemobj_tx_zalloc = dlsym(dlopen_val, "pmemobj_tx_zalloc")) == NULL){
        fprintf(stderr, "orig_pmemobj_tx_zalloc: %p\n%s\n", orig_pmemobj_tx_zalloc, dlerror());
        exit(1);
    }
}

// void *addr;
// size_t size;

PMEMobjpool *pmemobj_create(const char *path, const char *layout, size_t poolsize, mode_t mode){
    // printf("****wrap pmemobj_create****\n");
    void *addr = orig_pmemobj_create(path, layout, poolsize, mode);
    size_t size = poolsize;
    // printf("addr: %p, size: %lu\n", addr, size);
    if(addr != NULL){
        add_PMEMaddrset(addr, poolsize, path, PMEMOBJ_FILE);
    }
    // printf("****end pmemobj_create****\n");
    return addr;
}

PMEMobjpool *pmemobj_open(const char *path, const char *layout){
    // printf("****wrap pmemobj_open****\n");

    int fd = open(path, O_RDONLY);
    size_t size = lseek(fd, 0, SEEK_END);
    close(fd);

    void *addr = orig_pmemobj_open(path, layout);
    // printf("addr: %p, size: %lu\n", addr, size);

    if(addr != NULL){
        add_PMEMaddrset(addr, size, path, PMEMOBJ_FILE);
    }
    // printf("****end pmemobj_open****\n");
    return addr;
}

void pmemobj_wrap_persist(PMEMobjpool *pop, const void *addr, size_t len, const char *file, int line){
    // printf("****wrap pmemobj_persist****\n");
    plus_persistcount(file, line);
    rand_set_abortflag(file, line);
    orig_pmemobj_persist(pop, addr, len);
    // printf("****end pmemobj_persist****\n");
}

void pmemobj_wrap_drain(PMEMobjpool *pop, const char *file, int line){
    // printf("****wrap pmemobj_drain****\n");
    plus_persistcount(file, line);
    rand_set_abortflag(file, line);
    orig_pmemobj_drain(pop);
    // printf("****end pmemobj_drain****\n");
}

int pmemobj_wrap_tx_add_range(PMEMoid oid, uint64_t hoff, size_t size, const char *file, int line){
    // printf("****wrap pmemobj_wrap_tx_add_range****\n");
    plus_persistcount(file, line);
    rand_set_abortflag(file, line);
    int ret = orig_pmemobj_tx_add_range(oid, hoff, size);
    // printf("****end pmemobj_wrap_tx_add_range****\n");
    return ret;
}

int pmemobj_wrap_tx_add_range_direct(const void *ptr, size_t size, const char *file, int line){
    //printf("add_range_direct start\n");
    plus_persistcount(file, line);
    rand_set_abortflag(file, line);
    //if(abortflag == 1) pmem_drain();
    
    //printf("add_range_direct end\n");
    return orig_pmemobj_tx_add_range_direct(ptr, size);
}


void pmemobj_wrap_close(PMEMobjpool *pop, const char *file, int line){
    plus_persistcount(file, line);
    rand_set_abortflag(file, line);
    if(abortflag == 1){
        pmem_drain();
    }
    delete_PMEMaddrset(pop);
    
    orig_pmemobj_close(pop);
}

void pmemobj_close(PMEMobjpool *pop){
    delete_PMEMaddrset(pop);
    
    orig_pmemobj_close(pop);
}

const char *tx_process_file = NULL;
int tx_process_line = 0;
int c_flag = 0;

void pmemobj_wrap_tx_process(const char *file, int line){
    //printf("****wrap pmemobj_wrap_tx_process**** file: %s, line: %d\n", file, line);

    if((tx_process_file == NULL) || (strcmp(tx_process_file, file) != 0) || (tx_process_line != line)){
        plus_persistcount(file, line);
        c_flag = 0;
        rand_set_abortflag(file, line);
    }
    else if(c_flag <= 1){
        c_flag++;
    }
    else{
        plus_persistcount(file, line);
        c_flag = 0;
        rand_set_abortflag(file, line);
    }

    // plus_persistcount(file, line);

    tx_process_file = file;
    tx_process_line = line;

    orig_pmemobj_tx_process();
    //printf("****end pmemobj_wrap_tx_process****\n");
}

void pmemobj_tx_process(){
    // printf("****wrap pmemobj_tx_process****\n");
    orig_pmemobj_tx_process();
    // printf("****end pmemobj_tx_process****\n");
}

void *pmemobj_wrap_memcpy_persist(PMEMobjpool *pop, void *dest, const void *src, size_t len, const char *file, int line){
    // printf("wrap pmemobj_wrap_memcpy_persist\n");
    void *ret = memcpy(dest, src, len);
    pmemobj_wrap_persist(pop, dest, len, file, line);
    return ret;
}

void *pmemobj_wrap_memset_persist(PMEMobjpool *pop, void *dest, int c, size_t len, const char *file, int line){
    // printf("wrap pmemobj_wrap_memset_persist\n");
    void *ret = memset(dest, c, len);
    pmemobj_wrap_persist(pop, dest, len, file, line);
    return ret;
}

int pmemobj_wrap_alloc(PMEMobjpool *pop, PMEMoid *oidp, size_t size, uint64_t type_num, pmemobj_constr constructor, void *arg, const char *file, int line){
    plus_persistcount(file, line);
    rand_set_abortflag(file, line);
    return orig_pmemobj_alloc(pop, oidp, size, type_num, constructor, arg);
}

int pmemobj_wrap_zalloc(PMEMobjpool *pop, PMEMoid *oidp, size_t size, uint64_t type_num, const char *file, int line){
    plus_persistcount(file, line);
    rand_set_abortflag(file, line);
    return orig_pmemobj_zalloc(pop, oidp, size, type_num);
}

int pmemobj_wrap_realloc(PMEMobjpool *pop, PMEMoid *oidp, size_t size, uint64_t type_num, const char *file, int line){
    plus_persistcount(file, line);
    rand_set_abortflag(file, line);
    return orig_pmemobj_realloc(pop, oidp, size, type_num);
}

void *pmemobj_wrap_direct(PMEMoid oid, const char *file, int line){
    plus_persistcount(file, line);
    rand_set_abortflag(file, line);
    if(abortflag == 1) pmem_drain();
    
    return orig_pmemobj_direct(oid);
}

PMEMoid pmemobj_wrap_tx_alloc(size_t size, uint64_t type_num, const char *file, int line){
    plus_persistcount(file, line);
    rand_set_abortflag(file, line);
    return orig_pmemobj_tx_alloc(size, type_num);
}

PMEMoid pmemobj_wrap_tx_zalloc(size_t size, uint64_t type_num, const char *file, int line){
    plus_persistcount(file, line);
    rand_set_abortflag(file, line);
    return orig_pmemobj_tx_zalloc(size, type_num);
}