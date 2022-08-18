// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <spl.h>
#include <lzma/LzmaTypes.h>
#include <lzma/LzmaDec.h>
#include <lzma/LzmaTools.h>
#include <xyzModem.h>
#include <linux/sizes.h>
#include <asm/addrspace.h>

#if defined(CONFIG_NAND_BOOT)
#include <nand.h>

#ifdef CONFIG_ENABLE_NAND_NMBM
#include <nmbm/nmbm.h>
#include <nmbm/nmbm-mtd.h>

static struct mtd_info *upper;
#endif
#endif

#include "spl_helper.h"

DECLARE_GLOBAL_DATA_PTR;

#ifndef CONFIG_SYS_BOOTM_LEN
/* use 8MByte as default max uncompress size */
#define CONFIG_SYS_BOOTM_LEN		SZ_8M
#endif

#define BUF_SIZE 1024

static ulong free_dram_bottom(void)
{
#if defined (CONFIG_MACH_MT7621)
	return CONFIG_SYS_SDRAM_BASE + 0x1000;
#else
	return CONFIG_SYS_SDRAM_BASE;
#endif
}

static int spl_try_load_image(struct spl_image_info *spl_image,
			      void *image_addr)
{
	int ret;
	struct image_header *uhdr, hdr;
	SizeT lzma_len;

	if ((size_t) image_addr % sizeof (void *)) {
		memcpy(&hdr, (const void *) image_addr, sizeof (hdr));
		uhdr = &hdr;
	} else {
		uhdr = (struct image_header *) image_addr;
	}

	ret = spl_parse_image_header(spl_image, uhdr);

	if (ret)
		return ret;

	if (!spl_image->entry_point)
		spl_image->entry_point = spl_image->load_addr;

	if (uhdr->ih_comp == IH_COMP_NONE) {
		/*
		* Load real U-Boot from its location to its
		* defined location in SDRAM
		*/

		if (spl_image->load_addr !=
		    (uintptr_t) (image_addr + sizeof(struct image_header))) {
			memmove((void *) spl_image->load_addr,
				(void *) (image_addr + sizeof(hdr)),
				spl_image->size);
		}
	}
#ifdef CONFIG_SPL_LZMA
	else if (uhdr->ih_comp == IH_COMP_LZMA) {
		/*
		* Uncompress real U-Boot to its defined location in SDRAM
		*/
		lzma_len = CONFIG_SYS_BOOTM_LEN;

		ret = lzmaBuffToBuffDecompress((u8 *) spl_image->load_addr,
			&lzma_len,
			(u8 *) (image_addr + sizeof(struct image_header)),
			spl_image->size);

		if (ret) {
			printf("Error: LZMA uncompression error: %d\n", ret);
			return ret;
		}

		spl_image->size = lzma_len;
	}
#endif /* CONFIG_SPL_LZMA */
	else {
		debug("Warning: Unsupported compression method found in image "
		      "header at offset 0x%p\n", image_addr);
		return -EINVAL;
	}

	flush_cache((unsigned long) spl_image->load_addr, spl_image->size);

	return 0;
}

static int spl_mtk_nor_load_image(struct spl_image_info *spl_image,
	struct spl_boot_device *bootdev)
{
	ulong search_start = get_mtk_image_search_start();
	ulong search_end = get_mtk_image_search_end();
	ulong search_sector_size = get_mtk_image_search_sector_size();

	/*
	* Loading of the payload to SDRAM is done with skipping of
	* the mkimage header
	*/
	spl_image->flags |= SPL_COPY_PAYLOAD_ONLY;

	/* Try booting without padding */
	if (!spl_try_load_image(spl_image, (void *) search_start))
		return 0;

	if (!search_sector_size)
		return -EINVAL;

	search_start = ALIGN(search_start, search_sector_size);

	while (search_start < search_end) {
		if (!spl_try_load_image(spl_image, (void *) search_start))
			return 0;

		search_start += search_sector_size;
	}

	return -EINVAL;
}
SPL_LOAD_IMAGE_METHOD("NOR", 0, BOOT_DEVICE_MTK_NOR, spl_mtk_nor_load_image);

#if defined(CONFIG_NAND_BOOT)

#ifdef CONFIG_ENABLE_NAND_NMBM
static int spl_nmbm_init(void)
{
	struct mtd_info *lower;
	int ret;

	printf("\n");
	printf("Initializing NMBM ...\n");

	lower = get_nand_dev_by_index(0);
	if (!lower) {
		printf("Failed to create NMBM device due to nand0 not found\n");
		return -ENODEV;
	}

	ret = nmbm_attach_mtd(lower, NMBM_F_CREATE, CONFIG_NMBM_MAX_RATIO,
		CONFIG_NMBM_MAX_BLOCKS, &upper);

	return ret;
}

int nand_spl_load_image(uint32_t offs, unsigned int size, void *dest)
{
	size_t retlen;

	return mtd_read(upper, offs, size, &retlen, dest);
}

void nand_deselect(void)
{
}
#endif

static int spl_mtk_read_nand_image(struct spl_image_info *spl_image,
				   ulong nand_addr, ulong *data_addr)
{
	struct image_header hdr;
	u32 old_crc;
	int ret;
	void *dst_addr;

	if (nand_spl_load_image(nand_addr, sizeof(hdr), &hdr))
		return -EINVAL;

	if (image_get_magic(&hdr) != IH_MAGIC)
		return -EINVAL;

	old_crc = image_get_hcrc(&hdr);
	hdr.ih_hcrc = 0;

	hdr.ih_hcrc = crc32(0, (u8 *) &hdr, sizeof(hdr));

	if (hdr.ih_hcrc != old_crc)
		return -EINVAL;

	ret = spl_parse_image_header(spl_image, &hdr);

	if (ret)
		return -EINVAL;

	dst_addr = (void *) free_dram_bottom();

	if (nand_spl_load_image(nand_addr,
	    sizeof(hdr) + image_get_data_size(&hdr), dst_addr))
		return -EINVAL;

	*data_addr = (ulong) dst_addr;

	return 0;
}

static int spl_mtk_nand_load_image(struct spl_image_info *spl_image,
	struct spl_boot_device *bootdev)
{
	ulong search_start = get_mtk_image_search_start();
	ulong search_end = get_mtk_image_search_end();
	ulong search_sector_size = get_mtk_image_search_sector_size();
	ulong image_addr;

#ifdef CONFIG_ENABLE_NAND_NMBM
	int ret;
#endif

	nand_init();

#ifdef CONFIG_ENABLE_NAND_NMBM
	ret = spl_nmbm_init();
	if (ret)
		return ret;
#endif

	/*
	* Loading of the payload to SDRAM is done with skipping of
	* the mkimage header
	*/
	spl_image->flags |= SPL_COPY_PAYLOAD_ONLY;

	/* Try booting without padding */
	if (!spl_mtk_read_nand_image(spl_image, search_start, &image_addr)) {
		if (!spl_try_load_image(spl_image, (void *) image_addr))
			return 0;
	}

	if (!search_sector_size)
		return -EINVAL;

	search_start = ALIGN(search_start, search_sector_size);

	while (search_start < search_end) {
		if (!spl_mtk_read_nand_image(spl_image, search_start,
					     &image_addr)) {
			if (!spl_try_load_image(spl_image,
						(void *) image_addr))
				return 0;
		}

		search_start += search_sector_size;
	}

	return -EINVAL;
}
SPL_LOAD_IMAGE_METHOD("NAND", 0, BOOT_DEVICE_MTK_NAND,
		      spl_mtk_nand_load_image);
#endif /* CONFIG_NAND_BOOT */

static int getcymodem(void)
{
	if (tstc())
		return (getc());
	return -1;
}

static int spl_mtk_ymodem_load_image(struct spl_image_info *spl_image,
				     struct spl_boot_device *bootdev)
{
	int err, ret, size = 0;
	char *buf, *stage2_buf;
	connection_info_t info;

	printf("\n");
	printf("Failed to load U-Boot image!\n");
	printf("Entering emergency mode.\n");
	printf("Please transmit a valid U-Boot image through this serial "
	       "console.\n");
	printf("The U-Boot image will be booted up directly, and not be "
	       "written to flash.\n");
	printf("Accepted mode is Ymoden-1K.\n");

	buf = (char *) free_dram_bottom();

	info.mode = xyzModem_ymodem;
	ret = xyzModem_stream_open(&info, &err);
	if (ret) {
		printf("spl: ymodem err - %s\n", xyzModem_error(err));
		return ret;
	}

	while ((ret = xyzModem_stream_read(buf + size, BUF_SIZE, &err)) > 0)
		size += ret;

	xyzModem_stream_close(&err);
	xyzModem_stream_terminate(false, &getcymodem);

	printf("Loaded %d bytes\n", size);

	stage2_buf = get_mtk_stage2_image_ptr(buf, size);

	if (!stage2_buf)
		return -EINVAL;

	return spl_try_load_image(spl_image, stage2_buf);
}
SPL_LOAD_IMAGE_METHOD("UART", 0, BOOT_DEVICE_MTK_UART,
		      spl_mtk_ymodem_load_image);