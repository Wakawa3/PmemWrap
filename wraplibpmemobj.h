#ifndef WRAPLIBPMEMOBJ_H
#define WRAPLIBPMEMOBJ_H 1

typedef struct pmemobjpool PMEMobjpool;
typedef int (*pmemobj_constr)(PMEMobjpool *pop, void *ptr, void *arg);

typedef struct pmemoid {
	uint64_t pool_uuid_lo;
	uint64_t off;
} PMEMoid;

extern PMEMobjpool *(*orig_pmemobj_create)(const char *path, const char *layout, size_t poolsize, mode_t mode);
extern PMEMobjpool *(*orig_pmemobj_open)(const char *path, const char *layout);
extern void (*orig_pmemobj_persist)(PMEMobjpool *pop, const void *addr, size_t len);
extern int (*orig_pmemobj_tx_add_range)(PMEMoid oid, uint64_t hoff, size_t size);
extern int (*orig_pmemobj_tx_add_range_direct)(const void *ptr, size_t size);
extern void (*orig_pmemobj_tx_process)();
extern void (*orig_pmemobj_close)(PMEMobjpool *pop);

PMEMobjpool *pmemobj_create(const char *path, const char *layout, size_t poolsize, mode_t mode);
PMEMobjpool *pmemobj_open(const char *path, const char *layout);
void pmemobj_wrap_persist(PMEMobjpool *pop, const void *addr, size_t len, const char *file, int line);
//void pmemobj_persist(PMEMobjpool *pop, const void *addr, size_t len);
int pmemobj_wrap_tx_add_range(PMEMoid oid, uint64_t hoff, size_t size, const char *file, int line);
void pmemobj_wrap_tx_process(const char *file, int line);
//void pmemobj_tx_process();

#endif