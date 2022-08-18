// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <spl.h>
#include <u-boot/crc.h>
#include <linux/sizes.h>
#include <asm/addrspace.h>

char *get_mtk_stage2_image_ptr(char *data, size_t size)
{
	struct mtk_spl_rom_cfg rom_cfg;
	struct image_header hdr;
	u32 old_chksum, chksum;
	u32 off;

	memcpy(&hdr, data, sizeof(hdr));

	if (image_get_magic(&hdr) == IH_MAGIC) {
		old_chksum = image_get_hcrc(&hdr);
		hdr.ih_hcrc = 0;
		chksum = crc32(0, (const u8 *) &hdr, sizeof(hdr));

		if (chksum != old_chksum)
			return NULL;

		return data;
	}

	/* Locate stage2 */
	memcpy(&rom_cfg, data + MTK_SPL_ROM_CFG_OFFS, sizeof(rom_cfg));

	if (rom_cfg.magic != MTK_ROM_CFG_MAGIC)
		return NULL;

	off = rom_cfg.size;

	if (!rom_cfg.align)
		return data + off;

	off = ALIGN(off, rom_cfg.align);

	while (off < size) {
		memcpy(&hdr, data + off, sizeof(hdr));

		if (image_get_magic(&hdr) == IH_MAGIC) {
			old_chksum = image_get_hcrc(&hdr);
			hdr.ih_hcrc = 0;
			chksum = crc32(0, (const u8 *) &hdr, sizeof(hdr));

			if (chksum == old_chksum)
				return data + off;
		}

		off += rom_cfg.align;
	}

	return NULL;
}

int check_mtk_stock_stage2_info(size_t *stage2_off_min, size_t *stage2_off,
				size_t *align_size)
{
#ifdef CONFIG_MAX_U_BOOT_SIZE
	struct mtk_spl_rom_cfg rom_cfg;
	struct image_header hdr;
	u32 old_chksum, chksum;
	u32 off, addr;

	memcpy(&rom_cfg, (char *) (CONFIG_SPI_ADDR + MTK_SPL_ROM_CFG_OFFS),
	       sizeof(rom_cfg));

	if (rom_cfg.magic != MTK_ROM_CFG_MAGIC)
		return 0;

	off = rom_cfg.size;

	if (!rom_cfg.align)
		goto end;

	off = ALIGN(off, rom_cfg.align);

	addr = off;

	while (addr < CONFIG_MAX_U_BOOT_SIZE) {
		memcpy(&hdr, (char *) (CONFIG_SPI_ADDR + addr), sizeof(hdr));

		if (image_get_magic(&hdr) == IH_MAGIC) {
			old_chksum = image_get_hcrc(&hdr);
			hdr.ih_hcrc = 0;
			chksum = crc32(0, (const u8 *) &hdr, sizeof(hdr));

			if (chksum == old_chksum) {
				*stage2_off_min = off;
				*stage2_off = addr;
				*align_size = rom_cfg.align;
				return 1;
			}
		}

		addr += rom_cfg.align;
	}

end:
	*stage2_off_min = off;
	*stage2_off = off;
	*align_size = rom_cfg.align;

	return 1;
#else
	return 0;
#endif
}