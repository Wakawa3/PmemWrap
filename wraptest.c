#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <stdint.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_PATH_LENGTH 256
#define CACHE_LINE_SIZE 64

#define MAX_FILE_LENGTH 256
#define MAX_LINE_LENGTH 256

typedef struct _pmemaddrset PMEMaddrset;

struct _pmemaddrset {
    void *orig_addr;
    void *fake_addr;
    PMEMaddrset *next;
    PMEMaddrset *prev;
    int len;
    int persist_count;
    char *orig_path;
};

PMEMaddrset *head = NULL;
PMEMaddrset *tail = NULL;

typedef struct _line_info LINEinfo;

struct _line_info{
    int line;
    int count;
};

char *file_list[MAX_FILE_LENGTH];
//LINEinfo *(persist_line_list[MAX_LINE_LENGTH])[MAX_FILE_LENGTH]; //ポインタ配列 ポインタはpersist_line_list[MAX_LINE_LENGTH]のアドレスを指す
LINEinfo persist_line_list[MAX_FILE_LENGTH][MAX_LINE_LENGTH];

void rand_memcpy(void *dest, const void *src, size_t n);
void rand_file_generate(PMEMaddrset *set, size_t n);

__attribute__ ((constructor))
static void constructor () {
    memset(file_list, 0, sizeof(char *) * MAX_FILE_LENGTH);
    memset(persist_line_list, 0, sizeof(LINEinfo) * MAX_FILE_LENGTH * MAX_LINE_LENGTH);
}

void plus_persistcount(char *file, int line){
    int matched = 0;
    int file_id;
    //ファイル名がマッチしたらそのときのid,マッチしないときは最後のid+1
    for(file_id=0; (file_id<MAX_FILE_LENGTH) && (file_list[file_id] != NULL); file_id++){
        if(strcmp(file_list[file_id], file) == 0){
            matched = 1;
            break;
        }
    }

    if(file_id == MAX_FILE_LENGTH){
        fprintf(stderr, "write_persistcount error, %s, %d\n", __FILE__, __LINE__);
        exit(1);
    }

    if(matched == 0){
        file_list[file_id] = (char *)malloc(strlen(file) + 1);
        strcpy(file_list[file_id], file);
        persist_line_list[file_id][file_id].line = line;
        persist_line_list[file_id][file_id].count = 1;
        return;
    }

    for(int i=0; i<MAX_LINE_LENGTH; i++){
        if((line == persist_line_list[file_id][i].line) 
                || (persist_line_list[file_id][i].line == 0)){
            persist_line_list[file_id][i].line = line;
            persist_line_list[file_id][i].count++;
            break;
        }
    }

}

void write_persistcountfile(){
    int fd = open("countfile.txt", O_WRONLY | O_TRUNC | O_CREAT, 0666);

    int file_id;
    int file_name_len;
    char *tmp;
    for(file_id=0; file_id<MAX_FILE_LENGTH && file_list[file_id] != NULL; file_id++){
        file_name_len = strlen(file_list[file_id]);
        tmp = (char *)malloc(file_name_len + 3); // _, \n, \0
        sprintf(tmp, "_%s\n", file_list[file_id]);
        if(write(fd, tmp, file_name_len + 2) != file_name_len + 2){
            fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
            exit(1);
        }
        free(tmp);

        for(int i=0;(i<MAX_LINE_LENGTH) && (persist_line_list[file_id][i].line != 0); i++){
            tmp = (char *)malloc(23); //int digit(10) + _ + int digit(10) + \n + \0
            sprintf(tmp, "%010d_%010d\n", persist_line_list[file_id][i].line, persist_line_list[file_id][i].count);
            if(write(fd, tmp, 22) != 22){
                fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
                exit(1);
            }
            free(tmp);
        }
    }

    close(fd);
}

void *pmem_map_file(const char *path, size_t len, int flags, mode_t mode, size_t *mapped_lenp, int *is_pmemp){
    srand((unsigned int)time(NULL));

    void *(*orig_pmem_map_file)(const char*, size_t, int, mode_t, size_t*, int*) = dlsym(RTLD_NEXT, "pmem_map_file");

    PMEMaddrset *addrset = (PMEMaddrset *)malloc(sizeof(PMEMaddrset));
    addrset->orig_addr = orig_pmem_map_file(path, len, flags, mode, mapped_lenp, is_pmemp);
    addrset->fake_addr = malloc(len);
    addrset->next = NULL;
    addrset->prev = tail;
    addrset->len = len;
    addrset->orig_path = (char *)malloc(strlen(path));
    strcpy(addrset->orig_path, path);
    addrset->persist_count = 0;
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

void pmem_persist(const void *addr, size_t len, char* file, int line){
    printf("wrap pmem_persist\n");
    void (*orig_pmem_persist)(const void*, size_t) = dlsym(RTLD_NEXT, "pmem_map_file");

    if(head == NULL){
        orig_pmem_persist(addr, len);
        return;
    }

    PMEMaddrset *set = head;

    while(set != NULL){
        if(set->fake_addr == addr){
            //rand_memcpy(set->orig_addr, set->fake_addr, len);
            //rand_file_generate(set, len);
            memcpy(set->orig_addr, set->fake_addr, len);
            orig_pmem_persist(set->orig_addr, len);
            plus_persistcount(file, line);
            printf("pmem_persist file:%s, line:%d\n", file, line);
            return;
        }
        set = set->next;
    }
}

int pmem_unmap(void *addr, size_t len){
    printf("wrap pmem_unmap\n");
    int (*orig_pmem_unmap)(void*, size_t) = dlsym(RTLD_NEXT, "pmem_unmap");

    if(head == NULL){
        return orig_pmem_unmap(addr, len);
    }

    PMEMaddrset *set = head;
    void *orig_addr;

    while(set != NULL){
        if(set->fake_addr == addr){
            if(set == head && set == tail){
                head = NULL;
                tail = NULL;
            }
            else if(set == head && set != tail){
                set->next->prev = NULL;
                head = set->next;
            }
            else if(set != head && set == tail){
                set->prev->next = NULL;
                tail = set->prev;
            }
            else{ //set != head && set != tail
                set->prev->next = set->next;
                set->next->prev = set->prev;
            }
            memcpy(set->orig_addr, set->fake_addr, len);
            orig_addr = set->orig_addr;
            free(set);
            return orig_pmem_unmap(orig_addr, len);
        }
        set = set->next;
    }

    return -1;
}

void rand_memcpy(void *dest, const void *src, size_t n){
    // srand((unsigned int)time(NULL));

    int i;
    for (i = 0; i < n; i += CACHE_LINE_SIZE){
        if(rand() % 2 == 0){
            memcpy(dest + i, src + i, CACHE_LINE_SIZE);
            //printf("%d\n", i);
        }
    }

    i -= CACHE_LINE_SIZE;
    int remainder = n - i; 
    if(remainder % CACHE_LINE_SIZE != 0){
        memcpy(dest + i, src + i, remainder);
        printf("remainder\n");
    }
}

void rand_file_generate(PMEMaddrset *set, size_t n){
    char generated_path[MAX_PATH_LENGTH];
    void *new_file;
    int fd;

    for(int i=0;i<3;i++){
        sprintf(generated_path, "%s_%d_%d", set->orig_path, set->persist_count, i);

        fd = open(generated_path, O_CREAT|O_RDWR, 0666);
        ftruncate(fd, set->len);
        new_file = mmap(NULL, set->len, PROT_WRITE, MAP_SHARED, fd, 0);

        memcpy(new_file, set->orig_addr, set->len);
        rand_memcpy(new_file, set->fake_addr, set->len);

        munmap(new_file, set->len);
        close(fd);
    }

    set->persist_count++;
}

__attribute__ ((destructor))
static void destructor () {
    printf("destructor\n");
    PMEMaddrset *set = head;

    while(set != NULL){
        memcpy(set->orig_addr, set->fake_addr, set->len);
        set = set->next;
    }

    write_persistcountfile();
}
// void *util_map_sync(void *addr, size_t len, int proto, int flags, int fd, long int offset, int *map_sync){
//     printf("wrap util_map_sync\n");
//     void* (*orig_util_map_sync)(void*, size_t, int, int, int, long int, int*) = dlsym(RTLD_NEXT, "util_map_sync");
//     return orig_util_map_sync(addr, len, proto, flags, fd, offset, map_sync);
// }


