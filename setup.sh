#!/bin/sh

clear
rm -f R.* >/dev/null 2>&1
rm res.res >/dev/null 2>&1
make
./create R 5 2 "0,1:1,1:2,1:3,1:4,1" #>/dev/null
./gendata 1255 5 | ./insert R | 
while read line
do
	echo "$line"
	echo "$line" | sed -E 's/^.* [01]{2}//' >>res.res
done
./operate.pl | sed 's/^.* //' > res.res1
./stats R | egrep '^\[' | sed -E 's/^[^,]*?,//' | sed -E 's/,.*$//' > res.res2
diff res.res1 res.res2

