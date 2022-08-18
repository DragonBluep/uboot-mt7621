// SPDX-License-Identifier:	GPL-2.0
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef	_SPL_HELPER_H_
#define	_SPL_HELPER_H_

extern char *get_mtk_stage2_image_ptr(char *data, size_t size);
extern int check_mtk_stock_stage2_info(size_t *stage2_off_min,
				       size_t *stage2_off,
				       size_t *align_size);

/* from board, get the nand device */
extern struct mtd_info *get_mtk_board_nand_mtd(void);

#endif /* _SPL_HELPER_H_ */
