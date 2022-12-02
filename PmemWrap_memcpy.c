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

//mapped_file1(元のPMDKファイル)にmapped_file2をコピー

int fd1, fd2;
size_t size1, size2;
void *mapped_file1, *mapped_file2;

int subseed = 0;
char *subseed_env;

void *normal_memcpy(void *p){
    int i = *(int*)p;
    size_t offset = i * size1 / THREADS / 64 * 64;
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
    size_t to;
    if(i != THREADS - 1)
        to = (i + 1) * size1 / THREADS / 64 * 64; 
    else
        to = size1;
    
    for(; offset < to; offset += CACHE_LINE_SIZE){
        if(rand() % 2 == 0){
            memcpy(mapped_file1 + offset, mapped_file2 + offset, CACHE_LINE_SIZE);
        }
    }

    offset -= CACHE_LINE_SIZE;
    size_t remainder = (to - offset) % CACHE_LINE_SIZE;
    if(remainder != 0){
        if(rand() % 2 == 0){
            memcpy(mapped_file1 + offset, mapped_file2 + offset, remainder);
        }
    }
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

    //fprintf(stderr, "multithread\n");

    lseek(fd1, 0, SEEK_SET);
    lseek(fd2, 0, SEEK_SET);

    mapped_file1 = mmap(NULL, size1, PROT_WRITE, MAP_SHARED, fd1, 0);
    mapped_file2 = mmap(NULL, size1, PROT_READ, MAP_PRIVATE, fd2, 0);

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

    unsigned int end = (unsigned int)time(NULL);
    fprintf(stderr, "memcpy time: %d\n", end - start);

    close(fd1);
    close(fd2);
}