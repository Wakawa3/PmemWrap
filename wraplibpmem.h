#ifndef WRAPLIBPMEM_H
#define WRAPLIBPMEM_H 1

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

#define PMEM_FILE 0
#define PMEMOBJ_FILE 1

#define NORMAL_MEMCPY 0
#define RAND_MEMCPY 1
#define NO_MEMCPY 2

#define MEMCPY_THREADS 1

#define LIBPMEMMAP_ALIGN_VAL 0x200000
#define COPYFILE_WORDENDING "_flushed"

typedef struct _pmemaddrset PMEMaddrset;

struct _pmemaddrset {
    void *orig_addr;
    void *fake_addr;
    PMEMaddrset *next;
    PMEMaddrset *prev;
    size_t len;
    int file_type;
    int flushed_copyfile_fd;
    char *flushed_copyfile_path;
};

extern PMEMaddrset *head;
extern PMEMaddrset *tail;

typedef struct _waitdrain_addrset Waitdrain_addrset;

struct _waitdrain_addrset{
    void *addr;
    Waitdrain_addrset *next;
    Waitdrain_addrset *prev;
    size_t len;
    PMEMaddrset *set;
};

extern Waitdrain_addrset *w_head;
extern Waitdrain_addrset *w_tail;

//int flushed = 0;

typedef struct _line_info LINEinfo;

struct _line_info{
    int line;
    int count;
    int prev_count;
    int abort_count;
};

extern char *file_list[MAX_FILE_LENGTH];
//LINEinfo *(persist_line_list[MAX_LINE_LENGTH])[MAX_FILE_LENGTH]; //ポインタ配列 ポインタはpersist_line_list[MAX_LINE_LENGTH]のアドレスを指す
extern LINEinfo persist_line_list[MAX_FILE_LENGTH][MAX_LINE_LENGTH];

extern int persist_count_sum;
extern int persist_place_sum;

extern void *(*orig_pmem_map_file)(const char*, size_t, int, mode_t, size_t*, int*);
//extern void (*orig_pmem_persist)(const void*, size_t);
extern int (*orig_pmem_unmap)(void*, size_t);
extern void (*orig_pmem_flush)(const void *, size_t);
extern void (*orig_pmem_drain)();
extern int (*orig_pmem_deep_drain)(const void *, size_t);

extern int abortflag;
extern int memcpyflag;

void plus_persistcount(const char *file, int line);
void read_persistcountfile();
void write_persistcountfile();
// void reset_persistcount();
void rand_set_abortflag(const char *file, int line);

void add_PMEMaddrset(void *orig_addr, size_t len, const char *path, int file_type);

void *pmem_map_file(const char *path, size_t len, int flags, mode_t mode, size_t *mapped_lenp, int *is_pmemp);
void pmem_wrap_persist(const void *addr, size_t len, const char *file, int line);
void pmem_persist(const void *addr, size_t len);
int pmem_wrap_msync(const void *addr, size_t len, const char *file, int line);
void delete_PMEMaddrset(void *addr);
int pmem_unmap(void *addr, size_t len);

void *sub_normal_memcpy(void *p);
void *sub_rand_memcpy(void *p);
void rollback_memcpy(PMEMaddrset *set);
// void rand_file_generate(PMEMaddrset *set, size_t n, uintptr_t d);//現在不使用

void add_waitdrainlist(const void *addr, size_t len);

void *pmem_wrap_memmove_persist(void *pmemdest, const void *src, size_t len, const char *file, int line);
void *pmem_wrap_memcpy_persist(void *pmemdest, const void *src, size_t len, const char *file, int line);
void *pmem_wrap_memset_persist(void *pmemdest, int c, size_t len, const char *file, int line);
void *pmem_memmove_persist(void *pmemdest, const void *src, size_t len);
void *pmem_memcpy_persist(void *pmemdest, const void *src, size_t len);
void *pmem_memset_persist(void *pmemdest, int c, size_t len);

void *pmem_memmove_nodrain(void *pmemdest, const void *src, size_t len);
void *pmem_memcpy_nodrain(void *pmemdest, const void *src, size_t len);
void *pmem_memset_nodrain(void *pmemdest, int c, size_t len);

void *pmem_wrap_memmove(void *pmemdest, const void *src, size_t len, unsigned flags, const char *file, int line);
void *pmem_wrap_memcpy(void *pmemdest, const void *src, size_t len, unsigned flags, const char *file, int line);
void *pmem_wrap_memset(void *pmemdest, int c, size_t len, unsigned flags, const char *file, int line);
void *pmem_memmove(void *pmemdest, const void *src, size_t len, unsigned flags);
void *pmem_memcpy(void *pmemdest, const void *src, size_t len, unsigned flags);
void *pmem_memset(void *pmemdest, int c, size_t len, unsigned flags);

void pmem_flush(const void *addr, size_t len);
void pmem_wrap_drain(const char *file, int line);
void pmem_drain();

int pmem_deep_persist(const void *addr, size_t len);
void pmem_deep_flush(const void *addr, size_t len);
int pmem_deep_drain(const void *addr, size_t len);

void force_abort_drain(const char *file, int line);

void rand_set_abortflag_plus_persistcount(const char *file, int line);

#endif