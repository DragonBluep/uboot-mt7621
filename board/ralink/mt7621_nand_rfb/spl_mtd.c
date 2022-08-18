
#include <nand.h>

int add_mtd_device(struct mtd_info *mtd)
{
	return 0;
}

void mtd_erase_callback(struct erase_info *instr)
{
	if (instr->callback)
		instr->callback(instr);
}

int mtd_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen,
	u_char *buf)
{
	int ret_code;
	*retlen = 0;
	if (from < 0 || from > mtd->size || len > mtd->size - from)
		return -EINVAL;
	if (!len)
		return 0;

	/*
	* In the absence of an error, drivers return a non-negative integer
	* representing the maximum number of bitflips that were corrected on
	* any one ecc region (if applicable; zero otherwise).
	*/
	ret_code = mtd->_read(mtd, from, len, retlen, buf);
	if (unlikely(ret_code < 0))
		return ret_code;
	if (mtd->ecc_strength == 0)
		return 0;	/* device lacks ecc */
	return ret_code >= mtd->bitflip_threshold ? -EUCLEAN : 0;
}

int mtd_read_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops)
{
	int ret_code;
	ops->retlen = ops->oobretlen = 0;
	if (!mtd->_read_oob)
		return -EOPNOTSUPP;
	/*
	* In cases where ops->datbuf != NULL, mtd->_read_oob() has semantics
	* similar to mtd->_read(), returning a non-negative integer
	* representing max bitflips. In other cases, mtd->_read_oob() may
	* return -EUCLEAN. In all cases, perform similar logic to mtd_read().
	*/
	ret_code = mtd->_read_oob(mtd, from, ops);
	if (unlikely(ret_code < 0))
		return ret_code;
	if (mtd->ecc_strength == 0)
		return 0;	/* device lacks ecc */
	return ret_code >= mtd->bitflip_threshold ? -EUCLEAN : 0;
}

int mtd_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
	if (ofs < 0 || ofs > mtd->size)
		return -EINVAL;
	if (!mtd->_block_isbad)
		return 0;
	return mtd->_block_isbad(mtd, ofs);
}

int mtd_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	if (!mtd->_block_markbad)
		return -EOPNOTSUPP;
	if (ofs < 0 || ofs > mtd->size)
		return -EINVAL;
	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	return mtd->_block_markbad(mtd, ofs);
}

int mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	if (instr->addr > mtd->size || instr->len > mtd->size - instr->addr)
		return -EINVAL;
	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	instr->fail_addr = MTD_FAIL_ADDR_UNKNOWN;
	if (!instr->len) {
		instr->state = MTD_ERASE_DONE;
		mtd_erase_callback(instr);
		return 0;
	}
	return mtd->_erase(mtd, instr);
}
