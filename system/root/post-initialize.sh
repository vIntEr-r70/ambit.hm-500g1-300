#!/bin/bash

# Наше устройство touchscreen
export TSLIB_TSDEVICE=/dev/input/event4
# Файл с результатами калибровки
export TSLIB_CALIBFILE=/home/pointercal

# Если калибровка не проводилась
# запускаем ее выполнение
if [ ! -f $TSLIB_CALIBFILE ];
then
    /usr/bin/ts_calibrate
fi

# $BIN/gate `cat /root/id.sid` &> /tmp/gate.log &

# $BIN/worker &> /tmp/worker.log &

# su - root -c xinit

# source /root/environment

# Удаляем файл, мешающий калибровке экрана
if [ -f /usr/share/X11/xorg.conf.d/40-libinput.conf ];
then
  mount -o remount,rw /
  rm /usr/share/X11/xorg.conf.d/40-libinput.conf
  mount -o remount,ro /
fi

$BIN/gate `cat /root/id.sid` &> /tmp/gate.log &

sleep 1

ip link set dev eth1 up

export LD_LIBRARY_PATH=/usr/local/lib
export SYBUS_IPC_NAME=/tmp/sibus.pipe
export QT_QPA_PLATFORM=linuxfb
export SIBUS_LEVELDB_FILE=/home/sibus-leveldb

#export QT_LOGGING_RULES="qt.qpa.input=true"
export QT_QPA_FB_HIDECURSOR=1
export QT_QPA_GENERIC_PLUGINS="tslib:/dev/input/event10"

ts_uinput -d -v

cd /usr/local/share/gui
/usr/local/bin/gui > /tmp/gui.log &

ethercatctl start
