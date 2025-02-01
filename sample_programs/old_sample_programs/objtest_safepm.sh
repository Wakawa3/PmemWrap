gcc hello_libpmemobjtest_safepm.c -fsanitize=address -lpmem -lpmemobj
export PMEMWRAP_ABORT=0
export PMEMWRAP_MEMCPY=RAND_MEMCPY
rm -f /mnt/pmem0/test_safepm /mnt/pmem0/test_safepm_flushed
./a.out -w /mnt/pmem0/test_safepm
./a.out -r /mnt/pmem0/test_safepm
# ./PmemWrap_memcpy.out /mnt/pmem0/test4 /mnt/pmem0/test4_flushed
# ./a.out /mnt/pmem0/test4
# rm -f /mnt/pmem0/test4 /mnt/pmem0/test4_flushed