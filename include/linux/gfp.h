#ifndef __LINUX_GFP_H
#define __LINUX_GFP_H

#include <linux/page-flags-layout.h>
#include <linux/bitops.h>
#include <linux/stddef.h>
#include <linux/linkage.h>
#include <linux/mmzone.h>

/* Plain integer GFP bitmasks. Do not use this directly. */
#define ___GFP_DMA				BIT(0)
#define ___GFP_NORMAL			BIT(1)
#define ___GFP_MOVABLE			BIT(2)
#define GFP_ZONE_SHIFT			(0)

#define ___GFP_ZERO				BIT(3)
#define ___GFP_NOWARN			BIT(4)

#define ___GFP_BITS_SHIFT		5

/* If the above are modified, __GFP_BITS_SHIFT may need updating */

/*
 * GFP bitmasks..
 *
 * Zone modifiers (see linux/mmzone.h - low three bits)
 *
 * Do not put any conditional on these. If necessary modify the definitions
 * without the underscores and use them consistently. The definitions here may
 * be used in bit comparisons.
 */
#define __GFP_DMA	((__force gfp_t)___GFP_DMA)
#define __GFP_NORMAL	((__force gfp_t)___GFP_NORMAL)
#define __GFP_MOVABLE	((__force gfp_t)___GFP_MOVABLE)
#define GFP_ZONEMASK	(__GFP_DMA|__GFP_NORMAL|__GFP_MOVABLE)

#define __GFP_ZERO	((__force gfp_t)___GFP_ZERO)	/* Return zeroed page on success */
#define __GFP_NOWARN	((__force gfp_t)___GFP_NOWARN)

#define __GFP_BITS_SHIFT ___GFP_BITS_SHIFT
#define __GFP_BITS_MASK ((__force gfp_t)((1 << __GFP_BITS_SHIFT) - 1))

#define GFP_DMA		(__GFP_DMA)
#define GFP_KERNEL	(__GFP_NORMAL)
#define GFP_MOVABLE		(__GFP_MOVABLE)

#define GFP_USER	(GFP_KERNEL)

static inline enum zone_type gfp_zone(gfp_t flags)
{
	if (unlikely(__GFP_DMA & flags))
		return ZONE_DMA;
	if (unlikely(__GFP_MOVABLE & flags))
		return ZONE_MOVABLE;
	
	return ZONE_NORMAL;
}

extern void __free_pages(struct page *page, unsigned int order);
extern void free_pages(u64 addr, unsigned int order);
extern void free_unref_page(struct page *page);
extern void page_frag_free(void *addr);

#define __free_page(page) __free_pages((page), 0)
#define free_page(addr) free_pages((addr), 0)

#endif /* __LINUX_GFP_H */
