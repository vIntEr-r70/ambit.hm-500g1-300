#!/bin/bash

if [ `id -u` -ne 0 ]
  then echo "Please run as root"
  exit
fi

# Запускаем порт сервер
socat -x PTY,raw,echo=0,mode=666,link=/dev/ttySOCAT0 TCP-LISTEN:10010 &> /dev/null &
socat -x PTY,raw,echo=0,mode=666,link=/dev/ttySOCAT1 TCP-LISTEN:10011 &> /dev/null &

sleep 10

# Запускаем порт клиент
socat -x PTY,raw,echo=0,mode=666,link=/dev/ttySOCAT10 tcp:127.0.0.1:10010 &> /dev/null &
socat -x PTY,raw,echo=0,mode=666,link=/dev/ttySOCAT11 tcp:127.0.0.1:10011 &> /dev/null &

echo "Запустили /dev/ttySOCAT0 <-> /dev/ttySOCAT10 эмуляцию"
echo "Запустили /dev/ttySOCAT1 <-> /dev/ttySOCAT11 эмуляцию"

