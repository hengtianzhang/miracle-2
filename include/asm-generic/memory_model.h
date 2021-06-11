/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_MEMORY_MODEL_H_
#define __ASM_MEMORY_MODEL_H_

#include <linux/pfn.h>

#ifndef __ASSEMBLY__

/* memmap is virtually contiguous.  */
#define __pfn_to_page(pfn)	(vmemmap + (pfn))
#define __page_to_pfn(page)	(u64)((page) - vmemmap)

/*
 * Convert a physical address to a Page Frame Number and back
 */
#define	__phys_to_pfn(paddr)	PHYS_PFN(paddr)
#define	__pfn_to_phys(pfn)	PFN_PHYS(pfn)

#define page_to_pfn __page_to_pfn
#define pfn_to_page __pfn_to_page

#endif /* !__ASSEMBLY__ */

#endif /* !__ASM_MEMORY_MODEL_H_ */
