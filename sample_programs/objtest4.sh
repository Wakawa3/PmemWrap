gcc hello_libpmemobjtest4.c -lwrappmem -lpmem -lpmemobj
export PMEMWRAP_ABORT=0
export PMEMWRAP_MEMCPY=RAND_MEMCPY
rm -f /mnt/pmem0/test4 /mnt/pmem0/test4_flushed
./a.out /mnt/pmem0/test4
${PMEMWRAP_HOME}/bin/PmemWrap_memcpy.out /mnt/pmem0/test4 /mnt/pmem0/test4_flushed
./a.out /mnt/pmem0/test4
rm -f /mnt/pmem0/test4 /mnt/pmem0/test4_flushed