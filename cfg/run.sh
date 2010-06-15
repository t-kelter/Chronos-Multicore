#!/bin/bash

declare main=/home/kelter/chronos/multicore/iterative/cfg/main
declare bench_path=/home/kelter/chronos/multicore/iterative/benchmarks

# get simulation time and ilp file for a benchmark
run_prog()
{
    cd $1
    arg=`head -1 $1.arg`
    $main $1 $arg
    cd ..
}

# get code size for analysis
code_size()
{
    cd $1
    arg=`head -1 $1.arg`
    cat $1.arg | awk '{printf("0x%s 0x%s\n", $1, $2)}' | awk --non-decimal-data '{print $2-$1}'
    cd ..
}

declare PREV_PWD=$(pwd)
cd $bench_path

if [ "$1" == "" ]; then
  for i in * ; do
    if [ -d $i ]; then
      echo $i
      echo "------------------------------------------------------------------"
      run_prog $i
      #code_size $i
      echo " "
    fi
  done
else
  run_prog $1
fi

cd $PREV_PWD
