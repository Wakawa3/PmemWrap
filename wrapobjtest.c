#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <stdint.h>

void *orig_addr = NULL;
void *fake_addr;

typedef struct pmemaddrset {
    void *orig_addr;
    void *fake_addr;
} PMEMaddrset;

int is_obj = 0;
int root_size = 0;
int obj_direct_used = 0;
void *orig_rootp;
void *fake_rootp;

typedef struct pmemoid {
	uint64_t pool_uuid_lo;
	uint64_t off;
} PMEMoid;

void *pmem_map_file(const char *path, size_t len, int flags, mode_t mode, size_t *mapped_lenp, int *is_pmemp){
    void *(*orig_pmem_map_file)(const char*, size_t, int, mode_t, size_t*, int*) = dlsym(RTLD_NEXT, "pmem_map_file");
    orig_addr = orig_pmem_map_file(path, len, flags, mode, mapped_lenp, is_pmemp);
    fake_addr = malloc(len);
    memcpy(fake_addr, orig_addr, len);
    printf("wrap pmem_map_file\n");
    return fake_addr;
}

void pmem_persist(const void *addr, size_t len){
    printf("wrap pmem_persist\n");
    void (*orig_pmem_persist)(const void*, size_t) = dlsym(RTLD_NEXT, "pmem_map_file");

    if(is_obj == 0){
        if(orig_addr == NULL){
            orig_pmem_persist(addr, len);
            //printf("wrap pmem_persist, 0, 0\n");
            return;
        }

        memcpy(orig_addr, fake_addr, len);
        orig_pmem_persist(orig_addr, len);
    }
    else{ //is_obj == 1
        if(obj_direct_used == 1){
            obj_direct_used = 0;
            memcpy(orig_rootp, fake_rootp, root_size);
        }
        orig_pmem_persist(addr, len);
    }
}

PMEMoid pmemobj_root(void *pop, size_t size){ //PMEMobjpool *pop, size_t size
    printf("wrap pmemobj_root\n");
    is_obj = 1;
    root_size = size;

    PMEMoid (*orig_pmemobj_root)(void*, size_t) = dlsym(RTLD_NEXT, "pmemobj_root");
    return orig_pmemobj_root(pop, size);
}

void *pmemobj_openU(const char *path, const char *layout){
    printf("wrap pmemobj_openU\n");
    void *(*orig_pmemobj_openU)(const char*, const char*) = dlsym(RTLD_NEXT, "pmemobj_openU");
    return orig_pmemobj_openU(path, layout);
}

// void *pmemobj_direct(PMEMoid oid){
//     printf("wrap pmemobj_direct\n");
//     if(is_obj == 0){
//         fprintf(stderr, "error: wrap pmemobj_direct\n");
//         exit(1);
//     }

//     if(obj_direct_used == 0){
//         obj_direct_used = 1;
//         void *(*orig_pmemobj_direct)(PMEMoid) = dlsym(RTLD_NEXT, "pmemobj_direct");
//         orig_rootp = orig_pmemobj_direct(oid);

//         fake_rootp = malloc(root_size);
//         memcpy(fake_rootp, orig_rootp, root_size);
//         return fake_rootp;
//     }
//     else{ //obj_direct_used == 1
//         return fake_rootp;
//     }
// }


