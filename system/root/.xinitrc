#!/bin/sh

source /root/environment

export QT_QPA_PLATFORM=xcb

xset -dpms s off

FCALIBRATION=/home/root/calibration.sh
if [ ! -f $FCALIBRATION ] || [ ! -s $FCALIBRATION ]; then
  xinput_calibrator --output-type xinput | tail -1 > $FCALIBRATION
else
  sh $FCALIBRATION
fi

cd $CFG_PATH
$BIN/Gui &> /tmp/Gui.log

