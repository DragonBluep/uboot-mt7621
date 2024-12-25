#!/bin/bash

# toolchain path
Toolchain=$(cd ../openwrt*/toolchain-mipsel*/bin; pwd)'/mipsel-openwrt-linux-'
Staging=${Toolchain%/toolchain-*}

echo "CROSS_COMPILE=${Toolchain}"
echo "STAGING_DIR=${Toolchain%/toolchain-*}"
cd $(dirname "$0")

# arguments:
# $1	string: flash type
# $2	string: partition table
# $3	string: kernel offset
# $4	number: reset pin
# $5	number: sysled gpio
# $6	number: cpu frequency
# $7	number: ram frequency
# $8	string: ddr param
# $9	string: baud rate

echo "Parse flash type: $1"
# simple check if partition table is valid
if [ -z $( echo -n "$2" | grep '),-(firmware)') ]; then
	echo "Invalid mtd partition table!"
	exit 1
fi
DEFCONFIG="configs/mt7621_build_defconfig"
if [ "$1" = 'NOR' ]; then
	cp configs/mt7621_nor_template_defconfig ${DEFCONFIG}
	echo -e "CONFIG_MTDPARTS_DEFAULT=\"mtdparts=raspi:$2\"" >> ${DEFCONFIG}
elif [ "$1" = 'NAND-AX' ]; then
	cp configs/mt7621_nand_template_defconfig ${DEFCONFIG}
	echo -e "CONFIG_MTDPARTS_DEFAULT=\"mtdparts=nand0:$2\"" >> ${DEFCONFIG}
 elif [ "$1" = 'NAND-NMBM' ]; then
	cp configs/mt7621_nmbm_template_defconfig ${DEFCONFIG}
	echo -e "CONFIG_MTDPARTS_DEFAULT=\"mtdparts=nmbm0:$2\"" >> ${DEFCONFIG}
else
	cp configs/mt7621_nmbm_template_defconfig ${DEFCONFIG}
	echo -e "CONFIG_MTDPARTS_DEFAULT=\"mtdparts=nmbm0:$2\"" >> ${DEFCONFIG}
fi
echo "set partition table: $2"

echo "set kernel offset: $3"
if [ "$1" = 'NOR' ]; then
	echo "CONFIG_DEFAULT_NOR_KERNEL_OFFSET=$3" >> ${DEFCONFIG}
else
	echo "CONFIG_DEFAULT_NAND_KERNEL_OFFSET=$3" >> ${DEFCONFIG}
fi

echo -e "#ifndef __CONFIG_MT7621_RESET_LED\n#define __CONFIG_MT7621_RESET_LED" \
	>> ./include/configs/mt7621-common.h
if [ "$4" -ge 0 -a "$4" -le 48 ]; then
	echo "set reset button pin: $4"
	echo "CONFIG_FAILSAFE_ON_BUTTON=y" >> ${DEFCONFIG}
	echo "#define MT7621_BUTTON_RESET $4" >> ./include/configs/mt7621-common.h
else
	echo "Reset button is disabled!"
fi

if [ "$5" -ge 0 -a "$5" -le 48 ]; then
	echo "set system led pin: $5"
	echo "#define MT7621_LED_STATUS1 $5" >> ./include/configs/mt7621-common.h
else
	echo "System LED is disabled!"
fi
echo "#endif" >> ./include/configs/mt7621-common.h

if [ "$6" -ge 400 -a "$6" -le 1200 ]; then
	echo "set CPU frequency: $6 MHz"
	echo "CONFIG_MT7621_CPU_FREQ_LEGACY=$6" >> ${DEFCONFIG}
else
	echo "Invalid CPU Frequency!"
	exit 1
fi

echo "set DRAM frequency: $7 MT/s"
echo "CONFIG_MT7621_DRAM_FREQ_$7_LEGACY=y" >> ${DEFCONFIG}

echo "Parse DDR init parameters: $8"
case "$8" in
DDR2-64MiB)
	echo "CONFIG_MT7621_DRAM_DDR2_512M_LEGACY=y" >> ${DEFCONFIG}
	;;
DDR2-128MiB)
	echo "CONFIG_MT7621_DRAM_DDR2_1024M_LEGACY=y" >> ${DEFCONFIG}
	;;
DDR2-W9751G6KB-64MiB-1066MHz)
	echo "CONFIG_MT7621_DRAM_DDR2_512M_W9751G6KB_A02_1066MHZ_LEGACY=y" >> ${DEFCONFIG}
	;;
DDR2-W971GG6KB25-128MiB-800MHz)
	echo "CONFIG_MT7621_DRAM_DDR2_1024M_W971GG6KB25_800MHZ_LEGACY=y" >> ${DEFCONFIG}
	;;
DDR2-W971GG6KB18-128MiB-1066MHz)
	echo "CONFIG_MT7621_DRAM_DDR2_1024M_W971GG6KB18_1066MHZ_LEGACY=y" >> ${DEFCONFIG}
	;;
DDR3-128MiB)
	echo "CONFIG_MT7621_DRAM_DDR3_1024M_LEGACY=y" >> ${DEFCONFIG}
	;;
DDR3-256MiB)
	echo "CONFIG_MT7621_DRAM_DDR3_2048M_LEGACY=y" >> ${DEFCONFIG}
	;;
DDR3-512MiB)
	echo "CONFIG_MT7621_DRAM_DDR3_4096M_LEGACY=y" >> ${DEFCONFIG}
	if [ -n $(cat ${DEFCONFIG} | grep MT7621_DRAM_FREQ_1200_LEGACY) ]; then
		echo "The max DRAM speed for 512 MiB RAM is 1066 MT/s"
		sed -i 's/MT7621_DRAM_FREQ_1200_LEGACY/MT7621_DRAM_FREQ_1066_LEGACY/' ${DEFCONFIG}
	fi
	;;
DDR3-128MiB-KGD)
	echo "CONFIG_MT7621_DRAM_DDR3_1024M_KGD_LEGACY=y" >> ${DEFCONFIG}
	;;
esac

echo "Set baud rate: $9"
if [ "$9" = '57600' ]; then
	echo "CONFIG_BAUDRATE=57600" >> ${DEFCONFIG}
else
	echo "CONFIG_BAUDRATE=115200" >> ${DEFCONFIG}
fi

make mt7621_build_defconfig
make CROSS_COMPILE=${Toolchain} STAGING_DIR=${Staging}
make savedefconfig
mkdir archive
cat defconfig > archive/mt7621_defconfig
mv u-boot-mt7621.bin archive/
mv u-boot.img archive/
