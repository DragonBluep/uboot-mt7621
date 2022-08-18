/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _MT7621_NAND_H_
#define _MT7621_NAND_H_

#include <nand.h>

#define REG_SET_VAL(_name, _val) \
	(((_name ## _M) & (_val)) << (_name ## _S))

#define REG_GET_VAL(_name, _val) \
	(((_val) >> (_name ## _S)) & (_name ## _M))

#define REG_MASK(_name) \
	((_name ## _M) << (_name ## _S))

#define NFI_CNFG_REG16					0x000
#define   OP_MODE_S					  12
#define   OP_MODE_M					  0x07
#define     OP_IDLE					    0
#define     OP_READ					    1
#define     OP_SINGLE_READ				    2
#define     OP_PROGRAM					    3
#define     OP_ERASE					    4
#define     OP_RESET					    5
#define     OP_CUSTOM					    6
#define   AUTO_FMT_EN_S				  	  9
#define   AUTO_FMT_EN_M				  	  0x01
#define   AUTO_FMT_EN				  	  REG_MASK(AUTO_FMT_EN)
#define   HW_ECC_EN_S					  8
#define   HW_ECC_EN_M					  0x01
#define   HW_ECC_EN					  REG_MASK(HW_ECC_EN)
#define   BYTE_RW_S					  6
#define   BYTE_RW_M					  0x01
#define   BYTE_RW					  REG_MASK(BYTE_RW)
#define   DMA_BURST_EN_S				  2
#define   DMA_BURST_EN_M				  0x01
#define   DMA_BURST_EN					  REG_MASK(DMA_BURST_EN)
#define   READ_MODE_S					  1
#define   READ_MODE_M					  0x01
#define   READ_MODE					  REG_MASK(READ_MODE)
#define   DMA_MODE_S					  0
#define   DMA_MODE_M					  0x01
#define   DMA_MODE					  REG_MASK(DMA_MODE)


#define NFI_PAGEFMT_REG16				0x004
#define   PAGEFMT_FDM_ECC_S				  12
#define   PAGEFMT_FDM_ECC_M				  0xf
#define   PAGEFMT_FDM_S					  8
#define   PAGEFMT_FDM_M					  0xf
#define   PAGEFMT_SPARE_S				  4
#define   PAGEFMT_SPARE_M				  0x3
#define     PAGEFMT_SPARE_16				    0
#define     PAGEFMT_SPARE_26				    1
#define     PAGEFMT_SPARE_27				    2
#define     PAGEFMT_SPARE_28				    3
#define   PAGEFMT_DBYTE_S				  3
#define   PAGEFMT_DBYTE_M				  0x01
#define   PAGEFMT_DBYTE					  REG_MASK(PAGEFMT_DBYTE)
#define   PAGEFMT_PAGE_S				  0
#define   PAGEFMT_PAGE_M				  0x3
#define     PAGEFMT_512					    0
#define     PAGEFMT_2K					    1
#define     PAGEFMT_4K					    2


#define NFI_CON_REG16					0x008
#define   NFI_SEC_S					  12
#define   NFI_SEC_M					  0xf
#define   NFI_BWR_S					  9
#define   NFI_BWR_M					  0x01
#define   NFI_BWR					  REG_MASK(NFI_BWR)
#define   NFI_BRD_S					  8
#define   NFI_BRD_M					  0x01
#define   NFI_BRD					  REG_MASK(NFI_BRD)
#define   NFI_NOB_S					  5
#define   NFI_NOB_M					  0x07
#define   NFI_SRD_S					  4
#define   NFI_SRD_M					  0x01
#define   NFI_SRD					  REG_MASK(NFI_SRD)
#define   NFI_STM_RST_S					  1
#define   NFI_STM_RST_M					  0x01
#define   NFI_STM_RST					  REG_MASK(NFI_STM_RST)
#define   NFI_FIFO_FLUSH_S				  0
#define   NFI_FIFO_FLUSH_M				  0x01
#define   NFI_FIFO_FLUSH				  REG_MASK(NFI_FIFO_FLUSH)


#define NFI_ACCCON_REG32				0x00c
#define   POECS_S					  28
#define   POECS_M					  0x0f
#define   PRECS_S					  22
#define   PRECS_M					  0x3f
#define   C2R_S						  16
#define   C2R_M						  0x3f
#define   W2R_S						  12
#define   W2R_M						  0x0f
#define   WH_S						  8
#define   WH_M						  0x0f
#define   WST_S						  4
#define   WST_M						  0x0f
#define   RLT_S						  0
#define   RLT_M						  0x0f


#define NFI_INTR_EN_REG16				0x010
#define NFI_INTR_REG16					0x014
#define   AHB_DONE_S					  6
#define   AHB_DONE_M					  0x01
#define   AHB_DONE					  REG_MASK(AHB_DONE)
#define   ACCESS_LOCK_INTR_S				  5
#define   ACCESS_LOCK_INTR_M				  0x01
#define   ACCESS_LOCK					  REG_MASK(ACCESS_LOCK)
#define   BUSY_RETURN_S					  4
#define   BUSY_RETURN_M					  0x01
#define   BUSY_RETURN					  REG_MASK(BUSY_RETURN)
#define   ERASE_DONE_S					  3
#define   ERASE_DONE_M					  0x01
#define   ERASE_DONE					  REG_MASK(ERASE_DONE)
#define   RESET_DONE_S					  2
#define   RESET_DONE_M					  0x01
#define   RESET_DONE					  REG_MASK(RESET_DONE)
#define   WR_DONE_S					  1
#define   WR_DONE_M					  0x01
#define   WR_DONE					  REG_MASK(WR_DONE)
#define   RD_DONE_S					  0
#define   RD_DONE_M					  0x01
#define   RD_DONE					  REG_MASK(RD_DONE)


#define NFI_CMD_REG16					0x020
#define   CMD_S						  0
#define   CMD_M						  0xff


#define NFI_ADDRNOB_REG16				0x030
#define   ADDR_ROW_NOB_S				  4
#define   ADDR_ROW_NOB_M				  0x07
#define   ADDR_COL_NOB_S				  0
#define   ADDR_COL_NOB_M				  0x07


#define NFI_COLADDR_REG32				0x034
#define   COL_ADDR3_S					  24
#define   COL_ADDR3_M					  0xff
#define   COL_ADDR2_S					  16
#define   COL_ADDR2_M					  0xff
#define   COL_ADDR1_S					  8
#define   COL_ADDR1_M					  0xff
#define   COL_ADDR0_S					  0
#define   COL_ADDR0_M					  0xff


#define NFI_ROWADDR_REG32				0x038
#define   ROW_ADDR3_S					  24
#define   ROW_ADDR3_M					  0xff
#define   ROW_ADDR2_S					  16
#define   ROW_ADDR2_M					  0xff
#define   ROW_ADDR1_S					  8
#define   ROW_ADDR1_M					  0xff
#define   ROW_ADDR0_S					  0
#define   ROW_ADDR0_M					  0xff


#define NFI_STRDATA_REG16				0x040
#define   STR_DATA_S					  0
#define   STR_DATA_M					  0x01
#define   STR_DATA					  REG_MASK(STR_DATA)


#define NFI_CNRN_REG16					0x044
#define   CB2R_TIME_S					  4
#define   CB2R_TIME_M					  0xf
#define   STR_CNRNB_S					  0
#define   STR_CNRNB_M					  0x01
#define   STR_CNRNB					  REG_MASK(STR_CNRNB)


#define NFI_DATAW_REG32					0x050
#define   DW3_S						  24
#define   DW3_M						  0xff
#define   DW2_S						  16
#define   DW2_M						  0xff
#define   DW1_S						  8
#define   DW1_M						  0xff
#define   DW0_S						  0
#define   DW0_M						  0xff


#define NFI_DATAR_REG32					0x054
#define   DR3_S						  24
#define   DR3_M						  0xff
#define   DR2_S						  16
#define   DR2_M						  0xff
#define   DR1_S						  8
#define   DR1_M						  0xff
#define   DR0_S						  0
#define   DR0_M						  0xff


#define NFI_PIO_DIRDY_REG16				0x058
#define   PIO_DIRDY_S					  0
#define   PIO_DIRDY_M					  0x01
#define   PIO_DIRDY					  REG_MASK(PIO_DIRDY)


#define NFI_STA_REG32					0x060
#define   NAND_FSM_S					  24
#define   NAND_FSM_M					  0x1f
#define   NFI_FSM_S					  16
#define   NFI_FSM_M					  0x0f
#define     FSM_IDLE					    0
#define     FSM_RESET					    1
#define     FSM_RESET_BUSY				    2
#define     FSM_RESET_DATA				    3
#define     FSM_PROGRAM_BUSY				    4
#define     FSM_PROGRAM_DATA				    5
#define     FSM_ERASE_BUSY				    8
#define     FSM_ERASE_DATA				    9
#define     FSM_CUSTOM_DATA				    14
#define     FSM_CUSTOM_MODE				    15
#define   READ_EMPTY_S					  12
#define   READ_EMPTY_M					  0x01
#define   READ_EMPTY					  REG_MASK(READ_EMPTY)
#define   BUSY2READY_S					  9
#define   BUSY2READY_M					  0x01
#define   BUSY2READY					  REG_MASK(BUSY2READY)
#define   BUSY_S					  8
#define   BUSY_M					  0x01
#define   BUSY						  REG_MASK(BUSY)
#define   ACCESS_LOCK_S					  4
#define   ACCESS_LOCK_M					  0x01
#define   ACCESS_LOCK					  REG_MASK(ACCESS_LOCK)
#define   DATAW_S					  3
#define   DATAW_M					  0x01
#define   DATAW						  REG_MASK(DATAW)
#define   DATAR_S					  2
#define   DATAR_M					  0x01
#define   DATAR						  REG_MASK(DATAR)
#define   ADDR_S					  1
#define   ADDR_M					  0x01
#define   ADDR						  REG_MASK(ADDR)
#define   STA_CMD_S					  0
#define   STA_CMD_M					  0x01
#define   STA_CMD					  REG_MASK(STA_CMD)


#define NFI_FIFOSTA_REG16				0x064
#define   WR_FULL_S					  15
#define   WR_FULL_M					  0x01
#define   WR_FULL					  REG_MASK(WR_FULL)
#define   WR_EMPTY_S					  14
#define   WR_EMPTY_M					  0x01
#define   WR_EMPTY					  REG_MASK(WR_EMPTY)
#define   WR_REMAIN_S					  8
#define   WR_REMAIN_M					  0x1f
#define   RD_FULL_S					  7
#define   RD_FULL_M					  0x01
#define   RD_FULL					  REG_MASK(RD_FULL)
#define   RD_EMPTY_S					  6
#define   RD_EMPTY_M					  0x01
#define   RD_EMPTY					  REG_MASK(RD_EMPTY)
#define   RD_REMAIN_S					  0
#define   RD_REMAIN_M					  0x1f


#define NFI_ADDRCNTR_REG16				0x070
#define   SEC_CNTR_S					  12
#define   SEC_CNTR_M					  0xf
#define   SEC_ADDR_S					  0
#define   SEC_ADDR_M					  0x3ff


#define NFI_STRADDR_REG32				0x080


#define NFI_BYTELEN_REG16				0x084
#define   BUS_SEC_CNTR_S				  12
#define   BUS_SEC_CNTR_M				  0xf
#define   BUS_SEC_ADDR_S				  0
#define   BUS_SEC_ADDR_M				  0x3ff


#define NFI_CSEL_REG16					0x090
#define   CSEL_S					  0
#define   CSEL_M					  0x01
#define   CSEL						  REG_MASK(CSEL)


#define NFI_IOCON_REG16					0x094
#define   BRSTN_S					  4
#define   BRSTN_M					  0x0f
#define   L2NW_S					  2
#define   L2NW_M					  0x01
#define   L2NW						  REG_MASK(L2NW)
#define   L2NR_S					  1
#define   L2NR_M					  0x01
#define   L2NR						  REG_MASK(L2NR)
#define   NLD_PD_S					  0
#define   NLD_PD_M					  0x01
#define   NLD_PD					  REG_MASK(NLD_PD)


#define NFI_FDM0L_REG32					0x0a0
#define   FDM0_3_S					  24
#define   FDM0_3_M					  0xff
#define   FDM0_2_S					  16
#define   FDM0_2_M					  0xff
#define   FDM0_1_S					  8
#define   FDM0_1_M					  0xff
#define   FDM0_0_S					  0
#define   FDM0_0_M					  0xff
#define NFI_FDML_REG32(n)				(0x0a0 + ((n) << 3))


#define NFI_FDM0M_REG32					0x0a4
#define   FDM0_7_S					  24
#define   FDM0_7_M					  0xff
#define   FDM0_6_S					  16
#define   FDM0_6_M					  0xff
#define   FDM0_5_S					  8
#define   FDM0_5_M					  0xff
#define   FDM0_4_S					  0
#define   FDM0_4_M					  0xff
#define NFI_FDMM_REG32(n)				(0x0a4 + ((n) << 3))


#define NFI_LOCK_REG16					0x100
#define   LOCK_ON_S					  0
#define   LOCK_ON_M					  0x01
#define   LOCK_ON					  REG_MASK(LOCK_ON)


#define NFI_LOCKCON_REG32				0x104
#define   LOCK15_CS_S					  31
#define   LOCK15_CS_M					  0x01
#define   LOCK15_CS					  REG_MASK(LOCK15_CS)
#define   LOCK15_EN_S					  30
#define   LOCK15_EN_M					  0x01
#define   LOCK15_EN					  REG_MASK(LOCK15_EN)
#define   LOCK14_CS_S					  29
#define   LOCK14_CS_M					  0x01
#define   LOCK14_CS					  REG_MASK(LOCK14_CS)
#define   LOCK14_EN_S					  28
#define   LOCK14_EN_M					  0x01
#define   LOCK14_EN					  REG_MASK(LOCK14_EN)
#define   LOCK13_CS_S					  27
#define   LOCK13_CS_M					  0x01
#define   LOCK13_CS					  REG_MASK(LOCK13_CS)
#define   LOCK13_EN_S					  26
#define   LOCK13_EN_M					  0x01
#define   LOCK13_EN					  REG_MASK(LOCK13_EN)
#define   LOCK12_CS_S					  25
#define   LOCK12_CS_M					  0x01
#define   LOCK12_CS					  REG_MASK(LOCK12_CS)
#define   LOCK12_EN_S					  24
#define   LOCK12_EN_M					  0x01
#define   LOCK12_EN					  REG_MASK(LOCK12_EN)
#define   LOCK11_CS_S					  23
#define   LOCK11_CS_M					  0x01
#define   LOCK11_CS					  REG_MASK(LOCK11_CS)
#define   LOCK11_EN_S					  22
#define   LOCK11_EN_M					  0x01
#define   LOCK11_EN					  REG_MASK(LOCK11_EN)
#define   LOCK10_CS_S					  21
#define   LOCK10_CS_M					  0x01
#define   LOCK10_CS					  REG_MASK(LOCK10_CS)
#define   LOCK10_EN_S					  20
#define   LOCK10_EN_M					  0x01
#define   LOCK10_EN					  REG_MASK(LOCK10_EN)
#define   LOCK9_CS_S					  19
#define   LOCK9_CS_M					  0x01
#define   LOCK9_CS					  REG_MASK(LOCK9_CS)
#define   LOCK9_EN_S					  18
#define   LOCK9_EN_M					  0x01
#define   LOCK9_EN					  REG_MASK(LOCK9_EN)
#define   LOCK8_CS_S					  17
#define   LOCK8_CS_M					  0x01
#define   LOCK8_CS					  REG_MASK(LOCK8_CS)
#define   LOCK8_EN_S					  16
#define   LOCK8_EN_M					  0x01
#define   LOCK8_EN					  REG_MASK(LOCK8_EN)
#define   LOCK7_CS_S					  15
#define   LOCK7_CS_M					  0x01
#define   LOCK7_CS					  REG_MASK(LOCK7_CS)
#define   LOCK7_EN_S					  14
#define   LOCK7_EN_M					  0x01
#define   LOCK7_EN					  REG_MASK(LOCK7_EN)
#define   LOCK6_CS_S					  13
#define   LOCK6_CS_M					  0x01
#define   LOCK6_CS					  REG_MASK(LOCK6_CS)
#define   LOCK6_EN_S					  12
#define   LOCK6_EN_M					  0x01
#define   LOCK6_EN					  REG_MASK(LOCK6_EN)
#define   LOCK5_CS_S					  11
#define   LOCK5_CS_M					  0x01
#define   LOCK5_CS					  REG_MASK(LOCK5_CS)
#define   LOCK5_EN_S					  10
#define   LOCK5_EN_M					  0x01
#define   LOCK5_EN					  REG_MASK(LOCK5_EN)
#define   LOCK4_CS_S					  9
#define   LOCK4_CS_M					  0x01
#define   LOCK4_CS					  REG_MASK(LOCK4_CS)
#define   LOCK4_EN_S					  8
#define   LOCK4_EN_M					  0x01
#define   LOCK4_EN					  REG_MASK(LOCK4_EN)
#define   LOCK3_CS_S					  7
#define   LOCK3_CS_M					  0x01
#define   LOCK3_CS					  REG_MASK(LOCK3_CS)
#define   LOCK3_EN_S					  6
#define   LOCK3_EN_M					  0x01
#define   LOCK3_EN					  REG_MASK(LOCK3_EN)
#define   LOCK2_CS_S					  5
#define   LOCK2_CS_M					  0x01
#define   LOCK2_CS					  REG_MASK(LOCK2_CS)
#define   LOCK2_EN_S					  4
#define   LOCK2_EN_M					  0x01
#define   LOCK2_EN					  REG_MASK(LOCK2_EN)
#define   LOCK1_CS_S					  3
#define   LOCK1_CS_M					  0x01
#define   LOCK1_CS					  REG_MASK(LOCK1_CS)
#define   LOCK1_EN_S					  2
#define   LOCK1_EN_M					  0x01
#define   LOCK1_EN					  REG_MASK(LOCK1_EN)
#define   LOCK0_CS_S					  1
#define   LOCK0_CS_M					  0x01
#define   LOCK0_CS					  REG_MASK(LOCK0_CS)
#define   LOCK0_EN_S					  0
#define   LOCK0_EN_M					  0x01
#define   LOCK0_EN					  REG_MASK(LOCK0_EN)


#define NFI_LOCKANOB_REG16				0x108
#define   PROG_RADD_NOB_M				  0x7
#define   PROG_RADD_NOB_S				  12
#define   PROG_CADD_NOB_M				  0x3
#define   PROG_CADD_NOB_S				  8
#define   ERASE_RADD_NOB_M				  0x7
#define   ERASE_RADD_NOB_S				  4
#define   ERASE_CADD_NOB_M				  0x7
#define   ERASE_CADD_NOB_S				  0


#define NFI_LOCK00ADD_REG16				0x110
#define   LOCK0_ROW3_S					  24
#define   LOCK0_ROW3_M					  0xff
#define   LOCK0_ROW2_S					  16
#define   LOCK0_ROW2_M					  0xff
#define   LOCK0_ROW1_S					  8
#define   LOCK0_ROW1_M					  0xff
#define   LOCK0_ROW0_S					  0
#define   LOCK0_ROW0_M					  0xff


#define NFI_LOCK00FMT_REG16				0x114
#define   LOCK0_FMT3_S					  24
#define   LOCK0_FMT3_M					  0xff
#define   LOCK0_FMT2_S					  16
#define   LOCK0_FMT2_M					  0xff
#define   LOCK0_FMT1_S					  8
#define   LOCK0_FMT1_M					  0xff
#define   LOCK0_FMT0_S					  0
#define   LOCK0_FMT0_M					  0xff


#define NFI_FIFODATA0_REG32				0x190
#define NFI_FIFODATA1_REG32				0x194
#define NFI_FIFODATA2_REG32				0x198
#define NFI_FIFODATA3_REG32				0x19c


#define NFI_MCON_REG16					0x200
#define   BMCLR_S					  1
#define   BMCLR_M					  0x01
#define   BMCLR						  REG_MASK(BMCLR)
#define   BMSTR_S					  0
#define   BMSTR_M					  0x01
#define   BMSTR						  REG_MASK(BMSTR)


#define NFI_TOTALCNT_REG32				0x204
#define NFI_RQCNT_REG32					0x208
#define NFI_ACCNT_REG32					0x20c


#define NFI_MASTERSTA_REG16				0x210
#define   MAS_ADDR_S					  9
#define   MAS_ADDR_M					  0x07
#define   MAS_RD_S					  6
#define   MAS_RD_M					  0x07
#define   MAS_WR_S					  3
#define   MAS_WR_M					  0x07
#define   MAS_RDDLY_S					  0
#define   MAS_RDDLY_M					  0x07


/* ECC registers */
#define ECC_ENCCON_REG16			0x000
#define   ENC_EN_S				  0
#define   ENC_EN_M				  0x01
#define   ENC_EN				  REG_MASK(ENC_EN)


#define ECC_ENCCNFG_REG32			0x004
#define   ENC_CNFG_MSG_S			  16
#define   ENC_CNFG_MSG_M			  0x1fff
#define   ENC_BURST_EN_S			  8
#define   ENC_BURST_EN_M			  0x01
#define   ENC_BURST_EN				  REG_MASK(ENC_BURST_EN)
#define   ENC_MODE_S				  4
#define   ENC_MODE_M				  0x03
#define     ENC_MODE_DMA			    0
#define     ENC_MODE_NFI			    1
#define     ENC_MODE_PIO			    2
#define   ENC_TNUM_S				  0
#define   ENC_TNUM_M				  0x07


#define ECC_ENCDIADDR_REG32			0x008


#define ECC_ENCIDLE_REG16			0x00c
#define   ENC_IDLE_S				  0
#define   ENC_IDLE_M				  0x01
#define   ENC_IDLE				  REG_MASK(ENC_IDLE)


#define ECC_ENCPAR0_REG32			0x010
#define ECC_ENCPAR1_REG32			0x014
#define ECC_ENCPAR2_REG32			0x018
#define ECC_ENCPAR3_REG32			0x01c
#define ECC_ENCPAR4_REG32			0x020


#define ECC_ENCSTA_REG32			0x024
#define   COUNT_MS_S				  16
#define   COUNT_MS_M				  0x3fff
#define   COUNT_PS_S				  7
#define   COUNT_PS_M				  0x1ff
#define   ENC_FSM_S				  0
#define   ENC_FSM_M				  0x3f


#define ECC_ENCIRQEN_REG16			0x028
#define ECC_ENCIRQSTA_REG16			0x02c
#define   ENC_IRQ_S				  0
#define   ENC_IRQ_M				  0x01
#define   ENC_IRQ				  REG_MASK(ENC_IRQ)


#define ECC_PIO_DIRDY_REG16			0x080
#define   PIO_DI_RDY_S				  0
#define   PIO_DI_RDY_M				  0x01
#define   PIO_DI_RDY				  REG_MASK(PIO_DI_RDY)


#define ECC_PIO_DI_REG32			0x084


#define ECC_DECCON_REG16			0x100
#define   DEC_EN_S				  0
#define   DEC_EN_M				  0x01
#define   DEC_EN				  REG_MASK(DEC_EN)


#define ECC_DECCNFG_REG32			0x104
#define   DEC_EMPTY_EN_S			  31
#define   DEC_EMPTY_EN_M			  0x01
#define   DEC_EMPTY_EN				  REG_MASK(DEC_EMPTY_EN)
#define   DEC_CS_S				  16
#define   DEC_CS_M				  0x1fff
#define   DEC_CON_S				  12
#define   DEC_CON_M				  0x03
#define     DEC_CON_FER				    1
#define     DEC_CON_EL				    2
#define     DEC_CON_CORRECT			    3
#define   DEC_BURST_EN_S			  8
#define   DEC_BURST_EN_M			  0x01
#define   DEC_BURST_EN				  REG_MASK(DEC_BURST_EN)
#define   DEC_MODE_S				  4
#define   DEC_MODE_M				  0x03
#define     DEC_MODE_DMA			    0
#define     DEC_MODE_NFI			    1
#define     DEC_MODE_PIO			    2
#define   DEC_TNUM_S				  0
#define   DEC_TNUM_M				  0x07


#define ECC_DECDIADDR_REG32			0x108


#define ECC_DECIDLE_REG16			0x10c
#define   DEC_IDLE_S				  0
#define   DEC_IDLE_M				  0x01
#define   DEC_IDLE				  REG_MASK(DEC_IDLE)


#define ECC_DECFER_REG16			0x110
#define   FER7_S				  7
#define   FER7_M				  0x01
#define   FER7					  REG_MASK(FER7)
#define   FER6_S				  6
#define   FER6_M				  0x01
#define   FER6					  REG_MASK(FER6)
#define   FER5_S				  5
#define   FER5_M				  0x01
#define   FER5					  REG_MASK(FER5)
#define   FER4_S				  4
#define   FER4_M				  0x01
#define   FER4					  REG_MASK(FER4)
#define   FER3_S				  3
#define   FER3_M				  0x01
#define   FER3					  REG_MASK(FER3)
#define   FER2_S				  2
#define   FER2_M				  0x01
#define   FER2					  REG_MASK(FER2)
#define   FER1_S				  1
#define   FER1_M				  0x01
#define   FER1					  REG_MASK(FER1)
#define   FER0_S				  0
#define   FER0_M				  0x01
#define   FER0					  REG_MASK(FER0)


#define ECC_DECENUM_REG32			0x114
#define   ERRNUM7_S				  28
#define   ERRNUM7_M				  0x0f
#define   ERRNUM6_S				  24
#define   ERRNUM6_M				  0x0f
#define   ERRNUM5_S				  20
#define   ERRNUM5_M				  0x0f
#define   ERRNUM4_S				  16
#define   ERRNUM4_M				  0x0f
#define   ERRNUM3_S				  12
#define   ERRNUM3_M				  0x0f
#define   ERRNUM2_S				  8
#define   ERRNUM2_M				  0x0f
#define   ERRNUM1_S				  4
#define   ERRNUM1_M				  0x0f
#define   ERRNUM0_S				  0
#define   ERRNUM0_M				  0x0f

#define   ERRNUM_S				  2
#define   ERRNUM_M				  0xf


#define ECC_DECDONE_REG16			0x118
#define   DONE7_S				  7
#define   DONE7_M				  0x01
#define   DONE7					  REG_MASK(DONE7)
#define   DONE6_S				  6
#define   DONE6_M				  0x01
#define   DONE6					  REG_MASK(DONE6)
#define   DONE5_S				  5
#define   DONE5_M				  0x01
#define   DONE5					  REG_MASK(DONE5)
#define   DONE4_S				  4
#define   DONE4_M				  0x01
#define   DONE4					  REG_MASK(DONE4)
#define   DONE3_S				  3
#define   DONE3_M				  0x01
#define   DONE3					  REG_MASK(DONE3)
#define   DONE2_S				  2
#define   DONE2_M				  0x01
#define   DONE2					  REG_MASK(DONE2)
#define   DONE1_S				  1
#define   DONE1_M				  0x01
#define   DONE1					  REG_MASK(DONE1)
#define   DONE0_S				  0
#define   DONE0_M				  0x01
#define   DONE0					  REG_MASK(DONE0)


#define ECC_DECEL0_REG32			0x11c
#define   DEC_EL1_S				  16
#define   DEC_EL1_M				  0x1fff
#define   DEC_EL0_S				  0
#define   DEC_EL0_M				  0x1fff


#define ECC_DECEL1_REG32			0x120
#define   DEC_EL3_S				  16
#define   DEC_EL3_M				  0x1fff
#define   DEC_EL2_S				  0
#define   DEC_EL2_M				  0x1fff


#define ECC_DECEL2_REG32			0x124
#define   DEC_EL5_S				  16
#define   DEC_EL5_M				  0x1fff
#define   DEC_EL4_S				  0
#define   DEC_EL4_M				  0x1fff


#define ECC_DECEL3_REG32			0x128
#define   DEC_EL7_S				  16
#define   DEC_EL7_M				  0x1fff
#define   DEC_EL6_S				  0
#define   DEC_EL6_M				  0x1fff


#define ECC_DECEL4_REG32			0x12c
#define   DEC_EL9_S				  16
#define   DEC_EL9_M				  0x1fff
#define   DEC_EL8_S				  0
#define   DEC_EL8_M				  0x1fff


#define ECC_DECEL5_REG32			0x130
#define   DEC_EL11_S				  16
#define   DEC_EL11_M				  0x1fff
#define   DEC_EL10_S				  0
#define   DEC_EL10_M				  0x1fff

#define ECC_DECEL_REG32(n)			(0x11c + (n) * 4)
#define   DEC_EL_EVEN_S				  0
#define   DEC_EL_ODD_S				  16
#define   DEC_EL_M				  0x1fff
#define   DEC_EL_BIT_POS_M			  0x7
#define   DEC_EL_BYTE_POS_S			  3


#define ECC_DECIRQEN_REG16			0x134
#define ECC_DECIRQSTA_REG16			0x138
#define   DEC_IRQ_S				  0
#define   DEC_IRQ_M				  0x01
#define   DEC_IRQ				  REG_MASK(DEC_IRQ)


#define ECC_FDMADDR_REG32			0x13c


#define ECC_DECFSM_REG32			0x140
#define   AUTOC_FSM_S				  24
#define   AUTOC_FSM_M				  0x1f
#define   CHIEN_FSM_S				  16
#define   CHIEN_FSM_M				  0x1f
#define   BMA_FSM_S				  8
#define   BMA_FSM_M				  0x1f
#define   SYN_FSM_S				  5
#define   SYN_FSM_M				  0x3f


#define ECC_SYNSTA_REG32			0x144
#define   SYN_SNUM_S				  29
#define   SYN_SNUM_M				  0x07
#define   DIBW_S				  20
#define   DIBW_M				  0x3f
#define   NFI_SEC_NUMNFI_SEC_NUM_S		  16
#define   NFI_SEC_NUMNFI_SEC_NUM_M		  0x07
#define   NFI_STR_SET_S				  15
#define   NFI_STR_SET_M				  0x01
#define   NFI_STR_SET				  REG_MASK(NFI_STR_SET)
#define   SYN_COUNT_CS_S			  0
#define   SYN_COUNT_CS_M			  0x3fff


#define ECC_NFIDIDECNFIDI_REG32			0x148


#define ECC_SYN0_REG32				0x14c
#define   DEC_SYN3_S				  16
#define   DEC_SYN3_M				  0x1fff
#define   DEC_SYN1_S				  0
#define   DEC_SYN1_M				  0x1fff


/* ENC_TNUM and DEC_TNUM */
#define     ECC_CAP_4B				    0
#define     ECC_CAP_6B				    1
#define     ECC_CAP_8B				    2
#define     ECC_CAP_10B				    3
#define     ECC_CAP_12B				    4

/* ENCIDLE and DECIDLE */
#define   ECC_IDLE_S				  0
#define   ECC_IDLE_M				  0x01
#define   ECC_IDLE				  REG_MASK(ECC_IDLE)

typedef struct mt7621_nfc_sel {
	struct nand_chip nand;
	int cs;

	u32 pagefmt_val;
	u32 acccon_val;

	u32 rowaddr_nob;
	u32 spare_per_sector;
	u32 oobsize_avail;

	struct nand_ecclayout ecc_layout;

	void *page_cache;
	void *oob_mb_cache;
} mt7621_nfc_sel_t;

typedef struct mt7621_nfc {
	void __iomem *nfi_base;
	void __iomem *ecc_base;

	mt7621_nfc_sel_t sels[CONFIG_SYS_NAND_MAX_CHIPS];

} mt7621_nfc_t;

typedef struct mt7621_nfc_timing {
	u8 id[8];
	u32 id_len;
	u32 acccon_val;
} mt7621_nfc_timing_t;


#define NFI_DEFAULT_ACCESS_TIMING			0x30c77fff
#define NFI_ECC_SECTOR_SIZE				512
#define NFI_ECC_FDM_SIZE				8
#define NFI_ECC_PARITY_BITS				13

#define NFI_STATUS_WAIT_TIMEOUT_US			1000000

/* for SPL */
void mt7621_nfc_spl_init(mt7621_nfc_t *nfc, int cs);
int mt7621_nfc_spl_post_init(mt7621_nfc_t *nfc, int cs);

#endif /* _MT7621_NAND_H_ */
