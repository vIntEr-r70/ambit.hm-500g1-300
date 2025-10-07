#!/bin/sh

source /root/environment

# Удаляем файл, мешающий калибровке экрана
if [ -f /usr/share/X11/xorg.conf.d/40-libinput.conf ];
then
  mount -o remount,rw /
  rm /usr/share/X11/xorg.conf.d/40-libinput.conf
  mount -o remount,ro /
fi

$BIN/gate `cat /root/id.sid` &> /tmp/gate.log &

$BIN/worker &> /tmp/worker.log &

su - root -c xinit

