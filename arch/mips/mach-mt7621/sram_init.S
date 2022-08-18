/* SPDX-License-Identifier:	GPL-2.0+ */
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <asm/addrspace.h>
#include <asm/asm.h>
#include <asm/regdef.h>
#include <mach/mt7621_regs.h>

/*
 * Initialize SRAM
 */
LEAF(mips_stack_sram_init)
#ifdef CONFIG_NAND_BOOT
	# Disable DMA route for PSE SRAM set by BootROM
	PTR_LI	t0, CKSEG1ADDR(MT7621_DMA_CFG_ARB_BASE)
	sw	zero, MT7621_DMA_ROUTE_REG(t0)
#endif

	PTR_LI	t0, CKSEG1ADDR(MT7621_FE_BASE)
	li	t1, REG_SET_VAL(FE_PSE_RESET, 1)
	sw	t1, MT7621_FE_RST_GLO_REG(t0)

	li	t1, (REG_SET_VAL(FE_PSE_RAM, 1) | \
		     REG_SET_VAL(FE_PSE_MEM_EN, 1))
	sw	t1, MT7621_FE_RST_GLO_REG(t0)

	jr	ra
	END(mips_stack_sram_init)