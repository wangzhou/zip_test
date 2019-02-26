#!/bin/bash
#
# This is a simple script to help to test zip pass through

# load zip/qm driver in host before running cmd below

echo 1 > /sys/devices/pci0000:74/0000:74:00.0/0000:75:00.0/sriov_numvfs
echo 0000:75:00.1 > /sys/bus/pci/drivers/hisi_zip/unbind
echo vfio-pci > /sys/devices/pci0000:74/0000:74:00.0/0000:75:00.1/driver_override
echo 0000:75:00.1 > /sys/bus/pci/drivers_probe

# You may need to change correct patch about qemu-system-aarch64
# And make sure there are Image and guestfs.cpio.gz
qemu-system-aarch64 -machine virt,gic_version=3 -enable-kvm -cpu host -m 1024 \
   -kernel ./Image -initrd ./guestfs.cpio.gz -nographic -append               \
   "rdinit=init console=ttyAMA0 earlycon=pl011,0x9000000 kpti=off"            \
   -device vfio-pci,host=0000:75:00.1

