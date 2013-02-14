#!/bin/bash

. ./_export_env.sh                              # setting environment variables

export TET_SUITE_ROOT=`pwd`
FILE_NAME_EXTENSION=`date +%s`

RESULT_DIR=results
HTML_RESULT=$RESULT_DIR/build-tar-result-$FILE_NAME_EXTENSION.html
JOURNAL_RESULT=$RESULT_DIR/build-tar-result-$FILE_NAME_EXTENSION.journal

mkdir -p $RESULT_DIR

echo -e "#ifndef __CONFIG_H__\n""#define __CONFIG_H__\n""#define TC_PREFIX \"`pwd`/\"\n""#endif\n" > config.h

edje_cc edje/data/edje_test.edc -id edje/data/images edje/data/edje_test.edj

tcc -c -p ./
tcc -b -j $JOURNAL_RESULT -p ./
grw -c 7 -f chtml -o $HTML_RESULT $JOURNAL_RESULT

rm config.h

