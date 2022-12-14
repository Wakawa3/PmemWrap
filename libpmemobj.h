/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2014-2020, Intel Corporation */

/*
 * libpmemobj.h -- definitions of libpmemobj entry points
 *
 * This library provides support for programming with persistent memory (pmem).
 *
 * libpmemobj provides a pmem-resident transactional object store.
 *
 * See libpmemobj(7) for details.
 */

#ifndef LIBPMEMOBJ_H
#define LIBPMEMOBJ_H 1

//#define PMEMOBJ_DIRECT_NON_INLINE

#include <libpmemobj/action.h>
#include <libpmemobj/atomic.h>
#include <libpmemobj/ctl.h>
#include <libpmemobj/iterator.h>
#include <libpmemobj/lists_atomic.h>
#include <libpmemobj/pool.h>
#include <libpmemobj/thread.h>
#include <libpmemobj/tx.h>

void pmemobj_wrap_persist(PMEMobjpool *pop, const void *addr, size_t len, const char *file, int line);
void pmemobj_wrap_drain(PMEMobjpool *pop, const char *file, int line);
int pmemobj_wrap_tx_add_range(PMEMoid oid, uint64_t hoff, size_t size, const char *file, int line);
int pmemobj_wrap_tx_add_range_direct(const void *ptr, size_t size, const char *file, int line);
void pmemobj_wrap_tx_process(const char *file, int line);
void *pmemobj_wrap_memcpy_persist(PMEMobjpool *pop, void *dest, const void *src, size_t len, const char *file, int line);
void *pmemobj_wrap_memset_persist(PMEMobjpool *pop, void *dest, int c, size_t len, const char *file, int line);
int pmemobj_wrap_alloc(PMEMobjpool *pop, PMEMoid *oidp, size_t size, uint64_t type_num, pmemobj_constr constructor, void *arg, const char *file, int line);
int pmemobj_wrap_zalloc(PMEMobjpool *pop, PMEMoid *oidp, size_t size, uint64_t type_num, const char *file, int line);
int pmemobj_wrap_realloc(PMEMobjpool *pop, PMEMoid *oidp, size_t size, uint64_t type_num, const char *file, int line);
void *pmemobj_wrap_direct(PMEMoid oid, const char *file, int line);
PMEMoid pmemobj_wrap_tx_alloc(size_t size, uint64_t type_num, const char *file, int line);
PMEMoid pmemobj_wrap_tx_zalloc(size_t size, uint64_t type_num, const char *file, int line);

void force_abort_drain(const char *file, int line);
void force_set_abortflag(const char *file, int line);
void rand_set_abortflag(const char *file, int line);

#define pmemobj_tx_process() pmemobj_wrap_tx_process(__FILE__, __LINE__)
#define pmemobj_tx_add_range(oid, hoff, size) pmemobj_wrap_tx_add_range((oid), (hoff), (size), __FILE__, __LINE__)
#define pmemobj_tx_add_range_direct(ptr, size) pmemobj_wrap_tx_add_range_direct((ptr), (size), __FILE__, __LINE__)
#define pmemobj_persist(pop, addr, len) pmemobj_wrap_persist((pop), (addr), (len), __FILE__, __LINE__)
#define pmemobj_drain(pop) pmemobj_wrap_drain((pop), __FILE__, __LINE__)
#define pmemobj_memcpy_persist(pop, dest, src, len) pmemobj_wrap_memcpy_persist((pop), (dest), (src), (len), __FILE__, __LINE__)
#define pmemobj_memset_persist(pop, dest, c, len) pmemobj_wrap_memset_persist((pop), (dest), (c), (len), __FILE__, __LINE__)
#define pmemobj_alloc(pop, oidp, size, type_num, constructor, arg) pmemobj_wrap_alloc((pop), (oidp), (size), (type_num), (constructor), (arg), __FILE__, __LINE__)
#define pmemobj_zalloc(pop, oidp, size, type_num) pmemobj_wrap_zalloc((pop), (oidp), (size), (type_num), __FILE__, __LINE__)
#define pmemobj_realloc(pop, oidp, size, type_num) pmemobj_wrap_realloc((pop), (oidp), (size), (type_num), __FILE__, __LINE__)
// #define pmemobj_direct(oid) pmemobj_wrap_direct((oid), __FILE__, __LINE__)
#define pmemobj_tx_alloc(size, type_num) pmemobj_wrap_tx_alloc((size), (type_num), __FILE__, __LINE__)
#define pmemobj_tx_zalloc(size, type_num) pmemobj_wrap_tx_zalloc((size), (type_num), __FILE__, __LINE__)

#define PMEMWRAP_FORCE_ABORT() force_abort_drain(__FILE__, __LINE__)
#define PMEMWRAP_SET_ABORTFLAG() force_set_abortflag(__FILE__, __LINE__)
#define PMEMWRAP_PRINT_OFFSET(__stream, p, p2) fprintf((__stream), "fprint_offset: %lx\n", (p2) - (p))
#define PMEMWRAP_RAND_SET_ABORTFLAG()  rand_set_abortflag(__FILE__, __LINE__)

#endif	/* libpmemobj.h */
