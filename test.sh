#!/bin/sh

clear
rm -f R.* >/dev/null 2>&1
make
./gendata 1000 5 >test_data.txt
./gendata 100 5 >>test_data.txt
./gendata 100 5 30 >>test_data.txt
./gendata 20 5 >>test_data.txt
./gendata 300 5 103 >>test_data.txt
./gendata 300 5 230 >>test_data.txt
./create R 5 2 "0,1:1,1:2,1:3,1:4,1" >/dev/null
cat test_data.txt | ./insert R >/dev/null
cat test_source |
while read line
do
	query=`echo "$line" | sed 's/ .*$//'`
	regex=`echo "$line" | sed 's/^.* //'`
	query_result=`./select R "$query" | sort | tr '\n' ' '`
	regex_result=`egrep "$regex" test_data.txt | sort | tr '\n' ' '` 
	if (test "$query_result" = "$regex_result")
	then
		echo "./select R $query --good"
	else
		echo "./select R $query --not match"
	fi
done
./stats R >test_res
