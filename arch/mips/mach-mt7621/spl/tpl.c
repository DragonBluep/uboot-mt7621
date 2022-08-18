// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <image.h>
#include <asm/sections.h>
#include <asm/cacheops.h>
#include <asm/mipsregs.h>
#include <asm/mipsmtregs.h>
#include <asm/addrspace.h>
#include <asm/system.h>
#include <asm/cm.h>
#include <spl.h>

#define INDEX_STORE_DATA_SD	0x0f

#define write_c0_sdatalo(val)	__write_32bit_c0_register($28, 5, val)
#define write_c0_sdatahi(val)	__write_32bit_c0_register($29, 5, val)

typedef void __noreturn (*image_entry_noargs_t)(void);

/*
 * Lock L2 cache and fill data
 * Assume that data is 4-byte aligned and start_addr/size is 32-byte aligned
 */
static void fill_lock_l2cache(u32 *data, ulong start_addr, ulong size)
{
	ulong slsize = CONFIG_SYS_DCACHE_LINE_SIZE;
	ulong end_addr = start_addr + size;
	u32 errctl, val;
	ulong i, addr;

	/* Clear WSC & SPR bit in ErrCtl */
	errctl = read_c0_ecc();
	errctl &= 0xcfffffff;
	write_c0_ecc(errctl);
	ehb();

	for (addr = start_addr; addr < end_addr; addr += slsize) {
		/* Set STagLo to lock cache line */
		write_c0_staglo((addr & 0x1ffff800) | 0xa0);
		mips_cache(INDEX_STORE_TAG_SD, (void *) addr);

		/* Fill data */
		for (i = 0; i < slsize; i += 8) {
			val = *data++;
			write_c0_sdatalo(val);
			val = *data++;
			write_c0_sdatahi(val);
			mips_cache(INDEX_STORE_DATA_SD, (void *) (addr + i));
		}
	}
}

/* A simple function to initialize MT7621's cache */
static void mt7621_cache_init(void)
{
	ulong lsize = CONFIG_SYS_DCACHE_LINE_SIZE;
	u32 val;
	ulong addr;

	/* Enable CCA override. Set to uncached */
	val = readl(mips_cm_base() + GCR_BASE);
	val &= ~CCA_DEFAULT_OVERRIDE_VALUE_MASK;
	val |= CCA_DEFAULT_OVERRIDE_ENABLE | \
	       (2 << CCA_DEFAULT_OVERRIDE_VALUE_SHIFT);
	writel(val, mips_cm_base() + GCR_BASE);

	/* Initialize L1 I-Cache */
	write_c0_taglo(0);
	write_c0_taghi(0);

	for (addr = 0; addr < CONFIG_SYS_ICACHE_SIZE; addr += lsize)
		mips_cache(INDEX_STORE_TAG_I, (void *) addr);

	/* Initialize L1 D-Cache */
	write_c0_dtaglo(0);
	__write_32bit_c0_register($29, 2, 0); /* dtaghi */

	for (addr = 0; addr < CONFIG_SYS_DCACHE_SIZE; addr += lsize)
		mips_cache(INDEX_STORE_TAG_D, (void *) addr);

	/* Initialize L2 Cache */
	write_c0_staglo(0);
	__write_32bit_c0_register($29, 4, 0); /* staghi */

	for (addr = 0; addr < (256 << 10); addr += lsize)
		mips_cache(INDEX_STORE_TAG_SD, (void *) addr);

	/* Dsiable CCA override */
	val = readl(mips_cm_base() + GCR_BASE);
	val &= ~(CCA_DEFAULT_OVERRIDE_VALUE_MASK |
		 CCA_DEFAULT_OVERRIDE_ENABLE);
	writel(val, mips_cm_base() + GCR_BASE);

	/* Set KSEG0 to non-coherent cached (important!) */
	val = read_c0_config();
	val &= ~CONF_CM_CMASK;
	val |= CONF_CM_CACHABLE_NONCOHERENT;
	write_c0_config(val);
	ehb();

	/* Again, invalidate L1 D-Cache */
	for (addr = 0; addr < CONFIG_SYS_DCACHE_SIZE; addr += lsize)
		mips_cache(INDEX_WRITEBACK_INV_D, (void *) addr);

	/* Invalidate L1 I-Cache */
	for (addr = 0; addr < CONFIG_SYS_ICACHE_SIZE; addr += lsize)
		mips_cache(INDEX_INVALIDATE_I, (void *) addr);

	/* Disable L2 cache bypass */
	val = read_c0_config2();
	val &= ~MIPS_CONF_IMPL;
	write_c0_config2(val);
	ehb();
}

void __noreturn board_init_f(ulong dummy)
{
	struct spl_image_info tpl_image;
	image_entry_noargs_t image_entry;
	u32 *data;
	int ret;

	/* Initialize the cache first */
	mt7621_cache_init();

	/* Load TPL image header from the end of TPL itself */
	memset(&tpl_image, '\0', sizeof(tpl_image));

	tpl_image.flags |= SPL_COPY_PAYLOAD_ONLY;

	ret = spl_parse_image_header(&tpl_image,
			(const struct image_header *)__image_copy_end);
	if (ret)
		goto failed;

	/* Load TPL image to L2 cache */
	data = (u32 *)((u32) __image_copy_end + sizeof(struct image_header));
	fill_lock_l2cache(data, (u32)tpl_image.load_addr, tpl_image.size);

	/* Jump to SPL */
	image_entry =
		(image_entry_noargs_t)tpl_image.entry_point;

	image_entry();

failed:
	unreachable();
}