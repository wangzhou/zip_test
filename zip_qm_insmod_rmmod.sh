#!/bin/bash
#
# This is a simple script to help to test zip/qm insmod and rmmod.


for i in `seq 100`
do
	insmod qm.ko
	rmmod qm.ko
done

for j in `seq 100`
do
	insmod qm.ko
	insmod hisi_zip.ko
	rmmod hisi_zip.ko
	rmmod qm.ko
done
