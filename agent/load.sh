module=${1-"./sodero.ko"}

sync
rmmod $module
insmod $module gFace=eth4
dmesg -c
chmod 666 /dev/sodero
