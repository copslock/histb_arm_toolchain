#!/bin/sh

OSCAM_PATH=`pwd`
# set standalone toolchain path
HISILICON_TOOLCHAIN=$OSCAM_PATH/arm-hisiv200-linux/bin
ANDROID_TOOLCHAIN=$OSCAM_PATH/android9-gcc4.7-toolchain/bin
RELEASE_PATH=$OSCAM_PATH/u4.release

export USE_HISKY=1
export CONF_DIR=/data/oscam

export HISDK_INCLUDE=$OSCAM_PATH/u4sdk/include
export HISDK_LIBRARY=$OSCAM_PATH/u4sdk/lib

export PATH=$ANDROID_TOOLCHAIN:$HISILICON_TOOLCHAIN:$PATH

cd oscam.source
echo $PATH
gcc --version
make clean
make android-arm-hisky -j16 \
		 HISKY_LIB="-L$HISDK_LIBRARY -lhi_msp -lhi_common" \
		 HISKY_FLAGS="-D__ARMEL__ -I$HISDK_INCLUDE -DWITH_HISILICON=1 -DSDKV600 -DSDKV660" \


SVN_REVISION=`./config.sh --oscam-revision`
echo SVN_REVISION:$SVN_REVISION
echo

echo $RELEASE_PATH.........
mkdir -p $RELEASE_PATH
cp -af Distribution/oscam-1.20.sky.$SVN_REVISION $RELEASE_PATH/oscam-1.20
cp -af Distribution/skybox/* $RELEASE_PATH/.
echo

