
obj-y += dramc.o

ifeq ($(CONFIG_MT7621_DRAMC_DEBUG_LEGACY),y)
DRAMC_BIN_FILE := mt7621_stage_sram.bin
else
DRAMC_BIN_FILE := mt7621_stage_sram_noprint.bin
endif

AFLAGS_dramc.o += \
	-DDRAMC_BIN_FILE="\"$(obj)/$(DRAMC_BIN_FILE)\""

spl/arch/mips/mach-mt7621/dramc-legacy/dramc.o: spl/arch/mips/mach-mt7621/dramc-legacy/$(DRAMC_BIN_FILE)

spl/arch/mips/mach-mt7621/dramc-legacy/$(DRAMC_BIN_FILE): arch/mips/mach-mt7621/dramc-legacy/$(DRAMC_BIN_FILE)
	@cp $< $@

tpl/arch/mips/mach-mt7621/dramc-legacy/dramc.o: tpl/arch/mips/mach-mt7621/dramc-legacy/$(DRAMC_BIN_FILE)

tpl/arch/mips/mach-mt7621/dramc-legacy/$(DRAMC_BIN_FILE): arch/mips/mach-mt7621/dramc-legacy/$(DRAMC_BIN_FILE)
	@cp $< $@
