# SPDX-License-Identifier:	GPL-2.0+

obj-$(CONFIG_SPI_BOOT) 		+= spl_helper_nor.o
obj-$(CONFIG_NAND_BOOT) 	+= spl_helper_nand.o

ifdef CONFIG_SPL_BUILD
ifndef CONFIG_TPL_BUILD
obj-y += spl.o
endif
endif

ifndef CONFIG_SPL_BUILD
obj-y += cmd_mtkupgrade.o
obj-y += cmd_mtkautoboot.o
obj-$(CONFIG_MTK_DUAL_IMAGE_SUPPORT) 	+= dual_image.o
endif
