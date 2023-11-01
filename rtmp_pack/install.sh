#!/bin/sh

APP=loto_rtmp_402
WORK_DIR=/root/WaController
# echo "WORK_DIR=$WORK_DIR"
KIT_DIR=$(pwd)
# echo "KIT_DIR=$KIT_DIR"

KIT_WORK_DIR=$KIT_DIR/root/WaController
KIT_CONF_DIR=$KIT_DIR/etc

echo "===== 1. work folder ====="
mv $KIT_WORK_DIR/$APP $WORK_DIR
chmod 777 $WORK_DIR/$APP
mv $KIT_WORK_DIR/push.conf $WORK_DIR
mv $KIT_WORK_DIR/update.sh $WORK_DIR

if [ -d "$WORK_DIR/res" ]; then
    rm -rf $WORK_DIR/res
    mv $KIT_WORK_DIR/res/ $WORK_DIR
else
    mv $KIT_WORK_DIR/res/ $WORK_DIR
fi

if [ -d "$WORK_DIR/scripts" ]; then
    rm -rf $WORK_DIR/scripts
    mv $KIT_WORK_DIR/scripts/ $WORK_DIR
    chmod 777 $WORK_DIR/scripts/*
else
    mv $KIT_WORK_DIR/scripts/ $WORK_DIR
    chmod 777 $WORK_DIR/scripts/*
fi

echo "===== 2. etc ====="
mv $KIT_CONF_DIR/resolv.conf /etc

echo "===== 3. init.d ====="
mv $KIT_CONF_DIR/init.d/* /etc/init.d/
chmod 777 /etc/init.d/*
rm $KIT_CONF_DIR/init.d/ -rf

echo "===== install complete ====="
echo "Please modify some essential files"
echo "1. rcS:           the value of argument 'osmem' of program 'load3516dv300'"
echo "2. loto_conf.sh:  IP address and so on"
echo "3. push.conf:   device_num, push_url, requested_url, profile and so on"
