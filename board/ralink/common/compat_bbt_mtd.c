// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <dm.h>
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <linux/err.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/rawnand.h>
#include <linux/errno.h>
#include <linux/sizes.h>
#include <asm/io.h>

#ifdef CONFIG_SYS_MAX_NAND_DEVICE
const u8 mtk_bbt_sig[] = "mtknand";
#define MTK_BBT_SIG_OFF			1
#define MTK_BBT_MAX_BLOCK_NUM		32
#define MTK_BBT_BITS_PER_BLOCK		2
#define MTK_BBT_GOOD_BLOCK_MASK		0
#define MTK_BBT_RETRIES			3

struct mt7621_nand_bbt_compat {
	struct mtd_info mtd;		/* new mtd layer */
	struct mtd_info *nand_mtd;	/* original NAND-based mtd */

	char mtdname[32];

	u8 *bbt;
	u32 bbt_block;			/* Physical block number */
	u32 bbt_num_blocks;
	u32 bbt_size;
	int bbt_exist;
	u32 num_blocks;

	s32 *bmt;		/* Logic block index, physical block number */
	u32 avail_blocks;
};

static int mnbc_erase(struct mtd_info *mtd, struct erase_info *instr);
static int mnbc_write_oob(struct mtd_info *mtd, loff_t to,
	struct mtd_oob_ops *ops);

static struct mt7621_nand_bbt_compat mnbc_list[CONFIG_SYS_MAX_NAND_DEVICE];
static int mnbc_num;

static inline struct mt7621_nand_bbt_compat *mtd_to_mnbc(struct mtd_info *mtd)
{
	return container_of(mtd, struct mt7621_nand_bbt_compat, mtd);
}

static void mnbc_bbt_mark_bad(struct mt7621_nand_bbt_compat *mnbc,
			      u32 blockidx)
{
	u32 idx, pos;

	if (blockidx >= mnbc->num_blocks)
		return;

	idx = blockidx / (8 / MTK_BBT_BITS_PER_BLOCK);
	pos = blockidx % (8 / MTK_BBT_BITS_PER_BLOCK);

	mnbc->bbt[idx] |= ((1 << MTK_BBT_BITS_PER_BLOCK) - 1) <<
			  (pos * MTK_BBT_BITS_PER_BLOCK);
}

static int mnbc_bbt_is_bad(struct mt7621_nand_bbt_compat *mnbc, u32 blockidx)
{
	u32 idx, pos;

	if (blockidx >= mnbc->num_blocks)
		return -1;

	idx = blockidx / (8 / MTK_BBT_BITS_PER_BLOCK);
	pos = blockidx % (8 / MTK_BBT_BITS_PER_BLOCK);

	return ((mnbc->bbt[idx] >> (pos * MTK_BBT_BITS_PER_BLOCK)) &
		((1 << MTK_BBT_BITS_PER_BLOCK) - 1)) !=
		MTK_BBT_GOOD_BLOCK_MASK;
}

static void mnbc_update_bmt(struct mt7621_nand_bbt_compat *mnbc)
{
	u32 pb, lb;

	for (pb = 0, lb = 0; pb < mnbc->num_blocks; pb++) {
		if (mnbc_bbt_is_bad(mnbc, pb))
			continue;

		mnbc->bmt[lb++] = pb;
	}

	mnbc->avail_blocks = lb;

	/* Unusable blocks */
	for (pb = lb; pb < mnbc->num_blocks; pb++)
		mnbc->bmt[pb] = -1;
}

static void __maybe_unused mnbc_rescan_bbt_bmt(
	struct mt7621_nand_bbt_compat *mnbc)
{
	u32 pb;
	loff_t addr;

	memset(mnbc->bbt, 0, mnbc->bbt_size);

	/* Scan for bad blocks not marked in BBT */
	for (pb = 0; pb < mnbc->num_blocks; pb++) {
		addr = pb << mnbc->nand_mtd->erasesize_shift;

		if (mtd_block_isbad(mnbc->nand_mtd, addr))
			mnbc_bbt_mark_bad(mnbc, pb);
	}

	mnbc_update_bmt(mnbc);
}

static int mnbc_bbt_wb(struct mt7621_nand_bbt_compat *mnbc)
{
	int retries = 0;
	u32 i;
	loff_t off = 0;
	struct mtd_oob_ops ops;
	struct erase_info instr;

#ifdef CONFIG_COMPAT_NAND_BBT_WB
	return -ENOSYS;
#endif

	/* Convert physical block number to logic block number */
	for (i = mnbc->num_blocks - 1; i > 0; i++) {
		if (mnbc->bmt[i] == mnbc->bbt_block) {
			off = (loff_t) i << mnbc->mtd.erasesize_shift;
			break;
		}
	}

	memset(&instr, 0, sizeof(instr));

	instr.len = (loff_t) mnbc->bbt_num_blocks << mnbc->mtd.erasesize_shift;

	memset(&ops, 0, sizeof(ops));
	ops.datbuf = mnbc->bbt;
	ops.oobbuf = (u8 *) mtk_bbt_sig;
	ops.len = mnbc->bbt_size;
	ops.ooblen = sizeof(mtk_bbt_sig) - 1;
	ops.ooboffs = MTK_BBT_SIG_OFF;
	ops.mode = MTD_OPS_PLACE_OOB;

	while (retries++ < MTK_BBT_RETRIES) {
		if (retries || !off)
			off = (loff_t) (mnbc->avail_blocks -
					mnbc->bbt_num_blocks) <<
					mnbc->mtd.erasesize_shift;

		instr.addr = off;
		if (mnbc_erase(&mnbc->mtd, &instr))
			continue;

		if (instr.state != MTD_ERASE_DONE)
			continue;

		if (mnbc_write_oob(&mnbc->mtd, off, &ops))
			continue;

		if (ops.retlen != ops.len || ops.oobretlen != ops.ooblen)
			continue;

		return 0;
	}

	printf("%s: failed to write BBT\n", __func__);
	return 1;
}

static int mnbc_read_bbt_skip_bad(struct mt7621_nand_bbt_compat *mnbc,
				  u32 blockidx)
{
	u32 size_read = 0, size_to_read;
	loff_t off;
	size_t retlen;
	int ret;

	while (blockidx < mnbc->num_blocks) {
		if (size_read >= mnbc->bbt_size)
			return 0;

		off = blockidx << mnbc->nand_mtd->erasesize_shift;

		if (mtd_block_isbad(mnbc->nand_mtd, off)) {
			blockidx++;
			continue;
		}

		size_to_read = mnbc->bbt_size - size_read;
		if (mnbc->nand_mtd->erasesize < size_to_read)
			size_to_read = mnbc->nand_mtd->erasesize;

		ret = mtd_read(mnbc->nand_mtd, off, size_to_read, &retlen,
			       mnbc->bbt + size_read);
		if ((ret == 0 || ret == -EUCLEAN) && (retlen == size_to_read))
			size_read += size_to_read;
	}

	if (size_read < mnbc->bbt_size)
		return 1;

	return 0;
}

static int mnbc_init_bbt(struct mt7621_nand_bbt_compat *mnbc)
{
	struct mtd_oob_ops ops;
	u8 sig[sizeof (mtk_bbt_sig) + MTK_BBT_SIG_OFF];
	u32 num_bits, pb;
	loff_t addr;
	int ret;
	int bbtwb = 0;

	mnbc->num_blocks = mnbc->nand_mtd->size >>
			   mnbc->nand_mtd->erasesize_shift;

	/* Number of bits needed for BBT */
	num_bits = mnbc->num_blocks * MTK_BBT_BITS_PER_BLOCK;

	/* Bits to bytes */
	mnbc->bbt_size = DIV_ROUND_UP(num_bits, 8);

	mnbc->bbt_num_blocks =
		DIV_ROUND_UP(mnbc->bbt_size, mnbc->nand_mtd->erasesize);

	mnbc->bbt = (u8 *) malloc(mnbc->bbt_size);
	if (!mnbc->bbt) {
		printf("mt7621_nand_bbt_compat: "
		       "Unable to allocate %d bytes for BBT\n",
			mnbc->bbt_size);
		return -EINVAL;
	}

	/* Try to read existing bbt */
	memset(&ops, 0, sizeof(ops));
	ops.datbuf = NULL;
	ops.oobbuf = sig;
	ops.len = 0;
	ops.ooblen = sizeof(sig);
	ops.mode = MTD_OPS_RAW;

	for (pb = mnbc->num_blocks - mnbc->bbt_num_blocks;
	     pb >= mnbc->num_blocks - MTK_BBT_MAX_BLOCK_NUM;
	     pb--) {
		addr = pb << mnbc->nand_mtd->erasesize_shift;

		if (mtd_block_isbad(mnbc->nand_mtd, addr))
			continue;

		ret = mtd_read_oob(mnbc->nand_mtd, addr, &ops);

		if (ret < 0 && ret != -EUCLEAN)
			continue;

		if (memcmp(sig + MTK_BBT_SIG_OFF, mtk_bbt_sig,
			    sizeof(mtk_bbt_sig) - 1))
			continue;

		if (!mnbc_read_bbt_skip_bad(mnbc, pb)) {
			debug("Factory BBT found 0x%08llx\n", addr);
			mnbc->bbt_block = pb;
			mnbc->bbt_exist = 1;
			break;
		}
	}

	/* If BBT does not exists in NAND, create new one */
	if (!mnbc->bbt_exist)
		memset(mnbc->bbt, 0, mnbc->bbt_size);

	/* Scan for bad blocks not marked in BBT */
	for (pb = 0; pb < mnbc->num_blocks; pb++) {
		addr = pb << mnbc->nand_mtd->erasesize_shift;

		if (mtd_block_isbad(mnbc->nand_mtd, addr)) {
			if (!mnbc_bbt_is_bad(mnbc, pb)) {
				mnbc_bbt_mark_bad(mnbc, pb);
				bbtwb = 1;
			}
		}
	}

	/* Create Block Mapping Table (memory only) */
	mnbc->bmt = (s32 *) malloc(mnbc->num_blocks * sizeof(*mnbc->bmt));
	if (!mnbc->bmt) {
		printf("mt7621_nand_bbt_compat: "
		       "Unable to allocate %d bytes for BMT\n",
		       mnbc->num_blocks * sizeof(*mnbc->bmt));
		free(mnbc->bbt);
		return -ENOMEM;
	}

	mnbc_update_bmt(mnbc);

	/* Need to write back BBT */
	if (bbtwb)
		mnbc_bbt_wb(mnbc);

	return 0;
}

static int mnbc_erase_block_check(struct mtd_info *mtd, u32 pb)
{
	struct mt7621_nand_bbt_compat *mnbc = mtd_to_mnbc(mtd);
	u32 page_per_block = mtd->erasesize >> mtd->writesize_shift;
	struct mtd_oob_ops ops;
	u8 buf[SZ_4K + SZ_1K];
	loff_t addr;
	u32 i, j;
	int ret;

	if (mtd->writesize + mtd->oobsize > sizeof(buf))
		return 1;

	memset(&ops, 0, sizeof(ops));

	addr = pb << mtd->erasesize_shift;

	for (i = 0; i < page_per_block; i++) {
		ops.datbuf = buf;
		ops.len = mtd->writesize;
		ops.oobbuf = buf + mtd->writesize;
		ops.ooblen = mtd->oobsize;
		ops.ooboffs = 0;
		ops.mode = MTD_OPS_RAW;

		ret = mtd_read_oob(mnbc->nand_mtd, addr, &ops);
		if ((ret && ret != -EUCLEAN) || (ops.retlen != ops.len) ||
		    (ops.oobretlen != ops.ooblen))
			return 1;

		for (j = 0; j < mtd->writesize + mtd->oobsize; j++)
			if (buf[j] != 0xff)
				return 1;

		addr += mtd->writesize;
	}

	return 0;
}

static int mnbc_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct mt7621_nand_bbt_compat *mnbc = mtd_to_mnbc(mtd);
	struct erase_info ei;
	loff_t start, end;
	u32 pb, lb;
	int ret, retries = 0;

	start = instr->addr & ~mtd->erasesize_mask;
	end = ALIGN(instr->addr + instr->len, mtd->erasesize);

	ei = *instr;
	ei.mtd = mnbc->nand_mtd;
	ei.next = NULL;
	ei.callback = NULL;

	while (start < end) {
		lb = start >> mtd->erasesize_shift;
		pb = mnbc->bmt[lb];

		if ((s32) pb == -1) {
			printf("%s: no usable block at 0x%08llx\n",
				__func__, (loff_t) lb << mtd->erasesize_shift);
			instr->state = MTD_ERASE_FAILED;
			instr->fail_addr = (loff_t) lb << mtd->erasesize_shift;
			goto erase_exit;
		}

		ei.addr = pb << mtd->erasesize_shift;
		ei.len = mtd->erasesize;
		debug("erasing lb %d (%08llx), pb %d (%08llx)\n", lb, start,
		      pb, ei.addr);

retry:
		ret = mtd_erase(mnbc->nand_mtd, &ei);
		if (ret) {
			if (mnbc_erase_block_check(mtd, pb)) {
				if (retries < MTK_BBT_RETRIES) {
					nand_reset(mtd_to_nand(mnbc->nand_mtd),
						   0);
					retries++;
					goto retry;
				} else {
					printf("New bad block during erasing: "
					       "lb %d (%08llx), pb %d (%08llx)"
					       "\n", lb, start, pb, ei.addr);

					mnbc->nand_mtd->_block_markbad(
						mnbc->nand_mtd,
						pb << mtd->erasesize_shift);
					mnbc_bbt_mark_bad(mnbc, pb);

					mnbc_update_bmt(mnbc);

					continue;
				}
			}
		}

		instr->state = ei.state;
		if (ei.state & MTD_ERASE_FAILED) {
			if (ei.fail_addr != MTD_FAIL_ADDR_UNKNOWN)
				instr->fail_addr = start;
			else
				instr->fail_addr = MTD_FAIL_ADDR_UNKNOWN;
		}

		start += mtd->erasesize;
	}

erase_exit:

	ret = instr->state == MTD_ERASE_DONE ? 0 : -EIO;

	/* Do call back function */
	if (!ret)
		mtd_erase_callback(instr);

	/* Return more or less happy */
	return ret;
}

static int mnbc_read(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, uint8_t *buf)
{
	struct mt7621_nand_bbt_compat *mnbc = mtd_to_mnbc(mtd);
	struct mtd_ecc_stats old_stats;
	loff_t start, end, off;
	size_t size_to_read, size_read, _retlen;
	int ret, euclean = 0, retries = 0;
	u32 pb, lb;

	start = from;
	end = from + len;
	size_read = 0;

	while (start < end) {
		lb = start >> mtd->erasesize_shift;
		pb = mnbc->bmt[lb];
		if ((s32) pb == -1) {
			printf("%s: no usable block at 0x%08llx\n",
				__func__, (loff_t) lb << mtd->erasesize_shift);
			mtd->ecc_stats = mnbc->nand_mtd->ecc_stats;
			*retlen = size_read;
			return -EIO;
		}

		off = start & mtd->erasesize_mask;
		size_to_read = mtd->erasesize - off;
		if (len - size_read < size_to_read)
			size_to_read = len - size_read;

		debug("reading lb %d (%08llx), pb %d (%08llx), size %08x\n",
		      lb, start, pb, (pb << mtd->erasesize_shift) | off,
		      size_to_read);

		old_stats = mnbc->nand_mtd->ecc_stats;

retry:
		ret = mtd_read(mnbc->nand_mtd,
			       (pb << mtd->erasesize_shift) | off,
			       size_to_read, &_retlen, buf + size_read);
		if ((ret && ret != -EUCLEAN) || _retlen != size_to_read) {
			if (ret == -EBADMSG) {
				mtd->ecc_stats = mnbc->nand_mtd->ecc_stats;
				*retlen = size_read + _retlen;
				debug("read with ecc failed, %d bytes read\n",
				       _retlen);
				return -EBADMSG;
			}

			mnbc->nand_mtd->ecc_stats = old_stats;
			if (retries < MTK_BBT_RETRIES) {
				nand_reset(mtd_to_nand(mnbc->nand_mtd), 0);
				retries++;
				goto retry;
			} else {
				printf("New bad block during reading: "
				       "lb %d (%08llx), pb %d (%08llx)\n",
				       lb, start, pb,
				       (pb << mtd->erasesize_shift) | off);

				mnbc->nand_mtd->_block_markbad(mnbc->nand_mtd,
					pb << mtd->erasesize_shift);
				mnbc_bbt_mark_bad(mnbc, pb);

				mnbc_update_bmt(mnbc);

				continue;
			}
		}

		if (ret == -EUCLEAN)
			euclean = 1;

		size_read += size_to_read;
		start += size_to_read;
	}

	mtd->ecc_stats = mnbc->nand_mtd->ecc_stats;
	*retlen = len;

	return euclean ? -EUCLEAN : 0;
}

static int mnbc_write(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const uint8_t *buf)
{
	struct mt7621_nand_bbt_compat *mnbc = mtd_to_mnbc(mtd);
	loff_t start, end, off;
	size_t size_to_write, size_written, _retlen;
	u32 pb, lb;
	int ret, retries = 0;

	start = to;
	end = to + len;
	size_written = 0;

	while (start < end) {
		lb = start >> mtd->erasesize_shift;
		pb = mnbc->bmt[lb];
		if ((s32) pb == -1) {
			printf("%s: no usable block at 0x%08llx\n",
				__func__, (loff_t) lb << mtd->erasesize_shift);
			*retlen = size_written;
			return -EIO;
		}

		off = start & mtd->erasesize_mask;
		size_to_write = mtd->erasesize - off;
		if (len - size_written < size_to_write)
			size_to_write = len - size_written;

		debug("writing lb %d (%08llx), pb %d (%08llx), size %08x\n",
		       lb, start, pb, (pb << mtd->erasesize_shift) | off,
		       size_to_write);

retry:
		ret = mtd_write(mnbc->nand_mtd,
				(pb << mtd->erasesize_shift) | off,
				size_to_write, &_retlen,
				buf + size_written);
		if (ret || _retlen != size_to_write) {
			if (retries < MTK_BBT_RETRIES) {
				nand_reset(mtd_to_nand(mnbc->nand_mtd), 0);
				retries++;
				goto retry;
			} else {
				printf("New bad block during reading: "
				       "lb %d (%08llx), pb %d (%08llx)\n",
				       lb, start, pb,
				       (pb << mtd->erasesize_shift) | off);

				mnbc->nand_mtd->_block_markbad(mnbc->nand_mtd,
					pb << mtd->erasesize_shift);
				mnbc_bbt_mark_bad(mnbc, pb);

				mnbc_update_bmt(mnbc);

				continue;
			}
		}

		size_written += size_to_write;
		start += size_to_write;
	}

	*retlen = len;

	return 0;
}

static int mnbc_panic_write(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const uint8_t *buf)
{
	return mnbc_write(mtd, to, len, retlen, buf);
}

static int mnbc_read_oob(struct mtd_info *mtd, loff_t from,
	struct mtd_oob_ops *ops)
{
	struct mt7621_nand_bbt_compat *mnbc = mtd_to_mnbc(mtd);
	u32 pb, lb;
	loff_t off;

	lb = from >> mtd->erasesize_shift;
	pb = mnbc->bmt[lb];
	if ((s32) pb == -1) {
		printf("%s: no usable block at 0x%08llx\n",
		       __func__, (loff_t) lb << mtd->erasesize_shift);
		return -EIO;
	}

	off = from & mtd->erasesize_mask;

	debug("reading oob lb %d (%08llx), pb %d (%08llx)\n", lb, from, pb,
	      (pb << mtd->erasesize_shift) | off);

	return mtd_read_oob(mnbc->nand_mtd,
			    (pb << mtd->erasesize_shift) | off, ops);
}

static int mnbc_write_oob(struct mtd_info *mtd, loff_t to,
	struct mtd_oob_ops *ops)
{
	struct mt7621_nand_bbt_compat *mnbc = mtd_to_mnbc(mtd);
	u32 pb, lb;
	loff_t off;

	lb = to >> mtd->erasesize_shift;
	pb = mnbc->bmt[lb];
	if ((s32) pb == -1) {
		printf("%s: no usable block at 0x%08llx\n",
		       __func__, (loff_t) lb << mtd->erasesize_shift);
		return -EIO;
	}

	off = to & mtd->erasesize_mask;

	debug("writting oob lb %d (%08llx), pb %d (%08llx)\n", lb, to, pb,
	      (pb << mtd->erasesize_shift) | off);

	return mtd_write_oob(mnbc->nand_mtd,
			     (pb << mtd->erasesize_shift) | off, ops);
}

static void mnbc_sync(struct mtd_info *mtd)
{
	struct mt7621_nand_bbt_compat *mnbc = mtd_to_mnbc(mtd);

	mtd_sync(mnbc->nand_mtd);
}

static int mnbc_block_isreserved(struct mtd_info *mtd, loff_t ofs)
{
	return 0;
}

static int mnbc_block_isbad(struct mtd_info *mtd, loff_t offs)
{
	struct mt7621_nand_bbt_compat *mnbc = mtd_to_mnbc(mtd);
	u32 pb, lb;

	lb = offs >> mtd->erasesize_shift;
	pb = mnbc->bmt[lb];
	if ((s32) pb == -1) {
		printf("%s: no usable block at 0x%08llx\n",
		       __func__, (loff_t) lb << mtd->erasesize_shift);
		return 1;
	}

	return 0;
}

static int mnbc_block_markbad(struct mtd_info *mtd, loff_t offs)
{
	return 0;
}

int mt7621_nand_bbt_compat_create(const char *mtdname)
{
	struct mt7621_nand_bbt_compat *mnbc;
	struct mtd_info *nand_mtd;
	u64 size_avail;
	int ret;

	nand_mtd = get_mtd_device_nm(mtdname);
	if (!nand_mtd)
		return -ENODEV;

	if (mnbc_num >= CONFIG_SYS_MAX_NAND_DEVICE)
		return -ENODEV;

	mnbc = &mnbc_list[mnbc_num++];

	mnbc->nand_mtd = nand_mtd;

	/* Load BBT and scan */
	ret = mnbc_init_bbt(mnbc);
	if (ret < 0)
		return ret;

	/* Calculate the available size */
	size_avail = (uint64_t) (mnbc->avail_blocks - mnbc->bbt_num_blocks) *
				 nand_mtd->erasesize;

	/* Create new MTD device, cloning the original MTD device */
	sprintf(mnbc->mtdname, "mnbc%d", mnbc_num - 1);

	mnbc->mtd.name = mnbc->mtdname;
	mnbc->mtd.size = size_avail;
	mnbc->mtd.type = nand_mtd->type;
	mnbc->mtd.flags = nand_mtd->flags;
	mnbc->mtd.erasesize = nand_mtd->erasesize;
	mnbc->mtd.writesize = nand_mtd->writesize;
	mnbc->mtd.writebufsize = nand_mtd->writebufsize;
	mnbc->mtd.oobsize = nand_mtd->oobsize;
	mnbc->mtd.oobavail = nand_mtd->oobavail;
	mnbc->mtd.erasesize_shift = nand_mtd->erasesize_shift;
	mnbc->mtd.writesize_shift = nand_mtd->writesize_shift;
	mnbc->mtd.erasesize_mask = nand_mtd->erasesize_mask;
	mnbc->mtd.writesize_mask = nand_mtd->writesize_mask;
	mnbc->mtd.bitflip_threshold = nand_mtd->bitflip_threshold;
	mnbc->mtd.ooblayout = nand_mtd->ooblayout;
	mnbc->mtd.ecclayout = nand_mtd->ecclayout;
	mnbc->mtd.ecc_step_size = nand_mtd->ecc_step_size;
	mnbc->mtd.ecc_strength = nand_mtd->ecc_strength;
	mnbc->mtd.numeraseregions = nand_mtd->numeraseregions;
	mnbc->mtd.eraseregions = nand_mtd->eraseregions;
	mnbc->mtd.subpage_sft = nand_mtd->subpage_sft;
	mnbc->mtd.eraseregions = nand_mtd->eraseregions;

	/* But use our own interface functions */
	if (nand_mtd->_erase)
		mnbc->mtd._erase = mnbc_erase;
	if (nand_mtd->_read)
		mnbc->mtd._read = mnbc_read;
	if (nand_mtd->_write)
		mnbc->mtd._write = mnbc_write;
	if (nand_mtd->_panic_write)
		mnbc->mtd._panic_write = mnbc_panic_write;
	if (nand_mtd->_read_oob)
		mnbc->mtd._read_oob = mnbc_read_oob;
	if (nand_mtd->_write_oob)
		mnbc->mtd._write_oob = mnbc_write_oob;
	if (nand_mtd->_sync)
		mnbc->mtd._sync = mnbc_sync;
	mnbc->mtd._block_isreserved = mnbc_block_isreserved;
	mnbc->mtd._block_isbad = mnbc_block_isbad;
	mnbc->mtd._block_markbad = mnbc_block_markbad;

	/* Add MTD device */
	add_mtd_device(&mnbc->mtd);

	return 0;
}

#ifndef CONFIG_SPL_BUILD
static int do_mnbc(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct mt7621_nand_bbt_compat *mnbc;
	u32 i, num_bb = 0, erasesize_shift;
	struct mtd_info *mtd;
	struct erase_info ei;
	size_t retlen;
	u32 addr, len;
	int idx, ret;
	u64 off;

	static int curr_idx;

	if (argc == 1)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "dev")) {
		if (argc == 2)
			return CMD_RET_USAGE;

		if (!mnbc_num) {
			printf("No device available.\n");
			return -EINVAL;
		}

		idx = simple_strtoul(argv[2], NULL, 0);

		if (idx >= mnbc_num) {
			printf("Invalid dev index. Max. is %d\n", mnbc_num - 1);
			return -EINVAL;
		}

		curr_idx = idx;
		printf("%s is current device\n", mnbc_list[curr_idx].mtdname);

		return 0;
	}

	if (curr_idx < 0 || !mnbc_num) {
		printf("No device available.\n");
		return -EINVAL;
	}

	mnbc = &mnbc_list[curr_idx];
	mtd = &mnbc->mtd;
	erasesize_shift = mnbc->mtd.erasesize_shift;

	if (!strcmp(argv[1], "info")) {
		printf("Current device: %d\n", curr_idx);
		printf("Lower layer NAND MTD device: %s\n",
		       mnbc->nand_mtd->name);
		printf("Upper layer NAND MTD device: %s\n",
			mnbc->mtd.name);
		printf("BBT exists in NAND: %s\n",
		       mnbc->bbt_exist ? "yes" : "no");
		printf("BBT size in blocks: %d\n",
		       mnbc->bbt_num_blocks);
		printf("BBT position (physical block number): %d\n",
		       mnbc->bbt_block);
		printf("Total blocks: %d\n",
		       mnbc->num_blocks);
		printf("Available blocks: %d\n",
		       mnbc->avail_blocks);
		printf("Available size: 0x%llx\n",
			mnbc->mtd.size);
		return 0;
	}

	if (!strcmp(argv[1], "bbt")) {
		for (i = 0; i < mnbc->num_blocks; i++) {
			if (mnbc_bbt_is_bad(mnbc, i)) {
				putc('B');
				num_bb++;
			} else {
				putc('-');
			}
		}
		puts("\n");

		if (num_bb) {
			puts("\nBad block summary:\n");
			puts("   BLK    Addr\n");
			for (i = 0; i < mnbc->num_blocks; i++) {
				if (mnbc_bbt_is_bad(mnbc, i)) {
					printf("% 6d    %08x\n", i,
						i << erasesize_shift);
				}
			}
			puts("\n");
		}

		return 0;
	}

	if (!strcmp(argv[1], "bmt")) {
		printf("   LBN ->    PBN    LA       -> PA\n");

		for (i = 0; i < mnbc->num_blocks; i++) {
			if (mnbc->bmt[i] != -1)
				printf("% 6d -> % 6d    %08x -> %08x\n",
					i, mnbc->bmt[i], i << erasesize_shift,
					mnbc->bmt[i] << erasesize_shift);
			else
				printf("% 6d -> % 6d    %08x -> N/A\n",
					i, mnbc->bmt[i], i << erasesize_shift);
		}

		return 0;
	}

	if (!strcmp(argv[1], "rescan")) {
		mnbc_rescan_bbt_bmt(mnbc);

		printf("Bad block rescanned successfully\n");

		return 0;
	}

#ifdef CONFIG_COMPAT_NAND_BBT_WB
	if (!strcmp(argv[1], "wb")) {
		if (!mnbc_bbt_wb(mnbc))
			printf("BBT has been written back\n");
		return 0;
	}
#endif

	if (!strcmp(argv[1], "read") || !strcmp(argv[1], "write")) {
		if (argc < 5)
			return CMD_RET_USAGE;

		addr = simple_strtoul(argv[2], NULL, 0);
		off = simple_strtoull(argv[3], NULL, 0);
		len = simple_strtoul(argv[4], NULL, 0);

		if (off >= mtd->size) {
			printf("Start address exceeds flash size!\n");
			return -EINVAL;
		}

		if ((off + len) >= mtd->size) {
			printf("End address exceeds flash size!\n");
			return -EINVAL;
		}

		if (!strcmp(argv[1], "read")) {
			printf("Reading from %s at 0x%llx, size 0x%x ... ",
				mnbc->nand_mtd->name, off, len);

			ret = mtd_read(mtd, off, len, &retlen, (void *)addr);

			if ((ret && ret != -EUCLEAN) || retlen != len)
				printf("failed\n");
			else
				printf("OK\n");
		} else {
			printf("Writing to %s at 0x%llx, size 0x%x ... ",
				mnbc->nand_mtd->name, off, len);

			ret = mtd_write(mtd, off, len, &retlen, (void *)addr);

			if (ret || retlen != len)
				printf("failed\n");
			else
				printf("OK\n");
		}

		return 0;
	}

	if (!strcmp(argv[1], "erase")) {
		if (argc < 4)
			return CMD_RET_USAGE;

		off = simple_strtoull(argv[2], NULL, 0);
		len = simple_strtoul(argv[3], NULL, 0);

		if (off >= mtd->size) {
			printf("Start address exceeds flash size!\n");
			return -EINVAL;
		}

		if ((off + len) >= mtd->size) {
			printf("End address exceeds flash size!\n");
			return -EINVAL;
		}

		memset(&ei, 0, sizeof(ei));

		ei.addr = off;
		ei.len = len;

		printf("Erasing %s at 0x%llx, size 0x%x ... ",
			mnbc->nand_mtd->name, off, len);

		ret = mtd_erase(mtd, &ei);

		if (ret || ei.state != MTD_ERASE_DONE)
			printf("failed\n");
		else
			printf("OK\n");

		return 0;
	}

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	mnbc, CONFIG_SYS_MAXARGS, 0, do_mnbc,
	"MT7621 NAND bad block compatible management",
	"mnbc dev [idx] - select nand mnbc device\n"
	"mnbc info      - show information of current mnbc device\n"
	"mnbc bbt       - show bad block table\n"
	"mnbc bmt       - show bad block mapping table\n"
	"mnbc rescan    - discard current bbt and rescan (not written back)\n"
#ifdef CONFIG_COMPAT_NAND_BBT_WB
	"mnbc wb        - write back bbt\n"
#endif
	"\n"
	"mnbc read addr off size       - read data\n"
	"mnbc write addr off size      - write data\n"
	"mnbc erase off size           - erase range\n"
);
#endif

#endif