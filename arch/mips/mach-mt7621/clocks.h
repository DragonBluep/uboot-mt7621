/* SPDX-License-Identifier:	GPL-2.0+ */
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _MACH_MT7621_CLOCKS_H_
#define _MACH_MT7621_CLOCKS_H_

extern void mt7621_get_clocks(u32 *pcpu_clk, u32 *pbus_clk, u32 *pddr_clk, u32 *pxtal_clk);
extern ulong get_cpu_freq(ulong dummy);

#endif /* _MACH_MT7621_CLOCKS_H_ */
