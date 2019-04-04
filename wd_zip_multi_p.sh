#/bin/bash
#
# $1: process number
# $2: input file
# $3: output file

for i in `seq $1`
do
	./test_hisi_zip -g -c 1 -b 4096 -q < $2 > $3 &
done
