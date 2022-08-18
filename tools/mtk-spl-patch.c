// SPDX-License-Identifier:	GPL-2.0+
/*
* Patch SPL data record for MediaTek's specific SPL U-Boot
*
* Copyright (C) 2018  MediaTek, Inc.
* Author: Weijie Gao <weijie.gao@mediatek.com>
*
*/

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#define MTK_SPL_ROM_CFG_OFFS		0x40

#define MTK_MAGIC_MT7621		0x31323637

struct mtk_spl_rom_cfg {
	uint32_t magic;
	uint32_t size;
	uint32_t align;
};

static int mtk_spl_pad(char *buff, off_t size);

static void usage(const char *prog)
{
	fprintf(stderr,
		"  Usage: %s <spl_bin>\n"
		"\n"
		"  Patch SPL data record for MediaTek specific SPL U-Boot\n",
		prog
	);

	exit(0);
}

int main(int argc, char *argv[])
{
	int err;
	int fd = -1;
	struct stat sbuf;
	char *ptr;

	if (!argv[1])
		usage(argv[0]);

	fd = open(argv[1], O_RDWR | O_BINARY);
	if (fd == -1) {
		fprintf(stderr, "%s: Can't open %s: %s\n",
			argv[0], argv[1], strerror(errno));
		return errno;
	}

	if (fstat(fd, &sbuf) < 0) {
		fprintf(stderr, "%s: Can't stat %s: %s\n",
			argv[0], argv[1], strerror(errno));
		return errno;
	}

	ptr = mmap(0, sbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED) {
		fprintf(stderr, "%s: Can't map %s: %s\n",
			argv[0], argv[1], strerror(errno));
		return errno;
	}

	err = mtk_spl_pad(ptr, sbuf.st_size);

	(void) munmap((void *) ptr, sbuf.st_size);

	/* We're a bit of paranoid */
#if defined(_POSIX_SYNCHRONIZED_IO) && \
   !defined(__sun__) && \
   !defined(__FreeBSD__) && \
   !defined(__OpenBSD__) && \
   !defined(__APPLE__)
	(void) fdatasync(fd);
#else
	(void) fsync(fd);
#endif

	if (close(fd)) {
		fprintf(stderr, "%s: Write error on %s: %s\n",
			argv[0], argv[1], strerror(errno));
		return errno;
	}

	return err;
}

static int mtk_spl_pad(char *buff, off_t size)
{
	struct mtk_spl_rom_cfg *rom_cfg;

	/* Fetch ROM config */
	rom_cfg = (struct mtk_spl_rom_cfg *) (buff + MTK_SPL_ROM_CFG_OFFS);

	/* Check rom_cfg magic */
	switch (le32toh(rom_cfg->magic)) {
	case MTK_MAGIC_MT7621:
		break;
	default:
		fprintf(stderr, "Not a MediaTek specific SPL binray\n");
		return EINVAL;
	}

	/* Write SPL size */
	rom_cfg->size = htole32(size);

	return 0;
}