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
int mtk_board_flash_adjust_offset(void *flashdev, uint64_t base,
				  uint64_t offset, uint64_t limit,
				  uint64_t *result);
int mtk_board_flash_erase(void *flashdev, uint64_t offset, uint64_t len,
			  uint64_t limit);
int mtk_board_flash_read(void *flashdev, uint64_t offset, size_t len,
			uint64_t limit, void *buf);
int mtk_board_flash_write(void *flashdev, uint64_t offset, size_t len,
			  uint64_t limit, const void *buf);
int mtk_board_flash_verify_cust(void *flashdev, uint64_t offset, size_t len,
				uint64_t limit, const void *buf, void *vbuf,
				size_t vbuf_size);
int mtk_board_flash_verify(void *flashdev, uint64_t offset, size_t len,
			   uint64_t limit, const void *buf);

#endif /* _BOARD_RALINK_FLASH_HELPER_H_ */
