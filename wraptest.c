#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <stdint.h>

typedef struct _pmemaddrset PMEMaddrset;

struct _pmemaddrset {
    void *orig_addr;
    void *fake_addr;
    PMEMaddrset *next;
    PMEMaddrset *prev;
};

PMEMaddrset *head = NULL;
PMEMaddrset *tail = NULL;

void *pmem_map_file(const char *path, size_t len, int flags, mode_t mode, size_t *mapped_lenp, int *is_pmemp){
    void *(*orig_pmem_map_file)(const char*, size_t, int, mode_t, size_t*, int*) = dlsym(RTLD_NEXT, "pmem_map_file");

    PMEMaddrset *addrset = (PMEMaddrset *)malloc(sizeof(PMEMaddrset));
    addrset->orig_addr = orig_pmem_map_file(path, len, flags, mode, mapped_lenp, is_pmemp);
    addrset->fake_addr = malloc(len);
    addrset->next = NULL;
    addrset->prev = tail;
    memcpy(addrset->fake_addr, addrset->orig_addr, len);
    printf("wrap pmem_map_file\n");

    if(head == NULL){
        head = addrset;
    }
    else{
        tail->next = addrset;
    }
    tail = addrset;

    return addrset->fake_addr;
}

void pmem_persist(const void *addr, size_t len){
    printf("wrap pmem_persist\n");
    void (*orig_pmem_persist)(const void*, size_t) = dlsym(RTLD_NEXT, "pmem_map_file");

    if(head == NULL){
        orig_pmem_persist(addr, len);
        return;
    }

    PMEMaddrset *tmp = head;

    while(tmp != NULL){
        if(tmp->fake_addr == addr){
            memcpy(tmp->orig_addr, tmp->fake_addr, len);
            orig_pmem_persist(tmp->orig_addr, len);
            return;
        }
        tmp = tmp->next;
    }
}

int pmem_unmap(void *addr, size_t len){
    printf("wrap pmem_unmap\n");
    int (*orig_pmem_unmap)(void*, size_t) = dlsym(RTLD_NEXT, "pmem_unmap");

    if(head == NULL){
        return orig_pmem_unmap(addr, len);
    }

    PMEMaddrset *tmp = head;
    void *orig_addr;

    while(tmp != NULL){
        if(tmp->fake_addr == addr){
            if(tmp == head && tmp == tail){
                head = NULL;
                tail = NULL;
            }
            else if(tmp == head && tmp != tail){
                tmp->next->prev = NULL;
                head = tmp->next;
            }
            else if(tmp != head && tmp == tail){
                tmp->prev->next = NULL;
                tail = tmp->prev;
            }
            else{ //tmp != head && tmp != tail
                tmp->prev->next = tmp->next;
                tmp->next->prev = tmp->prev;
            }
            memcpy(tmp->orig_addr, tmp->fake_addr, len);
            orig_addr = tmp->orig_addr;
            free(tmp);
            return orig_pmem_unmap(orig_addr, len);
        }
        tmp = tmp->next;
    }

    return -1;
}