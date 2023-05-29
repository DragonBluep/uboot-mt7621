/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _BOARD_RALINK_DUAL_IMAGE_H_
#define _BOARD_RALINK_DUAL_IMAGE_H_

#include <linux/types.h>

struct di_part_info {
	const char *name;
	u64 addr;
	u64 size;
	u32 blocksize;
	void *flash;
};

struct di_image_info {
	struct di_part_info  part;
	void *data;

	u32 kernel_size;
	u32 padding_size;
	u32 rootfs_size;
	u32 marker_size;
};

int dual_image_check(void);
int dual_image_check_single_ram(void *data, size_t size, const char *partname,
				struct di_image_info *ii);
int dual_image_update_backup(const struct di_image_info *iif);

#endif /* _BOARD_RALINK_DUAL_IMAGE_H_ */
