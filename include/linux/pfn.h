/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_PFN_H_
#define __LINUX_PFN_H_

#ifndef __ASSEMBLY__

#include <linux/types.h>

/*
 * pfn_t: encapsulates a page-frame number that is optionally backed
 * by memmap (struct page).  Whether a pfn_t has a 'struct page'
 * backing is indicated by flags in the high bits of the value.
 */
typedef struct {
	u64 val;
} pfn_t;

#endif /* !__ASSEMBLY__ */

#define PFN_ALIGN(x)	(((u64)(x) + (PAGE_SIZE - 1)) & PAGE_MASK)
#define PFN_UP(x)	(((x) + PAGE_SIZE - 1) >> PAGE_SHIFT)
#define PFN_DOWN(x)	((x) >> PAGE_SHIFT)
#define PFN_PHYS(x)	((phys_addr_t)(x) << PAGE_SHIFT)
#define PHYS_PFN(x)	((u64)((x) >> PAGE_SHIFT))

#endif /* !__LINUX_PFN_H_ */
