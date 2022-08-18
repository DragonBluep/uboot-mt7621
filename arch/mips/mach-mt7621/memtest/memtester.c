/*
* memtester version 4
*
* Very simple but very effective user-space memory tester.
* Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
* Version 2 by Charles Cazabon <charlesc-memtester@pyropus.ca>
* Version 3 not publicly released.
* Version 4 rewrite:
* Copyright (C) 2004-2012 Charles Cazabon <charlesc-memtester@pyropus.ca>
* Licensed under the terms of the GNU General Public License version 2 (only).
* See the file COPYING for details.
*
*/

#define __version__ "4.3.0"

#include <common.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <linux/sizes.h>
#include <asm/addrspace.h>
#include <asm/mipsregs.h>
#include <asm/io.h>
#include <mach/mt7621_regs.h>

#include "tests.h"

DECLARE_GLOBAL_DATA_PTR;

#define UART_BASE		CONFIG_DEBUG_UART_BASE

#define UART_RBR		0x00

#define UART_LSR		0x14
#define LSR_DR			(1 << 0)

#define ENTRYLO_ATTRS		((CONF_CM_UNCACHED << ENTRYLO_C_SHIFT) | \
				ENTRYLO_D | ENTRYLO_V | ENTRYLO_G)

struct test tests[] = {
	{ "Random Value", test_random_value },
	{ "Compare XOR", test_xor_comparison },
	{ "Compare SUB", test_sub_comparison },
	{ "Compare MUL", test_mul_comparison },
	{ "Compare DIV",test_div_comparison },
	{ "Compare OR", test_or_comparison },
	{ "Compare AND", test_and_comparison },
	{ "Sequential Increment", test_seqinc_comparison },
	{ "Solid Bits", test_solidbits_comparison },
	{ "Block Sequential", test_blockseq_comparison },
	{ "Checkerboard", test_checkerboard_comparison },
	{ "Bit Spread", test_bitspread_comparison },
	{ "Bit Flip", test_bitflip_comparison },
	{ "Walking Ones", test_walkbits1_comparison },
	{ "Walking Zeroes", test_walkbits0_comparison },
#ifdef TEST_NARROW_WRITES
	{ "8-bit Writes", test_8bit_wide_random },
	{ "16-bit Writes", test_16bit_wide_random },
#endif
	{ NULL, NULL }
};

extern void write_one_tlb(int index, u32 pagemask, u32 hi, u32 low0, u32 low1);

static int early_tstc(void)
{
	return readl((void *) KSEG1ADDR(UART_BASE + UART_LSR)) & LSR_DR;
}

static int early_getc(void)
{
	while (!early_tstc())
		;

	return readl((void *) KSEG1ADDR(UART_BASE + UART_RBR));
}

static void do_memtest(void *base, size_t size)
{
	size_t halflen, count;
	ulv *bufa, *bufb;
	ul i;

	halflen = size / 2;
	count = halflen / sizeof(ul);
	bufa = (ulv *) base;
	bufb = (ulv *) ((size_t) base + halflen);

	printf("  %-20s: ", "Stuck Address");

	if (!test_stuck_address(base, size / sizeof(ul)))
		printf("ok\n");

	for (i = 0;; i++) {
		if (!tests[i].name)
			break;

		printf("  %-20s: ", tests[i].name);
		if (!tests[i].fp(bufa, bufb, count))
			printf("ok\n");
	}

	printf("\n");
	printf("Done.\n");
}

void memtest(u32 memsize)
{
	u32 st;
	char act = 0;
	void *test_base = (void *) KSEG1;
	int loop_cond = 0;

	while (early_tstc())
		early_getc();

	printf("Press 1 to start memtest, 2 to start infinite memtest: ");

	st = get_timer(0);
	while (get_timer(st) < 1000) {
		if (early_tstc()) {
			act = early_getc();
			break;
		}
	}

	putc(act);
	printf("\n\n");

	switch (act) {
	case '1':
		break;
	case '2':
		loop_cond = 1;
		break;
	default:
		return;
	}

	/*
	 * Setup MMU to map maximum 512MB of DRAM into a single linear region
	 *
	 * Organization:
	 *    Base: KSEG2 0xc0000000
	 *    TLB : 0 -> 128M even 0x00000000 - 0x08000000
	 *               128M odd  0x08000000 - 0x10000000
	 *          1 -> 32M  even 0x10000000 - 0x12000000
	 *               32M  odd  0x12000000 - 0x14000000
	 *          2 -> 32M  even 0x14000000 - 0x16000000
	 *               32M  odd  0x16000000 - 0x18000000
	 *          3 -> 32M  even 0x18000000 - 0x1a000000
	 *               32M  odd  0x1a000000 - 0x1c000000
	 *          4 -> 32M  even 0x20000000 - 0x22000000
	 *               32M  odd  0x22000000 - 0x24000000
	 */

	if (memsize > SZ_256M) {
		write_one_tlb(0, PM_256M, KSEG2,
			(0x00000000 >> 6) | ENTRYLO_ATTRS,
			(0x08000000 >> 6) | ENTRYLO_ATTRS);
		write_one_tlb(1, PM_64M, KSEG2 + 0x10000000,
			(0x10000000 >> 6) | ENTRYLO_ATTRS,
			(0x12000000 >> 6) | ENTRYLO_ATTRS);
		write_one_tlb(2, PM_64M, KSEG2 + 0x14000000,
			(0x14000000 >> 6) | ENTRYLO_ATTRS,
			(0x16000000 >> 6) | ENTRYLO_ATTRS);
		write_one_tlb(3, PM_64M, KSEG2 + 0x18000000,
			(0x18000000 >> 6) | ENTRYLO_ATTRS,
			(0x1a000000 >> 6) | ENTRYLO_ATTRS);
		write_one_tlb(4, PM_64M, KSEG2 + 0x1c000000,
			(0x20000000 >> 6) | ENTRYLO_ATTRS,
			(0x22000000 >> 6) | ENTRYLO_ATTRS);

		test_base = (void *) KSEG2;
	}

	do {
		do_memtest(test_base, memsize);
	} while (loop_cond);

	/* Invalidate TLB */
	if (memsize > SZ_256M) {
		write_one_tlb(0, 0, 0, 0, 0);
		write_one_tlb(1, 0, 0, 0, 0);
		write_one_tlb(2, 0, 0, 0, 0);
		write_one_tlb(3, 0, 0, 0, 0);
		write_one_tlb(4, 0, 0, 0, 0);
	}
}
