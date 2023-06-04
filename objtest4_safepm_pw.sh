# export ASAN_OPTIONS=handle_segv=0
# handle_segv=1
export ASAN_OPTIONS=halt_on_error=0:suppressions=MyASan.supp
# suppressions=MyASan.supp
gcc hello_libpmemobjtest4_safepm_pw.c -fsanitize=address -fsanitize-recover=all -ggdb -lwrappmem -lpmem -lpmemobj
# -fsanitize-recover=address 
export PMEMWRAP_ABORT=0
export PMEMWRAP_MEMCPY=RAND_MEMCPY
export PMEMWRAP_WRITECOUNTFILE=ADD
rm -f /mnt/pmem0/test4_safepm_pw /mnt/pmem0/test4_safepm_pw_flushed
./a.out /mnt/pmem0/test4_safepm_pw
./PmemWrap_memcpy.out /mnt/pmem0/test4_safepm_pw /mnt/pmem0/test4_safepm_pw_flushed
./a.out /mnt/pmem0/test4_safepm_pw
rm -f /mnt/pmem0/test4_safepm_pw /mnt/pmem0/test4_safepm_pw_flushed