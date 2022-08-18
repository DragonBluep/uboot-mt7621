/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _BOARD_RALINK_FLASH_HELPER_H_
#define _BOARD_RALINK_FLASH_HELPER_H_

#include <linux/types.h>

int get_mtd_part_info(const char *partname, uint64_t *off, uint64_t *size);

void *mtk_board_get_flash_dev(void);
size_t mtk_board_get_flash_erase_size(void *flashdev);
int mtk_board_flash_erase(void *flashdev, uint64_t offset, uint64_t len);
int mtk_board_flash_read(void *flashdev, uint64_t offset, size_t len,
			 void *buf);
int mtk_board_flash_write(void *flashdev, uint64_t offset, size_t len,
			  const void *buf);

#endif /* _BOARD_RALINK_FLASH_HELPER_H_ */
