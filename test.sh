#!/bin/sh

clear
rm -f R.* >/dev/null 2>&1
make
./gendata 1000 5 >test_data.txt
./gendata 100 5 >>test_data.txt
./gendata 100 5 30 >>test_data.txt
./gendata 20 5 >>test_data.txt
./create R 5 2 "0,1:1,1:2,1:3,1:4,1" >/dev/null
cat test_data.txt | ./insert R >/dev/null
if (test `./select R 1,?,?,?,? | wc -l` -eq `egrep '^1,' test_data.txt | wc -l`)
then
	echo "./select R 1,?,?,?,? -- good"
else
	echo "./select R 1,?,?,?,? -- not match"
fi

if (test `./select R 9,?,?,?,? | wc -l` -eq `egrep '^9,' test_data.txt | wc -l`)
then
	echo "./select R 9,?,?,?,? -- good"
else
	echo "./select R 9,?,?,?,? -- not match"
fi

if (test `./select R 103,?,?,?,? | wc -l` -eq `egrep '^103,' test_data.txt | wc -l`)
then
	echo "./select R 103,?,?,?,? -- good"
else
	echo "./select R 103,?,?,?,? -- not match"
fi

if (test `./select R ?,book,?,?,? | wc -l` -eq `egrep '^[^,]*,book,' test_data.txt | wc -l`)
then
	echo "./select R ?,book,?,?,? -- good"
else
	echo "./select R ?,book,?,?,? -- not match"
fi

if (test `./select R ?,?,?,?,book | wc -l` -eq `egrep ',book$' test_data.txt | wc -l`)
then
	echo "./select R ?,?,?,?,book -- good"
else
	echo "./select R ?,?,?,?,book -- not match"
fi

if (test `./select R ?,?,?,?,? | wc -l` -eq `egrep '.' test_data.txt | wc -l`)
then
	echo "./select R ?,?,?,?,? -- good"
else
	echo "./select R ?,?,?,?,? -- not match"
fi

if (test `./select R ?,map,?,?,vacuum | wc -l` -eq `egrep '^.*?,map,.*?,.*?,vacuum$' test_data.txt | wc -l`)
then
	echo "./select R ?,map,?,?,vacuum -- good"
else
	echo "./select R ?,map,?,?,vacuum -- not match"
fi

if (test `./select R 20,gate,rocket,dress,horoscope | wc -l` -eq `egrep '^20,gate,rocket,dress,horoscope$' test_data.txt | wc -l`)
then
	echo "./select R 20,gate,rocket,dress,horoscope -- good"
else
	echo "./select R 20,gate,rocket,dress,horoscope -- not match"
fi


