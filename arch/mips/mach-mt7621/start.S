/* SPDX-License-Identifier:	GPL-2.0+ */
/*
 *  Startup Code for MIPS32 CPU-core
 *
 *  Copyright (c) 2003	Wolfgang Denk <wd@denx.de>
 */

#include <asm-offsets.h>
#include <config.h>
#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/addrspace.h>
#include <asm/mipsregs.h>
#include <asm/mipsmtregs.h>
#include <asm/cm.h>

#ifdef CONFIG_SPL
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_SDRAM_BASE + 0x8000)
#else
#define CONFIG_SYS_INIT_SP_ADDR		0xbe10d000
#endif

	.set noreorder

	.macro init_wr sel
	MTC0	zero, CP0_WATCHLO,\sel
	mtc0	t1, CP0_WATCHHI,\sel
	 nop
	.endm

	.macro uhi_mips_exception
	move	k0, t9		# preserve t9 in k0
	move	k1, a0		# preserve a0 in k1
	li	t9, 15		# UHI exception operation
	li	a0, 0		# Use hard register context
	sdbbp	1		# Invoke UHI operation
	.endm

	.macro setup_stack_gd
	li	t0, -16
	PTR_LI	t1, CONFIG_SYS_INIT_SP_ADDR
	and	sp, t1, t0		# force 16 byte alignment
	PTR_SUBU \
		sp, sp, GD_SIZE		# reserve space for gd
	and	sp, sp, t0		# force 16 byte alignment
	move	k0, sp			# save gd pointer
#if CONFIG_VAL(SYS_MALLOC_F_LEN)
	li	t2, CONFIG_VAL(SYS_MALLOC_F_LEN)
	PTR_SUBU \
		sp, sp, t2		# reserve space for early malloc
	and	sp, sp, t0		# force 16 byte alignment
#endif
	move	fp, sp

	/* Clear gd */
	move	t0, k0
1:
	PTR_S	zero, 0(t0)
	blt	t0, t1, 1b
	 PTR_ADDIU t0, PTRSIZE

#if CONFIG_VAL(SYS_MALLOC_F_LEN)
	PTR_S	sp, GD_MALLOC_BASE(k0)	# gd->malloc_base offset
#endif
	.endm

ENTRY(_start)
	/* U-Boot entry point */
	b	reset
	 mtc0	zero, CP0_COUNT	# clear cp0 count for most accurate boot timing

#ifndef CONFIG_SPL
	/*
	 * Exception vector entry points. When running from ROM, an exception
	 * cannot be handled. Halt execution and transfer control to debugger,
	 * if one is attached.
	 */
	.org 0x200
	/* TLB refill, 32 bit task */
	uhi_mips_exception

	.org 0x280
	/* XTLB refill, 64 bit task */
	uhi_mips_exception

	.org 0x300
	/* Cache error exception */
	uhi_mips_exception

	.org 0x380
	/* General exception */
	uhi_mips_exception

	.org 0x400
	/* Catch interrupt exceptions */
	uhi_mips_exception

	.org 0x480
	/* EJTAG debug exception */
1:	b	1b
	 nop

	.org 0x500
#endif

reset:
	/* MT7530 reset assert */
	li	t0, 0xbe000000
	lw	t1, 0x34(t0)
	ori	t1, (1 << 2)
	sw	t1, 0x34(t0)

	/* Init CP0 Status */
	mfc0	t0, CP0_STATUS
	and	t0, ST0_IMPL
	or	t0, ST0_BEV | ST0_ERL
	mtc0	t0, CP0_STATUS

	/* Clear Watch Status bits and disable watch exceptions */
	li	t1, 0x7		# Clear I, R and W conditions
	init_wr	0
	init_wr	1
	init_wr	2
	init_wr	3

	/* Clear WP, IV and SW interrupts */
	mtc0	zero, CP0_CAUSE

	/* Clear timer interrupt (CP0_COUNT cleared on branch to 'reset') */
	mtc0	zero, CP0_COMPARE

#ifndef CONFIG_SPL
#ifdef CONFIG_MIPS_CM
	PTR_LA	t9, mips_cm_map
	jalr	t9
	 nop
#endif

	/* Non-VPE0 goes to LAUNCH wait code directly */
	mfc0	t0, CP0_TCBIND
	andi	t0, TCBIND_CURVPE
	bnez	t0, launch_vpe_entry
	 nop

	/* Non-Core0 */
	PTR_LI	t0, CKSEG1ADDR(CONFIG_MIPS_CM_BASE)
	lw	t1, GCR_CL_ID(t0)
	bnez	t1, launch_core_entry
	 nop

	mfc0	t0, CP0_CONFIG
	and	t0, t0, MIPS_CONF_IMPL
	or	t0, t0, CONF_CM_UNCACHED
	mtc0	t0, CP0_CONFIG
	ehb

	/* Configure the SRAM before it can be used */
	PTR_LA	t9, mips_stack_sram_init
	jalr	t9
	 nop
#endif

	/* Set up initial stack and global data */
	setup_stack_gd

#ifdef CONFIG_DEBUG_UART
	/* Earliest point to set up debug uart */
	PTR_LA	t9, debug_uart_init
	jalr	t9
	 nop
#endif

#ifndef CONFIG_SPL
	/* Initialize any external memory */
	PTR_LA	t9, lowlevel_init
	jalr	t9
	 nop

	/* Initialize caches... */
	PTR_LA	t9, mips_cache_reset
	jalr	t9
	 nop
#endif

	move	a0, zero		# a0 <-- boot_flags = 0
	PTR_LA	t9, board_init_f

	jr	t9
	 move	ra, zero

	END(_start)