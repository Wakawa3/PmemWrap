#ifndef NOPMDK_API_H
#define NOPMDK_API_H 1

#include <stdlib.h>

void pmem_flush(const void *addr, size_t len);
void pmem_wrap_drain(const char *file, int line);
void *nopmdk_mmap(const char *path, size_t len);
void set_abort_through(int flag);

#define PMEMWRAP_FLUSH(addr, len) pmem_flush(addr, len)
#define PMEMWRAP_DRAIN() pmem_wrap_drain(__FILE__, __LINE__)
#define PMEMWRAP_MMAP(path, len) nopmdk_mmap(path, len)
#define PMEMWRAP_SET_ABORT_THROUGH(flag) set_abort_through(flag)

#endif