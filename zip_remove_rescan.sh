#!/bin/bash

echo 1 > /sys/devices/pci0000:74/0000:74:00.0/0000:75:00.0/remove
echo 1 > /sys/devices/pci0000:74/0000:74:00.0/rescan
