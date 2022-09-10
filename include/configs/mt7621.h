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
#define CONFIG_SPL_MAX_SIZE		0x10000
#define CONFIG_SPI_ADDR			0xbfc00000
#define CONFIG_SYS_UBOOT_START		CONFIG_SYS_TEXT_BASE

/* TPL */
#define CONFIG_TPL_BSS_START_ADDR	CONFIG_SPL_BSS_START_ADDR
#define CONFIG_TPL_BSS_MAX_SIZE		CONFIG_SPL_BSS_MAX_SIZE

/* Serial Port */
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_SERIAL_SUPPORT)
#define CONFIG_SYS_NS16550_COM1		0xbe000c00
#endif

/* MMC */
#define MMC_SUPPORTS_TUNING

/* Network */
#define CONFIG_IPADDR			192.168.1.1
#define CONFIG_SERVERIP			192.168.1.2
#define CONFIG_NETMASK			255.255.255.0

#endif  /* __CONFIG_H */
