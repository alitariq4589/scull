#!/bin/sh

MODULE="scull"
DEVICE="scull"

mode="664"

/sbin/insmod ./${MODULE}.ko $* || exit 1

rm -rf "/dev/${DEVICE}0"

major=$(awk "\\$2==\"$module\" {print \\$1}" /proc/devices)

mknod /dev/${DEVICE} c $major 0

group="root"

chgrp $group /dev/${DEVICE}0
chmod $mode /dev/${DEVICE}0
