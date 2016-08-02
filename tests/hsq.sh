#!/bin/bash

# hsq.sh
# This HSQ test will decompress, compress and check if newly created HSQ file
# differs from original HSQ.
# Copyright (C) 2016 Jan Havran

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

HSQ_DIR=./
if [[ $# == "1" ]]
then
	HSQ_DIR=$1
fi

TEST_DIR=`mktemp -d /tmp/HSQ-XXXX` || ( echo "Cannot create temporary folder"; exit 1 )

passed=0
total=0

UTILS_DIR=$(dirname "${BASH_SOURCE[0]}")/../utils
for hsq_file in $HSQ_DIR/*.HSQ
do
	echo -n $(basename $hsq_file)
	$UTILS_DIR/hsq -d $hsq_file -o $TEST_DIR/TEST.BIN > /dev/null 2> /dev/null
	$UTILS_DIR/hsq -c $TEST_DIR/TEST.BIN -o $TEST_DIR/TEST.HSQ > /dev/null 2> /dev/null

	if cmp $hsq_file $TEST_DIR/TEST.HSQ > /dev/null 2>/dev/null
	then
		printf "\t[${GREEN}ok${NC}]\n"
		passed=$(($passed + 1))
	else
		printf "\t[${RED}failed${NC}]\n"
	fi
	total=$((total + 1))
done

echo "Passed: $passed/$total"
rm -rf $TEST_DIR

