gcc hello_libpmemobjtest4.c -lwrappmem -lpmem -lpmemobj
export PMEMWRAP_ABORT=0
export PMEMWRAP_MEMCPY=NORMAL_MEMCPY
rm /mnt/pmem0/test4 /mnt/pmem0/test4_flushed
./a.out /mnt/pmem0/test4
./PmemWrap_memcpy.out /mnt/pmem0/test4 /mnt/pmem0/test4_flushed
./a.out /mnt/pmem0/test4