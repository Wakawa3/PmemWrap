export ASAN_OPTIONS=halt_on_error=0
gcc hello_libpmemobjtest4_safepm.c -fsanitize=address -lpmem -lpmemobj
rm -f /mnt/pmem0/test4
./a.out /mnt/pmem0/test4
./a.out /mnt/pmem0/test4
rm -f /mnt/pmem0/test4 