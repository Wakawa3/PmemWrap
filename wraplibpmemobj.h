#ifndef WRAPLIBPMEMOBJ_H
#define WRAPLIBPMEMOBJ_H 1

typedef struct pmemobjpool PMEMobjpool;

PMEMobjpool *(*orig_pmemobj_create)(const char *path, const char *layout, size_t poolsize, mode_t mode);
void (*orig_pmemobj_persist)(PMEMobjpool *pop, const void *addr, size_t len);

PMEMobjpool *pmemobj_create(const char *path, const char *layout, size_t poolsize, mode_t mode);
void pmemobj_persist(PMEMobjpool *pop, const void *addr, size_t len);

#endif