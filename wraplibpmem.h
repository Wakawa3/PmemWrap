#ifndef WRAPLIBPMEM_H
#define WRAPLIBPMEM_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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

void plus_persistcount(char *file, int line);
void read_persistcountfile();
void write_persistcountfile();
void reset_persistcount();

void *pmem_map_file(const char *path, size_t len, int flags, mode_t mode, size_t *mapped_lenp, int *is_pmemp);
void pmem_wrappersist(const void *addr, size_t len, char* file, int line);
void pmem_persist(const void *addr, size_t len);
int pmem_unmap(void *addr, size_t len);

void rand_memcpy(PMEMaddrset *set);
// void rand_file_generate(PMEMaddrset *set, size_t n, uintptr_t d);//現在不使用

void add_waitdrainlist(const void *addr, size_t len);

void *pmem_wrapmemmove_persist(void *pmemdest, const void *src, size_t len, char* file, int line);
void *pmem_wrapmemcpy_persist(void *pmemdest, const void *src, size_t len, char* file, int line);
void *pmem_wrapmemset_persist(void *pmemdest, int c, size_t len, char* file, int line);
void *pmem_memmove_persist(void *pmemdest, const void *src, size_t len);
void *pmem_memcpy_persist(void *pmemdest, const void *src, size_t len);
void *pmem_memset_persist(void *pmemdest, int c, size_t len);

void *pmem_memmove_nodrain(void *pmemdest, const void *src, size_t len);
void *pmem_memcpy_nodrain(void *pmemdest, const void *src, size_t len);
void *pmem_memset_nodrain(void *pmemdest, int c, size_t len);

void *pmem_wrapmemmove(void *pmemdest, const void *src, size_t len, unsigned flags, char* file, int line);
void *pmem_wrapmemcpy(void *pmemdest, const void *src, size_t len, unsigned flags, char* file, int line);
void *pmem_wrapmemset(void *pmemdest, int c, size_t len, unsigned flags, char* file, int line);
void *pmem_memmove(void *pmemdest, const void *src, size_t len, unsigned flags);
void *pmem_memcpy(void *pmemdest, const void *src, size_t len, unsigned flags);
void *pmem_memset(void *pmemdest, int c, size_t len, unsigned flags);

void pmem_flush(const void *addr, size_t len);
void pmem_wrapdrain(char* file, int line);
void pmem_drain();

#endif