#/bin/bash
#
# $1: process number
#
# unbind zip driver for zip1(we have zip0 and zip1 in Hi1620 2P system) ahead
#
# this script helps to test wd reset.

set -m

for i in `seq $1`
do
	./test_hisi_zip -g -c 1 -b 128 -q 1 < Image > output_$i &
	pid=$!
	taskset -p 0xffffffff $pid
done

sleep 1

# set a nfe ras irq for zip to trigger zipo controller reset
busybox devmem 0x14110000c 32 0x1

# wait all wd_qs are released
loop=0
available_instance=0
while [ $available_instance != "64" ]
do
	sleep 2
	let loop++
	available_instance=`cat /sys/class/uacce/hisi_zip-0/attrs/available_instances`
	# let's make 1min timeout
	if [ $loop -gt 30 ];then
		echo "available_instance is wrong, $available_instance"
		exit -1
	fi
done

# test if hardware is ok after reset
./test_hisi_zip -g -c 1 -b 4096 -q 1 < Makefile > Makefile_c
./test_hisi_zip -gd -c 1 -b 4096 -q 1 < Makefile_c > Makefile_d

diff Makefile Makefile_d
if [ $? -ne 0 ];then
	echo "hardware can not use"
	exit -2
fi

rm Makefile_c Makefile_d output_*
echo "test is OK"
exit 0
