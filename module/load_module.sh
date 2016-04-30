#!/bin/sh
module="test"
device="nothing"
mode="664"

#$* is all the script arguments for the module
insmod $module.ko $* || exit 1
 
major=$(awk "\\$2==\"$module\" {print \\$1}" /proc/devices)
echo major=$major
mknod /dev/${devices}0 c $major 0
mknod /dev/${devices}1 c $major 1

group="staff"
grep -q '^staff:' /etc/group || group="wheel"

chgrp $group /dev/${device}[0-3]
chmod $mode /dev/${device}[0-3]
