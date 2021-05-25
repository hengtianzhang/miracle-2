/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ld script to make ARM Linux kernel
 * taken from the i386 version by Russell King
 * Written by Martin Mares <mj@atrey.karlin.mff.cuni.cz>
 */
#include <asm-generic/vmlinux.lds.h>

#include <asm/page.h>

#define ARM_EXIT_KEEP(x)	x
#define ARM_EXIT_DISCARD(x)

OUTPUT_ARCH(aarch64)
ENTRY(_text)

SECTIONS
{
	/*
	 * XXX: The linker does not define how output sections are
	 * assigned to input sections when there are multiple statements
	 * matching the same input section name.  There is no documented
	 * order of matching.
	 */
	/DISCARD/ : {
		ARM_EXIT_DISCARD(EXIT_TEXT)
		ARM_EXIT_DISCARD(EXIT_DATA)
		*(.discard)
		*(.discard.*)
		*(.interp .dynamic)
		*(.dynsym .dynstr .hash .gnu.hash)
		*(.eh_frame)
	}

	. = 0;

	.head.text : {
		_text = .;
		HEAD_TEXT
	}
	.text : {			/* Real text segment		*/
		_stext = .;		/* Text and read-only data	*/
			__exception_text_start = .;
			*(.exception.text)
			__exception_text_end = .;
			IRQENTRY_TEXT
			ENTRY_TEXT
			TEXT_TEXT
			SCHED_TEXT
			CPUIDLE_TEXT
			LOCK_TEXT
			*(.fixup)
			*(.gnu.warning)
		. = ALIGN(16);
		*(.got)			/* Global offset table		*/
	}

	. = ALIGN(4096);
	_etext = .;			/* End of text section */

	RO_DATA(PAGE_SIZE)		/* everything from this point to     */
	EXCEPTION_TABLE(8)		/* __init_begin will be marked RO NX */
	NOTES

	swapper_pg_dir = .;
	. += PAGE_SIZE;
	swapper_pg_end = .;

	. = ALIGN(4096);
	__init_begin = .;
	__inittext_begin = .;

	INIT_TEXT_SECTION(8)
	.exit.text : {
		ARM_EXIT_KEEP(EXIT_TEXT)
	}

	. = ALIGN(4);
	.altinstructions : {
		__alt_instructions = .;
		*(.altinstructions)
		__alt_instructions_end = .;
	}
	.altinstr_replacement : {
		*(.altinstr_replacement)
	}

	. = ALIGN(PAGE_SIZE);
	__inittext_end = .;
	__initdata_begin = .;

	.init.data : {
		INIT_DATA
		INIT_SETUP(16)
		INIT_CALLS
		CON_INITCALL
		INIT_RAM_FS
		*(.init.rodata.* .init.bss)	/* from the EFI stub */
	}

	PERCPU_SECTION(1 << 8)

	.rela.dyn : ALIGN(8) {
		*(.rela .rela*)
	}

	. = ALIGN(4096);
	__initdata_end = .;
	__init_end = .;

	_data = .;
	_sdata = .;
	RW_DATA_SECTION(1 << 8, PAGE_SIZE, 4096)

	BSS_SECTION(0, 0, 0)

	STABS_DEBUG
}
