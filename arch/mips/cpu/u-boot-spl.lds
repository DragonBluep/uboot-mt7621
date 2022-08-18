/* SPDX-License-Identifier: GPL-2.0+ */

MEMORY { .spl_mem : ORIGIN = CONFIG_VAL(TEXT_BASE), \
		LENGTH = CONFIG_VAL(MAX_SIZE) }
MEMORY { .bss_mem : ORIGIN = CONFIG_VAL(BSS_START_ADDR), \
		LENGTH = CONFIG_VAL(BSS_MAX_SIZE) }

OUTPUT_ARCH(mips)
ENTRY(_start)
SECTIONS
{
	. = 0x00000000;

	. = ALIGN(4);
	.text : {
		__text_start = .;
		*(.text*)
		__text_end = .;
	} > .spl_mem

	. = ALIGN(4);
	.rodata : {
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*)))
	} > .spl_mem

	. = ALIGN(4);
	.data : {
		*(SORT_BY_ALIGNMENT(.data*))
		*(SORT_BY_ALIGNMENT(.sdata*))
	} > .spl_mem

	. = ALIGN(4);
	.u_boot_list : {
		KEEP(*(SORT(.u_boot_list*)));
	} > .spl_mem

	. = ALIGN(4);
	.binman_sym_table : {
		__binman_sym_start = .;
		KEEP(*(SORT(.binman_sym*)));
		__binman_sym_end = .;
	} > .spl_mem

	. = ALIGN(4);
	__image_copy_end = .;

	__ld_rom_size = __image_copy_end - __text_start;

	.bss (NOLOAD) : {
		__bss_start = .;
		*(.bss*)
		*(.sbss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end = .;
	} > .bss_mem

	/DISCARD/ : {
		*(.rel.dyn)
		*(.MIPS.abiflags)
	}

	.dynsym : {
		*(.dynsym)
	}

	.dynbss : {
		*(.dynbss)
	}

	.dynstr : {
		*(.dynstr)
	}

	.dynamic : {
		*(.dynamic)
	}

	.plt : {
		*(.plt)
	}

	.interp : {
		*(.interp)
	}

	.gnu : {
		*(.gnu*)
	}

	.MIPS.stubs : {
		*(.MIPS.stubs)
	}

	.hash : {
		*(.hash)
	}
}
