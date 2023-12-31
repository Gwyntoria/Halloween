#!/bin/sh

list_process="loto_conf.sh loto_rtmp_402"

for process in $list_process; do
    pid=$(ps w | grep "$process" | grep -v "grep" | awk '{print $1}')

    if [ -n "$pid" ]; then
        echo "Killing process $process, pid=$pid"
        kill "$pid"
    else
        echo "Process $process is not running"
    fi
done
