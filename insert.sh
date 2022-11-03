mkdir orig_c
cp *.c orig_c
sed -i '1s/^/#include "libpmemobj.h"\n/' *.c
sed -i '1s/^/#include "libpmem.h"\n/' *.c