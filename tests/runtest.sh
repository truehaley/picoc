#!/bin/bash

if [ $# -ne 2 ];
then
    echo "Usage:"
    echo "$0 <picoc_executable> <test_file>"
    exit 1
fi

picoc=$1
testfile=$2
testname=$(basename $testfile)  # strip path
testname=${testname%.*}         # strip file extension
expect="${testfile%.*}.expect"
output="${testname}.output"

echo Test: ${testname}...
if [ "x`echo ${testname} | grep args`" != "x" ]
then
	$picoc ${testfile} - arg1 arg2 arg3 arg4 2>&1 >${output}
elif [ "x`echo ${testname} | grep script`" != "x" ]
then
	$picoc -s ${testfile} 2>&1 >${output}
else
	$picoc ${testfile} 2>&1 >${output}
fi
if [ "x`diff -qb ${expect} ${output}`" != "x" ]
then
	echo "error in test ${test}"
	diff -u ${expect} ${output}
	rm -f ${output}
	exit 1
fi
rm -f ${output}
exit 0
