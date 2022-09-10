# SPDX-License-Identifier: GPL-2.0+

extra-y	= start.o

obj-y += reset.o
obj-y += cpu.o
obj-y += clocks.o
obj-y += sram_init.o
obj-y += launch.o
obj-y += lowlevel_init.o
obj-y += cps.o
obj-y += launch.o
obj-y += launch_ll.o

ifeq ($(CONFIG_MT7621_LEGACY_DRAMC_BIN), y)
obj-y += dramc-legacy/
else
obj-y += dramc/
endif

obj-$(CONFIG_SPL_BUILD) += spl/
obj-$(CONFIG_MT7621_MEMTEST) += memtest/
