#include "wraplibpmem.h"

#define ABORTFLAG_COEFFICIENT 10
#define ABORTFLAG_COEFFICIENT2 100

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

void *(*orig_pmem_memmove_persist)(void *pmemdest, const void *src, size_t len);
void *(*orig_pmem_memcpy_persist)(void *pmemdest, const void *src, size_t len);
void *(*orig_pmem_memset_persist)(void *pmemdest, int c, size_t len);
void *(*orig_pmem_memmove)(void *pmemdest, const void *src, size_t len, unsigned flags);
void *(*orig_pmem_memcpy)(void *pmemdest, const void *src, size_t len, unsigned flags);
void *(*orig_pmem_memset)(void *pmemdest, int c, size_t len, unsigned flags);

int pmemwrap_abort = 0;
int abortflag = 0;
int memcpyflag = NORMAL_MEMCPY;
int abort_count_minus = 0;
int abort_through = 0;

int rand_set_count = 0;
int subseed = 0;

pthread_mutex_t mutex;//plus_persistcount rand_set_abortflag add_PMEMaddrset delete_PMEMaddrset
//rand_set_abortflag add_waitdrainlist pmem_drain

__attribute__ ((constructor))
static void constructor () {
    char* seedenv = getenv("PMEMWRAP_SEED");
    char* abortcount_loop_env = getenv("PMEMWRAP_ABORTCOUNT_LOOP");
    if(seedenv != NULL){
        int seednum = atoi(seedenv);
        subseed = seednum * 1000000;
        if(abortcount_loop_env != NULL)
            abort_count_minus = seednum / atoi(abortcount_loop_env);
        else
            abort_count_minus = seednum / 50;
    }

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

    char *solib_path = getenv("PMEMWRAP_SOLIB_PATH");
    char sofile_path[1024];
    sprintf(sofile_path, "%s%s", solib_path, "/libpmem.so.1");
    void *dlopen_val = dlopen(sofile_path, RTLD_NOW);

    // void *dlopen_val = dlopen("/home/satoshi/testlib/lib/libpmem.so.1", RTLD_NOW);
    // void *dlopen_val = dlopen("/home/satoshi/safepm/build/pmdk/install/lib/pmdk_debug/libpmem.so.1", RTLD_NOW);

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

    if((orig_pmem_memmove_persist = dlsym(dlopen_val, "pmem_memmove_persist")) == NULL){
        fprintf(stderr, "orig_pmem_memmove_persist: %p\n%s\n", orig_pmem_memmove_persist, dlerror());
        exit(1);
    }

    if((orig_pmem_memcpy_persist = dlsym(dlopen_val, "pmem_memcpy_persist")) == NULL){
        fprintf(stderr, "orig_pmem_memcpy_persist: %p\n%s\n", orig_pmem_memcpy_persist, dlerror());
        exit(1);
    }

    if((orig_pmem_memset_persist = dlsym(dlopen_val, "pmem_memset_persist")) == NULL){
        fprintf(stderr, "orig_pmem_memset_persist: %p\n%s\n", orig_pmem_memset_persist, dlerror());
        exit(1);
    }

    if((orig_pmem_memmove = dlsym(dlopen_val, "pmem_memmove")) == NULL){
        fprintf(stderr, "orig_pmem_memmove: %p\n%s\n", orig_pmem_memmove, dlerror());
        exit(1);
    }

    if((orig_pmem_memcpy = dlsym(dlopen_val, "pmem_memcpy")) == NULL){
        fprintf(stderr, "orig_pmem_memcpy: %p\n%s\n", orig_pmem_memcpy, dlerror());
        exit(1);
    }

    if((orig_pmem_memset = dlsym(dlopen_val, "pmem_memset")) == NULL){
        fprintf(stderr, "orig_pmem_memset: %p\n%s\n", orig_pmem_memset, dlerror());
        exit(1);
    }
}

Backtraces_info *plus_persistcount(const char *file, int line, char *bt){
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

    if(matched == 0){// ファイル名がマッチしなかったとき
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

        persist_line_list[file_id][0].binfo = (Backtraces_info *)malloc(sizeof(Backtraces_info));
        persist_line_list[file_id][0].binfo->count = 1;
        persist_line_list[file_id][0].binfo->prev_count = 0;
        persist_line_list[file_id][0].binfo->abort_count = 0;
        persist_line_list[file_id][0].binfo->next = NULL;
        strcpy(persist_line_list[file_id][0].binfo->backtrace, bt);

        pthread_mutex_unlock(&mutex);
        return persist_line_list[file_id][0].binfo;
    }

    Backtraces_info *binfo_now = NULL;

    for(int i=0; i<MAX_LINE_LENGTH; i++){
        if((line == persist_line_list[file_id][i].line) //　すでにlineがあるとき
                || (persist_line_list[file_id][i].line == 0)){ // lineがないとき(初期値が0)
            persist_line_list[file_id][i].line = line;
            persist_line_list[file_id][i].count++;

            if(persist_line_list[file_id][i].binfo == NULL){
                persist_line_list[file_id][i].binfo = (Backtraces_info *)malloc(sizeof(Backtraces_info));
                persist_line_list[file_id][i].binfo->count = 1;
                persist_line_list[file_id][i].binfo->prev_count = 0;
                persist_line_list[file_id][i].binfo->abort_count = 0;
                persist_line_list[file_id][i].binfo->next = NULL;
                                
                strcpy(persist_line_list[file_id][i].binfo->backtrace, bt);

                binfo_now = persist_line_list[file_id][i].binfo;
            }
            else{
                binfo_now = persist_line_list[file_id][i].binfo;
                while(strcmp(bt, binfo_now->backtrace) != 0){
                    if(binfo_now->next == NULL){
                        binfo_now->next = (Backtraces_info *)malloc(sizeof(Backtraces_info));
                        binfo_now = binfo_now->next;
                        strcpy(binfo_now->backtrace, bt);
                        binfo_now->count = 0;
                        binfo_now->prev_count = 0;
                        binfo_now->abort_count = 0;
                        binfo_now->next = NULL;
                        break;
                    }
                    binfo_now = binfo_now->next;
                }
                binfo_now->count++;
            }
            break;
        }
    }

    pthread_mutex_unlock(&mutex);

    if(binfo_now == NULL){
        perror(__func__);
        fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
        exit(1);
    }

    return binfo_now;
}

void read_persistcountfile(){
    int fd = open("countfile_plus.txt", O_RDONLY);
    if(fd == -1){
        printf("countfile.txt doesn't exist.\n");
        return;
    }
    
    long file_size = lseek(fd, 0, SEEK_END);
    if(file_size == 0){
            return;
    }
    lseek(fd, 0, SEEK_SET);

    char *tmp = (char *)malloc(file_size + 1);
    if(tmp == NULL){
        perror(__func__);
        fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
        exit(1);
    }

    read(fd, tmp, file_size);
    char *now_reading = tmp;

    for(int i=0;;i++){
        char *fstart = now_reading + 1;
        char *fend = strchr(now_reading, '\n');
        *fend = '\0';

        file_list[i] = (char*)malloc(fend - fstart + 2);
        if(file_list[i] == NULL){
            perror(__func__);
            fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
            exit(1);
        }
        strcpy(file_list[i], fstart);

        now_reading = fend + 1;
        for (int j=0;;j++){
            if((now_reading >= tmp + file_size) || (now_reading[0] == '_')){
                break;
            }

            now_reading[10] = '\0';
            now_reading[21] = '\0';
            now_reading[32] = '\0';

            persist_line_list[i][j].line = atoi(now_reading);
            persist_line_list[i][j].count = 0;
            persist_line_list[i][j].prev_count = atoi(now_reading + 11);
            persist_line_list[i][j].abort_count = atoi(now_reading + 22);
            persist_count_sum += persist_line_list[i][j].prev_count;
            if(persist_line_list[i][j].prev_count != 0) persist_place_sum++;

            now_reading = now_reading + 33;

            persist_line_list[i][j].binfo = NULL;
            Backtraces_info **binfo_now = &(persist_line_list[i][j].binfo);
            while((now_reading < tmp + file_size) && (now_reading[0] == '+')){
                *binfo_now = (Backtraces_info *)malloc(sizeof(Backtraces_info));
                now_reading += 7;
                now_reading[10] = '\0';
                now_reading[21] = '\0';

                (*binfo_now)->count = 0;
                (*binfo_now)->prev_count = atoi(now_reading);
                (*binfo_now)->abort_count = atoi(now_reading + 11);
                (*binfo_now)->next = NULL;
                abort_count_sum += (*binfo_now)->abort_count;
                now_reading += 22;

                char *bend = strchr(now_reading, ';');
                *bend = '\0';
                strcpy((*binfo_now)->backtrace, now_reading);
                binfo_now = &((*binfo_now)->next);
                now_reading = bend + 2;
            }
        }
        
        if(now_reading >= tmp + file_size)  break;
    }

    free(tmp);
}

void write_persistcountfile(){
    char *env = getenv("PMEMWRAP_WRITECOUNTFILE");
    if(env != NULL && strcmp(env, "NO") == 0){
        //fprintf(stderr, "pmemwrap_writecountfile == 0\n");
        // pthread_mutex_unlock(&mutex);
        return;
    }

    int fd = open("countfile.txt", O_WRONLY | O_TRUNC | O_CREAT, 0666);
    if(fd == -1){
        perror(__func__);
        fprintf(stderr, " %s, %d, %s\n", __FILE__, __LINE__, __func__);
        exit(1);
    }

    int fd2 = open("countfile_plus.txt", O_WRONLY | O_TRUNC | O_CREAT, 0666);
        if(fd2 == -1){
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
        if(write(fd, tmp, strlen(tmp)) != strlen(tmp)){
            fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
            exit(1);
        }
        if(write(fd2, tmp, strlen(tmp)) != strlen(tmp)){
            fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
            exit(1);
        }
        free(tmp);

        tmp = (char *)malloc(45); //int digit(10) + _ + int digit(10) + _ + int digit(10) + _ + int digit(10) + \n + \0
        if(tmp == NULL){
            perror(__func__);
            fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
            exit(1);
        }

        for(int i=0;(i<MAX_LINE_LENGTH) && (persist_line_list[file_id][i].line != 0); i++){
            persist_line_list[file_id][i].abort_count = 0;
            Backtraces_info *binfo_now = persist_line_list[file_id][i].binfo;
            while(binfo_now != NULL){
                persist_line_list[file_id][i].abort_count += binfo_now->abort_count;
                binfo_now = binfo_now->next;
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

            if(write(fd, tmp, strlen(tmp)) != strlen(tmp)){
                fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
                exit(1);
            }
            if(write(fd2, tmp, strlen(tmp)) != strlen(tmp)){
                fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
                exit(1);
            }
            
            binfo_now = persist_line_list[file_id][i].binfo;
            while(binfo_now != NULL){
                if(env != NULL && strcmp(env, "ADD") == 0){
                    int larger_count;
                    if(binfo_now->count > binfo_now->prev_count)
                        larger_count = binfo_now->count;
                    else
                        larger_count = binfo_now->prev_count;
                    sprintf(tmp, "+stack_%010d_%010d\n", larger_count, binfo_now->abort_count);
                }
                else{
                    sprintf(tmp, "+stack_%010d_%010d\n", binfo_now->count, 0);
                }
   

                write(fd2, tmp, strlen(tmp));
                write(fd2, binfo_now->backtrace, strlen(binfo_now->backtrace));
                write(fd2, ";\n", 2);
                binfo_now = binfo_now->next;
            }
        }
        free(tmp);
        //printf("write_persistcountfile file_id: %d\n", file_id);
    }

    close(fd);
}

void rand_set_abortflag(const char *file, int line, Backtraces_info *binfo){
    //printf("wrap rand_set_abortflag\n");
    if(pmemwrap_abort == 0 || abort_through == 1){
        //fprintf(stderr, "DEBUG file: %s, line: %d\n", file, line);
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

    if(binfo->count == 0){
        abortflag = 0;
        pthread_mutex_unlock(&mutex);
        return;
    }

    int rand_num = rand();
    double probability = (double)ABORTFLAG_COEFFICIENT * 1 / ((double)binfo->prev_count * (double)persist_place_sum);
    if (abort_count_sum != 0){
        for(int j=0; j < binfo->abort_count - abort_count_minus; j++)
            probability *= 1 / (double)ABORTFLAG_COEFFICIENT2;
    }

    double rand_number = (double)rand_num / RAND_MAX;
    if(rand_number < probability){
        //正しい挙動
        fprintf(stderr, "stack:\n%s\n", binfo->backtrace);
        fprintf(stderr, "set abortflag file: %s, line: %d, rand_set_count: %d\n", file, line, rand_set_count);
        //
        binfo->abort_count++;
        abortflag = 1;
    }
    else{
        abortflag = 0;
    }

    pthread_mutex_unlock(&mutex);
    return;

    // for(file_id=0; file_id<MAX_FILE_LENGTH && file_list[file_id] != NULL; file_id++){
    //     if(strcmp(file, file_list[file_id]) != 0){
    //         continue;
    //     }
    //     for(int i=0;(i<MAX_LINE_LENGTH) && (persist_line_list[file_id][i].line != 0); i++){
    //         if(persist_line_list[file_id][i].line == line){
    //             if(persist_line_list[file_id][i].prev_count == 0){
    //                 abortflag = 0;
    //                 pthread_mutex_unlock(&mutex);
    //                 return;
    //             }
    //             //ランダムにabortflagをセット
    //             int rand_num = rand();
    //             double probability = (double)ABORTFLAG_COEFFICIENT * 1 / ((double)persist_line_list[file_id][i].prev_count * (double)persist_place_sum);
    //             if (abort_count_sum != 0){
    //                 for(int j=0; j<persist_line_list[file_id][i].abort_count - abort_count_minus; j++)
    //                     probability *= 1 / (double)ABORTFLAG_COEFFICIENT2;
    //             }
                    
    //             //fprintf(stderr, "1, %f 2, %f 3, %f\n", probability, (double)ABORTFLAG_COEFFICIENT * 1 / ((double)persist_line_list[file_id][i].prev_count * (double)persist_place_sum), (double)persist_line_list[file_id][i].abort_count / (double)abort_count_sum);
    //             double rand_number = (double)rand_num / RAND_MAX;
    //             // printf("probability: %lf, rand_number%lf\n", probability, rand_number);
    //             if(rand_number < probability){
    //                 //正しい挙動
    //                 fprintf(stderr, "set abortflag file: %s, line: %d, rand_set_count: %d\n", file, line, rand_set_count);
    //                 //
    //                 persist_line_list[file_id][i].abort_count++;
    //                 abortflag = 1;
    //                 pthread_mutex_unlock(&mutex);
    //                 return;
    //             }
    //             else{
    //                 abortflag = 0;
    //                     //printf("end rand_set_abortflag\n");
    //                 pthread_mutex_unlock(&mutex);
    //                 return;
    //             }
    //         }
    //     }
    // }

    // abortflag = 0;

    // pthread_mutex_unlock(&mutex);
    // return;
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
    // addrset->flushed_copyfile_fd = open(addrset->flushed_copyfile_path, (O_RDWR | O_CREAT), 0666);
    // fallocate(addrset->flushed_copyfile_fd, 0, 0, len);
    addrset->fake_addr = orig_pmem_map_file(addrset->flushed_copyfile_path, len, (1 << 0)/*PMEM_FILE_CREATE*/, 0666, NULL, NULL);
    // for(size_t i=0; i<len; i++){
    //     if(*((char*)addrset->fake_addr + i) != 0){
    //         printf("i: %ld\n", i);
    //     }
    // }
    // addrset->fake_addr = mmap(NULL, len, PROT_WRITE, MAP_SHARED, addrset->flushed_copyfile_fd, 0);

    if(addrset->fake_addr == NULL){
        perror(__func__);
        fprintf(stderr, "error, %s, %d, %s\n", __FILE__, __LINE__, __func__);
        exit(1);
    }

    addrset->next = NULL;
    addrset->prev = tail;
    addrset->len = len;
    addrset->file_type = file_type;
    // unsigned char vector[4];
    // printf("mincore: %d, pagesize: %ld\n", mincore(addrset->orig_addr, sysconf(_SC_PAGESIZE) * 4, vector), sysconf(_SC_PAGESIZE));
    // printf("vector: %d %d %d %d\n", vector[0],vector[1],vector[2],vector[3]);
    if(memcpyflag != NO_MEMCPY)
        if(file_type == PMEM_FILE){
            memcpy(addrset->fake_addr, addrset->orig_addr, len);
        }
        else{//file_type == PMEMOBJ_FILE
            // memset(addrset->orig_addr, 1, len);
            // memcpy(addrset->fake_addr, addrset->orig_addr, len);
            memcpy(addrset->fake_addr + 4096, addrset->orig_addr + 4096, len - 4096);
        }

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
    rand_set_abortflag_plus_persistcount(file, line);
    pmem_drain();
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
            // munmap(set->fake_addr, set->len);
            // close(set->flushed_copyfile_fd);
            orig_pmem_unmap(set->fake_addr, set->len);
            remove(set->flushed_copyfile_path);
            free(set->flushed_copyfile_path);
            free(set);
            pthread_mutex_unlock(&mutex);
            return;
        }
        set = set->next;
    }

    pthread_mutex_unlock(&mutex);
    return;
}

int pmem_wrap_unmap(void *addr, size_t len, const char *file, int line){
    rand_set_abortflag_plus_persistcount(file, line);
    if(abortflag == 1){
        pmem_drain();
    }
    delete_PMEMaddrset(addr);

    return orig_pmem_unmap(addr, len);
}

int pmem_unmap(void *addr, size_t len){
    // printf("wrap pmem_unmap\n");
    //int (*orig_pmem_unmap)(void*, size_t) = dlsym(RTLD_NEXT, "pmem_unmap");

    delete_PMEMaddrset(addr);

    return orig_pmem_unmap(addr, len);
}

__attribute__ ((destructor))
static void destructor () {
    printf("destructor\n");
    PMEMaddrset *set = head;

    while(set != NULL){
        //printf("%c\n", *(char*)set->fake_addr);
        // munmap(set->fake_addr, set->len);
        // close(set->flushed_copyfile_fd);
        orig_pmem_unmap(set->fake_addr, set->len);
        remove(set->flushed_copyfile_path);
        set = set->next;
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
    rand_set_abortflag_plus_persistcount(file, line);
    return orig_pmem_memmove_persist(pmemdest, src, len);
}

void *pmem_wrap_memcpy_persist(void *pmemdest, const void *src, size_t len, const char *file, int line){
    // printf("wrap pmem_wrap_memcpy_persist\n");
    rand_set_abortflag_plus_persistcount(file, line);
    return orig_pmem_memcpy_persist(pmemdest, src, len);
}

void *pmem_wrap_memset_persist(void *pmemdest, int c, size_t len, const char *file, int line){
    // printf("wrap pmem_wrap_memset_persist\n");
    rand_set_abortflag_plus_persistcount(file, line);
    return orig_pmem_memset_persist(pmemdest, c, len);
}

void *pmem_wrap_memmove(void *pmemdest, const void *src, size_t len, unsigned flags, const char *file, int line){
    // printf("wrap pmem_wrap_memmove\n");
    rand_set_abortflag_plus_persistcount(file, line);
    return orig_pmem_memmove(pmemdest, src, len, flags);
}

void *pmem_wrap_memcpy(void *pmemdest, const void *src, size_t len, unsigned flags, const char *file, int line){
    // printf("wrap pmem_wrap_memcpy\n");
    rand_set_abortflag_plus_persistcount(file, line);
    return orig_pmem_memcpy(pmemdest, src, len, flags);
}

void *pmem_wrap_memset(void *pmemdest, int c, size_t len, unsigned flags, const char *file, int line){
    // printf("wrap pmem_wrap_memset\n");
    rand_set_abortflag_plus_persistcount(file, line);
    return orig_pmem_memset(pmemdest, c, len, flags);
}

void pmem_flush(const void *addr, size_t len){
    //printf("wrap pmem_flush  addr: %p, len: %lu\n", addr, len);

    if(memcpyflag == NO_MEMCPY)
        return;
    
    add_waitdrainlist(addr, len);
}

void pmem_wrap_drain(const char *file, int line){
    // printf("wrap pmem_wrap_drain\n");
    rand_set_abortflag_plus_persistcount(file, line);

    pmem_drain();
    
    // printf("pmem_wrap_drain file:%s, line:%d\n", file, line);
}

void pmem_drain(){//waitdrainに入れたものだけをdrain
    //printf("wrap pmem_drain\n");
    // void *trace[64];
    // int n = backtrace(trace, sizeof(trace) / sizeof(trace[0]));
    // backtrace_symbols_fd(trace, n, 1);


    pthread_mutex_lock(&mutex);

    Waitdrain_addrset *w_set = w_head;

    if (abortflag == 0){
        while(w_set != NULL){
            uintptr_t d = w_set->addr - w_set->set->orig_addr;
            void *target_addr = (void *)(w_set->set->fake_addr + d);// fake_addr

            //64ビットのビット演算でやったほうがいい
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
            // munmap(set->fake_addr, set->len);
            // close(set->flushed_copyfile_fd);
            orig_pmem_flush(set->fake_addr, set->len);
            orig_pmem_drain();
            orig_pmem_unmap(set->fake_addr, set->len);

            if(set->file_type == PMEM_FILE){
                orig_pmem_flush(set->orig_addr, set->len);
            }
            else{//set->file_type == PMEMOBJ_FILE
                orig_pmem_flush(set->orig_addr + 4096, set->len - 4096);
            }
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
    // if(pmemwrap_abort == 1){
        abortflag = 1;
        //正しい挙動
        fprintf(stderr, "FORCE set abortflag file: %s, line: %d\n", file, line);
        //
        pmem_drain();
    // }
}

void force_set_abortflag(const char *file, int line){
    abortflag = 1;
    //正しい挙動
    fprintf(stderr, "FORCE set abortflag file: %s, line: %d\n", file, line);
    //
}

void fprint_offset(FILE *__restrict__ __stream, void *p, void *p2){
    fprintf(__stream, "fprint_offset: %lx\n", p2 - p);
}

void pmem_drain_nowrap(){
    pmem_drain();
}

void rand_set_abortflag_plus_persistcount(const char *file, int line){
    char bt[8000];
    void *trace[64];
    int n = backtrace(trace, sizeof(trace) / sizeof(trace[0]));
    backtrace_file_offset(trace, n, bt, 0);

    Backtraces_info *binfo = plus_persistcount(file, line, bt);
    rand_set_abortflag(file, line, binfo);

    if(abortflag == 1){
        int b_fd = open("backtrace.txt", O_WRONLY | O_TRUNC | O_CREAT, 0666);
        if(b_fd == -1){
            perror(__func__);
            fprintf(stderr, " %s, %d, %s\n", __FILE__, __LINE__, __func__);
            exit(1);
        }

        char buf[8020];
        backtrace_file_offset(trace, n, buf, 0);
        sprintf(buf, "+stack\n%s;\n", bt);
        write(b_fd, buf, strlen(buf));

        close(b_fd);
    }
}

void debug_print_line(const char *file, int line){
    fprintf(stderr, "debug_print_line, file: %s, line: %d\n", file, line);
}

void backtrace_file_offset_fd(void *const *array, int size, int fd)
{
    Dl_info info[128];
    int status[128];
    size_t total = 0;
    char *buf = (char *)malloc(8192);
    
    /* Fill in the information we can get from `dladdr'.  */
    for (int i = 0; i < size; ++i)
    {
        status[i] = dladdr(array[i], &info[i]);
        if (status[i] && info[i].dli_fname && info[i].dli_fname[0] != '\0')
        {
            size_t offset = array[i] - info[i].dli_fbase;
            sprintf(buf, "%s_++0x%lx\n", info[i].dli_fname, offset);
            write(fd, buf, strlen(buf));
        }
        else{
            sprintf(buf, "backtrace_error\n");
            write(fd, buf, strlen(buf));
        }
    }

    free(buf);
}

void backtrace_file_offset(void *const *array, int size, char* buf, int start_trace)
{
    Dl_info info[128];
    int status[128];
    size_t total = 0;
    
    buf[0] = '\0';
    /* Fill in the information we can get from `dladdr'.  */
    for (int i = start_trace; i < size; ++i)
    {
        status[i] = dladdr(array[i], &info[i]);
        if (status[i] && info[i].dli_fname && info[i].dli_fname[0] != '\0')
        {
            size_t offset = array[i] - info[i].dli_fbase;
            if(strstr(buf, "libwrappmem.so") == NULL){
                sprintf(buf, "%s%s_++0x%lx\n", buf, info[i].dli_fname, offset);
            }
            else{
                sprintf(buf, "%s_++0x%lx\n", info[i].dli_fname, offset);
            }
        }
        else{
            sprintf(buf, "%sbacktrace_error\n", buf);
        }
    }
}

void *nopmdk_mmap(const char *path, size_t len){
    int fd = open(path, O_RDWR);
    if(fd == -1){
        perror(__func__);
        fprintf(stderr, " %s, %d, %s\n", __FILE__, __LINE__, __func__);
        exit(1);
    }
    void *pmem_addr = mmap(NULL, len, (PROT_WRITE | PROT_READ), MAP_SHARED, fd, 0);
    add_PMEMaddrset(pmem_addr, len, path, PMEM_FILE);

    return pmem_addr;
}

void set_abort_through(int flag){
    abort_through = flag;
}