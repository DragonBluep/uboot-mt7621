/*
 * Very simple yet very effective memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Version 3 not publicly released.
 * Version 4 rewrite:
 * Copyright (C) 2004-2012 Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Licensed under the terms of the GNU General Public License version 2 (only).
 * See the file COPYING for details.
 *
 * This file contains the declarations for the functions for the actual tests,
 * called from the main routine in memtester.c.  See other comments in that 
 * file.
 *
 */

#ifndef _TESTS_H_
#define _TESTS_H_

/* Function declaration. */

#include "memtester.h"

#define rand32() ((unsigned int) rand() | ( (unsigned int) rand() << 16))

#define rand_ul() rand32()
#define UL_ONEBITS 0xffffffff
#define CHECKERBOARD1 0x55555555
#define CHECKERBOARD2 0xaaaaaaaa
#define UL_BYTE(x) (((x) | (x) << 8 | (x) << 16 | (x) << 24))

int test_stuck_address(ulv *bufa, size_t count);
int test_random_value(ulv *bufa, ulv *bufb, size_t count);
int test_xor_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_sub_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_mul_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_div_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_or_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_and_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_seqinc_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_solidbits_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_checkerboard_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_blockseq_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_walkbits0_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_walkbits1_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_bitspread_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_bitflip_comparison(ulv *bufa, ulv *bufb, size_t count);
#ifdef TEST_NARROW_WRITES
int test_8bit_wide_random(ulv *bufa, ulv *bufb, size_t count);
int test_16bit_wide_random(ulv *bufa, ulv *bufb, size_t count);
#endif

#endif /* _TESTS_H_ */
