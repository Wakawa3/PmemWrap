PM_ROOT=${HOME}/pmdk_orig/src/examples/libpmemobj

TEST_ROOT=${HOME}/PmemWrap2

#WORKLOAD=btree
WORKLOAD=$1
WORKLOAD_LOC=${PM_ROOT}/tree_map/${WORKLOAD}_map.c

#PATCH=race4
PATCH=$2
PATCH_LOC=${TEST_ROOT}/patch/${WORKLOAD}_${PATCH}.patch

OUT_LOC=${TEST_ROOT}/outputs

PMIMAGE=/mnt/pmem0/${WORKLOAD}_testex


patch ${WORKLOAD_LOC} < ${PATCH_LOC}

cd ${PM_ROOT}
patch Makefile < ${TEST_ROOT}/patch/MakefilePatch/libpmemobj.patch
sed -i '1s/^/#include "libpmemobj.h"\n/' *.c
sed -i '1s/^/#include "libpmem.h"\n/' *.c

cd map
patch Makefile < ${TEST_ROOT}/patch/MakefilePatch/libpmemobj_map.patch
sed -i '1s/^/#include "libpmemobj.h"\n/' *.c
sed -i '1s/^/#include "libpmem.h"\n/' *.c

cd ../tree_map
patch Makefile < ${TEST_ROOT}/patch/MakefilePatch/libpmemobj_tree_map.patch
sed -i '1s/^/#include "libpmemobj.h"\n/' *.c
sed -i '1s/^/#include "libpmem.h"\n/' *.c

cd ${PM_ROOT}

cp ${TEST_ROOT}/libpmem.h ${PM_ROOT}
cp ${TEST_ROOT}/libpmem.h ${PM_ROOT}/map
cp ${TEST_ROOT}/libpmem.h ${PM_ROOT}/tree_map
cp ${TEST_ROOT}/libpmemobj.h ${PM_ROOT}
cp ${TEST_ROOT}/libpmemobj.h ${PM_ROOT}/map
cp ${TEST_ROOT}/libpmemobj.h ${PM_ROOT}/tree_map


make -j$(nproc)

export PMEMWRAP_ABORT=0
export PMEMWRAP_WRITECOUNTFILE=1
${PM_ROOT}/map/data_store ${WORKLOAD} ${PMIMAGE} 200 > /dev/null
export PMEMWRAP_WRITECOUNTFILE=0

echo "" > ${OUT_LOC}/${WORKLOAD}_${PATCH}_abort.txt
echo "" > ${OUT_LOC}/${WORKLOAD}_${PATCH}_error.txt

for i in `seq 100`
do
    export PMEMWRAP_ABORT=1
    export PMEMWRAP_SEED=${i}${i}${i}${i}${i}${i}${i}
    ${PM_ROOT}/map/data_store ${WORKLOAD} ${PMIMAGE} 200 > /dev/null 2>> ${OUT_LOC}/${WORKLOAD}_${PATCH}_abort.txt
    echo "${i}\n" >> ${OUT_LOC}/${WORKLOAD}_${PATCH}_abort.txt
    export PMEMWRAP_ABORT=0
    bash -c "${PM_ROOT}/map/data_store ${WORKLOAD} ${PMIMAGE} 200 > /dev/null 2>> ${OUT_LOC}/${WORKLOAD}_${PATCH}_error.txt" 2>> ${OUT_LOC}/${WORKLOAD}_${PATCH}_abort.txt
    rm ${PMIMAGE}
done

rm countfile.txt

make clean -j$(nproc)

rm  ${PM_ROOT}/libpmem.h
rm  ${PM_ROOT}/map/libpmem.h
rm  ${PM_ROOT}/tree_map/libpmem.h
rm  ${PM_ROOT}/libpmemobj.j
rm  ${PM_ROOT}/map/libpmemobj.j
rm  ${PM_ROOT}/tree_map/libpmemobj.j

cd ${PM_ROOT}
patch -R Makefile < ${TEST_ROOT}/patch/MakefilePatch/libpmemobj.patch
sed -i '1,2d' *.c

cd map
patch -R Makefile < ${TEST_ROOT}/patch/MakefilePatch/libpmemobj_map.patch
sed -i '1,2d' *.c

cd ../tree_map
patch -R Makefile < ${TEST_ROOT}/patch/MakefilePatch/libpmemobj_tree_map.patch
sed -i '1,2d' *.c

cd ${PM_ROOT}

patch -R ${WORKLOAD_LOC} < ${PATCH_LOC}