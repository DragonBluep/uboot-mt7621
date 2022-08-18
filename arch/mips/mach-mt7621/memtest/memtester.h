/*
 * Very simple (yet, for some reason, very effective) memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Version 3 not publicly released.
 * Version 4 rewrite:
 * Copyright (C) 2004-2012 Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Licensed under the terms of the GNU General Public License version 2 (only).
 * See the file COPYING for details.
 *
 * This file contains the declarations for external variables from the main file.
 * See other comments in that file.
 *
 */

#ifndef _MEMTESTER_H_
#define _MEMTESTER_H_

#include <linux/types.h>

#define UL_LEN 32

typedef unsigned long ul;
typedef unsigned long long ull;
typedef unsigned long volatile ulv;
typedef unsigned char volatile u8v;
typedef unsigned short volatile u16v;

struct test {
	char *name;
	int(*fp)(ulv *bufa, ulv *bufb, size_t count);
};

union {
	unsigned char bytes[UL_LEN / 8];
	ul val;
} mword8;

union {
	unsigned short u16s[UL_LEN / 16];
	ul val;
} mword16;

void memtest(u32 memsize);

#endif /* _MEMTESTER_H_ */
