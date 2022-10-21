#!/bin/sh

export DIFF=diff
#export DIFF=delta

make submission
make bonus

./troons testcases/example.in > example.out
./troons testcases/sample1.in > sample1.out
./troons testcases/sample2.in > sample2.out

$DIFF example.out testcases/example.output
$DIFF sample1.out testcases/sample1.output
$DIFF sample2.out testcases/sample2.output

./troons_bonus testcases/example.in > example.out
./troons_bonus testcases/sample1.in > sample1.out
./troons_bonus testcases/sample2.in > sample2.out

$DIFF example.out testcases/example.output
$DIFF sample1.out testcases/sample1.output
$DIFF sample2.out testcases/sample2.output
