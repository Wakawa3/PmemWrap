gcc hello_libpmemobjtest4_safepm_pw.c -fsanitize=address -fsanitize-recover=all -ggdb -lwrappmem -lpmem -lpmemobj
export PMEMWRAP_ABORT=0
export PMEMWRAP_MEMCPY=RAND_MEMCPY
export PMEMWRAP_WRITECOUNTFILE=ADD
rm -f /mnt/pmem0/test4_safepm_pw /mnt/pmem0/test4_safepm_pw_flushed
./a.out /mnt/pmem0/test4_safepm_pw #この実行は途中でクラッシュする
${PMEMWRAP_HOME}/bin/PmemWrap_memcpy.out /mnt/pmem0/test4_safepm_pw /mnt/pmem0/test4_safepm_pw_flushed #永続化処理されていない領域を一部のみコピーする
./a.out /mnt/pmem0/test4_safepm_pw #この実行時，PM上のデータを読み込む際に確率的にクラッシュし，不具合があることがわかる
rm -f /mnt/pmem0/test4_safepm_pw /mnt/pmem0/test4_safepm_pw_flushed