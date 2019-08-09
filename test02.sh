#!/bin/sh

rm -f R.*
./create R 3 2 "0,0:1,0:2,0:0,1:1,1:2,1"
./insert R < data1.txt
./select R '1042,?,?' | sort
echo "next"
./select R '?,horoscope,?' | sort
echo "next"
./select R '?,?,shoes' | sort
echo "next"
./select R '?,shoes,?' | sort
echo "next"
./select R '?,chair,shoes' | sort
echo "next"
./select R '?,shoes,chair' | sort
echo "next"
./select R '?,?,?' | wc -l
echo "next"
./select R '42,?,?'
echo "next"
./select R '?,wombat,?'
echo "next"
./select R '?,?,wombat'
echo "next"
./select R '?,shoes,finger'
echo "next"
./select R '1001,chair,shoes'
