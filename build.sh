#!/bin/bash

# toolchain path
if [ -d ../openwrt*/toolchain-mipsel*/bin ]; then
	Toolchain=$(cd ../openwrt*/toolchain-mipsel*/bin; pwd)'/mipsel-openwrt-linux-'
elif [ -d ../openwrt*/staging_dir/toolchain-mipsel*/bin ]; then
	Toolchain=$(cd ../openwrt*/staging_dir/toolchain-mipsel*/bin; pwd)'/mipsel-openwrt-linux-'
else
	echo "can not find the toolchain"
	exit
fi
Staging=${Toolchain%/toolchain-*}

echo "CROSS_COMPILE=${Toolchain}"
echo "STAGING_DIR=${Toolchain%/toolchain-*}"
cd $(dirname "$0")

# add board name here
Boards=( \
	h3c_tx1801-plus \
	h3c_tx1801-plus-nmbm \
	raisecom_msg1500-x00 \
	raisecom_msg1500-x00-nmbm \
	sim_simax1800t \
	sim_simax1800t-nmbm \
	asus_rt-ac1200gu \
	dlink_dir-878-a1 \
	)

if [ ! -d "./bin" ]; then
	mkdir ./bin
fi

for Board in ${Boards[@]}
do
	echo "Build ${Board}"
	make distclean
	make ${Board}_defconfig
	make CROSS_COMPILE=${Toolchain} STAGING_DIR=${Staging}
	if [ ! -d "./bin/$Board" ]; then
		mkdir ./bin/$Board
	else
		rm ./bin/$Board/*
	fi
	mv ./u-boot.img ./bin/$Board/u-boot.img
	mv ./u-boot-mt7621.bin ./bin/$Board/u-boot-mt7621.bin
done

tar -acvf u-boot-mt7621.tar.xz ./bin/*
