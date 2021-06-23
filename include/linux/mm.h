/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_MM_H_
#define __LINUX_MM_H_

#include <linux/errno.h>

#ifdef __KERNEL__

#include <linux/overflow.h>
#include <linux/gfp.h>

#include <asm/page.h>

#ifndef __pa_symbol
#define __pa_symbol(x)  __pa(RELOC_HIDE((u64)(x), 0))
#endif

#ifndef page_to_virt
#define page_to_virt(x)	__va(PFN_PHYS(page_to_pfn(x)))
#endif

#ifndef lm_alias
#define lm_alias(x)	__va(__pa_symbol(x))
#endif

/*
 * To prevent common memory management code establishing
 * a zero page mapping on a read fault.
 * This macro should be defined within <asm/pgtable.h>.
 * s390 does this to prevent multiplexing of hardware bits
 * related to the physical page in case of virtualization.
 */
#ifndef mm_forbids_zeropage
#define mm_forbids_zeropage(X)	(0)
#endif

/*
 * On some architectures it is expensive to call memset() for small sizes.
 * Those architectures should provide their own implementation of "struct page"
 * zeroing by defining this macro in <asm/pgtable.h>.
 */
#ifndef mm_zero_struct_page
#define mm_zero_struct_page(pp)  ((void)memset((pp), 0, sizeof(struct page)))
#endif

/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr) ALIGN(addr, PAGE_SIZE)

/* test whether an address (u64 or pointer) is aligned to PAGE_SIZE */
#define PAGE_ALIGNED(addr)	IS_ALIGNED((u64)(addr), PAGE_SIZE)

#define pfn_valid_within(pfn) pfn_valid(pfn)

#define offset_in_page(p)	((u64)(p) & ~PAGE_MASK)

void kvfree(const void *addr);
void *kvmalloc(size_t size, gfp_t flags);

static inline void *kvzalloc(size_t size, gfp_t flags)
{
	return kvmalloc(size, flags | __GFP_ZERO);
}

static inline void *kvmalloc_array(size_t n, size_t size, gfp_t flags)
{
	size_t bytes;

	if (unlikely(check_mul_overflow(n, size, &bytes)))
		return NULL;

	return kvmalloc(bytes, flags);
}

static inline void *kvcalloc(size_t n, size_t size, gfp_t flags)
{
	return kvmalloc_array(n, size, flags | __GFP_ZERO);
}

#endif /* __KERNEL__ */
#endif /* !__LINUX_MM_H_ */
