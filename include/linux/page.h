/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_PAGE_H_
#define __LINUX_PAGE_H_

#include <linux/mm_types.h>
#include <linux/mmzone.h>
#include <linux/page-flags-layout.h>
#include <linux/page-flags.h>
#include <linux/page_ref.h>
#include <linux/gfp.h>

#include <asm/page.h>

#define ZONES_PGOFF		((sizeof(u64)*8) - ZONES_WIDTH)
#define ZONES_PGSHIFT		(ZONES_PGOFF * (ZONES_WIDTH != 0))

#define ZONEID_SHIFT		(ZONES_SHIFT)
#define ZONEID_PGOFF		ZONES_PGOFF
#define ZONEID_PGSHIFT		(ZONEID_PGOFF * (ZONEID_SHIFT != 0))

#define ZONES_MASK		((1ULL << ZONES_WIDTH) - 1)
#define ZONEID_MASK		((1ULL << ZONEID_SHIFT) - 1)

static inline enum zone_type page_zonenum(const struct page *page)
{
	return (page->flags >> ZONES_PGSHIFT) & ZONES_MASK;
}

static inline int page_zone_id(struct page *page)
{
	return (page->flags >> ZONEID_PGSHIFT) & ZONEID_MASK;
}

static inline unsigned int compound_order(struct page *page)
{
	if (!PageHead(page))
		return 0;
	return page[1].compound_order;
}

static inline void set_compound_order(struct page *page, unsigned int order)
{
	page[1].compound_order = order;
}

#define page_private(page)		((page)->private)
#define set_page_private(page, v)	((page)->private = (v))

/*
 * This function returns the order of a free page in the buddy system. In
 * general, page_zone(page)->lock must be held by the caller to prevent the
 * page from being allocated in parallel and returning garbage as the order.
 * If a caller does not hold page_zone(page)->lock, it must guarantee that the
 * page cannot be allocated or merged in parallel. Alternatively, it must
 * handle invalid values gracefully, and use page_order_unsafe() below.
 */
static inline unsigned int page_order(struct page *page)
{
	/* PageBuddy() must be checked by the caller */
	return page_private(page);
}

static inline int put_page_testzero(struct page *page)
{
	BUG_ON(page_ref_count(page) == 0);

	return page_ref_dec_and_test(page);
}

/*
 * Try to grab a ref unless the page has a refcount of zero, return false if
 * that is the case.
 * This can be called when MMU is off so it must not access
 * any of the virtual mappings.
 */
static inline int get_page_unless_zero(struct page *page)
{
	return page_ref_add_unless(page, 1, 0);
}

static inline struct page *virt_to_head_page(const void *x)
{
	struct page *page = virt_to_page(x);

	return compound_head(page);
}

static inline void get_page(struct page *page)
{
	page = compound_head(page);
	/*
	 * Getting a normal page or the head of a compound page
	 * requires to already have an elevated page->_refcount.
	 */
	BUG_ON(page_ref_count(page) <= 0);

	page_ref_inc(page);
}

static inline void put_page(struct page *page)
{
	page = compound_head(page);

	if (put_page_testzero(page))
		__free_pages(page, compound_order(page));
}

static inline struct zone *page_zone(const struct page *page)
{
	return &NODE_DATA()->node_zones[page_zonenum(page)];
}

static inline pg_data_t *page_pgdat(const struct page *page)
{
	return NODE_DATA();
}

static inline void set_page_zone(struct page *page, enum zone_type zone)
{
	page->flags &= ~(ZONES_MASK << ZONES_PGSHIFT);
	page->flags |= (zone & ZONES_MASK) << ZONES_PGSHIFT;
}

static inline void set_page_links(struct page *page, enum zone_type zone,
	u64 pfn)
{
	set_page_zone(page, zone);
}

#define page_address(page) page_to_virt(page)

static inline void __free_reserved_page(struct page *page)
{
	ClearPageReserved(page);
	init_page_count(page);
	__free_page(page);
}

static inline void free_reserved_page(struct page *page)
{
	__free_reserved_page(page);
}

static inline void mark_page_reserved(struct page *page)
{
	SetPageReserved(page);
}

void arch_free_page(struct page *page, int order);
void arch_alloc_page(struct page *page, int order);

void prep_compound_page(struct page *page, unsigned int order);

void memblock_free_pages(struct page *page, u64 pfn, unsigned int order);
u64 free_reserved_area(void *start, void *end, int poison, const char *s);

bool is_free_buddy_page(struct page *page);
void zone_pcp_reset(struct zone *zone);

static inline u64 totalram_pages(void)
{
	return (u64)NODE_DATA()->node_spanned_pages;
}

#endif /* !__LINUX_PAGE_H_ */
