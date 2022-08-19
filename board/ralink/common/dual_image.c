// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
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
#include "dual_image.h"

#define SQUASHFS_MAGIC		0x73717368

struct squashfs_super_block {
	__le32 s_magic;
	__le32 pad0[9];
	__le64 bytes_used;
};

/**
 * part_read_data() - Wrapper for reading from flash partition
 *
 * @param part: Flash partition
 * @param offset: Partition offset to read from
 * @param size: Size to be read
 * @param data: memory address to store the read data
 * @return true if succeeded
 */
static bool part_read_data(struct di_part_info  *part, u64 offset, size_t size,
			   void *data)
{
	int ret;

	ret = mtk_board_flash_read(part->flash, part->addr + offset, size,
				   data);

	debug("%s: addr = 0x%llx + 0x%llx, size = 0x%zx, ret = %d\n", __func__,
	      part->addr, offset, size, ret);

	if (ret) {
		printf("Fatal: failed to read from partition '%s', ret = %d\n",
		       part->name, ret);
		return false;
	}

	return true;
}

/**
 * part_write_data() - Wrapper for writing to flash partition
 *
 * @param part: Flash partition
 * @param offset: Partition offset to write to
 * @param size: Size to be written
 * @param data: memory address storing the data
 * @return true if succeeded
 */
static bool part_write_data(struct di_part_info  *part, u64 offset, size_t size,
			    const void *data)
{
	u64 start, end;
	int ret;

	ret = mtk_board_flash_write(part->flash, part->addr + offset, size,
				    data);

	debug("%s: addr = 0x%llx + 0x%llx, size = 0x%zx, ret = %d\n", __func__,
	      part->addr, offset, size, ret);

	if (ret) {
		printf("Fatal: failed to write to partition '%s', ret = %d\n",
		       part->name, ret);
		return false;
	}

	return true;
}

/**
 * part_erase_range() - Wrapper for erasing blocks on flash partition
 *
 * @param part: Flash partition
 * @param offset: Partition offset to erase
 * @param size: Size to erase
 * @return true if succeeded
 */
static bool part_erase_range(struct di_part_info  *part, u64 offset, u64 size)
{
	u64 start, end;
	int ret;

	start = part->addr + offset;
	start &= ~(part->blocksize - 1);

	end = part->addr + offset + size;
	end = (end + part->blocksize - 1) & ~(part->blocksize - 1);

	ret = mtk_board_flash_erase(part->flash, start, end - start);

	debug("%s: addr = 0x%llx + 0x%llx [0x%llx], size = 0x%llx [0x%llx]\n",
	      __func__, part->addr, offset, start, size, end - start);

	if (ret) {
		printf("Fatal: failed to erase on partition '%s', ret = %d\n",
		       part->name, ret);
		return false;
	}

	return true;
}

/**
 * find_rootfs() - Find squashfs rootfs in the firmware partition
 *
 * @description:
 * There may be padding between kernel image and rootfs. This function will
 * find the rootfs by searching squashfs magic in the firmware partition
 * after the kernel image.
 *
 * @param ii: image information used for searching, and storing result
 * @return true if found, false if not found, or data corruption
 */
static bool find_rootfs(struct di_image_info *ii)
{
	struct squashfs_super_block sb;
	bool rootfs_truncated = false;
	u32 blocksize_mask;
	u64 size, offset;
	bool ret;

	debug("%s: addr = 0x%x, end = 0x%llx\n", __func__, ii->kernel_size,
	      ii->part.size);

	/*
	 * For the first round, the search offset will not be aligned to
	 * the block boundary (i.e. no padding)
	 */
	offset = ii->kernel_size;
	blocksize_mask = ii->part.blocksize - 1;

	while (offset < ii->part.size - sizeof(sb)) {
		ret = part_read_data(&ii->part, offset, sizeof(sb), &sb);
		if (!ret)
			goto next_offset;

		debug("%s: checking at 0x%llx, magic = 0x%08x, size = 0x%llx\n",
		      __func__, offset, le32_to_cpu(sb.s_magic),
		      le64_to_cpu(sb.bytes_used));

		/* Check little-endian magic only */
		if (le32_to_cpu(sb.s_magic) != SQUASHFS_MAGIC)
			goto next_offset;

		/* Whether the size field in the superblock is valid */
		size = le64_to_cpu(sb.bytes_used);
		if (offset + size > ii->part.size) {
			/*
			 * This may be a fake superblock, so we continue to
			 * check the next block. Mark this so we can print
			 * precise error message if no rootfs found at last.
			 */
			rootfs_truncated = true;
			goto next_offset;
		}

		/*
		 * We can't check the entire rootfs here, assuming this is a
		 * valid superblock.
		 */
		ii->padding_size = offset - ii->kernel_size;
		ii->rootfs_size = size;

		debug("%s: found at 0x%llx\n", __func__, offset);

		return true;

	next_offset:
		/* Align the search offset to next block */
		if (offset & blocksize_mask)
			offset = (offset + blocksize_mask) & (~blocksize_mask);
		else
			offset += ii->part.blocksize;
	}

	if (rootfs_truncated)
		printf("RootFS is incomplete\n");
	else
		printf("RootFS not found\n");

	return false;
}

/**
 * find_rootfs_ram() - Find squashfs rootfs in the whole image in RAM
 *
 * @description:
 * There may be padding between kernel image and rootfs. This function will
 * find the rootfs by searching squashfs magic after the kernel image.
 *
 * @param ii: image information used for searching, and storing result
 * @param size: size of the whole image in memory
 * @return true if found, false if not found, or data corruption
 */
static bool find_rootfs_ram(struct di_image_info *ii, size_t size)
{
	struct squashfs_super_block sb;
	bool rootfs_truncated = false;
	u32 blocksize_mask;
	uintptr_t offset;
	u64 sqsize;

	debug("%s: addr = 0x%p, end = 0x%p\n", __func__,
	      ii->data + ii->kernel_size, ii->data + size);

	/*
	 * For the first round, the search offset will not be aligned to
	 * the block boundary (i.e. no padding)
	 */
	offset = ii->kernel_size;
	blocksize_mask = ii->part.blocksize - 1;

	while (offset < size - sizeof(sb)) {
		memcpy(&sb, ii->data + offset, sizeof(sb));

		debug("%s: checking at 0x%p, magic = 0x%08x, size = 0x%llx\n",
		      __func__, ii->data + offset, le32_to_cpu(sb.s_magic),
		      le64_to_cpu(sb.bytes_used));

		/* Check little-endian magic only */
		if (le32_to_cpu(sb.s_magic) != SQUASHFS_MAGIC)
			goto next_offset;

		/* Whether the size field in the superblock is valid */
		sqsize = le64_to_cpu(sb.bytes_used);
		if (offset + sqsize > size) {
			/*
			 * This may be a fake superblock, so we continue to
			 * check the next block. Mark this so we can print
			 * precise error message if no rootfs found at last.
			 */
			rootfs_truncated = true;
			goto next_offset;
		}

		/*
		 * We can't check the entire rootfs here, assuming this is a
		 * valid superblock.
		 */
		ii->padding_size = offset - ii->kernel_size;
		ii->rootfs_size = sqsize;

		debug("%s: found at 0x%p\n", __func__, ii->data + offset);

		return true;

	next_offset:
		/* Align the search offset to next block */
		if (offset & blocksize_mask)
			offset = (offset + blocksize_mask) & (~blocksize_mask);
		else
			offset += ii->part.blocksize;
	}

	if (rootfs_truncated)
		printf("RootFS is incomplete\n");
	else
		printf("RootFS not found\n");

	return false;
}

/**
 * verify_rootfs_simple() - Simply verify the rootfs
 *
 * @description:
 * Since squashfs has no embedded checksum field, it's impossible to check
 * its integrity. But there's one special case which can be handled:
 * The firmware upgrade in u-boot will erase the partition before writing
 * rootfs. So if the rootfs is not written completely, the last byte of
 * rootfs will be 0xff.
 *
 * @param data: Pointer to rootfs data
 * @param size: Size of rootfs data
 * @return true if check passed
 */
static bool verify_rootfs_simple(const void *data, size_t size)
{
	const u8 *ptr;

	ptr = data + size - 1;
	if (*ptr == 0xff) {
		printf("RootFS is corrupted\n");
		return false;
	}

	return true;
}

#if defined(CONFIG_IMAGE_FORMAT_LEGACY)
/**
 * verify_image_legacy() - Integrity verification of legacy image
 *
 * @description:
 * Load image data into memory (optional) and verify its integrity.
 * On success, the whole image with rootfs will be loaded into memory (opt.),
 * and image information will be recorded.
 *
 * @param ii: on success, stores the image information
 * @param ram: whether the image has been fully loaded in memory
 * @param size: if ram == true, indicates the size of the whole image
 * @return true if integrity verification passed
 */
static bool verify_image_legacy(struct di_image_info *ii, bool ram, size_t size)
{
	void *rootfs;
	bool ret;

	if (!image_check_hcrc(ii->data)) {
		printf("Bad header CRC\n");
		return false;
	}

	ii->kernel_size = image_get_image_size(ii->data);
	if (ii->part.size && ii->kernel_size > ii->part.size) {
		printf("Image size is too large, assuming corrupted\n");
		return false;
	}

	if (!ram) {
		/* Read the whole kernel image into memory */
		ret = part_read_data(&ii->part, 0, ii->kernel_size, ii->data);
		if (!ret)
			return false;
	}

	printf("Verifying data checksum ...\n");
	if (!image_check_dcrc(ii->data)) {
		printf("Bad data CRC\n");
		return false;
	}

	/* Locate the rootfs */
	if (ram)
		ret = find_rootfs_ram(ii, size);
	else
		ret = find_rootfs(ii);

	if (!ret)
		return false;

	if (!ram) {
		/* Read the whole rootfs with padding into memory */
		ret = part_read_data(&ii->part, ii->kernel_size,
				     ii->padding_size + ii->rootfs_size,
				     ii->data + ii->kernel_size);

		if (!ret)
			return false;
	} else {
		ii->marker_size = size - ii->kernel_size - ii->padding_size -
			ii->rootfs_size;
	}

	rootfs = ii->data + ii->kernel_size + ii->padding_size;

	return verify_rootfs_simple(rootfs, ii->rootfs_size);
}
#endif

#if defined(CONFIG_FIT)
/**
 * verify_rootfs_fit() - Verify rootfs with given hash from FIT image
 *
 * @description:
 * Verify the rootfs with given hash value and algo from FIT image.
 *
 * @param fit: Pointer to FIT image data
 * @param noffset: Node offset of rootfs in FIT image
 * @param data: Pointer to rootfs data
 * @param size: Size of rootfs data
 * @return zero if passed, negative if failed, positive if not supported
 */
static bool verify_rootfs_fit(const void *fit, int noffset, const void *data,
			      size_t size)
{
	uint8_t value[FIT_MAX_HASH_LEN];
	int value_len, fit_value_len;
	uint8_t *fit_value;
	char *algo;

	if (fit_image_hash_get_algo(fit, noffset, &algo)) {
		printf("Warning: algo property is missing\n");
		return false;
	}

	if (fit_image_hash_get_value(fit, noffset, &fit_value,
				     &fit_value_len)) {
		printf("Warning: hash property is missing\n");
		return false;
	}

	if (calculate_hash(data, size, algo, value, &value_len)) {
		printf("Warning: Unsupported hash algorithm '%s'\n", algo);
		return false;
	}

	if (value_len != fit_value_len) {
		printf("Error: Bad hash value length\n");
		return false;
	} else if (memcmp(value, fit_value, value_len) != 0) {
		printf("Error: Bad hash value\n");
		return false;
	}

	return true;
}

/**
 * verify_image_fit() - Integrity verification of FIT image
 *
 * @description:
 * Load image data into memory (optional) and verify its integrity.
 * On success, the whole image with rootfs will be loaded into memory (opt.),
 * and image information will be recorded.
 *
 * @param ii: on success, stores the image information
 * @param ram: whether the image has been fully loaded in memory
 * @param size: if ram == true, indicates the size of the whole image
 * @return true if integrity verification passed
 */
static bool verify_image_fit(struct di_image_info *ii, bool ram, size_t size, char* pc_hash , int pc_hash_len)
{
	bool ret, rootfs_hash_passed = false;
	int len, rootfs_noffset, noffset;
	u32 fit_rootfs_size;
	const u32 *cell;
	void *rootfs;

	ii->kernel_size = fit_get_size(ii->data);
	if (ii->part.size && ii->kernel_size > ii->part.size) {
		printf("Image size is too large, assuming corrupted\n");
		return false;
	}

	if (!ram) {
		/* Read the whole kernel image into memory */
		ret = part_read_data(&ii->part, 0, ii->kernel_size, ii->data);
		if (!ret)
			return false;
	}

	if (!fit_check_format(ii->data)) {
		printf("Wrong FIT image format\n");
		return false;
	}

	if (!fit_all_image_verify_finish(ii->data, pc_hash , pc_hash_len)) {
		printf("FIT image integrity checking failed\n");
		return false;
	}

	/* Locate the rootfs */
	if (ram)
		ret = find_rootfs_ram(ii, size);
	else
		ret = find_rootfs(ii);

	if (!ret)
		return false;

	/* Find rootfs node */
	rootfs_noffset = fdt_path_offset(ii->data, "/rootfs");

	debug("%s: rootfs_noffset = 0x%x\n", __func__, rootfs_noffset);

	if (rootfs_noffset >= 0) {
		/* Read the actual rootfs size */
		cell = fdt_getprop(ii->data, rootfs_noffset, "size", &len);
		if (!cell || len != sizeof(*cell)) {
			printf("'size' property does not exist in FIT\n");
			return false;
		}

		fit_rootfs_size = fdt32_to_cpu(*cell);
		if (!fit_rootfs_size) {
			printf("Invalid rootfs size in FIT\n");
			return false;
		}

		if (fit_rootfs_size != ii->rootfs_size) {
			printf("Rootfs size in FIT mismatch\n");
			return false;
		}
	}

	if (!ram) {
		/* Read the whole rootfs with padding into memory */
		ret = part_read_data(&ii->part, ii->kernel_size,
				     ii->padding_size + ii->rootfs_size,
				     ii->data + ii->kernel_size);

		if (!ret)
			return false;
	} else {
		ii->marker_size = size - ii->kernel_size - ii->padding_size -
			ii->rootfs_size;
	}

	rootfs = ii->data + ii->kernel_size + ii->padding_size;

	/* If rootfs node not found, use the simple verification */
	if (rootfs_noffset < 0)
		return verify_rootfs_simple(rootfs, ii->rootfs_size);

	printf("Verifying rootfs ...\n");

	/*
	 * Process all hash subnodes, and succeed with at least one hash
	 * verification passed.
	 */
	fdt_for_each_subnode(noffset, ii->data, rootfs_noffset) {
		const char *name = fit_get_name(ii->data, noffset, NULL);

		if (strncmp(name, FIT_HASH_NODENAME,
			    strlen(FIT_HASH_NODENAME)))
			continue;

		debug("%s: verifying hash node '%s'\n", __func__, name);

		ret = verify_rootfs_fit(ii->data, noffset, rootfs,
					ii->rootfs_size);

		if (ret) {
			rootfs_hash_passed = true;

			debug("%s: hash node '%s' verification passed\n",
			      __func__, name);
		}
	}

	if (!rootfs_hash_passed)
		printf("Error: no valid hash node verified\n");

	return rootfs_hash_passed;
}
#endif

/**
 * verify_image() - Integrity verification of image
 *
 * @description:
 * Load image data into memory and verify its integrity.
 * On success, the whole image with rootfs will be loaded into memory,
 * and image information will be recorded.
 *
 * @param ii: on success, stores the image information
 * @return true if integrity verification passed
 */
static bool verify_image(struct di_image_info *ii, char* pc_hash , int pc_hash_len)
{
	bool ret;

	debug("%s: partition = '%s', data = 0x%p\n", __func__, ii->part.name,
	      ii->data);

	/* Read header */
	ret = part_read_data(&ii->part, 0, sizeof(image_header_t), ii->data);
	if (!ret)
		return false;

	switch (genimg_get_format(ii->data)) {
#if defined(CONFIG_IMAGE_FORMAT_LEGACY)
	case IMAGE_FORMAT_LEGACY:
		return verify_image_legacy(ii, false, 0);
#endif
#if defined(CONFIG_FIT)
	case IMAGE_FORMAT_FIT:
		return verify_image_fit(ii, false, 0 , pc_hash , pc_hash_len);
#endif
	default:
		printf("Invalid image format\n");
		return false;
	}
}

/**
 * jffs2_eofs_pad() - Pad image with jffs2 end-of-filesystem mark
 *
 * @description:
 * On restoring image, JFFS2 end-of-filesystem marker (0xdeadc0de) should also
 * be written back in order to discard previous rootfs_data.
 *
 * @param ii: Image information for padding
 */
static void jffs2_eofs_pad(struct di_image_info *ii)
{
	uintptr_t rootfs_end, marker_start, marker_end, marker_pos = SZ_4K;
	uintptr_t blocksize_mask = ii->part.blocksize - 1;

	static const u8 jffs2_marker[4] = { 0xde, 0xad, 0xc0, 0xde };

	rootfs_end = (uintptr_t)ii->data + ii->kernel_size + ii->padding_size +
		ii->rootfs_size;

	debug("%s: rootfs_end = 0x%lx\n", __func__, rootfs_end);

	marker_start = rootfs_end & (~blocksize_mask);
	marker_end = (rootfs_end + blocksize_mask) & (~blocksize_mask);
	marker_end += sizeof(jffs2_marker);

	ii->marker_size = marker_end - rootfs_end;

	if (ii->marker_size == sizeof(jffs2_marker)) {
		memcpy((void *)rootfs_end, jffs2_marker, sizeof(jffs2_marker));
		return;
	}

	memset((void *)rootfs_end, 0xff, ii->marker_size);

	while (marker_start + marker_pos < marker_end) {
		if (marker_start + marker_pos >= rootfs_end) {
			memcpy((void *)marker_start + marker_pos, jffs2_marker,
			       sizeof(jffs2_marker));

			debug("%s: add marker at 0x%lx\n", __func__,
			      marker_start + marker_pos);
		}

		marker_pos *= 2;
	}
}

/**
 * copy_image() - Copy image from one partition to another partition
 *
 * @description:
 * Restore verified good image to a partition
 *
 * @param iit: Image information containing the partition to restore the image
 * @param iif: Verified good image information
 * @return true if restore complete
 */
static bool copy_image(struct di_image_info *iit,
		       const struct di_image_info *iif)
{
	u32 image_size, chksz, verify_offset = 0;
	bool ret;

	image_size = iif->kernel_size + iif->padding_size + iif->rootfs_size +
		iif->marker_size;

	debug("%s: image_size = 0x%x\n", __func__, image_size);

	if (image_size > iit->part.size) {
		printf("Error: Partition '%s' is too small to restore image\n",
		       iit->part.name);
		return false;
	}

	ret = part_erase_range(&iit->part, 0, image_size);
	if (!ret)
		return false;

	ret = part_write_data(&iit->part, 0, image_size, iif->data);
	if (!ret)
		return false;

	/*
	 * Verify the image just written.
	 * This will destroy the data pointed by iit->data
	 */
	while (image_size) {
		chksz = min(image_size, iit->part.blocksize);

		ret = part_read_data(&iit->part, verify_offset, chksz,
				     iit->data);
		if (!ret)
			goto verify_fail;

		if (memcmp(iit->data, iif->data + verify_offset, chksz))
			goto verify_fail;

		image_size -= chksz;
		verify_offset += chksz;
	}

	return true;

verify_fail:
	printf("Error: Image verification failed\n");
	return false;
}

/**
 * dual_image_check() - Start dual-image check
 *
 * @return 0 if check passed,
 *         1 if main image has been successfully restored,
 *         2 if backup image has been successfully restored,
 *         -1 if configuration error,
 *         -2 if both image were corrupted,
 *         -3 if image restoration failed
 */
int dual_image_check(void)
{
	struct di_image_info ii1, ii2, *iit, *iif;
	bool image1_ok, image2_ok, restore_result , image2_same = true ;
	char cHash1[4] , cHash2[4];
	uintptr_t la, bs;
	void *flash;
	int ret;

#if defined(CONFIG_LOADADDR)
	la = CONFIG_LOADADDR;
#elif defined(CONFIG_SYS_LOAD_ADDR)
	la = CONFIG_SYS_LOAD_ADDR;
#endif

	printf("\nStarting dual image checking ...\n");
	printf("\n");

	flash = mtk_board_get_flash_dev();
	if (!flash) {
		printf("Fatal: failed to get flash device\n");
		goto err_bypassed;
	}

	bs = mtk_board_get_flash_erase_size(flash);

	memset(&cHash1, 0, sizeof(cHash1));
	memset(&cHash2, 0, sizeof(cHash2));

	memset(&ii1, 0, sizeof(ii1));

	ii1.part.flash = flash;
	ii1.part.blocksize = bs;
	ii1.part.name = CONFIG_MTK_DUAL_IMAGE_PARTNAME_MAIN;

	ret = get_mtd_part_info(ii1.part.name, &ii1.part.addr, &ii1.part.size);
	if (ret) {
		printf("Fatal: failed to get main image partition\n");
		goto err_bypassed;
	}

	memset(&ii2, 0, sizeof(ii2));

	ii2.part.flash = flash;
	ii2.part.blocksize = bs;
	ii2.part.name = CONFIG_MTK_DUAL_IMAGE_PARTNAME_BACKUP;

	ret = get_mtd_part_info(ii2.part.name, &ii2.part.addr, &ii2.part.size);
	if (ret) {
		printf("Fatal: failed to get backup image partition\n");
		goto err_bypassed;
	}

	/* Align the data pointer for image1 to a block boundary */
	ii1.data = (void *)((la + bs - 1) & (~(bs - 1)));

	printf("Verifying main image at 0x%llx ...\n", ii1.part.addr);
	image1_ok = verify_image(&ii1 , &cHash1[0] , sizeof(cHash1));

	printf("\n");
	printf("\n LCC CRC: [%X%X%X%X] %d \n" , cHash1[0] ,cHash1[1], cHash1[2] ,cHash1[3], sizeof(cHash1));

	
	/* Reserve space for jffs2 padding of image1 */
	la = (uintptr_t)ii1.data + ii1.kernel_size + ii1.padding_size +
		ii1.rootfs_size;
	la = (la + bs - 1) & (~(bs - 1));

	ii2.data = (void *)(la + bs);

	printf("Verifying backup image at 0x%llx ...\n", ii2.part.addr);
	image2_ok = verify_image(&ii2, &cHash2[0] , sizeof(cHash2));

	printf("\n");
	printf("\n LCC CRC: [%X%X%X%X] %d\n" , cHash2[0] ,cHash2[1], cHash2[2] ,cHash2[3] , sizeof(cHash1));

	if (!image1_ok && !image2_ok) {
		printf("Fatal: both images are broken.\n");
		return -2;
	}
	
	if(memcmp(cHash1, cHash2, sizeof(cHash1)) != 0) {
		image2_same = false;
	}


	if (image1_ok && image2_ok && image2_same) {
		printf("Passed\n");
		return 0;
	}

	/* Start image restoration */
	if (image1_ok) {
		iif = &ii1;
		iit = &ii2;
		ret = 1;

		printf("Restoring backup image ...\n");
	} else {
		iif = &ii2;
		iit = &ii1;
		ret = 2;

		printf("Restoring main image ...\n");
	}

	jffs2_eofs_pad(iif);

	restore_result = copy_image(iit, iif);
	if (restore_result)
		printf("Done\n");
	else
		ret = -3;

	return ret;

err_bypassed:
	printf("Dual image checking is bypassed\n");
	return -1;
}

/**
 * dual_image_check_single_ram() - Check image integrity in RAM
 *
 * @return 0 if check passed,
 *         1 if image verification failed,
 *         -1 if image format not supported,
 *         -ENODEV if flash partition not found,
 *         -EINVAL if parameter error
 */
int dual_image_check_single_ram(void *data, size_t size, const char *partname,
				struct di_image_info *ii)
{
	struct di_image_info iif;
	void *flash;
	bool bret;
	int ret;

	if (!data || !size)
		return -EINVAL;

	debug("%s: data = 0x%p, size = 0x%zx\n", __func__, data, size);

	memset(&iif, 0, sizeof(iif));

	iif.data = data;

	if (partname) {
		flash = mtk_board_get_flash_dev();
		if (!flash) {
			printf("Fatal: failed to get flash device\n");
			return -ENODEV;
		}

		iif.part.flash = flash;
		iif.part.blocksize = mtk_board_get_flash_erase_size(flash);
		iif.part.name = partname;

		ret = get_mtd_part_info(partname, &iif.part.addr,
					&iif.part.size);

		if (ret) {
			printf("Fatal: failed to get partition '%s'\n",
			       partname);
			return -ENODEV;
		}
	}

	switch (genimg_get_format(iif.data)) {
#if defined(CONFIG_IMAGE_FORMAT_LEGACY)
	case IMAGE_FORMAT_LEGACY:
		bret = verify_image_legacy(&iif, true, size);
		break;
#endif
#if defined(CONFIG_FIT)
	case IMAGE_FORMAT_FIT:
		bret = verify_image_fit(&iif, true, size,NULL,0);
		break;
#endif
	default:
		printf("Invalid image format\n");
		return -1;
	}

	if (bret) {
		if (ii)
			memcpy(ii, &iif, sizeof(iif));

		return 0;
	}

	return 1;
}

/**
 * dual_image_update_backup() - Update backup partition with specified image
 *
 * @return 0 if success, otherwise failed
 */
int dual_image_update_backup(const struct di_image_info *iif)
{
	struct di_image_info iit;
	uintptr_t la, bs;
	void *flash;
	int ret;

	if (!iif)
		return -EINVAL;

	flash = mtk_board_get_flash_dev();
	if (!flash) {
		printf("Fatal: failed to get flash device\n");
		return -ENODEV;
	}

	bs = mtk_board_get_flash_erase_size(flash);

	memset(&iit, 0, sizeof(iit));

	iit.part.flash = flash;
	iit.part.blocksize = bs;
	iit.part.name = CONFIG_MTK_DUAL_IMAGE_PARTNAME_BACKUP;

	ret = get_mtd_part_info(iit.part.name, &iit.part.addr, &iit.part.size);
	if (ret) {
		printf("Fatal: failed to get backup image partition\n");
		return -ENODEV;
	}

	/* Reserve space for existing image */
	la = (uintptr_t)iif->data + iif->kernel_size + iif->padding_size +
		iif->rootfs_size + iif->marker_size;
	la = (la + bs - 1) & (~(bs - 1));

	iit.data = (void *)(la + bs);

	if (copy_image(&iit, iif))
		return 0;

	return 1;
}
