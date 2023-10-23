#!/bin/sh

BASE=${PWD}
HOST=arm-himix200-linux
HI_CC=${HOST}-gcc
HI_CXX=${HOST}-g++
INSTALL_PATH=${BASE}/CROSS_INSTALL

# echo "#################### UNINSTALL ####################"
# make uninstall
# make clean

autoreconf -fiv

echo "#################### CONFIGURE ####################"
./configure \
	CC=${HI_CC} \
	CXX=${HI_CXX} \
	--host=${HOST} \
	--prefix=${INSTALL_PATH} \
	CFLAGS="-O3 -mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4" \
	HAVE_ARM_NEON_INTR=1

echo "####################   MAKE    ####################"
make

echo "####################  INSTALL  ####################"
make install

# echo "####################   CHECK   ####################"
# make check
