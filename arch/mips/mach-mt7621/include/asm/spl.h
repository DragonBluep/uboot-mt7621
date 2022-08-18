/* SPDX-License-Identifier:	GPL-2.0+ */
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef	_MT7621_ASM_SPL_H_
#define	_MT7621_ASM_SPL_H_

#ifndef __ASSEMBLY__
enum {
	BOOT_DEVICE_MTK_NOR,
	BOOT_DEVICE_MTK_NAND,
	BOOT_DEVICE_MTK_UART,
	BOOT_DEVICE_NAND,
	BOOT_DEVICE_UART,
	BOOT_DEVICE_NONE
};

struct mtk_spl_rom_cfg {
	uint32_t magic;
	uint32_t size;
	uint32_t align;
};

extern struct mtk_spl_rom_cfg __rom_cfg;

extern ulong get_mtk_image_search_start(void);
extern ulong get_mtk_image_search_end(void);
extern ulong get_mtk_image_search_sector_size(void);
extern void *get_mtk_spl_image_ptr(void *data, size_t size);
#endif /* __ASSEMBLY__ */

#define MTK_ROM_CFG_MAGIC		0x31323637

#define MTK_SPL_ROM_CFG_OFFS		0x40

#endif
