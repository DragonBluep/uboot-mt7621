// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <stddef.h>
#include <stdbool.h>
#include <image.h>
#include <div64.h>
#include <linux/sizes.h>
#include <linux/mtd/mtd.h>
#include <jffs2/jffs2.h>

#include "flash_helper.h"

#define SQUASHFS_MAGIC		0x73717368

struct squashfs_super_block {
	__le32 s_magic;
	__le32 pad0[9];
	__le64 bytes_used;
};

static int verify_legacy_image(void *flash, uint64_t offset, uint64_t maxsize,
			       void *load_addr, size_t *image_size)
{
	image_header_t *hdr = load_addr;
	uint32_t size;
	int ret;

	if (!image_check_hcrc(hdr)) {
		printf("Bad header CRC\n");
		return 1;
	}

	size = image_get_image_size(hdr);
	if (size > maxsize) {
		printf("Image size is too large, assuming damaged\n");
		return 1;
	}

	ret = mtk_board_flash_read(flash, offset, size, load_addr);
	if (ret) {
		if (ret == -EBADMSG) {
			printf("Image data has uncorrectable ECC error\n");
			return 1;
		}
		printf("Fatal: failed to read image data\n");
		return -EIO;
	}

	printf("Verifying data checksum ...\n");
	if (!image_check_dcrc(hdr)) {
		printf("Bad data CRC\n");
		return 1;
	}

	if (image_size)
		*image_size = size;

	return 0;
}

#if defined(CONFIG_FIT)
static int verify_fit_image(void *flash, uint64_t offset, uint64_t maxsize,
			    void *load_addr, size_t *image_size)
{
	size_t size;
	int ret;

	size = fit_get_size(load_addr);
	if (size > maxsize) {
		printf("Image size is too large, assuming damaged\n");
		return 1;
	}

	ret = mtk_board_flash_read(flash, offset, size, load_addr);
	if (ret) {
		if (ret == -EBADMSG) {
			printf("Image data has uncorrectable ECC error\n");
			return 1;
		}
		printf("Fatal: failed to read image data\n");
		return -EIO;
	}

	if (!fit_check_format(load_addr)) {
		printf("Wrong FIT image format\n");
		return 1;
	}

	if (!fit_all_image_verify(load_addr)) {
		printf("FIT image integrity checking failed\n");
		return 1;
	}

	if (image_size)
		*image_size = size;

	return 0;
}
#endif

static int verify_image(void *flash, uint64_t offset, uint64_t maxsize,
			size_t *image_size)
{
	void *load_addr;
	int ret;

#if defined(CONFIG_LOADADDR)
	load_addr = (void *)CONFIG_LOADADDR;
#elif defined(CONFIG_SYS_LOAD_ADDR)
	load_addr = (void *)CONFIG_SYS_LOAD_ADDR;
#endif

	ret = mtk_board_flash_read(flash, offset, sizeof(image_header_t),
				   load_addr);
	if (ret) {
		if (ret == -EBADMSG) {
			printf("Image data has uncorrectable ECC error\n");
			return 1;
		}
		printf("Fatal: failed to read image data\n");
		return -EIO;
	}

	switch (genimg_get_format(load_addr)) {
	case IMAGE_FORMAT_LEGACY:
		return verify_legacy_image(flash, offset, maxsize, load_addr,
					   image_size);
#if defined(CONFIG_FIT)
	case IMAGE_FORMAT_FIT:
		return verify_fit_image(flash, offset, maxsize, load_addr,
					image_size);
#endif
	default:
		printf("Invalid image format\n");
		return 1;
	}
}

static int verify_squashfs(void *flash, uint64_t offset, uint64_t end,
			   size_t *rootfs_size)
{
	struct squashfs_super_block sb;
	uint64_t size;
	uint8_t data;
	int ret;

	ret = mtk_board_flash_read(flash, offset, sizeof(sb), &sb);
	if (ret) {
		if (ret == -EBADMSG) {
			printf("RootFS data has uncorrectable ECC error\n");
			return 1;
		}
		printf("Fatal: failed to read image data\n");
		return -EIO;
	}

	if (le32_to_cpu(sb.s_magic) != SQUASHFS_MAGIC)
		return 1;

	size = le64_to_cpu(sb.bytes_used);
	if (offset + size >= end) {
		printf("RootFS is truncated\n");
		return 1;
	}

#ifdef CONFIG_MTK_DUAL_IMAGE_SQUASHFS_DATA_CHECK
	ret = mtk_board_flash_read(flash, offset + size - 1, sizeof(data),
				   &data);
	if (ret) {
		printf("Warn: failed to read rootfs data\n");
		return 0;
	}

	if (data == 0xff) {
		printf("RootFS is incomplete\n");
		return 1;
	}
#endif

	if (rootfs_size)
		*rootfs_size = size;

	return 0;
}

static int verify_rootfs(void *flash, uint64_t offset, uint64_t maxsize,
			 size_t *header_prefix_bytes, size_t *rootfs_size)
{
	uint64_t end = offset + maxsize, leading, extra_bytes, tmp;
	int ret;

	ret = verify_squashfs(flash, offset, end, rootfs_size);
	if (!ret) {
		if (header_prefix_bytes)
			*header_prefix_bytes = 0;
		return 0;
	}

	tmp = offset;
	leading = do_div(tmp, mtk_board_get_flash_erase_size(flash));

	if (!leading)
		return 1;

	extra_bytes = mtk_board_get_flash_erase_size(flash) - leading;
	offset += extra_bytes;

	ret = verify_squashfs(flash, offset, end, rootfs_size);
	if (!ret) {
		if (header_prefix_bytes)
			*header_prefix_bytes = extra_bytes;
		return 0;
	}

	printf("No SquashFS found\n");
	return 1;
}

static int copy_firmware(void *flash, uint64_t src_offset, uint64_t dst_offset,
			 uint64_t size, bool deadc0de)
{
	size_t sizeleft = size, chunksz, erasesize;
	uint64_t addr = dst_offset;
	uint8_t *buff, *verify;
	int ret;

#if defined(CONFIG_LOADADDR)
	buff = (uint8_t *)CONFIG_LOADADDR;
#elif defined(CONFIG_SYS_LOAD_ADDR)
	buff = (uint8_t *)CONFIG_SYS_LOAD_ADDR;
#endif

	erasesize = mtk_board_get_flash_erase_size(flash);
	verify = buff + erasesize;

	while (sizeleft) {
		if (sizeleft > erasesize)
			chunksz = erasesize;
		else
			chunksz = sizeleft;

		ret = mtk_board_flash_read(flash, src_offset, chunksz, buff);
		if (ret) {
			if (ret == -EBADMSG)
				printf("Source image data has uncorrectable ECC error\n");
			else
				printf("Fatal: failed to read src image data\n");
			return -EIO;
		}

		ret = mtk_board_flash_erase(flash, addr, erasesize);
		if (ret) {
			printf("Fatal: failed to erase dst image area\n");
			return -EIO;
		}

		ret = mtk_board_flash_write(flash, addr, chunksz, buff);
		if (ret) {
			printf("Fatal: failed to write dst image data\n");
			return -EIO;
		}

		ret = mtk_board_flash_read(flash, addr, chunksz, verify);
		if (ret) {
			if (ret == -EBADMSG)
				printf("Dest image data has uncorrectable ECC error\n");
			else
				printf("Fatal: failed to read dst image data\n");
			return -EIO;
		}

		if (memcmp(buff, verify, chunksz)) {
			printf("Image data verification failed\n");
			return 1;
		}

		src_offset += chunksz;
		addr += chunksz;
		sizeleft -= chunksz;
	}

	if (!deadc0de)
		return 0;

	addr = (addr + erasesize - 1) & ~(erasesize - 1);

	ret = mtk_board_flash_erase(flash, addr, erasesize);
	if (ret) {
		printf("Fatal: failed to erase dst image area\n");
		return -EIO;
	}

	ret = mtk_board_flash_write(flash, addr, 4, "\xde\xad\xc0\xde");
	if (ret) {
		printf("Fatal: failed to write jffs2 end-of-filesystem marker\n");
		return -EIO;
	}

	return 0;
}

int dual_image_check(void)
{
	uint64_t image1_off, image2_off, image1_partsize, image2_partsize;
	size_t image1_size, image2_size, rootfs1_size = 0, rootfs2_size = 0;
	size_t image1_padding_bytes = 0, image2_padding_bytes = 0;
	bool image1_ok, image2_ok;
	uint64_t image_total_size;
	bool deadc0de = false;
	void *flash;
	int ret;

	printf("\nStarting dual image checking ...\n");

	flash = mtk_board_get_flash_dev();
	if (!flash) {
		printf("Fatal: failed to get flash device\n");
		printf("Dual image checking is bypassed\n");
		return -1;
	}

	ret = get_mtd_part_info(CONFIG_MTK_DUAL_IMAGE_PARTNAME_MAIN,
		&image1_off, &image1_partsize);
	if (ret) {
		printf("Fatal: failed to get main image partition\n");
		printf("Dual image checking is bypassed\n");
		return -1;
	}

	ret = get_mtd_part_info(CONFIG_MTK_DUAL_IMAGE_PARTNAME_BACKUP,
		&image2_off, &image2_partsize);
	if (ret) {
		printf("Fatal: failed to get backup image partition\n");
		printf("Dual image checking is bypassed\n");
		return -1;
	}

	printf("Verifying main image at 0x%llx...\n", image1_off);
	ret = verify_image(flash, image1_off, image1_partsize, &image1_size);
	if (ret < 0) {
		printf("Dual image checking is bypassed\n");
		return 0;
	}

	if (ret == 0) {
		ret = verify_rootfs(flash, image1_off + image1_size,
				    image1_partsize - image1_size,
				    &image1_padding_bytes, &rootfs1_size);
	}

	image1_ok = ret == 0;

	printf("Verifying backup image at 0x%llx...\n", image2_off);
	ret = verify_image(flash, image2_off, image2_partsize, &image2_size);
	if (ret < 0) {
		printf("Dual image checking is bypassed\n");
		return 0;
	}

	if (ret == 0) {
		ret = verify_rootfs(flash, image2_off + image2_size,
				    image2_partsize - image2_size,
				    &image2_padding_bytes, &rootfs2_size);
	}

	image2_ok = ret == 0;

	if (!image1_ok && !image2_ok) {
		printf("Fatal: both images are broken.\n");
		return 3;
	}

	if (image1_ok && image2_ok) {
		printf("Passed\n");
		return 0;
	}

	if (!image2_ok) {
		image_total_size = image1_size;

#ifndef CONFIG_MTK_DUAL_IMAGE_RESTORE_KERNEL_ONLY
		image_total_size += image1_padding_bytes + rootfs1_size;
		deadc0de = true;
#endif

		if (image_total_size > image2_partsize) {
			printf("Fatal: backup image partition can't hold main image\n");
			return 4;
		}

		printf("Restoring backup image ...\n");
		ret = copy_firmware(flash, image1_off, image2_off,
			image_total_size, deadc0de);
	} else {
		image_total_size = image2_size;

#ifndef CONFIG_MTK_DUAL_IMAGE_RESTORE_KERNEL_ONLY
		image_total_size += image2_padding_bytes + rootfs2_size;
		deadc0de = true;
#endif

		if (image_total_size > image1_partsize) {
			printf("Fatal: main image partition can't hold backup image\n");
			return 4;
		}

		printf("Restoring main image ...\n");
		ret = copy_firmware(flash, image2_off, image1_off,
			image_total_size, deadc0de);
	}

	if (!ret)
		printf("Done\n");

	return ret;
}
