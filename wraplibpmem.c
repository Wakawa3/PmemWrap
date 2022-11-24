#include "wraplibpmem.h"

#include <pthread.h>

#define ABORTFLAG_COEFFICIENT 10
#define ABORTFLAG_COEFFICIENT2 50

PMEMaddrset *head = NULL;
PMEMaddrset *tail = NULL;

Waitdrain_addrset *w_head = NULL;
Waitdrain_addrset *w_tail = NULL;

char *file_list[MAX_FILE_LENGTH];
LINEinfo persist_line_list[MAX_FILE_LENGTH][MAX_LINE_LENGTH];

int persist_count_sum = 0;
int persist_place_sum = 0;
int abort_count_sum = 0;

void *(*orig_pmem_map_file)(const char*, size_t, int, mode_t, size_t*, int*);
//void (*orig_pmem_persist)(const void*, size_t);
int (*orig_pmem_unmap)(void*, size_t);
int (*orig_pmem_msync)(const void *addr, size_t len);
void (*orig_pmem_flush)(const void *, size_t);
void (*orig_pmem_drain)();
int (*orig_pmem_deep_drain)(const void *, size_t);

int pmemwrap_abort = 0;
int abortflag = 0;
int memcpyflag = NORMAL_MEMCPY;

int rand_set_count = 0;
int subseed = 0;

pthread_mutex_t mutex;//plus_persistcount rand_set_abortflag add_PMEMaddrset delete_PMEMaddrset
//rand_set_abortflag add_waitdrainlist pmem_drain

__attribute__ ((constructor))
static void constructor () {
    char* seedenv = getenv("PMEMWRAP_SEED");
    if(seedenv != NULL)
        subseed = atoi(seedenv) * 1000000;

    memset(file_list, 0, sizeof(char *) * MAX_FILE_LENGTH);
    memset(persist_line_list, 0, sizeof(LINEinfo) * MAX_FILE_LENGTH * MAX_LINE_LENGTH);
    read_persistcountfile();

    pthread_mutex_init(&mutex, NULL);

    char *tmp = getenv("PMEMWRAP_ABORT");
    if(tmp != NULL && strcmp(tmp, "1") == 0){
        pmemwrap_abort = 1;
    }

    char* memcpyflag_env = getenv("PMEMWRAP_MEMCPY");
    if(memcpyflag_env == NULL || strcmp(memcpyflag_env, "NORMAL_MEMCPY") == 0){
        memcpyflag = NORMAL_MEMCPY;
    }
    else if(strcmp(memcpyflag_env, "RAND_MEMCPY") == 0){
        memcpyflag = RAND_MEMCPY;
    }
    else if(strcmp(memcpyflag_env, "NO_MEMCPY") == 0){
        memcpyflag = NO_MEMCPY;
    }
    else{
        memcpyflag = NORMAL_MEMCPY;
    }

    void *dlopen_val = dlopen("/home/satoshi/testlib/lib/libpmem.so.1", RTLD_NOW);

    if((orig_pmem_map_file = dlsym(dlopen_val, "pmem_map_file")) == NULL){
        fprintf(stderr, "orig_pmem_map_file: %p\n%s\n", orig_pmem_map_file, dlerror());
        exit(1);
    }

    if((orig_pmem_unmap = dlsym(dlopen_val, "pmem_unmap")) == NULL){
        fprintf(stderr, "orig_pmem_unmap: %p\n%s\n", orig_pmem_unmap, dlerror());
        exit(1);
    }

    if((orig_pmem_msync = dlsym(dlopen_val, "pmem_msync")) == NULL){
        fprintf(stderr, "orig_pmem_msync: %p\n%s\n", orig_pmem_msync, dlerror());
        exit(1);
    }

    if((orig_pmem_flush = dlsym(dlopen_val, "pmem_flush")) == NULL){
        fprintf(stderr, "orig_pmem_flush: %p\n%s\n", orig_pmem_flush, dlerror());
        exit(1);
    }

    if((orig_pmem_drain = dlsym(dlopen_val, "pmem_drain")) == NULL){
        fprintf(stderr, "orig_pmem_drain: %p\n%s\n", orig_pmem_drain, dlerror());
        exit(1);
    }

    if((orig_pmem_deep_drain = dlsym(dlopen_val, "pmem_deep_drain")) == NULL){
        fprintf(stderr, "orig_pmem_deep_drain: %p\n%s\n", orig_pmem_deep_drain, dlerror());
        exit(1);
    }
}

void plus_persistcount(const char *file, int line){
    pthread_mutex_lock(&mutex);

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
        fprintf(stderr, "plus_persistcount error, %s, %d\n", __FILE__, __LINE__);
        exit(1);
    }

    if(matched == 0){
        file_list[file_id] = (char *)malloc(strlen(file) + 1);
        if(file_list[file_id] == NULL){
            perror(__func__);
            fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
            exit(1);
        }

        strcpy(file_list[file_id], file);
        persist_line_list[file_id][0].line = line;
        persist_line_list[file_id][0].count = 1;
        persist_line_list[file_id][0].prev_count = 0;
        persist_line_list[file_id][0].abort_count = 0;
        pthread_mutex_unlock(&mutex);
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

    pthread_mutex_unlock(&mutex);
}

void read_persistcountfile(){
    // printf("read_persistcountfile\n");
    int fd = open("countfile.txt", O_RDONLY);
    if(fd == -1){
        printf("countfile.txt doesn't exist.\n");
        // perror(__func__);
        // fprintf(stderr, " %s, %d, %s\n", __FILE__, __LINE__, __func__);
        // exit(1);
        return;
    }

    if(lseek(fd, 0, SEEK_END) == 0){
            return;
    }
    lseek(fd, 0, SEEK_SET);
    
    int file_id;
    int file_name_len;
    char *tmp = (char *)malloc(MAX_FILE_LENGTH + 1);
    if(tmp == NULL){
        perror(__func__);
        fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
        exit(1);
    }

    int r, offset;

    for(int i=0;;i++){
        offset = lseek(fd, 0, SEEK_CUR);
        //printf("%d offset: %d\n", __LINE__, offset);
        pread(fd, tmp, MAX_FILE_LENGTH, offset);
        char *new_line_ptr = strchr(tmp, '\n');
        tmp[new_line_ptr - tmp] = '\0'; //get file name
        offset = lseek(fd, new_line_ptr - tmp + 1, SEEK_CUR);
        //printf("%d tmp: %s\n", __LINE__, tmp);

        file_list[i] = malloc(strlen(tmp + 1) + 1);//skip '_'
        if(file_list[i] == NULL){
            perror(__func__);
            fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
            exit(1);
        }

        strcpy(file_list[i], tmp + 1);
        // printf("%d file_list[%d]: %s\n", __LINE__, i, file_list[i]);

        for (int j=0;;j++){
            r = pread(fd, tmp, 32, offset); // int digit + _ + int digit + _ + int digit
            if((r == 0) || (tmp[0] == '_')){
                break;
            }
            tmp[32] = '\0';
            // printf("%d tmp: %s, r: %d\n", __LINE__, tmp, r);
            offset = lseek(fd, 33, SEEK_CUR);

            tmp[10] = '\0';

            tmp[21] = '\0';

            persist_line_list[i][j].line = atoi(tmp);
            persist_line_list[i][j].count = 0;
            persist_line_list[i][j].prev_count = atoi(tmp + 11);
            persist_line_list[i][j].abort_count = atoi(tmp + 22);
            persist_count_sum += persist_line_list[i][j].prev_count;
            abort_count_sum += persist_line_list[i][j].abort_count;
            if(persist_line_list[i][j].prev_count != 0) persist_place_sum++;
            // printf("%d persist_line_list[%d][%d] line: %d, count: %d, prev_count: %d\n", __LINE__, i, j, persist_line_list[i][j].line, persist_line_list[i][j].count, persist_line_list[i][j].prev_count);
        }
        
        if(r == 0)  break;
    }

    free(tmp);
}

void write_persistcountfile(){
    char *env = getenv("PMEMWRAP_WRITECOUNTFILE");
    if(env != NULL && strcmp(env, "NO") == 0){
        //fprintf(stderr, "pmemwrap_writecountfile == 0\n");
        pthread_mutex_unlock(&mutex);
        return;
    }

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
        if(tmp == NULL){
            perror(__func__);
            fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
            exit(1);
        }

        sprintf(tmp, "_%s\n", file_list[file_id]);
        if(write(fd, tmp, file_name_len + 2) != file_name_len + 2){
            fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
            exit(1);
        }
        free(tmp);

        for(int i=0;(i<MAX_LINE_LENGTH) && (persist_line_list[file_id][i].line != 0); i++){
            tmp = (char *)malloc(34); //int digit(10) + _ + int digit(10) + _ + int digit(10) + \n + \0
            if(tmp == NULL){
                perror(__func__);
                fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
                exit(1);
            }

            if(env != NULL && strcmp(env, "ADD") == 0){
                int larger_count;
                if(persist_line_list[file_id][i].count > persist_line_list[file_id][i].prev_count)
                    larger_count = persist_line_list[file_id][i].count;
                else
                    larger_count = persist_line_list[file_id][i].prev_count;
                sprintf(tmp, "%010d_%010d_%010d\n", persist_line_list[file_id][i].line, larger_count, persist_line_list[file_id][i].abort_count);
            }
            else{
                sprintf(tmp, "%010d_%010d_%010d\n", persist_line_list[file_id][i].line, persist_line_list[file_id][i].count, 0);
            }
            if(write(fd, tmp, 33) != 33){
                fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
                exit(1);
            }
            free(tmp);
        }
        //printf("write_persistcountfile file_id: %d\n", file_id);
    }

    close(fd);
}

// void reset_persistcount(){
//     for(int i = 0; i<MAX_FILE_LENGTH && file_list[i] != NULL; i++){
//         for(int j = 0; j<MAX_LINE_LENGTH; j++){
//             persist_line_list[i][j].count = 0;
//         }
//     }
// }



void rand_set_abortflag(const char *file, int line){
    //printf("wrap rand_set_abortflag\n");
    if(pmemwrap_abort == 0){
        //fprintf(stderr, "pmemwrap_abort == 0\n");
        //abortflag = 0;
        return;
    }

    pthread_mutex_lock(&mutex);

    if(abortflag == 1){
        pthread_mutex_unlock(&mutex);
        return;
    }

    srand((unsigned int)time(NULL) + subseed + rand_set_count);
    rand_set_count++;
    int file_id;

    char *tmp;
    for(file_id=0; file_id<MAX_FILE_LENGTH && file_list[file_id] != NULL; file_id++){
        if(strcmp(file, file_list[file_id]) != 0){
            continue;
        }
        for(int i=0;(i<MAX_LINE_LENGTH) && (persist_line_list[file_id][i].line != 0); i++){
            if(persist_line_list[file_id][i].line == line){
                if(persist_line_list[file_id][i].prev_count == 0){
                    abortflag = 0;
                    pthread_mutex_unlock(&mutex);
                    return;
                }
                //ランダムにabortflagをセット
                int rand_num = rand();
                double probability = (double)ABORTFLAG_COEFFICIENT * 1 / ((double)persist_line_list[file_id][i].prev_count * (double)persist_place_sum);
                if (abort_count_sum != 0){
                    for(int j=0; j<persist_line_list[file_id][i].abort_count; j++)
                        probability *= 1 / (double)ABORTFLAG_COEFFICIENT2;
                }
                    
                //fprintf(stderr, "1, %f 2, %f 3, %f\n", probability, (double)ABORTFLAG_COEFFICIENT * 1 / ((double)persist_line_list[file_id][i].prev_count * (double)persist_place_sum), (double)persist_line_list[file_id][i].abort_count / (double)abort_count_sum);
                double rand_number = (double)rand_num / RAND_MAX;
                // printf("probability: %lf, rand_number%lf\n", probability, rand_number);
                if(rand_number < probability){
                    //正しい挙動
                    fprintf(stderr, "set abortflag file: %s, line: %d, rand_set_count: %d, rand_num: %d\n", file, line, rand_set_count, rand_num);
                    //
                    persist_line_list[file_id][i].abort_count++;
                    abortflag = 1;
                    pthread_mutex_unlock(&mutex);
                    return;
                }
                else{
                    abortflag = 0;
                        //printf("end rand_set_abortflag\n");
                    pthread_mutex_unlock(&mutex);
                    return;
                }
            }
        }
    }

    abortflag = 0;
    //printf("end rand_set_abortflag\n");

    pthread_mutex_unlock(&mutex);
    return;
}

void add_PMEMaddrset(void *orig_addr, size_t len, const char *path, int file_type){
    if(memcpyflag == NO_MEMCPY){
        return;
    }

    pthread_mutex_lock(&mutex);

    PMEMaddrset *addrset = (PMEMaddrset *)malloc(sizeof(PMEMaddrset));
    if(addrset == NULL){
        perror(__func__);
        fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
        exit(1);
    }

    addrset->orig_addr = orig_addr;

    addrset->flushed_copyfile_path = malloc(MAX_PATH_LENGTH);
    sprintf(addrset->flushed_copyfile_path, "%s%s", path, COPYFILE_WORDENDING);
    addrset->flushed_copyfile_fd = open(addrset->flushed_copyfile_path, (O_RDWR | O_CREAT), 0666);
    fallocate(addrset->flushed_copyfile_fd, 0, 0, len);
    addrset->fake_addr = mmap(NULL, len, PROT_WRITE, MAP_SHARED, addrset->flushed_copyfile_fd, 0);

    //addrset->fake_addr = aligned_alloc(LIBPMEMMAP_ALIGN_VAL, len);

    if(addrset->fake_addr == NULL){
        perror(__func__);
        fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
        exit(1);
    }

    addrset->next = NULL;
    addrset->prev = tail;
    addrset->len = len;
    addrset->file_type = file_type;
    if(memcpyflag != NO_MEMCPY)
        memcpy(addrset->fake_addr, addrset->orig_addr, len);

    if(head == NULL){
        head = addrset;
    }
    else{
        tail->next = addrset;
    }
    tail = addrset;

    pthread_mutex_unlock(&mutex);

    return;
}

// void *pmem_wrap_map_file(const char *path, size_t len, int flags, mode_t mode, size_t *mapped_lenp, int *is_pmemp, const char *file, int line){
//     // printf("wrap pmem_wrap_map_file\n");
    
//     PMEMaddrset *addrset = add_PMEMaddrset(orig_pmem_map_file(path, len, flags, mode, mapped_lenp, is_pmemp), len, PMEM_FILE);

//     // plus_persistcount(file, line);
//     // rand_set_abortflag(file, line);

//     return addrset->orig_addr;
// }

void *pmem_map_file(const char *path, size_t len, int flags, mode_t mode, size_t *mapped_lenp, int *is_pmemp){
    // printf("wrap pmem_map_file\n");

    void *addr = orig_pmem_map_file(path, len, flags, mode, mapped_lenp, is_pmemp);
    
    add_PMEMaddrset(addr, len, path, PMEM_FILE);

    return addr;
}

void pmem_wrap_persist(const void *addr, size_t len, const char *file, int line){
    //printf("wrap pmem_wrappersist\n");

    pmem_flush(addr, len);
    pmem_wrap_drain(file, line);
}

void pmem_persist(const void *addr, size_t len){
    // printf("wrap pmem_persist\n");
    pmem_flush(addr, len);
    pmem_drain();
}

int pmem_wrap_msync(const void *addr, size_t len, const char *file, int line){
    pmem_flush(addr, len);
    plus_persistcount(file, line);
    rand_set_abortflag(file, line);
    return orig_pmem_msync(addr, len);
}

void delete_PMEMaddrset(void *addr){
    pthread_mutex_lock(&mutex);
    PMEMaddrset *set = head;

    while(set != NULL){
        if(set->orig_addr == addr){//領域が同じ場合は？
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
            munmap(set->fake_addr, set->len);
            close(set->flushed_copyfile_fd);
            //remove(set->flushed_copyfile_path);
            free(set->flushed_copyfile_path);
            //free(set->fake_addr);
            free(set);
            pthread_mutex_unlock(&mutex);
            return;
        }
        set = set->next;
    }

    pthread_mutex_unlock(&mutex);
    return;
}

int pmem_unmap(void *addr, size_t len){
    // printf("wrap pmem_unmap\n");
    //int (*orig_pmem_unmap)(void*, size_t) = dlsym(RTLD_NEXT, "pmem_unmap");

    delete_PMEMaddrset(addr);

    return orig_pmem_unmap(addr, len);
}

void rand_memcpy(PMEMaddrset *set){
    size_t i = 0;
    for(; i < set->len; i += CACHE_LINE_SIZE){
        if(rand() % 2 == 0){
            memcpy(set->orig_addr + i, set->fake_addr + i, CACHE_LINE_SIZE);
        }
    }

    i -= CACHE_LINE_SIZE;
    size_t remainder = set->len - i;
    if(remainder % CACHE_LINE_SIZE != 0){
        //printf("remainder\n");
        if(rand() % 2 == 0){
            memcpy(set->orig_addr + i, set->fake_addr + i, remainder);
        }
    }

    // int d = src - set->fake_addr;

    // int i;
    // i = d % CACHE_LINE_SIZE;
    // printf("i : %d\n", i);
    // if(i % CACHE_LINE_SIZE !=0){
    //     //printf("first\n");
    //     if(rand() % 2 == 0){
    //         memcpy(dest - i, src - i, CACHE_LINE_SIZE);
    //     }
    //     i = CACHE_LINE_SIZE - i;
    // }

    // for (; i < n; i += CACHE_LINE_SIZE){
    //     if(rand() % 2 == 0){
            
    //         memcpy(dest + i, src + i, CACHE_LINE_SIZE);
    //         //printf("%d\n", i);
    //     }
    // }

    // i -= CACHE_LINE_SIZE;
    // int remainder = n - i; 
    // printf("2i : %d, remainder : %d\n", i, remainder);
    // if(remainder % CACHE_LINE_SIZE != 0){
    //     //printf("remainder\n");
    //     if(rand() % 2 == 0){
    //         if(d + i + CACHE_LINE_SIZE < set->len)
    //             memcpy(dest + i, src + i, CACHE_LINE_SIZE);
    //         else{
    //             memcpy(dest + i, src + i, remainder);
    //         }
    //     }
    // }
}

// void rand_file_generate(PMEMaddrset *set, size_t n, uintptr_t d){
//     char generated_path[MAX_PATH_LENGTH];
//     void *new_file;
//     int fd;

//     for(int i=0;i<6;i++){
//         sprintf(generated_path, "%s_%d_%d", set->orig_path, set->persist_count, i);

//         fd = open(generated_path, O_CREAT|O_RDWR, 0666);
//         ftruncate(fd, set->len);
//         new_file = mmap(NULL, set->len, PROT_WRITE, MAP_SHARED, fd, 0);

//         memcpy(new_file, set->orig_addr, set->len);
//         rand_memcpy(new_file + d, set->fake_addr + d, n, set);

//         munmap(new_file, set->len);
//         close(fd);
//     }

//     set->persist_count++;
// }

__attribute__ ((destructor))
static void destructor () {
    printf("destructor\n");
    PMEMaddrset *set = head;

    while(set != NULL){
        //printf("%c\n", *(char*)set->fake_addr);
        //munmap(set->fake_addr, set->len); //destructorでmunmapすると処理が止まる？
        close(set->flushed_copyfile_fd);
    }

    pthread_mutex_destroy(&mutex);

    write_persistcountfile();
}

void add_waitdrainlist(const void *addr, size_t len){
    // printf("pmem_flush_not_NULL2\n");
    pthread_mutex_lock(&mutex);

    // printf("pmem_flush_not_NULL1\n");
    PMEMaddrset *set = head;
    while(set != NULL){
        if((addr >= set->orig_addr) && (addr + len <= set->orig_addr + set->len)){
            break;
        }
        else if((addr >= set->orig_addr) && (addr <= set->orig_addr + set->len) && (addr + len > set->orig_addr + set->len)){
            fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
            exit(1);
        }
        set = set->next;
    }

    if(set == NULL){
        //fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
        // printf("not registered\n");
        orig_pmem_flush(addr, len);

        pthread_mutex_unlock(&mutex);

        // printf("pmem_flush_NULL\n");
        return;
        //exit(1);
    }
    // printf("pmem_flush_not_NULL\n");
    //flushed = 1;

    Waitdrain_addrset *w_set = w_head;
    while(w_set != NULL){
        w_set = w_set->next;
    }

    w_set = (Waitdrain_addrset *)malloc(sizeof(Waitdrain_addrset));
    if(w_set == NULL){
        perror(__func__);
        fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
        exit(1);
    }

    w_set->addr = (char *)addr;
    w_set->len = len;
    w_set->next = NULL;
    w_set->prev = w_tail;
    w_set->set = set;

    if(set == NULL){
        fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
        exit(1);
    }

    if(w_head == NULL){
        w_head = w_set;
    }
    else{
        w_tail->next = w_set;
    }
    w_tail = w_set;

    pthread_mutex_unlock(&mutex);
}

void *pmem_wrap_memmove_persist(void *pmemdest, const void *src, size_t len, const char *file, int line){
    // printf("wrap pmem_wrap_memmove_persist\n");
    void *ret = memmove(pmemdest, src, len);
    pmem_wrap_persist(pmemdest, len, file, line);
    return ret;
}

void *pmem_wrap_memcpy_persist(void *pmemdest, const void *src, size_t len, const char *file, int line){
    // printf("wrap pmem_wrap_memcpy_persist\n");
    void *ret = memcpy(pmemdest, src, len);
    pmem_wrap_persist(pmemdest, len, file, line);
    return ret;
}

void *pmem_wrap_memset_persist(void *pmemdest, int c, size_t len, const char *file, int line){
    // printf("wrap pmem_wrap_memset_persist\n");
    void *ret = memset(pmemdest, c, len);
    pmem_wrap_persist(pmemdest, len, file, line);
    return ret;
}

void *pmem_memmove_persist(void *pmemdest, const void *src, size_t len){
    // printf("wrap pmem_memmove_persist\n");
    void *ret = memmove(pmemdest, src, len);
    pmem_persist(pmemdest, len);
    return ret;
}

void *pmem_memcpy_persist(void *pmemdest, const void *src, size_t len){
    // printf("wrap pmem_memcpy_persist\n");
    void *ret = memcpy(pmemdest, src, len);
    pmem_persist(pmemdest, len);
    return ret;
}

void *pmem_memset_persist(void *pmemdest, int c, size_t len){
    // printf("wrap pmem_memset_persist\n");
    void *ret = memset(pmemdest, c, len);
    pmem_persist(pmemdest, len);
    return ret;
}

void *pmem_memmove_nodrain(void *pmemdest, const void *src, size_t len){
    // printf("wrap pmem_memmove_nodrain\n");
    void *ret = memmove(pmemdest, src, len);
    pmem_flush(pmemdest, len);
    return ret;
}

void *pmem_memcpy_nodrain(void *pmemdest, const void *src, size_t len){
    // printf("wrap pmem_memcpy_nodrain\n");
    void *ret = memcpy(pmemdest, src, len);
    pmem_flush(pmemdest, len);
    return ret;
}

void *pmem_memset_nodrain(void *pmemdest, int c, size_t len){
    // printf("wrap pmem_memset_nodrain\n");
    void *ret = memset(pmemdest, c, len);
    pmem_flush(pmemdest, len);
    return ret;
}

void *pmem_wrap_memmove(void *pmemdest, const void *src, size_t len, unsigned flags, const char *file, int line){
    // printf("wrap pmem_wrap_memmove\n");
    void *ret;
    if((flags & (unsigned int)33U) == 0){
        ret = pmem_wrap_memmove_persist(pmemdest, src, len, file, line);
    }
    else{
        ret = pmem_memmove_nodrain(pmemdest, src, len);
    }
    return ret;
}

void *pmem_wrap_memcpy(void *pmemdest, const void *src, size_t len, unsigned flags, const char *file, int line){
    // printf("wrap pmem_wrap_memcpy\n");
    void *ret;
    if((flags & (unsigned int)33U) == 0){
        ret = pmem_wrap_memcpy_persist(pmemdest, src, len, file, line);
    }
    else{
        ret = pmem_memcpy_nodrain(pmemdest, src, len);
    }
    return ret;
}

void *pmem_wrap_memset(void *pmemdest, int c, size_t len, unsigned flags, const char *file, int line){
    // printf("wrap pmem_wrap_memset\n");
    void *ret;
    if((flags & (unsigned int)33U) == 0){
        ret = pmem_wrap_memset_persist(pmemdest, c, len, file, line);
    }
    else{
        ret = pmem_memset_nodrain(pmemdest, c, len);
    }
    return ret;
}

void *pmem_memmove(void *pmemdest, const void *src, size_t len, unsigned flags){
    // printf("wrap pmem_memmove\n");
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
    // printf("wrap pmem_memcpy\n");
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
    // printf("wrap pmem_memset\n");
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
    //printf("wrap pmem_flush  addr: %p, len: %lu\n", addr, len);

    if(memcpyflag == NO_MEMCPY)
        return;
    
    add_waitdrainlist(addr, len);
}

void pmem_wrap_drain(const char *file, int line){
    // printf("wrap pmem_wrap_drain\n");
    plus_persistcount(file, line);
    rand_set_abortflag(file, line);

    pmem_drain();
    
    // printf("pmem_wrap_drain file:%s, line:%d\n", file, line);
}

void pmem_drain(){//waitdrainに入れたものだけをdrain
    //printf("wrap pmem_drain\n");
    pthread_mutex_lock(&mutex);

    Waitdrain_addrset *w_set = w_head;

    if (abortflag == 0){
        while(w_set != NULL){
            uintptr_t d = w_set->addr - w_set->set->orig_addr;
            void *target_addr = (void *)(w_set->set->fake_addr + d);// fake_addr
            // printf("d : %ld, 0x%lx\n", d, d);

            // unsigned long int nth_power = CACHE_LINE_SIZE;
            // uintptr_t real_len = ((uintptr_t)w_set->addr + w_set->len + (nth_power - 1)) & ~(nth_power - 1) - (uintptr_t)w_set->addr & ~(nth_power - 1); 
            // memcpy(target_addr & ~(nth_power - 1), w_set->addr & ~(nth_power - 1), real_len);

            //rand_memcpyを適用させておく場合はコメントアウト 64ビットのビット演算でやったほうがいい
            size_t tmp = d % CACHE_LINE_SIZE;
            size_t tmp2 = (CACHE_LINE_SIZE - ((w_set->len + tmp) % CACHE_LINE_SIZE)) % 64;
            size_t copylen = w_set->len + tmp + tmp2;
            size_t offset = 0;
            while(copylen > __INT_MAX__){
                memcpy(target_addr - tmp + offset, w_set->addr - tmp + offset, __INT_MAX__);
                copylen -= __INT_MAX__;
                offset += __INT_MAX__;
            }
            memcpy(target_addr - tmp + offset, w_set->addr - tmp + offset, copylen); // fake_addr < orig_addr
            //printf("w_set->addr: %p, target_addr: %p, target_addr - tmp: %p\n", w_set->addr, target_addr, target_addr - tmp);

            // void *from_orig = (void*)((size_t)w_set->addr / 64 * 64);
            // void *to_orig = (void*)((size_t)(w_set->addr + w_set->len + 63) / 64 * 64);
            // void *from_fake = (void*)((size_t)target_addr / 64 * 64);

            // printf("from_orig: %p, to_orig: %p, from_fake:")
            
            // size_t copylen = to_orig - from_orig;
            // size_t offset = 0;
            // while(copylen > __INT_MAX__){
            //     memcpy(from_fake + offset, from_orig + offset, __INT_MAX__);
            //     copylen -= __INT_MAX__;
            //     offset += __INT_MAX__;
            // }
            // memcpy(from_fake + offset, from_orig + offset, copylen); // fake_addr < orig_addr
            //ここまで

            orig_pmem_flush(w_set->addr, w_set->len);

            Waitdrain_addrset *tmp_w_set = w_set;
            w_set = w_set->next;
            free(tmp_w_set);
        }
        w_head = NULL;
        w_tail = NULL;

        orig_pmem_drain();
    }
    else{//abortflag == 1
        PMEMaddrset *set = head;
        while(set != NULL){
            char* multi_env = getenv("PMEMWRAP_MULTITHREAD");
            if(multi_env != NULL && strcmp(multi_env, "SINGLE") == 0){
                fprintf(stderr, "singlethread_memcpy\n");
                if(memcpyflag == NORMAL_MEMCPY){
                    size_t copylen = set->len;
                    size_t offset = 0;
                    while(copylen > __INT_MAX__){
                        memcpy(set->orig_addr + offset, set->fake_addr + offset, __INT_MAX__);
                        copylen -= __INT_MAX__;
                        offset += __INT_MAX__;
                    }
                    memcpy(set->orig_addr + offset, set->fake_addr + offset, copylen);
                }
                else if(memcpyflag == RAND_MEMCPY){
                    rand_memcpy(set);
                }
            }
            munmap(set->fake_addr, set->len);
            close(set->flushed_copyfile_fd);

            orig_pmem_flush(set->orig_addr, set->len);
            orig_pmem_drain();
            set = set->next;
        }

        write_persistcountfile();
        abort();
    }
    
    pthread_mutex_unlock(&mutex);
    return;
}

int pmem_deep_persist(const void *addr, size_t len){
    // printf("wrap pmem_deep_persist\n");
    pmem_flush(addr, len);
    return pmem_deep_drain(addr, len);

    // fprintf(stderr, "pmem_deep_persist is used: , %s, %d\n", __FILE__, __LINE__);
    // exit(1);
    // return -1;
}

void pmem_deep_flush(const void *addr, size_t len){
    // printf("wrap pmem_deep_flush\n");
    pmem_flush(addr, len);
}

int pmem_deep_drain(const void *addr, size_t len){
    // printf("wrap pmem_deep_drain\n");
    pmem_drain();
    return orig_pmem_deep_drain(addr, len);

    // fprintf(stderr, "pmem_deep_drain is used: , %s, %d\n", __FILE__, __LINE__);
    // exit(1);
    // return -1;
}



void force_abort_drain(const char *file, int line){
    abortflag = 1;
    //正しい挙動
    fprintf(stderr, "FORCE set abortflag file: %s, line: %d\n", file, line);
    //
    pmem_drain();
}

void pmemwrap_copy(){
    memcpy(head->fake_addr, head->orig_addr, head->len);
}