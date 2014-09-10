#!/bin/bash
USER="root"
PASS="proteusys"

for var in "$@"
do
    sshpass -p $PASS ssh $USER@10.0.2.$var './enable.sh'
    echo 10.0.2.$var enable ok
done
