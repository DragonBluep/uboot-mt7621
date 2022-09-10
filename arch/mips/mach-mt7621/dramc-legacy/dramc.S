/* SPDX-License-Identifier:	GPL-2.0 */

#include <config.h>
#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/addrspace.h>
#include <mach/mt7621_regs.h>

#define DRAMC_BIN_LOADADDR	0xBE108800

#define RG_MEPL_FBDIV_S		4
#define RG_MEPL_FBDIV_M		0x7f

#if defined(CONFIG_MT7621_DRAM_FREQ_400_LEGACY)
#define DDR_FREQ_PARAM		0x41000000
#elif defined(CONFIG_MT7621_DRAM_FREQ_800_LEGACY)
#define DDR_FREQ_PARAM		0x31000000
#elif defined(CONFIG_MT7621_DRAM_FREQ_1066_LEGACY)
#define DDR_FREQ_PARAM		0x21000000
#elif defined(CONFIG_MT7621_DRAM_FREQ_1200_LEGACY)
#define DDR_FREQ_PARAM		0x11000000
#else
#error Invalid DDR frequency
#endif

	.align	2
	.section	".rodata.dramc_bin", "a"
	.type	dramc_bin, @object
dramc_bin:
	.incbin DRAMC_BIN_FILE
	.equ	dramc_bin_size, . - dramc_bin
	.size	dramc_bin, dramc_bin_size

	.align	2
	.section	".rodata.dramc_ddr2_ac_timing_parameters", "a"
	.type	dramc_ddr2_ac_timing_parameters, @object
dramc_ddr2_ac_timing_parameters:
#if defined(CONFIG_MT7621_DRAM_DDR2_512M_LEGACY)
	.word	0xAA00AA00, 0xAA00AA00, 0x00000007, 0x22174441, 0x00000000, 0xF0748661, 0x40001273, 0x9F0A0481
	.word	0x0304692F, 0x15602842, 0x00008888, 0x88888888, 0x00000000, 0x00000000, 0x00000000, 0x07100000
	.word	0x00001B63, 0x00002000, 0x00004000, 0x00006000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
#elif defined(CONFIG_MT7621_DRAM_DDR2_512M_W9751G6KB_A02_1066MHZ_LEGACY)
	.word	0xAA00AA00, 0xAA00AA00, 0x00000007, 0x33484584, 0x00000000, 0xF07486A1, 0x50001273, 0x9F010481
	.word	0x0304693F, 0x15602842, 0x00008888, 0x88888888, 0x00000000, 0x00000000, 0x00000010, 0x07100000
	.word	0x00001F73, 0x00002000, 0x00004000, 0x00006000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
#elif defined(CONFIG_MT7621_DRAM_DDR2_1024M_LEGACY)
	.word	0xAA00AA00, 0xAA00AA00, 0x00000007, 0x22174441, 0x01000000, 0xF0748661, 0x40001273, 0x9F0F0481
	.word	0x0304692F, 0x15602842, 0x00008888, 0x88888888, 0x00000000, 0x00000000, 0x00000000, 0x07100000
	.word	0x00001B63, 0x00002000, 0x00004000, 0x00006000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
#elif defined(CONFIG_MT7621_DRAM_DDR2_1024M_W971GG6KB25_800MHZ_LEGACY)
	.word	0xAA00AA00, 0xAA00AA00, 0x00000007, 0x22174430, 0x01000000, 0xF0748661, 0x40001273, 0x9F0F0481
	.word	0x0304692F, 0x15602842, 0x00008888, 0x88888888, 0x00000000, 0x00000000, 0x00000000, 0x07100000
	.word	0x00001B63, 0x00002000, 0x00004000, 0x00006000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
#elif defined(CONFIG_MT7621_DRAM_DDR2_1024M_W971GG6KB18_1066MHZ_LEGACY)
	.word	0xAA00AA00, 0xAA00AA00, 0x00000007, 0x33484584, 0x01000000, 0xF07486A1, 0x50001273, 0x9F070481
	.word	0x0304693F, 0x15602842, 0x00008888, 0x88888888, 0x00000000, 0x00000000, 0x00000010, 0x07100000
	.word	0x00001F73, 0x00002000, 0x00004000, 0x00006000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
#else
#error Invalid DDR2 AC timing parameters
#endif
	.equ	dramc_ddr2_ac_timing_parameters_size, . - dramc_ddr2_ac_timing_parameters
	.size	dramc_ddr2_ac_timing_parameters, dramc_ddr2_ac_timing_parameters_size

	.align	2
	.section	".rodata.dramc_ddr3_ac_timing_parameters", "a"
	.type	dramc_ddr3_ac_timing_parameters, @object
dramc_ddr3_ac_timing_parameters:
#if defined(CONFIG_MT7621_DRAM_DDR3_1024M_LEGACY)
	.word	0xAA00AA00, 0xAA00AA00, 0x00000007, 0x44694683, 0x01000000, 0xF07486A1, 0xC287221D, 0x9F060481
	.word	0x03046948, 0x15602842, 0x00008888, 0x88888888, 0x00000000, 0x00000000, 0x00000210, 0x07100000
	.word	0x00001B61, 0x00002040, 0x00004010, 0x00006000, 0x0C000000, 0x07070000, 0x00000000, 0x00000000
#elif defined(CONFIG_MT7621_DRAM_DDR3_2048M_LEGACY)
	.word	0xAA00AA00, 0xAA00AA00, 0x00000007, 0x44694673, 0x01000000, 0xF07486A1, 0xC287221D, 0x9F050481
	.word	0x03046948, 0x15602842, 0x00008888, 0x88888888, 0x00000000, 0x00000000, 0x00000220, 0x07100000
	.word	0x00001B61, 0x00002040, 0x00004010, 0x00006000, 0x0C000000, 0x07070000, 0x00000000, 0x00000000
#elif defined(CONFIG_MT7621_DRAM_DDR3_4096M_LEGACY)
	.word	0xAA00AA00, 0xAA00AA00, 0x00000007, 0x44694683, 0x01000000, 0xF07486A1, 0xC287221D, 0x9F0F0481
	.word	0x03046948, 0x15602842, 0x00008888, 0x88888888, 0x00000000, 0x00000000, 0x00000240, 0x07100000
	.word	0x00001B61, 0x00002040, 0x00004010, 0x00006000, 0x0C000000, 0x07070000, 0x00000000, 0x00000000
#elif defined(CONFIG_MT7621_DRAM_DDR3_1024M_KGD_LEGACY)
	.word	0xFF00FF00, 0xFF00FF00, 0x00000007, 0x44694683, 0x01000000, 0xF07406A1, 0xC287221D, 0x9F060481
	.word	0x03046923, 0x152f2842, 0x00008888, 0x88888888, 0x00000000, 0x00000000, 0x00000210, 0x07100000
	.word	0x00001B61, 0x00002040, 0x00004010, 0x00006000, 0x0C000000, 0x07070000, 0x000C0000, 0x00000000
#else
#error Invalid DDR3 AC timing parameters
#endif
	.equ	dramc_ddr3_ac_timing_parameters_size, . - dramc_ddr3_ac_timing_parameters
	.size	dramc_ddr3_ac_timing_parameters, . - dramc_ddr3_ac_timing_parameters

	.text
	.set noreorder

LEAF(lowlevel_init)
	li	t0, 0xBE00001C
	lw	t1, 0(t0)
	li	t1, 0x0
	sw	t1, 0(t0)

	/* move code to SRAM */
	PTR_LA	t0, dramc_bin
	li	t1, DRAMC_BIN_LOADADDR
	PTR_LI	t3, dramc_bin_size

1:
	lw	t2, 0(t0)
	sw	t2, 0(t1)
	addiu	t0, t0, 4
	addiu	t1, t1, 4
	subu	t3, t3, 4
	bgtz	t3, 1b
	 nop

	/* Override DDR2 AC timing settings */
	PTR_LA	t0, dramc_ddr2_ac_timing_parameters
	li	t1, DRAMC_BIN_LOADADDR + 0xc8
	PTR_LI	t3, dramc_ddr2_ac_timing_parameters_size

1:
	lw	t2, 0(t0)
	sw	t2, 0(t1)
	addiu	t0, t0, 4
	addiu	t1, t1, 4
	subu	t3, t3, 4
	bgtz	t3, 1b
	 nop

	/* Override DDR3 AC timing settings */
	PTR_LA	t0, dramc_ddr3_ac_timing_parameters
	li	t1, DRAMC_BIN_LOADADDR + 0x60
	PTR_LI	t3, dramc_ddr3_ac_timing_parameters_size

1:
	lw	t2, 0(t0)
	sw	t2, 0(t1)
	addiu	t0, t0, 4
	addiu	t1, t1, 4
	subu	t3, t3, 4
	bgtz	t3, 1b
	 nop

	/* Set DDR frequency */
	PTR_LI	t0, DDR_FREQ_PARAM
	li	t1, DRAMC_BIN_LOADADDR + 0x24
	sw	t0, 0(t1)

	/* Set baudrate */
	PTR_LI	t0, CONFIG_BAUDRATE
	li	t1, DRAMC_BIN_LOADADDR + 0x130
	sw	t0, 0(t1)

	/* Set CPU frequency */
	PTR_LI	t0, CKSEG1ADDR(MT7621_SYSCTL_BASE)
	lw	t1, MT7621_SYS_SYSCFG0_REG(t0)
	srl	t1, t1, XTAL_MODE_SEL_S
	andi	t1, XTAL_MODE_SEL_M

	li	t2, 2
	ble	t1, t2, _xtal_20mhz
	 nop

	li	t2, 5
	ble	t1, t2, _xtal_40mhz
	 nop

_xtal_25mhz:
	li	a0, 25
	li	a1, 0xc0004802
	b	_calc_fbdiv
	 nop

_xtal_20mhz:
	li	a0, 20
	li	a1, 0xc0004802
	b	_calc_fbdiv
	 nop

_xtal_40mhz:
	li	a0, 20
	li	a1, 0xc0005802

_calc_fbdiv:
	li	t0, CONFIG_MT7621_CPU_FREQ_LEGACY
	div	t0, a0
	mflo	t0
	subu	t0, 1
	andi	t0, RG_MEPL_FBDIV_M
	sll	t0, RG_MEPL_FBDIV_S
	or	t0, t0, a1
	li	t1, DRAMC_BIN_LOADADDR + 0x20
	sw	t0, 0(t1)

	/* Save context */
	li	t0, 0xBE10DFF0
	sw	ra, 0(t0)

	.set	push
	.set	noat

	li	t0, 0xBE10DF00
	sw	$1, 0(t0)
	sw	$2, 4(t0)
	sw	$3, 8(t0)
	sw	$4, 12(t0)
	sw	$5, 16(t0)
	sw	$6, 20(t0)
	sw	$7, 24(t0)
	sw	$16, 28(t0)
	sw	$17, 32(t0)
	sw	$18, 36(t0)
	sw	$19, 40(t0)
	sw	$20, 44(t0)
	sw	$21, 48(t0)
	sw	$22, 52(t0)
	sw	$23, 56(t0)
	sw	$24, 60(t0)
	sw	$25, 64(t0)
	sw	$26, 68(t0)
	sw	$27, 72(t0)
	sw	$28, 76(t0)
	sw	$29, 80(t0)
	sw	$30, 84(t0)
	sw	$31, 88(t0)

	li	t9, 0xBE108800
	jalr	t9
	 nop

	li	t0, 0xBE10DFF0
	lw	ra, 0(t0)

	li	t0, 0xBE10DF00
	lw	$1, 0(t0)
	lw	$2, 4(t0)
	lw	$3, 8(t0)
	lw	$4, 12(t0)
	lw	$5, 16(t0)
	lw	$6, 20(t0)
	lw	$7, 24(t0)
	lw	$16,28(t0)
	lw	$17, 32(t0)
	lw	$18, 36(t0)
	lw	$19, 40(t0)
	lw	$20, 44(t0)
	lw	$21, 48(t0)
	lw	$22, 52(t0)
	lw	$23, 56(t0)
	lw	$24, 60(t0)
	lw	$25, 64(t0)
	lw	$26, 68(t0)
	lw	$27, 72(t0)
	lw	$28, 76(t0)
	lw	$29, 80(t0)
	lw	$30, 84(t0)
	lw	$31, 88(t0)

	.set	pop

	jr	ra
	 nop
	END(lowlevel_init)
