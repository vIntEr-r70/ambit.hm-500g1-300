#!/bin/env bash

SPWD=`pwd`

export LIAEM_RO_PATH=$SPWD/hm-500g1-300
export LIAEM_RW_PATH=$SPWD/home-root

cmake --build ./build -j8

cd ./system/etc/cnc-ctrl
$SPWD/build/src/gui/gui
# gdb $SPWD/build/src/gui/gui
