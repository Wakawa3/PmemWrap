TEST_ROOT=${PMEMWRAP_HOME}/test_targets/CCEH/CCEH-PMDK

mkdir -p outputs_safepm
mkdir -p bin

OUT_LOC=${TEST_ROOT}/outputs_safepm
TIME=`date +%m%d%H%M%S`

PMIMAGE=/mnt/pmem0/test_cceh
COPYFILE=${PMIMAGE}_flushed
BIN=single_threaded_cceh

TEST_MEMCPY_TYPE=RAND_MEMCPY

if [[ ${BIN} =~ ^(multi_threaded_cceh|multi_threaded_cceh_CoW)$ ]]; then
    THREAD_OPT=16
fi

cd ${TEST_ROOT}
make clean -j$(nproc)
make -j$(nproc)

rm -f ${PMIMAGE} ${COPYFILE}
rm -f countfile.txt countfile_plus.txt

export PMEMWRAP_ABORT=0
export PMEMWRAP_WRITECOUNTFILE=YES
export PMEMWRAP_MEMCPY=NO_MEMCPY
${TEST_ROOT}/bin/${BIN} ${PMIMAGE} 20 ${THREAD_OPT}
export PMEMWRAP_WRITECOUNTFILE=ADD
export PMEMWRAP_ABORTCOUNT_LOOP=15

OUTPUT_TEXT=${OUT_LOC}/${BIN}_${TEST_MEMCPY_TYPE}_${TIME}_output.txt
ERROR_TEXT=${OUT_LOC}/${BIN}_${TEST_MEMCPY_TYPE}_${TIME}_error.txt
SEMA_TEXT=${OUT_LOC}/${BIN}_${TEST_MEMCPY_TYPE}_${TIME}_sema.txt

echo "" > ${OUTPUT_TEXT}
echo "" > ${ERROR_TEXT}

for i in `seq 30`
do
    echo "__test_${i}__"
    echo "citest_${i}" >> ${OUTPUT_TEXT}
    echo "citest_${i}" >> ${ERROR_TEXT}
    echo "pre-failure" >> ${OUTPUT_TEXT}
    echo "pre-failure" >> ${ERROR_TEXT}
    export PMEMWRAP_ABORT=1
    export PMEMWRAP_SEED=${i}
    export PMEMWRAP_MEMCPY=${TEST_MEMCPY_TYPE}
    ${TEST_ROOT}/bin/${BIN} ${PMIMAGE} 20 ${THREAD_OPT} >> ${OUTPUT_TEXT} 2>> ${ERROR_TEXT}
    ${PMEMWRAP_HOME}/bin/PmemWrap_memcpy.out ${PMIMAGE} ${COPYFILE}

    echo "" >> ${OUTPUT_TEXT}
    echo "" >> ${ERROR_TEXT}
    echo "post_failure" >> ${OUTPUT_TEXT}
    echo "post_failure" >> ${ERROR_TEXT}
    export PMEMWRAP_ABORT=0
    export PMEMWRAP_MEMCPY=NO_MEMCPY
    timeout -k 1 30 bash -c "${TEST_ROOT}/bin/${BIN} ${PMIMAGE} 20 ${THREAD_OPT} >> ${OUTPUT_TEXT} 2>> ${ERROR_TEXT}" 2>>${ERROR_TEXT}
    echo "" >> ${OUTPUT_TEXT}
    echo "" >> ${ERROR_TEXT}
    echo "timeout return $?" >> ${ERROR_TEXT}
    rm -f ${PMIMAGE} ${COPYFILE}
    
    echo -e "\n" >> ${OUTPUT_TEXT}
    echo -e "\n" >> ${ERROR_TEXT}

    rm -f ${SEMA_TEXT}
    python3 ${PMEMWRAP_HOME}/sema_state_generate.py ${ERROR_TEXT} > ${SEMA_TEXT}
    cp ${SEMA_TEXT} ${SEMA_TEXT}_cp
done