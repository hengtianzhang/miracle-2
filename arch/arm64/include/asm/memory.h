/*
 * Based on arch/arm/include/asm/memory.h
 *
 * Copyright (C) 2000-2002 Russell King
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Note: this file should not be included by non-asm/.h files
 */
#ifndef __ASM_MEMORY_H_
#define __ASM_MEMORY_H_

#include <linux/compiler.h>
#include <linux/sizes.h>

#include <asm/page-def.h>

/*
 * VMEMMAP_SIZE - allows the whole linear region to be covered by
 *                a struct page array
 */
#define VMEMMAP_SIZE (ULL(1) << (VA_BITS - PAGE_SHIFT - 1 + STRUCT_PAGE_MAX_SHIFT))

/*
 * PAGE_OFFSET - the virtual address of the start of the linear map (top
 *		 (VA_BITS - 1))
 * KIMAGE_VADDR - the virtual address of the start of the kernel image
 * VA_BITS - the maximum number of bits for virtual addresses.
 * VA_START - the first kernel virtual address.
 */
#define VA_BITS			(CONFIG_ARM64_VA_BITS)
#define VA_START		(ULL(0xffffffffffffffff) - \
	(ULL(1) << VA_BITS) + 1)
#define PAGE_OFFSET		(ULL(0xffffffffffffffff) - \
	(ULL(1) << (VA_BITS - 1)) + 1)
#define KIMAGE_VADDR		(VA_START)
#define VMEMMAP_START		(PAGE_OFFSET - VMEMMAP_SIZE)
#define FIXADDR_TOP		(VMEMMAP_START - SZ_2M)

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
 *  4 KB granule:  16 level 3 entries, with contiguous bit
 * 16 KB granule:   4 level 3 entries, without contiguous bit
 * 64 KB granule:   1 level 3 entry
 */
#define SEGMENT_ALIGN			SZ_64K

/*
 * Memory types available.
 */
#define MT_DEVICE_nGnRnE	0
#define MT_DEVICE_nGnRE		1
#define MT_DEVICE_GRE		2
#define MT_NORMAL_NC		3
#define MT_NORMAL		4
#define MT_NORMAL_WT		5

/*
 * Memory types for Stage-2 translation
 */
#define MT_S2_NORMAL		0xf
#define MT_S2_DEVICE_nGnRE	0x1

/*
 * Memory types for Stage-2 translation when ID_AA64MMFR2_EL1.FWB is 0001
 * Stage-2 enforces Normal-WB and Device-nGnRE
 */
#define MT_S2_FWB_NORMAL	6
#define MT_S2_FWB_DEVICE_nGnRE	1

#ifdef CONFIG_ARM64_4K_PAGES
#define IOREMAP_MAX_ORDER	(PUD_SHIFT)
#else
#define IOREMAP_MAX_ORDER	(PMD_SHIFT)
#endif

#ifndef __ASSEMBLY__

#include <linux/bitops.h>

extern s64			memstart_addr;
/* PHYS_OFFSET - the physical address of the start of memory. */
#define PHYS_OFFSET		({ BUG_ON(memstart_addr & 1); memstart_addr; })

/* the virtual base of the kernel image (minus TEXT_OFFSET) */
extern u64			kimage_vaddr;

/* the offset between the kernel virtual and physical mappings */
extern u64			kimage_voffset;

/* the actual size of a user virtual address */
extern u64			vabits_user;

/*
 * Allow all memory at the discovery stage. We will clip it later.
 */
#define MIN_MEMBLOCK_ADDR	0
#define MAX_MEMBLOCK_ADDR	U64_MAX

/*
 * PFNs are used to describe any physical page; this means
 * PFN 0 == physical address 0.
 *
 * This is the PFN of the first RAM page in the kernel
 * direct-mapped view.  We assume this is the first page
 * of RAM in the mem_map as well.
 */
#define PHYS_PFN_OFFSET	(PHYS_OFFSET >> PAGE_SHIFT)

/*
 * When dealing with data aborts, watchpoints, or instruction traps we may end
 * up with a tagged userland pointer. Clear the tag to get a sane pointer to
 * pass on to access_ok(), for instance.
 */
#define untagged_addr(addr)	\
	((__typeof__(addr))sign_extend64((u64)(addr), 55))

#define __tag_set(addr, tag)	(addr)
#define __tag_reset(addr)	(addr)
#define __tag_get(addr)		0

/*
 * Physical vs virtual RAM address space conversion.  These are
 * private definitions which should NOT be used outside memory.h
 * files.  Use virt_to_phys/phys_to_virt/__pa/__va instead.
 */

/*
 * The linear kernel range starts in the middle of the virtual adddress
 * space. Testing the top bit for the start of the region is a
 * sufficient check.
 */
#define __is_lm_address(addr)	(!!((addr) & BIT(VA_BITS - 1)))

#define __lm_to_phys(addr)	(((addr) & ~PAGE_OFFSET) + PHYS_OFFSET)
#define __kimg_to_phys(addr)	((addr) - kimage_voffset)

#define __virt_to_phys_nodebug(x) ({					\
	phys_addr_t __x = (phys_addr_t)(x);				\
	__is_lm_address(__x) ? __lm_to_phys(__x) :			\
			       __kimg_to_phys(__x);			\
})

#define __pa_symbol_nodebug(x)	__kimg_to_phys((phys_addr_t)(x))

#define __virt_to_phys(x)	__virt_to_phys_nodebug(x)
#define __phys_addr_symbol(x)	__pa_symbol_nodebug(x)

#define __phys_to_virt(x)	((u64)((x) - PHYS_OFFSET) | PAGE_OFFSET)
#define __phys_to_kimg(x)	((u64)((x) + kimage_voffset))

/*
 * Convert a page to/from a physical address
 */
#define page_to_phys(page)	(__pfn_to_phys(page_to_pfn(page)))
#define phys_to_page(phys)	(pfn_to_page(__phys_to_pfn(phys)))

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

#define phys_to_virt phys_to_virt
static inline void *phys_to_virt(phys_addr_t x)
{
	return (void *)(__phys_to_virt(x));
}

/*
 * Drivers should NOT use these either.
 */
#define __pa(x)			__virt_to_phys((u64)(x))
#define __pa_symbol(x)		__phys_addr_symbol(RELOC_HIDE((u64)(x), 0))
#define __va(x)			((void *)__phys_to_virt((phys_addr_t)(x)))
#define pfn_to_kaddr(pfn)	__va((pfn) << PAGE_SHIFT)
#define virt_to_pfn(x)      __phys_to_pfn(__virt_to_phys((u64)(x)))
#define sym_to_pfn(x)	    __phys_to_pfn(__pa_symbol(x))

/*
 *  virt_to_page(k)	convert a _valid_ virtual address to struct page *
 *  virt_addr_valid(k)	indicates whether a virtual address is valid
 */
#define ARCH_PFN_OFFSET		((u64)PHYS_PFN_OFFSET)

#define __virt_to_pgoff(kaddr)	(((u64)(kaddr) & ~PAGE_OFFSET) / PAGE_SIZE * sizeof(struct page))
#define __page_to_voff(kaddr)	(((u64)(kaddr) & ~VMEMMAP_START) * PAGE_SIZE / sizeof(struct page))

#define page_to_virt(page)	({					\
	u64 __addr =						\
		((__page_to_voff(page)) | PAGE_OFFSET);			\
	__addr = __tag_set(__addr, 0xff);		\
	((void *)__addr);						\
})

#define virt_to_page(vaddr)	((struct page *)((__virt_to_pgoff(vaddr)) | VMEMMAP_START))

#define _virt_addr_valid(kaddr)	pfn_valid((((u64)(kaddr) & ~PAGE_OFFSET) \
					   + PHYS_OFFSET) >> PAGE_SHIFT)

#define _virt_addr_is_linear(kaddr)	\
	(__tag_reset((u64)(kaddr)) >= PAGE_OFFSET)
#define virt_addr_valid(kaddr)		\
	(_virt_addr_is_linear(kaddr) && _virt_addr_valid(kaddr))

#endif /* !__ASSEMBLY__ */

#include <asm-generic/memory_model.h>

#endif /* !__ASM_MEMORY_H_ */
