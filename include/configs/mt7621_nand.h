/* SPDX-License-Identifier:	GPL-2.0+ */
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include "mt7621-common.h"

/* SPL */
#define CONFIG_SPL_MAX_SIZE		0x20000
#define CONFIG_SYS_UBOOT_START		CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_NAND_U_BOOT_OFFS	CONFIG_SPL_MAX_SIZE

/* Serial Port */
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_SERIAL_SUPPORT)
#define CONFIG_SYS_NS16550_COM1		0xbe000c00
#endif
#if defined(CONFIG_ASUS_PRODUCT)
/* NVRAM */
#define NAND_NVRAM_OFFSET            0xe0000   /* NVRAM offset */
#define NAND_NVRAM_SIZE              0x100000  /* NVRAM max size : 128K + 128K(backup) */

/* FACTORY */
#define NAND_RF_OFFSET               0x1e0000
#define MAX_NAND_RF_SIZE             0x100000  /* RF max size : 256K + 256K(backup) */

/* FACTORY2 */
#define NAND_RF_OFFSET_2             0x2e0000
#define MAX_NAND_RF_SIZE_2           0x100000  /* RF max size : 256K + 256K(backup) */

/* KERNEL */
#define NAND_LINUX_OFFSET            0x3e0000
#define MAX_NAND_LINUX_SIZE          0x3200000

/* KERNEL2 */
#define NAND_LINUX_OFFSET_2          0x35e0000
#define MAX_NAND_LINUX_SIZE_2        0x3200000

#define MAX_NAND_PAGE_SIZE           2048
#define MAX_NAND_BLOCK_SIZE          262144

#define BOOTLOADER_VER_OFFSET        0x2ff7a
#define MAC_OFFSET                   0x4
#define MAC_5G_OFFSET                0xA
#define PIN_CODE_OFFSET              0x2ff70
#define COUNTRY_CODE_OFFSET          0x2ff78

#define DUAL_TRX
#define MAX_TRX_IMAGE_SIZE 0x280000
#endif

/* NAND */
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_MAX_CHIPS	1
#define CONFIG_SYS_NAND_ONFI_DETECTION
#define CONFIG_SPL_NAND_IDENT
#define CONFIG_SPL_NAND_BASE

#ifdef CONFIG_ENABLE_NAND_NMBM
#define CONFIG_SPL_NAND_INIT
#define CONFIG_SPL_NAND_DRIVERS
#endif

/* Network */
#define CONFIG_IPADDR			192.168.1.1
#define CONFIG_SERVERIP			192.168.1.70
#define CONFIG_NETMASK			255.255.255.0

#if defined(CONFIG_XD4S)
#define WPS_BTN                 8 
#define RST_BTN                 15
#define SYS_GLED		16 
#define SYS_BLED		13 
#define SYS_RLED		14
#define ASUS_BL			"u-boot-mt7621.bin"
#define ASUS_FW			"XD4S.trx"
#endif 

#if defined(CONFIG_RTAX53U) || defined(CONFIG_RTAX54) 
#define WPS_BTN                 15
#define RST_BTN                 16
#define PWR_LED                 13
#define WIFI_2G_LED             18
#define WIFI_5G_LED             26
#endif 

#ifdef CONFIG_4GAX56
#define WPS_BTN                 4
#define RST_BTN                 8
#define PWR_LED                 14
#define WIFI_2G_LED             13
#define WIFI_5G_LED             16
#define WAN_LED                 15
#define LTE_2G_LED              3
#define LTE_3G_LED              7
#define LTE_4G_LED              10
#define LTE_NO_SIG_LED          0
#endif

#endif  /* __CONFIG_H */
