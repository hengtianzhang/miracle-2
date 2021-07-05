/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Based on arch/arm/include/asm/memory.h
 *
 * Copyright (C) 2000-2002 Russell King
 * Copyright (C) 2012 ARM Ltd.
 *
 * Note: this file should not be included by non-asm/.h files
 */
#ifndef __ASM_MEMORY_H_
#define __ASM_MEMORY_H_

#include <linux/const.h>
#include <linux/sizes.h>

#include <asm/page-def.h>

/*
 * PAGE_OFFSET - the virtual address of the start of the linear map, at the
 *               start of the TTBR1 address space.
 * PAGE_END - the end of the linear map, where all other kernel mappings begin.
 * KIMAGE_VADDR - the virtual address of the start of the kernel image.
 * VA_BITS - the maximum number of bits for virtual addresses.
 */
#define VA_BITS			(CONFIG_ARM64_VA_BITS)

#if VA_BITS > 48
#define VA_BITS_MIN		(48)
#else
#define VA_BITS_MIN		(VA_BITS)
#endif

#define _PAGE_START(va)			(-(ULL(1) << (va)))
#define _PAGE_OFFSET(va)		(-(ULL(1) << ((va) - 1)))

#define VA_START	_PAGE_START(VA_BITS_MIN)
#define PAGE_OFFSET		(_PAGE_OFFSET(VA_BITS_MIN))

#define KIMAGE_VADDR		VA_START

#define ELFLOADER_TOOL_SIZE     (ULL(1) << (VA_BITS - PAGE_SHIFT - 1))
#define ELFLOADER_TOOL_START    (PAGE_OFFSET - ELFLOADER_TOOL_SIZE)
#define FIXMAP_TOP              (ELFLOADER_TOOL_START - SZ_2M)

#define KERNEL_START      _text
#define KERNEL_END        _end

#define MAX_USER_VA_BITS	VA_BITS

#define MIN_THREAD_SHIFT	(14)

#define THREAD_SHIFT		MIN_THREAD_SHIFT

#if THREAD_SHIFT >= PAGE_SHIFT
#define THREAD_SIZE_ORDER	(THREAD_SHIFT - PAGE_SHIFT)
#endif

#define THREAD_SIZE		(ULL(1) << THREAD_SHIFT)

#define THREAD_ALIGN		THREAD_SIZE

#define IRQ_STACK_SIZE		THREAD_SIZE

#define OVERFLOW_STACK_SIZE	SZ_4K

/*
 * Alignment of kernel segments (e.g. .text, .data).
 *
 *  4 KB granule:  16 level 3 entries, with contiguous bit
 * 16 KB granule:   4 level 3 entries, without contiguous bit
 * 64 KB granule:   1 level 3 entry
 */
#define SEGMENT_ALIGN		SZ_64K

/*
 * Memory types available.
 */
#define MT_DEVICE_nGnRnE	0
#define MT_DEVICE_nGnRE		1
#define MT_DEVICE_GRE		2
#define MT_NORMAL_NC		3
#define MT_NORMAL		4
#define MT_NORMAL_WT		5

extern s64			memstart_addr;
/* PHYS_OFFSET - the physical address of the start of memory. */
#define PHYS_OFFSET		({ BUG_ON(memstart_addr & 1); memstart_addr; })

/* the offset between the kernel virtual and physical mappings */
extern u64			kimage_voffset;

/*
 * The linear kernel range starts in the middle of the virtual adddress
 * space. Testing the top bit for the start of the region is a
 * sufficient check.
 */
#define __is_lm_address(addr)	(!!((addr) & BIT(VA_BITS - 1)))

#define __lm_to_phys(addr)	(((addr) & ~PAGE_OFFSET) + PHYS_OFFSET)
#define __kimg_to_phys(addr)	((addr) - kimage_voffset)

#define __virt_to_phys(x) ({					\
	phys_addr_t __x = (phys_addr_t)(x);				\
	__is_lm_address(__x) ? __lm_to_phys(__x) :			\
			       __kimg_to_phys(__x);			\
})

/*
 * Note: Drivers should NOT use these.  They are the wrong
 * translation for translating DMA addresses.  Use the driver
 * DMA support - see dma-mapping.h.
 */
#define virt_to_phys virt_to_phys
static inline phys_addr_t virt_to_phys(const volatile void *x)
{
	return __virt_to_phys((u64)(x));
}

#define __phys_to_virt(x)	((u64)((x) - PHYS_OFFSET) | PAGE_OFFSET)

#define phys_to_virt phys_to_virt
static inline void *phys_to_virt(phys_addr_t x)
{
	return (void *)(__phys_to_virt(x));
}

#define __phys_to_kimg(x)	((u64)((x) + kimage_voffset))

#define phys_to_kimg phys_to_kimg
static inline void *phys_to_kimg(phys_addr_t x)
{
    return (void *)(__phys_to_kimg(x));
}

#endif /* !__ASM_MEMORY_H_ */
