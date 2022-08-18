// SPDX-License-Identifier: GPL-2.0+
/*
 * Bit-reversed version of crc32c.c
 */

#ifdef __UBOOT__
#include <common.h>
#endif

#include <u-boot/crc.h>
#include <compiler.h>

uint32_t crc32c_be_cal(uint32_t crc, const char *data, int length,
		    uint32_t *crc32c_table)
{
	while (length--)
		crc = crc32c_table[(uint8_t)((crc >> 24) ^ *data++)] ^ (crc << 8);

	return crc;
}

void crc32c_be_init(uint32_t *crc32c_table, uint32_t pol)
{
	int i, j;
	uint32_t v;
	const uint32_t poly = pol; /* Bit-reflected CRC32C polynomial */

	for (i = 0; i < 256; i++) {
		v = i << 24;
		for (j = 0; j < 8; j++)
			v = (v << 1) ^ ((v & (1 << 31)) ? poly : 0);

		crc32c_table[i] = v;
	}
}
