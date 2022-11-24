gcc PmemKVS.c -lwrappmem -lpmem -lpmemobj
./a.out -w a 1
./a.out -wp a 1 data_flushed
./PmemWrap_memcpy.out /mnt/pmem0/data /mnt/pmem0/data_flushed
./a.out -w a 1