#!/bin/sh

MODULE="scull"
DEVICE="scull"
MODE="664"
GROUP="root"

# Insert module
/sbin/insmod ./${MODULE}.ko "$@" || exit 1

# Remove stale device node
rm -f /dev/${DEVICE}0

# Get major number from /proc/devices
major=$(awk -v module="$MODULE" '$2 == module {print $1}' /proc/devices)

if [ -z "$major" ]; then
    echo "Error: could not find major number for $MODULE in /proc/devices"
    exit 1
fi

# Create device node
mknod /dev/${DEVICE}0 c $major 0

# Set group and permissions
chgrp $GROUP /dev/${DEVICE}0
chmod $MODE /dev/${DEVICE}0

echo "Created /dev/${DEVICE}0 with major=$major"
