/* SPDX-License-Identifier:	GPL-2.0+ */
/*
* Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
*
* Author: Weijie Gao <weijie.gao@mediatek.com>
*/

#ifndef _MIPS_CMP_LAUNCH_H_
#define _MIPS_CMP_LAUNCH_H_

#define CMP_LAUNCH_WAITCODE_IN_RAM 0xA0000D00

#ifndef __ASSEMBLY__

typedef struct {
	unsigned long pc;
	unsigned long gp;
	unsigned long sp;
	unsigned long a0;
	unsigned long _pad[3]; /* pad to cache line size to avoid thrashing */
	unsigned long flags;
} cpulaunch_t;

extern char launch_wait_code_start;
extern char launch_wait_code_end;

extern void join_coherent_domain(int ncores);
extern void boot_vpe1(void);

#else

#define	LAUNCH_PC	0
#define	LAUNCH_GP	4
#define	LAUNCH_SP	8
#define	LAUNCH_A0	12
#define	LAUNCH_FLAGS	28

#endif

#define LOG2CPULAUNCH	5

#define LAUNCH_FREADY	1
#define LAUNCH_FGO	2
#define LAUNCH_FGONE	4

#define SCRLAUNCH	0x00000e00
#define CPULAUNCH	0x00000f00
#define NCPULAUNCH	8

/* Polling period in count cycles for secondary CPU's */
#define LAUNCHPERIOD	10000

#endif /* _MIPS_CMP_LAUNCH_H_ */
