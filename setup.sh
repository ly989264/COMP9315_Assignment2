#!/bin/sh

clear
rm -f R.* >/dev/null 2>&1
rm res.res >/dev/null 2>&1
make
./create R 5 2 "0,1:1,1:2,1:3,1:4,1" #>/dev/null
cat data0.txt | ./insert R | 
while read line
do
	echo "$line"
	echo "$line" | sed -E 's/^.* [01]{3}//' >>res.res
done
./operate.pl
./stats R


