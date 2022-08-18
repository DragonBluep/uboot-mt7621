// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <asm/io.h>
#include <asm/addrspace.h>
#include <asm/types.h>
#include <mach/mt7621_regs.h>

void _machine_restart(void)
{
	void __iomem *base;
	u32 mask;

	base = (void __iomem *) CKSEG1ADDR(MT7621_SYSCTL_BASE);

	mask = REG_MASK(MCM_RST) |
		REG_MASK(FE_RST) |
		REG_MASK(PIO_RST) |
		REG_MASK(NFI_RST) |
		REG_MASK(SPI_RST) |
		REG_MASK(ETH_RST) |
		REG_MASK(PPE_RST);

	setbits_le32(base + MT7621_SYS_RSTCTL_REG, mask);
	udelay(200);
	clrbits_le32(base + MT7621_SYS_RSTCTL_REG, mask);

	setbits_le32(base + MT7621_SYS_RSTCTL_REG, REG_MASK(SYS_RST));

	while (1)
		/* NOP */;
}

void _machine_restart_spl(void)
{
	void __iomem *base;

	base = (void __iomem *) CKSEG1ADDR(MT7621_SYSCTL_BASE);

	setbits_le32(base + MT7621_SYS_RSTCTL_REG, REG_MASK(SYS_RST));

	while (1)
		/* NOP */;
}
