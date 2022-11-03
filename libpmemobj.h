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

#include <libpmemobj/action.h>
#include <libpmemobj/atomic.h>
#include <libpmemobj/ctl.h>
#include <libpmemobj/iterator.h>
#include <libpmemobj/lists_atomic.h>
#include <libpmemobj/pool.h>
#include <libpmemobj/thread.h>
#include <libpmemobj/tx.h>

void pmemobj_wrappersist(PMEMobjpool *pop, const void *addr, size_t len, char *file, int line);
void pmemobj_wraptx_process(char *file, int line);
void *pmemobj_wrapmemcpy_persist(PMEMobjpool *pop, void *dest, const void *src, size_t len, char* file, int line);
void *pmemobj_wrapmemset_persist(PMEMobjpool *pop, void *dest, int c, size_t len, char* file, int line);

#define pmemobj_tx_process() pmemobj_wraptx_process(__FILE__, __LINE__)
#define pmemobj_persist(pop, addr, len) pmemobj_wrappersist((pop), (addr), (len), __FILE__, __LINE__)
#define pmemobj_memcpy_persist(pop, dest, src, len) pmemobj_wrapmemcpy_persist((pop), (dest), (src), (len), __FILE__, __LINE__)
#define pmemobj_memset_persist(pop, dest, c, len) pmemobj_wrapmemset_persist((pop), (dest), (c), (len), __FILE__, __LINE__)

#endif	/* libpmemobj.h */
