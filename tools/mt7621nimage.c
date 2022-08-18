// SPDX-License-Identifier:	GPL-2.0+
/*
 * Generate BootROM header for SPL/U-Boot image
 *
 * Copyright (C) 2018  MediaTek, Inc.
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 */

#include "imagetool.h"
#include "mkimage.h"

#include <sys/types.h>

#include <image.h>
#include <u-boot/crc.h>

#define MT7621_IH_NMLEN			12
#define MT7621_IH_CRC_POLYNOMIAL	0x04c11db7

typedef struct nand_header {
	uint8_t ih_name[MT7621_IH_NMLEN];
	uint32_t nand_ac_timing;
	uint32_t ih_stage_offset;
	uint32_t ih_bootloader_offset;
	uint32_t nand_info_1_data;
	uint32_t crc;
} nand_header_t;

typedef struct stage1_header {
	uint32_t rsvd0;
	uint32_t rsvd1;
	uint32_t sh_ep;
	uint32_t sh_size;
	uint32_t sh_has_stage2;
	uint32_t sh_next_ep;
	uint32_t sh_next_size;
	uint32_t sh_next_offset;
} stage1_header_t;

static image_header_t mt7621_hdr;
static uint32_t crc32tbl[256];

static void crc32_init(void)
{
	if (crc32tbl[1] == MT7621_IH_CRC_POLYNOMIAL)
		return;

	crc32c_be_init(crc32tbl, MT7621_IH_CRC_POLYNOMIAL);
}

static uint32_t crc32_cal(const uint8_t *data, int length)
{
	uint32_t crc = 0;
	char c;

	crc32_init();

	crc = crc32c_be_cal(crc, (char *) data, length, crc32tbl);

	for (; length; length >>= 8) {
		c = length & 0xff;
		crc = crc32c_be_cal(crc, &c, 1, crc32tbl);
	}

	return ~crc;
}

static int mt7621_check_params(struct image_tool_params *params)
{
	if (!params->addr) {
		fprintf(stderr, "Error: Load Address must be set.\n");
		return -EINVAL;
	}

	return 0;
}

static int mt7621_verify_header(unsigned char *ptr, int image_size,
	struct image_tool_params *params)
{
	image_header_t hdr;
	nand_header_t *nh;
	uint32_t crcval;

	memcpy(&hdr, ptr, sizeof(hdr));

	crcval = be32toh(hdr.ih_hcrc);
	hdr.ih_hcrc = 0;

	if (crcval != crc32(0, (uint8_t *) &hdr, sizeof(hdr)))
		return -EINVAL;

	nh = (nand_header_t *) &hdr.ih_name;

	crcval = be32toh(nh->crc);
	nh->crc = 0;

	if (crcval != crc32_cal((uint8_t *) &hdr, sizeof(hdr)))
		return -EINVAL;

	return 0;
}

static void mt7621_print_header(const void *ptr)
{
	const image_header_t *hdr = (const image_header_t *) ptr;
	const char *p, *os, *arch, *type, *comp;

	printf("MT7621 BootROM NAND image header\n");

	p = IMAGE_INDENT_STRING;
	printf("%sImage Name:   %.*s\n", p, MT7621_IH_NMLEN,
		image_get_name(hdr));
	if (IMAGE_ENABLE_TIMESTAMP) {
		printf("%sCreated:      ", p);
		genimg_print_time((time_t) image_get_time(hdr));
	}
	printf("%sImage Type:   ", p);
	os = genimg_get_os_name(image_get_os(hdr));
	arch = genimg_get_arch_name(image_get_arch(hdr));
	type = genimg_get_type_name(image_get_type(hdr));
	comp = genimg_get_comp_name(image_get_comp(hdr));
	printf("%s %s %s (%s)\n", arch, os, type, comp);
	printf("%sData Size:    ", p);
	genimg_print_size(image_get_data_size(hdr));
	printf("%sLoad Address: %08x\n", p, image_get_load(hdr));
	printf("%sEntry Point:  %08x\n", p, image_get_ep(hdr));
}

static void mt7621_set_header(void *ptr, struct stat *sbuf, int ifd,
	struct image_tool_params *params)
{
	image_header_t *hdr;
	nand_header_t *nh;
	stage1_header_t *shdr;
	uint32_t datasize;

	datasize = sbuf->st_size - sizeof(image_header_t);

	/* Set stage header first */
	shdr = (stage1_header_t *) (ptr + sizeof(image_header_t));

	shdr->sh_ep = htobe32(params->addr);
	shdr->sh_size = htobe32(datasize);

	/* Set MT7621 specific u-boot header */
	hdr = (image_header_t *) ptr;
	nh = (nand_header_t *) &hdr->ih_name;

	hdr->ih_magic = htobe32(IH_MAGIC);
	hdr->ih_time = htobe32(0);
	hdr->ih_size = htobe32(datasize);
	hdr->ih_load = htobe32(params->addr);
	hdr->ih_ep = htobe32(params->ep);
	hdr->ih_os = IH_OS_U_BOOT;
	hdr->ih_arch = IH_ARCH_MIPS;
	hdr->ih_type = IH_TYPE_STANDALONE;
	hdr->ih_comp = IH_COMP_NONE;
	hdr->ih_dcrc = htobe32(crc32(0, (uint8_t *) shdr, datasize));

	strncpy((char *) hdr->ih_name, "MT7621 NAND", sizeof(hdr->ih_name));

	nh->ih_stage_offset = htobe32(sizeof(image_header_t));
	nh->crc = htobe32(crc32_cal((uint8_t *) hdr, sizeof(image_header_t)));

	hdr->ih_hcrc = htobe32(crc32(0, (uint8_t *) hdr,
		sizeof(image_header_t)));
}

static int mt7621_check_image_type(uint8_t type)
{
	return type != IH_TYPE_MT7621_NAND;
}

U_BOOT_IMAGE_TYPE(
	mt7621_nand,
	"MediaTek MT7621 BootROM NAND Image support",
	sizeof(image_header_t),
	&mt7621_hdr,
	mt7621_check_params,
	mt7621_verify_header,
	mt7621_print_header,
	mt7621_set_header,
	NULL,
	mt7621_check_image_type,
	NULL,
	NULL
);
