/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_ELF_H_
#define __LINUX_ELF_H_

#include <asm/elf.h>
#include <uapi/linux/elf.h>

#if ELF_CLASS == ELFCLASS32

extern Elf32_Dyn _DYNAMIC [];
#define elfhdr		elf32_hdr
#define elf_phdr	elf32_phdr
#define elf_shdr	elf32_shdr
#define elf_note	elf32_note
#define elf_addr_t	Elf32_Off
#define Elf_Half	Elf32_Half
#define Elf_Word	Elf32_Word

#else

extern Elf64_Dyn _DYNAMIC [];
#define elfhdr		elf64_hdr
#define elf_phdr	elf64_phdr
#define elf_shdr	elf64_shdr
#define elf_note	elf64_note
#define elf_addr_t	Elf64_Off
#define Elf_Half	Elf64_Half
#define Elf_Word	Elf64_Word

#endif

#endif /* !__LINUX_ELF_H_ */
