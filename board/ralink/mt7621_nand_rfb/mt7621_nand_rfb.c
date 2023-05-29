// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <image.h>
#include <linux/types.h>
#include <linux/sizes.h>
#include <debug_uart.h>
#include <asm/spl.h>
#include <asm/io.h>
#include <nand.h>
#include <linux/mtd/mtd.h>
#include <mach/mt7621_regs.h>

#include <nmbm/nmbm.h>
#include <nmbm/nmbm-mtd.h>

#ifdef CONFIG_ENABLE_NAND_NMBM
static int nmbm_usable;
#endif

#ifdef CONFIG_DEBUG_UART_BOARD_INIT
void board_debug_uart_init(void)
{
#if defined(CONFIG_SPL_BUILD)
	void __iomem *base = (void __iomem *) CKSEG1ADDR(MT7621_SYSCTL_BASE);

	setbits_le32(base + MT7621_SYS_RSTCTL_REG, REG_SET_VAL(UART1_RST, 1));
	clrbits_le32(base + MT7621_SYS_RSTCTL_REG, REG_MASK(UART1_RST));
	clrbits_le32(base + MT7621_SYS_GPIO_MODE_REG, REG_MASK(UART1_MODE));
#endif
}
#endif

int board_nmbm_init(void)
{
#ifdef CONFIG_ENABLE_NAND_NMBM
	struct mtd_info *lower, *upper;
	int ret;

	printf("\n");
	printf("Initializing NMBM ...\n");

	lower = get_nand_dev_by_index(0);
	if (!lower) {
		printf("Failed to create NMBM device due to nand0 not found\n");
		return 0;
	}

	ret = nmbm_attach_mtd(lower, NMBM_F_CREATE, CONFIG_NMBM_MAX_RATIO,
		CONFIG_NMBM_MAX_BLOCKS, &upper);

	printf("\n");

	if (ret)
		return 0;

	add_mtd_device(upper);

	nmbm_usable = 1;
#endif

	return 0;
}

#ifdef CONFIG_SPL_BUILD
void board_boot_order(u32 *spl_boot_list)
{
	spl_boot_list[0] = BOOT_DEVICE_MTK_NAND;
	spl_boot_list[1] = BOOT_DEVICE_MTK_UART;
}

ulong get_mtk_image_search_start(void)
{
	if (__rom_cfg.magic != MTK_ROM_CFG_MAGIC)
		return CONFIG_SPL_ALIGN_TO;

	return __rom_cfg.size + sizeof(struct image_header);
}

ulong get_mtk_image_search_end(void)
{
	return CONFIG_MAX_U_BOOT_SIZE;
}

ulong get_mtk_image_search_sector_size(void)
{
	if (__rom_cfg.magic != MTK_ROM_CFG_MAGIC)
		return CONFIG_SPL_ALIGN_TO;

	return __rom_cfg.align;
}
#endif

struct mtd_info *get_mtk_board_nand_mtd(void)
{
#ifdef CONFIG_ENABLE_NAND_NMBM
	struct mtd_info *mtd;

	if (nmbm_usable) {
		mtd = get_mtd_device_nm("nmbm0");
		if (IS_ERR(mtd))
			return NULL;
		return mtd;
	}
#endif

	return get_nand_dev_by_index(0);
}

#ifndef CONFIG_SPL_BUILD
void *mtk_board_get_flash_dev(void)
{
	return get_mtk_board_nand_mtd();
}

size_t mtk_board_get_flash_erase_size(void *flashdev)
{
	struct mtd_info *mtd = (struct mtd_info *)flashdev;

	return mtd->erasesize;
}

int mtk_board_flash_adjust_offset(void *flashdev, uint64_t base,
				  uint64_t offset, uint64_t limit,
				  uint64_t *result)
{
	struct mtd_info *mtd = (struct mtd_info *)flashdev;
	uint64_t addr, end, end_lim, len_left;
	uint32_t boffs;
	int ret;

	debug("Adjusting offset, base: 0x%08llx, offset: 0x%08llx, limit: 0x%08llx\n",
	       base, offset, limit);

	if (limit)
		end_lim = limit;
	else
		end_lim = mtd->size;

	addr = base & (~mtd->erasesize_mask);
	end = base + offset;
	boffs = end & mtd->erasesize_mask;
	end &= ~mtd->erasesize_mask;
	len_left = end - addr;

	while (addr < end_lim) {
		ret = mtd_block_isbad(mtd, addr);
		if (ret > 0) {
			printf("Skipping bad block 0x%08llx\n", addr);
			addr += mtd->erasesize;
			continue;
		} else if (ret < 0) {
			printf("mtd_block_isbad() failed at 0x%08llx\n", addr);
			return ret;
		}

		if (!len_left)
			break;

		addr += mtd->erasesize;
		len_left -= mtd->erasesize;
	}

	if (len_left) {
		printf("Incomplete offset adjust end at 0x%08llx, 0x%llx left\n",
		       addr, len_left);
		return -EIO;
	}

	debug("Offset 0x%08llx adjusted to 0x%08llx\n", offset, addr + boffs - base);

	if (result)
		*result = addr + boffs - base;

	return 0;
}

int mtk_board_flash_erase(void *flashdev, uint64_t offset, uint64_t len,
			  uint64_t limit)
{
	struct mtd_info *mtd = (struct mtd_info *)flashdev;
	uint64_t addr, end, end_lim, len_left;
	uint32_t erasesize_mask;
	struct erase_info instr;
	int ret;

	erasesize_mask = mtd->erasesize_mask;
	addr = offset & (~erasesize_mask);

	end = (offset + len + erasesize_mask) & (~erasesize_mask);
	len_left = end - addr;

	if (limit)
		end_lim = (offset + limit + erasesize_mask) & (~erasesize_mask);
	else
		end_lim = mtd->size;

	memset(&instr, 0, sizeof(instr));

	while (len_left && addr < end_lim) {
		ret = mtd_block_isbad(mtd, addr);
		if (ret > 0) {
			printf("Skipping bad block 0x%08llx\n", addr);
			goto next_block;
		} else if (ret < 0) {
			printf("mtd_block_isbad() failed at 0x%08llx\n", addr);
			return ret;
		}

		debug("Erasing at 0x%08llx\n", addr);

		instr.addr = addr;
		instr.len = mtd->erasesize;

		ret = mtd_erase(mtd, &instr);
		if (ret) {
			printf("Erase failed at 0x%08llx, err = %d\n",
			       addr, ret);
			return ret;
		}

		len_left -= mtd->erasesize;

	next_block:
		addr += mtd->erasesize;
	}

	if (!len_left)
		return 0;

	printf("Incomplete erase end at 0x%08llx, 0x%llx left\n", addr,
	       len_left);

	return -EIO;
}

int mtk_board_flash_read(void *flashdev, uint64_t offset, size_t len,
			 uint64_t limit, void *buf)
{
	struct mtd_info *mtd = (struct mtd_info *)flashdev;
	uint8_t *ptr = (uint8_t *)buf;
	size_t readlen, retlen, boffs;
	uint64_t baddr, end_lim;
	int ret;

	if (limit)
		end_lim = limit;
	else
		end_lim = mtd->size;

	while (len && offset < end_lim) {
		baddr = offset & (~mtd->erasesize_mask);
		boffs = offset & mtd->erasesize_mask;

		ret = mtd_block_isbad(mtd, baddr);
		if (ret > 0) {
			printf("Skipping bad block 0x%08llx\n", baddr);
			offset += mtd->erasesize;
			continue;
		} else if (ret < 0) {
			printf("mtd_block_isbad() failed at 0x%08llx\n", baddr);
			return ret;
		}

		readlen = mtd->erasesize - boffs;
		if (readlen > len)
			readlen = len;
		if (offset + readlen >= end_lim)
			readlen = end_lim - offset;

		debug("Reading at 0x%08llx, size 0x%zx\n", offset, readlen);

		ret = mtd_read(mtd, offset, readlen, &retlen, ptr);
		if (ret && ret != -EUCLEAN) {
			printf("Read failure at 0x%llx, err = %d\n", offset,
			       ret);
			return ret;
		}

		if (retlen != readlen) {
			printf("Insufficient data read at 0x%llx, %zu/%zu read\n",
			       offset, retlen, readlen);
			return -EIO;
		}

		offset += readlen;
		ptr += readlen;
		len -= readlen;
	}

	if (len) {
		printf("Incomplete data read end at 0x%08llx, 0x%zx left\n",
		       offset, len);
		return 1;
	}

	return 0;
}

int mtk_board_flash_write(void *flashdev, uint64_t offset, size_t len,
			  uint64_t limit, const void *buf)
{
	struct mtd_info *mtd = (struct mtd_info *)flashdev;
	const uint8_t *ptr = (const uint8_t *)buf;
	size_t wrlen, retlen, boffs;
	uint64_t baddr, end_lim;
	int ret;

	if (limit)
		end_lim = limit;
	else
		end_lim = mtd->size;

	while (len && offset < end_lim) {
		baddr = offset & (~mtd->erasesize_mask);
		boffs = offset & mtd->erasesize_mask;

		ret = mtd_block_isbad(mtd, baddr);
		if (ret > 0) {
			printf("Skipping bad block 0x%08llx\n", baddr);
			offset += mtd->erasesize;
			continue;
		} else if (ret < 0) {
			printf("mtd_block_isbad() failed at 0x%08llx\n", baddr);
			return ret;
		}

		wrlen = mtd->erasesize - boffs;
		if (wrlen > len)
			wrlen = len;
		if (offset + wrlen >= end_lim)
			wrlen = end_lim - offset;

		debug("Writing at 0x%08llx, size 0x%zx\n", offset, wrlen);

		ret = mtd_write(mtd, offset, wrlen, &retlen, ptr);
		if (ret && ret != -EUCLEAN) {
			printf("Read failure at 0x%llx, err = %d\n", offset,
			       ret);
			return ret;
		}

		if (retlen != wrlen) {
			printf("Insufficient data write at 0x%llx, %zu/%zu written\n",
			       offset, retlen, wrlen);
			return -EIO;
		}

		offset += wrlen;
		ptr += wrlen;
		len -= wrlen;
	}

	if (len) {
		printf("Incomplete data write end at 0x%08llx, 0x%zx left\n",
		       offset, len);
		return 1;
	}

	return 0;
}

int mtk_board_flash_verify_cust(void *flashdev, uint64_t offset, size_t len,
				uint64_t limit, const void *buf, void *vbuf,
				size_t vbuf_size)
{
	struct mtd_info *mtd = (struct mtd_info *)flashdev;
	const uint8_t *ptr = (const uint8_t *)buf;
	uint64_t baddr, end_lim;
	size_t readlen, retlen;
	int ret;

	if (limit)
		end_lim = limit;
	else
		end_lim = mtd->size;

	while (len && offset < end_lim) {
		baddr = offset & (~mtd->erasesize_mask);

		ret = mtd_block_isbad(mtd, baddr);
		if (ret > 0) {
			printf("Skipping bad block 0x%08llx\n", baddr);
			offset += mtd->erasesize;
			continue;
		} else if (ret < 0) {
			printf("mtd_block_isbad() failed at 0x%08llx\n", baddr);
			return ret;
		}

		readlen = vbuf_size;
		if (readlen > len)
			readlen = len;
		if (offset + readlen >= end_lim)
			readlen = end_lim - offset;

		debug("Reading at 0x%08llx, size 0x%zx\n", offset, readlen);

		ret = mtd_read(mtd, offset, readlen, &retlen, vbuf);
		if (ret && ret != -EUCLEAN) {
			printf("Read failure at 0x%llx, err = %d\n", offset,
			       ret);
			return ret;
		}

		if (retlen != readlen) {
			printf("Insufficient data read at 0x%llx, %zu/%zu read\n",
			       offset, retlen, readlen);
			return -EIO;
		}

		if (memcmp(vbuf, ptr, readlen))
			return 1;

		offset += readlen;
		ptr += readlen;
		len -= readlen;
	}

	if (len) {
		printf("Incomplete data verification end at 0x%08llx, 0x%zx left\n",
		       offset, len);
		return 1;
	}

	return 0;
}

int mtk_board_flash_verify(void *flashdev, uint64_t offset, size_t len,
			   uint64_t limit, const void *buf)
{
	uint8_t data[SZ_4K];

	return mtk_board_flash_verify_cust(flashdev, offset, len, limit, buf,
					   data, sizeof(data));
}
#endif
