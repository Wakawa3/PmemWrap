gcc hello_libpmemobjtest3.c -lwrappmem -lpmem -lpmemobj
export PMEMWRAP_ABORT=0
export PMEMWRAP_MEMCPY=RAND_MEMCPY
rm /mnt/pmem0/test /mnt/pmem0/test_flushed
./a.out /mnt/pmem0/test
./PmemWrap_memcpy.out /mnt/pmem0/test /mnt/pmem0/test_flushed
./a.out /mnt/pmem0/test