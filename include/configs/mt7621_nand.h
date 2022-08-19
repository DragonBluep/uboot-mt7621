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

/* Environment */
#define CONFIG_ENV_SIZE			0x20000
#define CONFIG_ENV_OFFSET		0x80000
#define CONFIG_ENV_SECT_SIZE		0x20000

/* Network */
#define CONFIG_IPADDR			192.168.0.1
#define CONFIG_SERVERIP			192.168.0.101
#define CONFIG_NETMASK			255.255.255.0

#endif  /* __CONFIG_H */
