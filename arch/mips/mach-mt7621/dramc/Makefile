
ifeq ($(CONFIG_MT7621_DRAMC_DEBUG),y)
DRAMC_OBJ = mt7621_dramc_debug.obj
else
DRAMC_OBJ = mt7621_dramc.obj
endif

obj-y += $(DRAMC_OBJ)

spl/arch/mips/mach-mt7621/dramc/$(DRAMC_OBJ): arch/mips/mach-mt7621/dramc/$(DRAMC_OBJ)
	@cp $< $@

tpl/arch/mips/mach-mt7621/dramc/$(DRAMC_OBJ): arch/mips/mach-mt7621/dramc/$(DRAMC_OBJ)
	@cp $< $@

