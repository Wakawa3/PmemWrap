#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>

#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define THREADS 1
#define CACHE_LINE_SIZE 64
#define MEMCMP_SIZE 8092

//mapped_file1(元のPMDKファイル)にmapped_file2をコピー

int fd1, fd2;
size_t size1, size2;
void *mapped_file1, *mapped_file2;

int subseed = 0;
char *subseed_env;

int is_obj = 0;

void *normal_memcpy(void *p){
    int i = *(int*)p;
    size_t offset = i * size1 / THREADS / 64 * 64;
    if(is_obj && (offset < 4096)) {offset = 4096; printf("is_obj\n");}
    size_t len;
    if(i != THREADS - 1)
        len = (i + 1) * size1 / THREADS / 64 * 64 - offset; 
    else
        len = size1 - offset;

    const size_t memcpy_len_max = __INT_MAX__ / 64 * 64;

    while(len > memcpy_len_max){
        memcpy(mapped_file1 + offset, mapped_file2 + offset, memcpy_len_max);
        len -= memcpy_len_max;
        offset += memcpy_len_max;
    }

    memcpy(mapped_file1 + offset, mapped_file2 + offset, len);
}

void *rand_memcpy(void *p){
    int i = *(int*)p;
    srand((unsigned int)time(NULL) + subseed + i * 10000);

    size_t offset = i * size1 / THREADS / 64 * 64;
    if(is_obj && (offset < 4096)) {offset = 4096; printf("is_obj\n");}
    size_t to;
    if(i != THREADS - 1)
        to = (i + 1) * size1 / THREADS / 64 * 64; 
    else
        to = size1;

    int series_flag = 0;
    size_t from_offset = 0;
    
    for(; offset < to; offset += MEMCMP_SIZE){
        if(memcmp(mapped_file1 + offset, mapped_file2 + offset, MEMCMP_SIZE) != 0){
            for(size_t inner_offset = 0; inner_offset < MEMCMP_SIZE; inner_offset += CACHE_LINE_SIZE){
                size_t detailed_offset = offset + inner_offset;
                if(memcmp(mapped_file1 + detailed_offset, mapped_file2 + detailed_offset, CACHE_LINE_SIZE) != 0){
                    if(series_flag == 0){
                        from_offset = detailed_offset;
                        series_flag = 1;
                    }
                    
                    if(rand() % 2 == 0){
                        memcpy(mapped_file1 + detailed_offset, mapped_file2 + detailed_offset, CACHE_LINE_SIZE);
                    }
                }
                else{
                    if(series_flag == 1){
                        // printf("diff range: %lx - %lx\n", from_offset, detailed_offset);
                        series_flag = 0;
                    }
                }
            }
        }
    }

    offset -= MEMCMP_SIZE;
    size_t remainder = (to - offset) % MEMCMP_SIZE;
    int remainder_cacheline = (to - offset) % CACHE_LINE_SIZE;
    if(remainder != 0){
        if(memcmp(mapped_file1 + offset, mapped_file2 + offset, remainder) != 0){
            size_t inner_offset = 0;
            size_t detailed_offset = offset;
            for(; inner_offset < remainder; inner_offset += CACHE_LINE_SIZE){
                detailed_offset = offset + inner_offset;
                if(memcmp(mapped_file1 + detailed_offset, mapped_file2 + detailed_offset, CACHE_LINE_SIZE) != 0){
                    if(memcmp(mapped_file1 + detailed_offset, mapped_file2 + detailed_offset, CACHE_LINE_SIZE) != 0){
                        if(series_flag == 0){
                            from_offset = detailed_offset;
                            series_flag = 1;
                        }

                        if(rand() % 2 == 0){
                            memcpy(mapped_file1 + detailed_offset, mapped_file2 + detailed_offset, CACHE_LINE_SIZE);
                        }
                    }
                    if(series_flag == 1){
                        //printf("diff range: %lx - %lx\n", from_offset, detailed_offset);
                        series_flag = 0;
                    }
                }
            }
            detailed_offset -= CACHE_LINE_SIZE;
//ここ最後のほう
            if(remainder_cacheline != 0){
                if(memcmp(mapped_file1 + detailed_offset, mapped_file2 + detailed_offset, remainder_cacheline) != 0){
                    if(series_flag == 0){
                        from_offset = offset;
                        series_flag = 1;
                    }

                    if(rand() % 2 == 0){
                        memcpy(mapped_file1 + detailed_offset, mapped_file2 + detailed_offset, remainder_cacheline);
                    }
                }
            }
        }
        if(series_flag == 1){
            //printf("diff range: %lx - %lx\n", from_offset, offset + remainder);
            series_flag = 0;
        }
        // if(memcmp(mapped_file1 + offset, mapped_file2 + offset, remainder) != 0){
        //     if(series_flag == 0){
        //         from_offset = offset;
        //         series_flag = 1;
        //     }

        //     if(rand() % 2 == 0){
        //         memcpy(mapped_file1 + offset, mapped_file2 + offset, remainder);
        //     }
        // }
        // if(series_flag == 1){
        //     //printf("diff range: %lx - %lx\n", from_offset, offset + CACHE_LINE_SIZE);
        // }
    }
}

void selected_memcpy(){
    int fd_s = open("selected_range.txt", O_RDONLY);
    if(fd_s == -1){
        fprintf(stderr, "selected_range.txt doesn't exist.\n");
        return;
    }

    size_t size_s = lseek(fd_s, 0, SEEK_END);
    lseek(fd_s, 0, SEEK_SET);

    const size_t memcpy_len_max = __INT_MAX__ / 64 * 64;

    void *mapped_file_s = mmap(NULL, size_s, PROT_READ, MAP_SHARED, fd_s, 0);
    char *endptr = mapped_file_s - 1;
    char *startptr;
    do {
        startptr = endptr + 1;
        size_t offset = strtol(startptr, &endptr, 16);
        startptr = endptr + 1;
        size_t len = strtol(startptr, &endptr, 16);

        fprintf(stderr, "offset: %lx, len: %lx\n", offset, len);

        size_t to = (offset + len + CACHE_LINE_SIZE - 1) / CACHE_LINE_SIZE * CACHE_LINE_SIZE;
        offset = offset / CACHE_LINE_SIZE * CACHE_LINE_SIZE;
        len = to - offset;
        
        fprintf(stderr, "2,offset: %lx, len: %lx\n", offset, len);

        while(len > memcpy_len_max){
            memcpy(mapped_file1 + offset, mapped_file2 + offset, memcpy_len_max);
            len -= memcpy_len_max;
            offset += memcpy_len_max;
        }
        memcpy(mapped_file1 + offset, mapped_file2 + offset, len);
    } while(*endptr != '\0' || *(endptr+1) != '\0');
}

int main(int argc, char *argv[]){
    fd1 = open(argv[1], O_RDWR);
    if(fd1 == -1){
        perror("fd1");
        exit(1);
    }
    size1 = lseek(fd1, 0, SEEK_END);

    fd2 = open(argv[2], O_RDONLY);
    if(fd2 == -1){
        perror("fd2");
        exit(1);
    }
    size2 = lseek(fd2, 0, SEEK_END);

    if(size1 != size2){
        fprintf(stderr, "PmemWrap_memcpy: != size\n");
        exit(1);
    }
    
    //size1とsize2が割り切れないとき、拡張して後で拡張した分を消す手もある？

    //fprintf(stderr, "multithread\n");

    lseek(fd1, 0, SEEK_SET);
    lseek(fd2, 0, SEEK_SET);

    mapped_file1 = mmap(NULL, size1, PROT_WRITE, MAP_SHARED, fd1, 0);
    mapped_file2 = mmap(NULL, size1, PROT_READ, MAP_PRIVATE, fd2, 0);

    is_obj = 1;
    // for(int i = 0; i < 4096; i++){
    //     if(*(unsigned char*)(mapped_file2 + i) != 0) {
    //         printf("is_not_obj\n");
    //         is_obj = 0;
    //         break;
    //     }
    // }

    if(*(unsigned char*)mapped_file2 != 0 || (memcmp(mapped_file2, mapped_file2 + 1, 4095) != 0)){
        printf("is_not_obj\n");
        is_obj=0;
    }
    //if(argc == 4 && (strcmp(argv[3], "obj") == 0)) is_obj = 1;

    pthread_t pt[THREADS];

    subseed_env = getenv("PMEMWRAP_SEED");
    subseed = atoi(subseed_env);

    unsigned int start = (unsigned int)time(NULL);

    int arg[THREADS];
    char* memcpyflag_env = getenv("PMEMWRAP_MEMCPY");
    if(strcmp(memcpyflag_env, "NORMAL_MEMCPY") == 0){
        for(int i = 0; i < THREADS; i++){
            arg[i] = i;
            pthread_create(&pt[i], NULL, &normal_memcpy, &arg[i]);
        }
        for(int i = 0; i < THREADS; i++){
            pthread_join(pt[i], NULL);
        }
    }
    else if(strcmp(memcpyflag_env, "RAND_MEMCPY") == 0){
        for(int i = 0; i < THREADS; i++){
            arg[i] = i;
            pthread_create(&pt[i], NULL, &rand_memcpy, &arg[i]);
        }
        for(int i = 0; i < THREADS; i++){
            pthread_join(pt[i], NULL);
        }
    }
    else if(strcmp(memcpyflag_env, "SELECTED_MEMCPY") == 0){
        selected_memcpy();
    }

    unsigned int end = (unsigned int)time(NULL);
    fprintf(stderr, "memcpy time: %d\n", end - start);

    close(fd1);
    close(fd2);
}