#!/bin/env bash

SPWD=`pwd`

export LIAEM_RO_PATH=$SPWD/system/usr/local/share/machine-types/cnc-ctrl-2
export LIAEM_RW_PATH=$SPWD/home-root

cmake --build ./build -j8

cd ./system/etc/cnc-ctrl
$SPWD/build/src/gui/gui
