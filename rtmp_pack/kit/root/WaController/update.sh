#!/bin/sh

usage() {
    echo "Usage: $1 <tftp server address>"
}

if [ $# -eq 0 ]; then
    usage $0
    exit 1
fi

APP=loto_rtmp_402
WORK_DIR=/root/WaController

# kill processes
$WORK_DIR/scripts/kill_process.sh

sleep 1

# echo "=== Update push.conf ==="
# tftp -g -r push.conf $1

echo "=== Update loto_rtmp ==="
# cp loto_rtmp loto_rtmp_back
tftp -g -r $APP $1

chmod 777 $APP

echo "=== Finish updating loto_rtmp ==="
