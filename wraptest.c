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

#define LIBPMEMMAP_ALIGN_VAL 0x200000

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

typedef struct _waitdrain_addrset Waitdrain_addrset;

struct _waitdrain_addrset{
    void *addr;
    Waitdrain_addrset *next;
    Waitdrain_addrset *prev;
    int len;
    PMEMaddrset *set;
};

Waitdrain_addrset *w_head = NULL;
Waitdrain_addrset *w_tail = NULL;

//int flushed = 0;

typedef struct _line_info LINEinfo;

struct _line_info{
    int line;
    int count;
};

char *file_list[MAX_FILE_LENGTH];
//LINEinfo *(persist_line_list[MAX_LINE_LENGTH])[MAX_FILE_LENGTH]; //ポインタ配列 ポインタはpersist_line_list[MAX_LINE_LENGTH]のアドレスを指す
LINEinfo persist_line_list[MAX_FILE_LENGTH][MAX_LINE_LENGTH];

void *(*orig_pmem_map_file)(const char*, size_t, int, mode_t, size_t*, int*);
//void (*orig_pmem_persist)(const void*, size_t);
int (*orig_pmem_unmap)(void*, size_t);
void (*orig_pmem_flush)(const void *, size_t);
void (*orig_pmem_drain)();

void rand_memcpy(void *dest, const void *src, size_t n, PMEMaddrset *set);
void rand_file_generate(PMEMaddrset *set, size_t n, uintptr_t d);

void read_persistcountfile();

void pmem_wrappersist(const void *addr, size_t len, char* file, int line);
void pmem_persist(const void *addr, size_t len);

void pmem_flush(const void *addr, size_t len);
void pmem_wrapdrain(char* file, int line);
void pmem_drain();

__attribute__ ((constructor))
static void constructor () {
    memset(file_list, 0, sizeof(char *) * MAX_FILE_LENGTH);
    memset(persist_line_list, 0, sizeof(LINEinfo) * MAX_FILE_LENGTH * MAX_LINE_LENGTH);
    //read_persistcountfile();
    orig_pmem_map_file = dlsym(RTLD_NEXT, "pmem_map_file");
    //orig_pmem_persist = dlsym(RTLD_NEXT, "pmem_persist");
    orig_pmem_unmap = dlsym(RTLD_NEXT, "pmem_unmap");
    orig_pmem_flush = dlsym(RTLD_NEXT, "pmem_flush");
    orig_pmem_drain = dlsym(RTLD_NEXT, "pmem_drain");
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
        persist_line_list[file_id][0].line = line;
        persist_line_list[file_id][0].count = 1;
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

void read_persistcountfile(){
    int fd = open("countfile.txt", O_RDONLY);
    if(fd == -1){
        perror(__func__);
        fprintf(stderr, " %s, %d, %s\n", __FILE__, __LINE__, __func__);
        exit(1);
    }
    
    int file_id;
    int file_name_len;
    char *tmp = (char *)malloc(MAX_FILE_LENGTH + 1);
    int r, offset;

    for(int i=0;;i++){
        offset = lseek(fd, 0, SEEK_CUR);
        //printf("%d offset: %d\n", __LINE__, offset);
        pread(fd, tmp, MAX_FILE_LENGTH, offset);
        char *new_line_ptr = strchr(tmp, '\n');
        tmp[new_line_ptr - tmp] = '\0'; //get file name
        offset = lseek(fd, new_line_ptr - tmp + 1, SEEK_CUR);
        printf("%d tmp: %s\n", __LINE__, tmp);

        file_list[i] = malloc(strlen(tmp + 1) + 1);//skip '_'
        strcpy(file_list[i], tmp + 1);
        printf("%d file_list[%d]: %send\n", __LINE__, i, file_list[i]);

        for (int j=0;;j++){
            r = pread(fd, tmp, 21, offset); // int digit + _ + int digit
            if((r == 0) || (tmp[0] == '_')){
                break;
            }
            tmp[21] = '\0';
            printf("%d tmp: %s, r: %d\n", __LINE__, tmp, r);
            offset = lseek(fd, 22, SEEK_CUR);

            tmp[10] = '\0';

            persist_line_list[i][j].line = atoi(tmp);
            persist_line_list[i][j].count = atoi(tmp + 11);
            printf("%d persist_line_list[%d][%d] line: %d, count: %d\n", __LINE__, i, j, persist_line_list[i][j].line, persist_line_list[i][j].count);
        }
        
        if(r == 0)  break;
    }

    free(tmp);
}

void write_persistcountfile(){
    int fd = open("countfile.txt", O_WRONLY | O_TRUNC | O_CREAT, 0666);
    if(fd == -1){
        perror(__func__);
        fprintf(stderr, " %s, %d, %s\n", __FILE__, __LINE__, __func__);
        exit(1);
    }

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

    //void *(*orig_pmem_map_file)(const char*, size_t, int, mode_t, size_t*, int*) = dlsym(RTLD_NEXT, "pmem_map_file");

    PMEMaddrset *addrset = (PMEMaddrset *)malloc(sizeof(PMEMaddrset));
    addrset->orig_addr = orig_pmem_map_file(path, len, flags, mode, mapped_lenp, is_pmemp);
    addrset->fake_addr = aligned_alloc(LIBPMEMMAP_ALIGN_VAL, len);
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

void pmem_wrappersist(const void *addr, size_t len, char* file, int line){
    printf("wrap pmem_wrappersist\n");
    pmem_flush(addr, len);
    pmem_wrapdrain(file, line);
}

void pmem_persist(const void *addr, size_t len){
    printf("wrap pmem_persist\n");
    pmem_flush(addr, len);
    pmem_drain();
}

int pmem_unmap(void *addr, size_t len){
    printf("wrap pmem_unmap\n");
    //int (*orig_pmem_unmap)(void*, size_t) = dlsym(RTLD_NEXT, "pmem_unmap");

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

void rand_memcpy(void *dest, const void *src, size_t n, PMEMaddrset *set){
    int d = src - set->fake_addr;

    int i;
    i = d % CACHE_LINE_SIZE;
    printf("i : %d\n", i);
    if(i % CACHE_LINE_SIZE !=0){
        //printf("first\n");
        if(rand() % 2 == 0){
            memcpy(dest - i, src - i, CACHE_LINE_SIZE);
        }
        i = CACHE_LINE_SIZE - i;
    }

    for (; i < n; i += CACHE_LINE_SIZE){
        if(rand() % 2 == 0){
            
            memcpy(dest + i, src + i, CACHE_LINE_SIZE);
            //printf("%d\n", i);
        }
    }

    i -= CACHE_LINE_SIZE;
    int remainder = n - i; 
    printf("2i : %d, remainder : %d\n", i, remainder);
    if(remainder % CACHE_LINE_SIZE != 0){
        //printf("remainder\n");
        if(rand() % 2 == 0){
            if(d + i + CACHE_LINE_SIZE < set->len)
                memcpy(dest + i, src + i, CACHE_LINE_SIZE);
            else{
                memcpy(dest + i, src + i, remainder);
            }
        }
    }
}

void rand_file_generate(PMEMaddrset *set, size_t n, uintptr_t d){
    char generated_path[MAX_PATH_LENGTH];
    void *new_file;
    int fd;

    for(int i=0;i<6;i++){
        sprintf(generated_path, "%s_%d_%d", set->orig_path, set->persist_count, i);

        fd = open(generated_path, O_CREAT|O_RDWR, 0666);
        ftruncate(fd, set->len);
        new_file = mmap(NULL, set->len, PROT_WRITE, MAP_SHARED, fd, 0);

        memcpy(new_file, set->orig_addr, set->len);
        rand_memcpy(new_file + d, set->fake_addr + d, n, set);

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

void add_waitdrainlist(const void *addr, size_t len){
    PMEMaddrset *tmp = head;
    while(tmp != NULL){
        if((addr >= tmp->fake_addr) && (addr + len <= tmp->fake_addr + tmp->len)){
            break;
        }
        else if((addr >= tmp->fake_addr) && (addr <= tmp->fake_addr + tmp->len) && (addr + len > tmp->fake_addr + tmp->len)){
            fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
            exit(1);
        }
        tmp = tmp->next;
    }

    if(tmp == NULL){
        //fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
        printf("not registered\n");
        orig_pmem_flush(addr, len);
        return;
        //exit(1);
    }

    //flushed = 1;

    Waitdrain_addrset *w_tmp = w_head;
    while(w_tmp != NULL){
        w_tmp = w_tmp->next;
    }

    w_tmp = (Waitdrain_addrset *)malloc(sizeof(Waitdrain_addrset));
    w_tmp->addr = (char *)addr;
    w_tmp->len = len;
    w_tmp->next = NULL;
    w_tmp->prev = w_tail;

    PMEMaddrset *set = head;
    while(set != NULL){
        if((addr >= set->fake_addr) && (addr < set->fake_addr + set->len)){
            w_tmp->set = set;
            break;
        }
        set = set->next;
    }

    if(set == NULL){
        fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
        exit(1);
    }

    if(w_head == NULL){
        w_head = w_tmp;
    }
    else{
        w_tail->next = w_tmp;
    }
    w_tail = w_tmp;
}

void *pmem_wrapmemmove_persist(void *pmemdest, const void *src, size_t len, char* file, int line){
    printf("wrap pmem_wrapmemmove_persist\n");
    void *ret = memmove(pmemdest, src, len);
    pmem_wrappersist(pmemdest, len, file, line);
    return ret;
}

void *pmem_wrapmemcpy_persist(void *pmemdest, const void *src, size_t len, char* file, int line){
    printf("wrap pmem_wrapmemcpy_persist\n");
    void *ret = memcpy(pmemdest, src, len);
    pmem_wrappersist(pmemdest, len, file, line);
    return ret;
}

void *pmem_wrapmemset_persist(void *pmemdest, int c, size_t len, char* file, int line){
    printf("wrap pmem_wrapmemset_persist\n");
    void *ret = memset(pmemdest, c, len);
    pmem_wrappersist(pmemdest, len, file, line);
    return ret;
}

void *pmem_memmove_persist(void *pmemdest, const void *src, size_t len){
    printf("wrap pmem_memmove_persist\n");
    void *ret = memmove(pmemdest, src, len);
    pmem_persist(pmemdest, len);
    return ret;
}

void *pmem_memcpy_persist(void *pmemdest, const void *src, size_t len){
    printf("wrap pmem_memcpy_persist\n");
    void *ret = memcpy(pmemdest, src, len);
    pmem_persist(pmemdest, len);
    return ret;
}

void *pmem_memset_persist(void *pmemdest, int c, size_t len){
    printf("wrap pmem_memset_persist\n");
    void *ret = memset(pmemdest, c, len);
    pmem_persist(pmemdest, len);
    return ret;
}

void *pmem_memmove_nodrain(void *pmemdest, const void *src, size_t len){
    printf("wrap pmem_memmove_nodrain\n");
    void *ret = memmove(pmemdest, src, len);
    add_waitdrainlist(pmemdest, len);
    return ret;
}

void *pmem_memcpy_nodrain(void *pmemdest, const void *src, size_t len){
    printf("wrap pmem_memcpy_nodrain\n");
    void *ret = memcpy(pmemdest, src, len);
    add_waitdrainlist(pmemdest, len);
    return ret;
}

void *pmem_memset_nodrain(void *pmemdest, int c, size_t len){
    printf("wrap pmem_memset_nodrain\n");
    void *ret = memset(pmemdest, c, len);
    add_waitdrainlist(pmemdest, len);
    return ret;
}

void *pmem_wrapmemmove(void *pmemdest, const void *src, size_t len, unsigned flags, char* file, int line){
    printf("wrap pmem_wrapmemmove\n");
    void *ret;
    if((flags & (unsigned int)33U) == 0){
        ret = pmem_wrapmemmove_persist(pmemdest, src, len, file, line);
    }
    else{
        ret = pmem_memmove_nodrain(pmemdest, src, len);
    }
    return ret;
}


void *pmem_wrapmemcpy(void *pmemdest, const void *src, size_t len, unsigned flags, char* file, int line){
    printf("wrap pmem_wrapmemcpy\n");
    void *ret;
    if((flags & (unsigned int)33U) == 0){
        ret = pmem_wrapmemcpy_persist(pmemdest, src, len, file, line);
    }
    else{
        ret = pmem_memcpy_nodrain(pmemdest, src, len);
    }
    return ret;
}

void *pmem_wrapmemset(void *pmemdest, int c, size_t len, unsigned flags, char* file, int line){
    printf("wrap pmem_wrapmemset\n");
    void *ret;
    if((flags & (unsigned int)33U) == 0){
        ret = pmem_wrapmemset_persist(pmemdest, c, len, file, line);
    }
    else{
        ret = pmem_memset_nodrain(pmemdest, c, len);
    }
    return ret;
}

void *pmem_memmove(void *pmemdest, const void *src, size_t len, unsigned flags){
    printf("wrap pmem_memmove\n");
    void *ret;
    if((flags & (unsigned int)33U) == 0){
        ret = pmem_memmove_persist(pmemdest, src, len);
    }
    else{
        ret = pmem_memmove_nodrain(pmemdest, src, len);
    }
    return ret;
}


void *pmem_memcpy(void *pmemdest, const void *src, size_t len, unsigned flags){
    printf("wrap pmem_memcpy\n");
    void *ret;
    if((flags & (unsigned int)33U) == 0){
        ret = pmem_memcpy_persist(pmemdest, src, len);
    }
    else{
        ret = pmem_memcpy_nodrain(pmemdest, src, len);
    }
    return ret;
}

void *pmem_memset(void *pmemdest, int c, size_t len, unsigned flags){
    printf("wrap pmem_memset\n");
    void *ret;
    if((flags & (unsigned int)33U) == 0){
        ret = pmem_memset_persist(pmemdest, c, len);
    }
    else{
        ret = pmem_memset_nodrain(pmemdest, c, len);
    }
    return ret;
}

void pmem_flush(const void *addr, size_t len){
    printf("wrap pmem_flush\n");
    add_waitdrainlist(addr, len);
}

void pmem_wrapdrain(char* file, int line){
    printf("wrap pmem_wrapdrain\n");
    pmem_drain();

    printf("test\n");
    printf("test1 %p, %p\n", file, &line);
    printf("test %s, %d\n", file, line);
    plus_persistcount(file, line);
    printf("pmem_wrapdrain file:%s, line:%d\n", file, line);
}

void pmem_drain(){//waitdrainに入れたものだけをdrain
    printf("wrap pmem_drain\n");

    // if(flushed == 0){
    //     printf("not flushed\n");
    //     flushed = 0;
    //     return;
    // }
    
    //void (*orig_pmem_persist)(const void*, size_t) = dlsym(RTLD_NEXT, "pmem_persist");

    Waitdrain_addrset *w_set = w_head;

    while(w_set != NULL){
        uintptr_t d = w_set->addr - w_set->set->fake_addr;
        void *target_addr = (void *)(w_set->set->orig_addr + d);
        printf("d : %ld\n", d);

        rand_memcpy(target_addr, w_set->set->fake_addr + d, w_set->len, w_set->set);//範囲が重なっているとコピー確率が変わってしまう？

        // unsigned long int nth_power = CACHE_LINE_SIZE;
        // uintptr_t real_len = ((uintptr_t)w_set->addr + w_set->len + (nth_power - 1)) & ~(nth_power - 1) - (uintptr_t)w_set->addr & ~(nth_power - 1); 
        // memcpy(target_addr & ~(nth_power - 1), w_set->addr & ~(nth_power - 1), real_len);

        //memcpyを実際に行う場合使用 行わない場合はコメントアウト 64ビットのビット演算でやったほうがいい
        int tmp = d % CACHE_LINE_SIZE;
        int tmp2 = CACHE_LINE_SIZE - ((w_set->len + tmp) % CACHE_LINE_SIZE);
        memcpy(target_addr - tmp, w_set->addr - tmp, w_set->len + tmp + tmp2);
        //ここまで

        orig_pmem_flush(target_addr, w_set->len);

        Waitdrain_addrset *tmp_w_set = w_set;
        w_set = w_set->next;
        free(tmp_w_set);
    }
    w_head = NULL;
    w_tail = NULL;

    orig_pmem_drain();

    //abort();
    
    return;
}
