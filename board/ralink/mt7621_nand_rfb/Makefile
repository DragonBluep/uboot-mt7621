# SPDX-License-Identifier:	GPL-2.0+

obj-y	+= mt7621_nand_rfb.o

ifdef CONFIG_SPL_BUILD
obj-$(CONFIG_ENABLE_NAND_NMBM) += spl_mtd.o
endif

ifndef CONFIG_SPL_BUILD
obj-y	+= cmd_mtkboardboot.o
endif
