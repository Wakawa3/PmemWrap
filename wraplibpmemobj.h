#ifndef WRAPLIBPMEMOBJ_H
#define WRAPLIBPMEMOBJ_H 1

typedef struct pmemobjpool PMEMobjpool;

typedef struct pmemoid {
	uint64_t pool_uuid_lo;
	uint64_t off;
} PMEMoid;

extern PMEMobjpool *(*orig_pmemobj_create)(const char *path, const char *layout, size_t poolsize, mode_t mode);
extern PMEMobjpool *(*orig_pmemobj_open)(const char *path, const char *layout);
extern void (*orig_pmemobj_persist)(PMEMobjpool *pop, const void *addr, size_t len);
extern int (*orig_pmemobj_tx_add_range)(PMEMoid oid, uint64_t hoff, size_t size);
extern void (*orig_pmemobj_tx_process)();

PMEMobjpool *pmemobj_create(const char *path, const char *layout, size_t poolsize, mode_t mode);
PMEMobjpool *pmemobj_open(const char *path, const char *layout);
void pmemobj_persist(PMEMobjpool *pop, const void *addr, size_t len);
int pmemobj_tx_add_range(PMEMoid oid, uint64_t hoff, size_t size);
void pmemobj_tx_process();

#endif