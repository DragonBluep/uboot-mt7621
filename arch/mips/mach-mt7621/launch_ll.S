/* SPDX-License-Identifier:	GPL-2.0+ */
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <asm/addrspace.h>
#include <asm/asm.h>
#include <asm/cm.h>
#include <asm/mipsmtregs.h>
#include <asm/regdef.h>
#include <asm/cacheops.h>
#include <mach/mt7621_regs.h>
#include "launch.h"

	.macro cache_loop	curr, end, line_sz, op
10:	cache		\op, 0(\curr)
	PTR_ADDU	\curr, \curr, \line_sz
	bne		\curr, \end, 10b
	.endm

	.set	mt

/*
 * Join the coherent domain
 * a0 = number of cores
 */
LEAF(join_coherent_domain)
	/*
	 * Enable coherence and allow interventions from all other cores.
	 * (Write access enabled via GCR_ACCESS by core 0.)
	 */
	li	t1, 1
	sll	t1, a0
	addiu	t1, -1

	PTR_LI	t0, CKSEG1ADDR(CONFIG_MIPS_CM_BASE)
	sw	t1, GCR_Cx_COHERENCE(t0)
	ehb

	move	t2, zero

_next_coherent_core:
	sll	t1, t2, GCR_CL_OTHER_CORENUM_SHIFT
	sw	t1, GCR_CL_OTHER(t0)

_busy_wait_coherent_core:
	lw	t1, GCR_CO_COHERENCE(t0)
	beqz	t1, _busy_wait_coherent_core
	 nop

	addiu	t2, 1
	bne	t2, a0, _next_coherent_core
	 nop

	jr	ra
	END(join_coherent_domain)

/*
 * All VPEs other than VPE0 will go here.
 */
LEAF(launch_vpe_entry)
	mfc0	t0, CP0_EBASE
	and	t0, t0, EBASE_CPUNUM

	/* per-VPE cpulaunch_t */
	PTR_LI	a0, CKSEG0ADDR(CPULAUNCH)
	sll	t1, t0, LOG2CPULAUNCH
	addu	a0, t1

	/* Set CPU online flag */
	li	t0, LAUNCH_FREADY
	sw	t0, LAUNCH_FLAGS(a0)

	/* Enable count interrupt in mask, but do not enable interrupts */
	mfc0	t0, CP0_STATUS
	ori	t0, STATUSF_IP7
	mtc0	t0, CP0_STATUS

	/* VPEs executing in wait code do not need a stack */
	PTR_LI	t9, CMP_LAUNCH_WAITCODE_IN_RAM
	jr	t9
	END(launch_vpe_entry)

/*
 * This function will not be executed in place.
 * It will be copied into memory, and VPEs other than VPE0 will be
 * started to run into this in-memory function.
 */
LEAF(launch_wait_code)
	.globl	launch_wait_code_start
launch_wait_code_start:

	move	t0, a0

start_poll:
	/* Poll CPU go flag */
	mtc0	zero, CP0_COUNT
	li	t1, LAUNCHPERIOD
	mtc0	t1, CP0_COMPARE

time_wait:
	/* Software wait */
	mfc0	t2, CP0_COUNT
	subu	t2, t1
	bltz	t2, time_wait
	nop

	/* Check the launch flag */
	lw	t3, LAUNCH_FLAGS(t0)
	and	t3, LAUNCH_FGO
	beqz	t3, start_poll
	 nop

	/* Reset the counter and interrupts to give naive clients a chance */
	mfc0	t1, CP0_STATUS
	ins	t1, zero, STATUSB_IP7, 1
	mtc0	t1, CP0_STATUS

	mfc0	t1, CP0_COUNT
	subu	t1, 1
	mtc0	t1, CP0_COMPARE

	/* Jump to kernel */
	lw	ra, LAUNCH_PC(t0)
	lw	gp, LAUNCH_GP(t0)
	lw	sp, LAUNCH_SP(t0)
	lw	a0, LAUNCH_A0(t0)
	move	a1, zero
	move	a2, zero
	move	a3, zero
	ori	t3, LAUNCH_FGONE
	sw	t3, LAUNCH_FLAGS(t0)

	jr	ra

	.globl	launch_wait_code_end
launch_wait_code_end:
	END(launch_wait_code)

/*
 * All Cores other than Core0 will go here.
 */
LEAF(launch_core_entry)
	/* Set Cache Mode */
	mfc0	t0, CP0_CONFIG
	li	t1, CONF_CM_CACHABLE_COW
	ins	t0, t1, 0, 3
	mtc0	t0, CP0_CONFIG

	/* Initialize L1 cache only */
	li	a0, CONFIG_SYS_ICACHE_SIZE
	li	a1, CONFIG_SYS_ICACHE_LINE_SIZE
	li	a2, CONFIG_SYS_DCACHE_SIZE
	li	a3, CONFIG_SYS_DCACHE_LINE_SIZE

	mtc0	zero, CP0_TAGLO
	mtc0	zero, CP0_TAGLO, 2
	ehb

	/*
	 * Initialize the I-cache first,
	 */
	PTR_LI		t0, KSEG0
	PTR_ADDU	t1, t0, a0
	/* clear tag to invalidate */
	cache_loop	t0, t1, a1, INDEX_STORE_TAG_I
#ifdef CONFIG_SYS_MIPS_CACHE_INIT_RAM_LOAD
	/* fill once, so data field parity is correct */
	PTR_LI		t0, KSEG0
	cache_loop	t0, t1, a1, FILL
	/* invalidate again - prudent but not strictly neccessary */
	PTR_LI		t0, KSEG0
	cache_loop	t0, t1, a1, INDEX_STORE_TAG_I
#endif

	/*
	 * then initialize D-cache.
	 */
	PTR_LI		t0, KSEG0
	PTR_ADDU	t1, t0, a2
	/* clear all tags */
	cache_loop	t0, t1, a3, INDEX_STORE_TAG_D
#ifdef CONFIG_SYS_MIPS_CACHE_INIT_RAM_LOAD
	/* load from each line (in cached space) */
	PTR_LI		t0, KSEG0
2:	LONG_L		zero, 0(t0)
	PTR_ADDU	t0, a3
	 bne		t0, t1, 2b
	/* clear all tags */
	PTR_LI		t0, KSEG0
	cache_loop	t0, t1, a3, INDEX_STORE_TAG_D
#endif

	li	a0, 2
	bal	join_coherent_domain
	 nop

#ifndef CONFIG_MT7621_SINGLE_VPE
	bal	boot_vpe1
	 nop
#endif

	b	launch_vpe_entry
	END(launch_core_entry)

/*
 * Bootup VPE1.
 * This subroutine must be executed from VPE0 with VPECONF0[MVP] already set.
 */
LEAF(boot_vpe1)
	mfc0	t0, CP0_MVPCONF0

	/* a0 = number of TCs - 1 */
	ext	a0, t0, MVPCONF0_PTC_SHIFT, MVPCONF0_PTC_WIDTH
	beqz	a0, _vpe1_init_done
	 nop

	/* a1 = number of VPEs - 1 */
	ext	a1, t0, MVPCONF0_PVPE_SHIFT, MVPCONF0_PVPE_WIDTH
	beqz	a1, _vpe1_init_done
	 nop

	/* a2 = current TC No. */
	move	a2, zero

	/* Enter VPE Configuration State */
	mfc0	t0, CP0_MVPCONTROL
	or	t0, MVPCONTROL_VPC
	mtc0	t0, CP0_MVPCONTROL
	ehb

_next_tc:
	/* Set the TC number to be used on MTTR and MFTR instructions */
	mfc0	t0, CP0_VPECONTROL
	ins	t0, a2, VPECONTROL_TARGTC_SHIFT, VPECONTROL_TARGTC_WIDTH
	mtc0	t0, CP0_VPECONTROL
	ehb

	/* TC0 is already bound */
	beqz	a2, _next_vpe
	 nop

	/* Halt current TC */
	li	t0, TCHALT_H
	mttc0	t0, CP0_TCHALT
	ehb

	/* If there is spare TC, bind it to the last VPE (VPE[a1]) */
	slt	t1, a1, a2
	bnez	t1, _vpe_bind_tc
	 move	t1, a1

	/* Set Exclusive TC for active TC */
	mftc0	t0, CP0_VPECONF0
	ins	t0, a2, VPECONF0_XTC_SHIFT, VPECONF0_XTC_WIDTH
	mttc0	t0, CP0_VPECONF0

	move	t1, a2
_vpe_bind_tc:
	/* Bind TC to a VPE */
	mftc0	t0, CP0_TCBIND
	ins	t0, t1, TCBIND_CURVPE_SHIFT, TCBIND_CURVPE_WIDTH
	mttc0	t0, CP0_TCBIND

	/*
	 * Set up CP0_TCSTATUS register:
	 * Disable Coprocessor Usable bits
	 * Disable MDMX/DSP ASE
	 * Clear Dirty TC
	 * not dynamically allocatable
	 * not allocated
	 * Kernel mode
	 * interrupt exempt
	 * ASID 0
	 */
	li	t0, TCSTATUS_IXMT
	mttc0	t0, CP0_TCSTATUS

_next_vpe:
	slt	t1, a1, a2
	bnez	t1, _done_vpe	# No more VPEs
	 nop

	/* Disable TC multi-threading */
	mftc0	t0, CP0_VPECONTROL
	ins	t0, zero, VPECONTROL_TE_SHIFT, VPECONTROL_TE_WIDTH
	mttc0	t0, CP0_VPECONTROL

	/* Skip following configuration for TC0 */
	beqz	a2, _done_vpe
	 nop

	/* Deactivate VPE, set Master VPE */
	mftc0	t0, CP0_VPECONF0
	ins	t0, zero, VPECONF0_VPA_SHIFT, VPECONF0_VPA_WIDTH
	or	t0, VPECONF0_MVP
	mttc0	t0, CP0_VPECONF0

	mfc0	t0, CP0_STATUS
	mttc0	t0, CP0_STATUS

	mttc0	zero, CP0_EPC
	mttc0	zero, CP0_CAUSE

	mfc0	t0, CP0_CONFIG
	mttc0	t0, CP0_CONFIG

	/*
	 * VPE1 of each core can execute cached as its L1 I$ has already
	 * been initialized.
	 * and the L2$ has been initialized or "disabled" via CCA override.
	 */
	PTR_LA	t0, _start	# This address has been relocated to DRAM
	mttc0	t0, CP0_TCRESTART

	/* Unset Interrupt Exempt, set Activate Thread */
	mftc0	t0, CP0_TCSTATUS
	ins	t0, zero, TCSTATUS_IXMT_SHIFT, TCSTATUS_IXMT_WIDTH
	ori	t0, TCSTATUS_A
	mttc0	t0, CP0_TCSTATUS

	/* Resume TC */
	mttc0	zero, CP0_TCHALT

	/* Activate VPE */
	mftc0	t0, CP0_VPECONF0
	ori	t0, VPECONF0_VPA
	mttc0	t0, CP0_VPECONF0

_done_vpe:
	addu	a2, 1
	sltu	t0, a0, a2
	beqz	t0, _next_tc
	 nop

	mfc0	t0, CP0_MVPCONTROL
	/* Enable all activated VPE to execute */
	ori	t0, MVPCONTROL_EVP
	/* Exit VPE Configuration State */
	ins	t0, zero, MVPCONTROL_VPC_SHIFT, MVPCONTROL_VPC_WIDTH
	mtc0	t0, CP0_MVPCONTROL
	ehb

_vpe1_init_done:
	jr	ra
	END(boot_vpe1)