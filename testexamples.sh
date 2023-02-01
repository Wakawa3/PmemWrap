PM_ROOT=${HOME}/pmdk_orig2/src/examples/libpmemobj

TEST_ROOT=${HOME}/PmemWrap

MEMCPY_TYPE=NO
MEMCPY_TYPE=$1

#WORKLOAD=btree
WORKLOAD=$2
PATCH=$3

if [[ ${WORKLOAD} =~ ^(btree|rbtree|ctree)$ ]]; then
	if [[ ${PATCH} != "" && ${PATCH} != "hash" ]]; then
        WORKLOAD_LOC=${PM_ROOT}/tree_map/${WORKLOAD}_map.c
	fi
elif [[ ${WORKLOAD} =~ ^(hashmap_atomic|hashmap_tx)$ ]]; then
	if [[ ${PATCH} != "" && ${PATCH} != "hash" ]]; then
        WORKLOAD_LOC=${PM_ROOT}/hashmap/${WORKLOAD}.c
	fi
fi

PATCH_LOC=${TEST_ROOT}/patch/${WORKLOAD}_${PATCH}.patch

OUT_LOC=${TEST_ROOT}/outputs

PMIMAGE=/mnt/pmem0/${WORKLOAD}_testex
COPYFILE=${PMIMAGE}_flushed

if [[ ${PATCH} != "nobug" ]]; then
    patch ${WORKLOAD_LOC} < ${PATCH_LOC}
fi

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

cd ../hashmap
patch Makefile < ${TEST_ROOT}/patch/MakefilePatch/libpmemobj_hashmap.patch
sed -i '1s/^/#include "libpmemobj.h"\n/' *.c
sed -i '1s/^/#include "libpmem.h"\n/' *.c

cd ${PM_ROOT}

cp ${TEST_ROOT}/libpmem.h ${PM_ROOT}
cp ${TEST_ROOT}/libpmem.h ${PM_ROOT}/map
cp ${TEST_ROOT}/libpmem.h ${PM_ROOT}/tree_map
cp ${TEST_ROOT}/libpmem.h ${PM_ROOT}/hashmap
cp ${TEST_ROOT}/libpmemobj.h ${PM_ROOT}
cp ${TEST_ROOT}/libpmemobj.h ${PM_ROOT}/map
cp ${TEST_ROOT}/libpmemobj.h ${PM_ROOT}/tree_map
cp ${TEST_ROOT}/libpmemobj.h ${PM_ROOT}/hashmap

make clean -j$(nproc)

make -j$(nproc)

export PMEMWRAP_MULTITHREAD=SINGLE

export PMEMWRAP_ABORT=0
export PMEMWRAP_WRITECOUNTFILE=YES
export PMEMWRAP_MEMCPY=NO_MEMCPY
${PM_ROOT}/map/data_store ${WORKLOAD} ${PMIMAGE} 200 > /dev/null
export PMEMWRAP_WRITECOUNTFILE=ADD
export PMEMWRAP_ABORTCOUNT_LOOP=20

# ABORT_TEXT=${OUT_LOC}/${BIN}_${PMEMWRAP_MEMCPY}_abort.txt
# ERROR_TEXT=${OUT_LOC}/${BIN}_${PMEMWRAP_MEMCPY}_error.txt

ABORT_TEXT=${OUT_LOC}/${WORKLOAD}_${PATCH}_${MEMCPY_TYPE}_abort.txt
ERROR_TEXT=${OUT_LOC}/${WORKLOAD}_${PATCH}_${MEMCPY_TYPE}_error.txt

# echo "" > ${OUT_LOC}/${WORKLOAD}_${PATCH}_abort.txt
# echo "" > ${OUT_LOC}/${WORKLOAD}_${PATCH}_error.txt

echo "" > ${ABORT_TEXT}
echo "" > ${ERROR_TEXT}
for i in `seq 100`
do
    echo "${i}" >> ${ABORT_TEXT}
    echo "${i}" >> ${ERROR_TEXT}
    export PMEMWRAP_ABORT=1
    export PMEMWRAP_SEED=${i}
    export PMEMWRAP_MEMCPY=${MEMCPY_TYPE}_MEMCPY
    ${PM_ROOT}/map/data_store ${WORKLOAD} ${PMIMAGE} 200 > /dev/null 2>> ${ABORT_TEXT}
    ${TEST_ROOT}/PmemWrap_memcpy.out ${PMIMAGE} ${COPYFILE}

    export PMEMWRAP_ABORT=0
    export PMEMWRAP_MEMCPY=NO_MEMCPY
    timeout -k 1 20 bash -c "${PM_ROOT}/map/data_store ${WORKLOAD} ${PMIMAGE} 200 > /dev/null 2>> ${ERROR_TEXT}" 2>> ${ABORT_TEXT}
    echo "timeout $?" >> ${ABORT_TEXT}
    rm ${PMIMAGE} ${COPYFILE}
    echo "" >> ${ABORT_TEXT}
    echo "" >> ${ERROR_TEXT}
done

#rm countfile.txt

make clean -j$(nproc)

rm  ${PM_ROOT}/libpmem.h
rm  ${PM_ROOT}/map/libpmem.h
rm  ${PM_ROOT}/tree_map/libpmem.h
rm  ${PM_ROOT}/hashmap/libpmem.h
rm  ${PM_ROOT}/libpmemobj.h
rm  ${PM_ROOT}/map/libpmemobj.h
rm  ${PM_ROOT}/tree_map/libpmemobj.h
rm  ${PM_ROOT}/hashmap/libpmemobj.h

cd ${PM_ROOT}
patch -R Makefile < ${TEST_ROOT}/patch/MakefilePatch/libpmemobj.patch
sed -i '1,2d' *.c

cd map
patch -R Makefile < ${TEST_ROOT}/patch/MakefilePatch/libpmemobj_map.patch
sed -i '1,2d' *.c

cd ../tree_map
patch -R Makefile < ${TEST_ROOT}/patch/MakefilePatch/libpmemobj_tree_map.patch
sed -i '1,2d' *.c

cd ../hashmap
patch -R Makefile < ${TEST_ROOT}/patch/MakefilePatch/libpmemobj_hashmap.patch
sed -i '1,2d' *.c

cd ${PM_ROOT}

if [[ ${PATCH} != "nobug" ]]; then
    patch -R ${WORKLOAD_LOC} < ${PATCH_LOC}
fi