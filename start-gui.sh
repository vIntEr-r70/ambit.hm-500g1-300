export LIAEM_RO_PATH=$PWD/system/usr/local/share/machine-types/cnc-ctrl-2
export LIAEM_RW_PATH=$PWD/home-root

cmake --build ./src/Gui/build -j8 

SPWD=`pwd`
cd ./system/etc/cnc-ctrl
$SPWD/src/Gui/build/Gui
