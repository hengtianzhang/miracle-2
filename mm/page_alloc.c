/*
 *  linux/mm/page_alloc.c
 *
 *  Manages the free list, the system allocates free pages here.
 *  Note that kmalloc() lives in slab.c
 *
 *  Copyright (C) 1991, 1992, 1993, 1994  Linus Torvalds
 *  Swap reorganised 29.12.95, Stephen Tweedie
 *  Support of BIGMEM added by Gerhard Wichert, Siemens AG, July 1999
 *  Reshaped it to be a zoned allocator, Ingo Molnar, Red Hat, 1999
 *  Discontiguous memory support, Kanoj Sarcar, SGI, Nov 1999
 *  Zone balancing, Kanoj Sarcar, SGI, Jan 2000
 *  Per cpu hot/cold page lists, bulk allocation, Martin J. Bligh, Sept 2002
 *          (lots of bits borrowed from Ingo Molnar & Andrew Morton)
 */

#include <linux/stddef.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/page-flags.h>
#include <linux/page-flags-layout.h>
#include <linux/page_ref.h>
#include <linux/mmzone.h>
#include <linux/memblock.h>
#include <linux/kernel.h>
#include <linux/percpu.h>
#include <linux/page.h>
#include <linux/cpumask.h>
#include <linux/gfp.h>
#include <linux/prefetch.h>
#include <linux/spinlock.h>

#include "internal.h"

u64 max_pfn;
struct pglist_data node_data __read_mostly;
u64 highest_memmap_pfn __read_mostly;

static u64 arch_zone_lowest_possible_pfn[MAX_NR_ZONES] __initdata;
static u64 arch_zone_highest_possible_pfn[MAX_NR_ZONES] __initdata;

static u64 nr_kernel_pages __initdata;
static u64 nr_all_pages __initdata;
static u64 dma_reserve __initdata;

static DEFINE_PER_CPU(struct per_cpu_pageset, boot_pageset);

static char * const zone_names[MAX_NR_ZONES] = {
	 "DMA",
	 "Normal",
	 "Movable",
};

static void bad_page(struct page *page, const char *reason,
		u64 bad_flags)
{
	pr_alert("BUG: Bad page state pfn:%05llx %s\n", page_to_pfn(page), reason);
	bad_flags &= page->flags;
	if (bad_flags)
		pr_alert("bad because of flags: %#llx(%pGp)\n",
						bad_flags, &bad_flags);
}

static int free_tail_pages_check(struct page *head_page, struct page *page)
{
	int ret = 1;

	/*
	 * We rely page->lru.next never has bit 0 set, unless the page
	 * is PageTail(). Let's make sure that's true even for poisoned ->lru.
	 */
	BUILD_BUG_ON((u64)LIST_POISON1 & 1);

	if (unlikely(!PageTail(page))) {
		bad_page(page, "PageTail not set", 0);
		goto out;
	}
	if (unlikely(compound_head(page) != head_page)) {
		bad_page(page, "compound_head not consistent", 0);
		goto out;
	}
	ret = 0;
out:
	clear_compound_head(page);
	return ret;
}

/*
 * A bad page could be due to a number of fields. Instead of multiple branches,
 * try and check multiple fields with one check. The caller must do a detailed
 * check if necessary.
 */
static inline bool page_expected_state(struct page *page,
					u64 check_flags)
{
	if (unlikely(page_ref_count(page) | (page->flags & check_flags)))
		return false;

	return true;
}

static void free_pages_check_bad(struct page *page)
{
	const char *bad_reason;
	u64 bad_flags;

	bad_reason = NULL;
	bad_flags = 0;

	if (unlikely(page_ref_count(page) != 0))
		bad_reason = "nonzero _refcount";
	if (unlikely(page->flags & PAGE_FLAGS_CHECK_AT_FREE)) {
		bad_reason = "PAGE_FLAGS_CHECK_AT_FREE flag(s) set";
		bad_flags = PAGE_FLAGS_CHECK_AT_FREE;
	}

	bad_page(page, bad_reason, bad_flags);
}


static inline int free_pages_check(struct page *page)
{
	if (likely(page_expected_state(page, PAGE_FLAGS_CHECK_AT_FREE)))
		return 0;

	/* Something has gone sideways, find it */
	free_pages_check_bad(page);
	return 1;
}

void __weak arch_free_page(struct page *page, int order)
{

}

void __weak arch_alloc_page(struct page *page, int order)
{

}

static __always_inline bool free_pages_prepare(struct page *page,
					unsigned int order, bool check_free)
{
	int bad = 0;

	BUG_ON(PageTail(page));

	/*
	 * Check tail pages before head page information is cleared to
	 * avoid checking PageCompound for order-0 pages.
	 */
	if (unlikely(order)) {
		bool compound = PageCompound(page);
		int i;

		BUG_ON(compound && compound_order(page) != order);

		for (i = 1; i < (1 << order); i++) {
			if (compound)
				bad += free_tail_pages_check(page, page + i);
			if (unlikely(free_pages_check(page + i))) {
				bad++;
				continue;
			}
			(page + i)->flags &= ~PAGE_FLAGS_CHECK_AT_PREP;
		}
	}
	if (check_free)
		bad += free_pages_check(page);
	if (bad)
		return false;

	page->flags &= ~PAGE_FLAGS_CHECK_AT_PREP;

	arch_free_page(page, order);

	return true;
}

static bool free_pcp_prepare(struct page *page)
{
	return free_pages_prepare(page, 0, false);
}

static bool free_unref_page_prepare(struct page *page, u64 pfn)
{
	if (!free_pcp_prepare(page))
		return false;

	return true;
}

static bool bulkfree_pcp_prepare(struct page *page)
{
	return free_pages_check(page);
}

static inline void prefetch_buddy(struct page *page)
{
	u64 pfn = page_to_pfn(page);
	u64 buddy_pfn = __find_buddy_pfn(pfn, 0);
	struct page *buddy = page + (buddy_pfn - pfn);

	prefetch(buddy);
}

/*
 * This function checks whether a page is free && is the buddy
 * we can coalesce a page and its buddy if
 * (a) the buddy is not in a hole (check before calling!) &&
 * (b) the buddy is in the buddy system &&
 * (c) a page and its buddy have the same order &&
 * (d) a page and its buddy are in the same zone.
 *
 * For recording whether a page is in the buddy system, we set PageBuddy.
 * Setting, clearing, and testing PageBuddy is serialized by zone->lock.
 *
 * For recording page's order, we use page_private(page).
 */
static inline int page_is_buddy(struct page *page, struct page *buddy,
							unsigned int order)
{
	if (PageBuddy(buddy) && page_order(buddy) == order) {
		/*
		 * zone check is done late to avoid uselessly
		 * calculating zone/node ids for pages that could
		 * never merge.
		 */
		if (page_zone_id(page) != page_zone_id(buddy))
			return 0;

		BUG_ON(page_count(buddy) != 0);

		return 1;
	}
	return 0;
}

static inline void set_page_order(struct page *page, unsigned int order)
{
	set_page_private(page, order);
	__SetPageBuddy(page);
}

static inline void rmv_page_order(struct page *page)
{
	__ClearPageBuddy(page);
	set_page_private(page, 0);
}

static inline void __free_one_page(struct page *page,
		u64 pfn,
		struct zone *zone, unsigned int order)
{
	u64 combined_pfn;
	u64 uninitialized_var(buddy_pfn);
	struct page *buddy;
	unsigned int max_order;

	max_order = min_t(unsigned int, MAX_ORDER, pageblock_order + 1);

	BUG_ON(!zone_is_initialized(zone));
	BUG_ON(page->flags & PAGE_FLAGS_CHECK_AT_PREP);

	BUG_ON(pfn & ((1 << order) - 1));

continue_merging:
	while (order < max_order - 1) {
		buddy_pfn = __find_buddy_pfn(pfn, order);
		buddy = page + (buddy_pfn - pfn);

		if (!pfn_valid_within(buddy_pfn))
			goto done_merging;
		if (!page_is_buddy(page, buddy, order))
			goto done_merging;

		list_del(&buddy->lru);
		zone->free_area[order].nr_free--;
		rmv_page_order(buddy);

		combined_pfn = buddy_pfn & pfn;
		page = page + (combined_pfn - pfn);
		pfn = combined_pfn;
		order++;
	}
	if (max_order < MAX_ORDER) {
		max_order++;
		goto continue_merging;
	}

done_merging:
	set_page_order(page, order);

	/*
	 * If this is not the largest possible page, check if the buddy
	 * of the next-highest order is free. If it is, it's possible
	 * that pages are being freed that will coalesce soon. In case,
	 * that is happening, add the free page to the tail of the list
	 * so it's less likely to be used soon and more likely to be merged
	 * as a higher order page
	 */
	if ((order < MAX_ORDER-2) && pfn_valid_within(buddy_pfn)) {
		struct page *higher_page, *higher_buddy;
		combined_pfn = buddy_pfn & pfn;
		higher_page = page + (combined_pfn - pfn);
		buddy_pfn = __find_buddy_pfn(combined_pfn, order + 1);
		higher_buddy = higher_page + (buddy_pfn - combined_pfn);
		if (pfn_valid_within(buddy_pfn) &&
		    page_is_buddy(higher_page, higher_buddy, order + 1)) {
			list_add_tail(&page->lru,
				&zone->free_area[order].free_list);
			goto out;
		}
	}

	list_add(&page->lru, &zone->free_area[order].free_list);
out:
	zone->free_area[order].nr_free++;
}

/*
 * Frees a number of pages from the PCP lists
 * Assumes all pages on list are in same zone, and of same order.
 * count is the number of pages to free.
 *
 * If the zone was previously in an "all pages pinned" state then look to
 * see if this freeing clears that state.
 *
 * And clear the zone's pages_scanned counter, to hold off the "all pages are
 * pinned" detection logic.
 */
static void free_pcppages_bulk(struct zone *zone, int count,
					struct per_cpu_pages *pcp)
{
	int batch_free = 0;
	int prefetch_nr = 0;
	struct page *page, *tmp;
	LIST_HEAD(head);

	while (count) {
		struct list_head *list;

		/*
		 * Remove pages from lists in a round-robin fashion. A
		 * batch_free count is maintained that is incremented when an
		 * empty list is encountered.  This is so more pages are freed
		 * off fuller lists instead of spinning excessively around empty
		 * lists
		 */
		do {
			batch_free++;
			list = &pcp->lists;
		} while (list_empty(list));

		do {
			page = list_last_entry(list, struct page, lru);
			/* must delete to avoid corrupting pcp list */
			list_del(&page->lru);
			pcp->count--;

			if (bulkfree_pcp_prepare(page))
				continue;

			list_add_tail(&page->lru, &head);

			/*
			 * We are going to put the page back to the global
			 * pool, prefetch its buddy to speed up later access
			 * under zone->lock. It is believed the overhead of
			 * an additional test and calculating buddy_pfn here
			 * can be offset by reduced memory latency later. To
			 * avoid excessive prefetching due to large count, only
			 * prefetch buddy for the first pcp->batch nr of pages.
			 */
			if (prefetch_nr++ < pcp->batch)
				prefetch_buddy(page);
		} while (--count && --batch_free && !list_empty(list));
	}

	spin_lock(&zone->lock);

	/*
	 * Use safe version since after __free_one_page(),
	 * page->lru.next will not point to original list.
	 */
	list_for_each_entry_safe(page, tmp, &head, lru)
		__free_one_page(page, page_to_pfn(page), zone, 0);

	spin_unlock(&zone->lock);
}

static void free_unref_page_commit(struct page *page, u64 pfn)
{
	struct zone *zone = page_zone(page);
	struct per_cpu_pages *pcp;

	pcp = &this_cpu_ptr(zone->pageset)->pcp;
	list_add(&page->lru, &pcp->lists);
	pcp->count++;

	if (pcp->count >= pcp->high) {
		u64 batch = READ_ONCE(pcp->batch);
		free_pcppages_bulk(zone, batch, pcp);
	}
}

/*
 * Free a 0-order page
 */
void free_unref_page(struct page *page)
{
	u64 flags;
	u64 pfn = page_to_pfn(page);

	if (!free_unref_page_prepare(page, pfn))
		return;

	local_irq_save(flags);
	free_unref_page_commit(page, pfn);
	local_irq_restore(flags);
}

static void free_one_page(struct zone *zone,
				struct page *page, u64 pfn,
				unsigned int order)
{
	spin_lock(&zone->lock);
	__free_one_page(page, pfn, zone, order);
	spin_unlock(&zone->lock);
}

static void __free_pages_ok(struct page *page, unsigned int order)
{
	u64 flags;
	u64 pfn = page_to_pfn(page);

	if (!free_pages_prepare(page, order, true))
		return;

	local_irq_save(flags);
	free_one_page(page_zone(page), page, pfn, order);
	local_irq_restore(flags);
}

static inline bool prepare_alloc_pages(gfp_t gfp_mask, unsigned int order,
		struct alloc_context *ac)
{
	ac->zoneidx = gfp_zone(gfp_mask);

	return true;
}

static inline struct page *
__alloc_pages_slowpath(gfp_t gfp_mask, unsigned int order,
						struct alloc_context *ac)
{
	return NULL;
}

static inline void expand(struct zone *zone, struct page *page,
	int low, int high, struct free_area *area)
{
	u64 size = 1 << high;

	while (high > low) {
		area--;
		high--;
		size >>= 1;

		list_add(&page[size].lru, &area->free_list);
		area->nr_free++;
		set_page_order(&page[size], high);
	}
}

static __always_inline
struct page *__rmqueue_smallest(struct zone *zone, unsigned int order)
{
	unsigned int current_order;
	struct free_area *area;
	struct page *page;

	/* Find a page of the appropriate size in the preferred list */
	for (current_order = order; current_order < MAX_ORDER; ++current_order) {
		area = &(zone->free_area[current_order]);
		page = list_first_entry_or_null(&area->free_list,
							struct page, lru);
		if (!page)
			continue;
		list_del(&page->lru);
		rmv_page_order(page);
		area->nr_free--;
		expand(zone, page, order, current_order, area);

		return page;
	}

	return NULL;
}

static __always_inline struct page *
__rmqueue(struct zone *zone, unsigned int order,
						gfp_t gfp_flags)
{
	struct page *page;

	page = __rmqueue_smallest(zone, order);
	if (unlikely(!page))
		return NULL;

	return page;
}

static void check_new_page_bad(struct page *page)
{
	const char *bad_reason = NULL;
	u64 bad_flags = 0;

	if (unlikely(page_ref_count(page) != 0))
		bad_reason = "nonzero _count";
	if (unlikely(page->flags & __PG_HWPOISON)) {
		bad_reason = "HWPoisoned (hardware-corrupted)";
		bad_flags = __PG_HWPOISON;

		return;
	}
	if (unlikely(page->flags & PAGE_FLAGS_CHECK_AT_PREP)) {
		bad_reason = "PAGE_FLAGS_CHECK_AT_PREP flag set";
		bad_flags = PAGE_FLAGS_CHECK_AT_PREP;
	}

	bad_page(page, bad_reason, bad_flags);
}

/*
 * This page is about to be returned from the page allocator
 */
static inline int check_new_page(struct page *page)
{
	if (likely(page_expected_state(page,
				PAGE_FLAGS_CHECK_AT_PREP|__PG_HWPOISON)))
		return 0;

	check_new_page_bad(page);
	return 1;
}

static bool check_new_pages(struct page *page, unsigned int order)
{
	int i;
	for (i = 0; i < (1 << order); i++) {
		struct page *p = page + i;

		if (unlikely(check_new_page(p)))
			return true;
	}

	return false;
}

static bool check_pcp_refill(struct page *page)
{
	return check_new_page(page);
}
static bool check_new_pcp(struct page *page)
{
	return false;
}

static int rmqueue_bulk(struct zone *zone, unsigned int order,
			u64 count, struct list_head *list,
			gfp_t gfp_flags)
{
	int i, alloced = 0;

	spin_lock(&zone->lock);
	for (i = 0; i < count; ++i) {
		struct page *page = __rmqueue(zone, order, gfp_flags);
		if (unlikely(page == NULL))
			break;

		if (unlikely(check_pcp_refill(page)))
			continue;

		/*
		 * Split buddy pages returned by expand() are received here in
		 * physical page order. The page is added to the tail of
		 * caller's list. From the callers perspective, the linked list
		 * is ordered by page number under some conditions. This is
		 * useful for IO devices that can forward direction from the
		 * head, thus also in the physical page order. This is useful
		 * for IO devices that can merge IO requests if the physical
		 * pages are ordered properly.
		 */
		list_add_tail(&page->lru, list);
		alloced++;
	}

	spin_unlock(&zone->lock);

	return alloced;
}

/* Remove page from the per-cpu list, caller must protect the list */
static struct page *__rmqueue_pcplist(struct zone *zone,
			gfp_t gfp_flags,
			struct per_cpu_pages *pcp,
			struct list_head *list)
{
	struct page *page;

	do {
		if (list_empty(list)) {
			pcp->count += rmqueue_bulk(zone, 0,
					pcp->batch, list, gfp_flags);
			if (unlikely(list_empty(list)))
				return NULL;
		}

		page = list_first_entry(list, struct page, lru);
		list_del(&page->lru);
		pcp->count--;
	} while (check_new_pcp(page));

	return page;
}

/* Lock and remove page from the per-cpu list */
static struct page *rmqueue_pcplist(struct zone *zone, unsigned int order,
			gfp_t gfp_flags)
{
	struct per_cpu_pages *pcp;
	struct list_head *list;
	struct page *page;
	u64 flags;

	local_irq_save(flags);
	pcp = &this_cpu_ptr(zone->pageset)->pcp;
	list = &pcp->lists;
	page = __rmqueue_pcplist(zone, gfp_flags, pcp, list);
	local_irq_restore(flags);

	return page;
}

static inline
struct page *rmqueue(struct zone *zone, unsigned int order,
			gfp_t gfp_flags)
{
	u64 flags;
	struct page *page;

	if (likely(order == 0)) {
		page = rmqueue_pcplist(zone, order, gfp_flags);
		goto out;
	}

	spin_lock_irqsave(&zone->lock, flags);

	do {
		page = NULL;
		if (!page)
			page = __rmqueue(zone, order, gfp_flags);
	} while (page && check_new_pages(page, order));
	spin_unlock(&zone->lock);
	if (!page)
		goto failed;

	local_irq_restore(flags);

out:
	return page;

failed:
	local_irq_restore(flags);

	return NULL;
}

static inline void post_alloc_hook(struct page *page, unsigned int order,
				gfp_t gfp_flags)
{
	set_page_private(page, 0);
	set_page_refcounted(page);

	arch_alloc_page(page, order);
}

void prep_compound_page(struct page *page, unsigned int order)
{
	int i;
	int nr_pages = 1 << order;

	set_compound_order(page, order);
	__SetPageHead(page);
	for (i = 1; i < nr_pages; i++) {
		struct page *p = page + i;
		set_page_count(p, 0);
		set_compound_head(p, page);
	}
}

static void prep_new_page(struct page *page, unsigned int order, gfp_t gfp_flags)
{
	int i;

	post_alloc_hook(page, order, gfp_flags);

	if (gfp_flags & __GFP_ZERO)
		for (i = 0; i < (1 << order); i++)
			clear_page(page_address(page + i));

	if (order)
		prep_compound_page(page, order);
}

/*
 * get_page_from_freelist goes through the zonelist trying to allocate
 * a page.
 */
static struct page *
get_page_from_freelist(gfp_t gfp_mask, unsigned int order, const struct alloc_context *ac)
{
	struct page *page;
	struct zone *zone = NODE_DATA()->node_zones + ac->zoneidx;

	page = rmqueue(zone, order, gfp_mask);
	if (page) {
		prep_new_page(page, order, gfp_mask);

		return page;
	}

	return NULL;
}

/*
 * This is the 'heart' of the zoned buddy allocator.
 */
struct page *__alloc_pages(gfp_t gfp_mask, unsigned int order)
{
	struct page *page;
	struct alloc_context ac = { };

	if (unlikely(order >= MAX_ORDER)) {
		WARN_ON_ONCE(!(gfp_mask & __GFP_NOWARN));
		return NULL;
	}

	if (!prepare_alloc_pages(gfp_mask, order, &ac))
		return NULL;

	/* First allocation attempt */
	page = get_page_from_freelist(gfp_mask, order, &ac);
	if (likely(page))
		goto out;
	
	page = __alloc_pages_slowpath(gfp_mask, order, &ac);

out:

	return page;
}

u64 __get_free_pages(gfp_t gfp_mask, unsigned int order)
{
	struct page *page;

	page = alloc_pages(gfp_mask, order);
	if (!page)
		return 0;
	return (u64)page_address(page);
}

u64 get_zeroed_page(gfp_t gfp_mask)
{
	return __get_free_pages(gfp_mask | __GFP_ZERO, 0);
}

static inline void free_the_page(struct page *page, unsigned int order)
{
	if (order == 0)		/* Via pcp? */
		free_unref_page(page);
	else
		__free_pages_ok(page, order);
}

void __free_pages(struct page *page, unsigned int order)
{
	if (put_page_testzero(page))
		free_the_page(page, order);
}

void free_pages(u64 addr, unsigned int order)
{
	if (addr != 0) {
		BUG_ON(!virt_addr_valid((void *)addr));
		__free_pages(virt_to_page((void *)addr), order);
	}
}

/*
 * Frees a page fragment allocated out of either a compound or order 0 page.
 */
void page_frag_free(void *addr)
{
	struct page *page = virt_to_head_page(addr);

	if (unlikely(put_page_testzero(page)))
		free_the_page(page, compound_order(page));
}

static void __init __free_pages_boot_core(struct page *page, unsigned int order)
{
	unsigned int nr_pages = 1 << order;
	struct page *p = page;
	unsigned int loop;

	prefetchw(p);
	for (loop = 0; loop < (nr_pages - 1); loop++, p++) {
		prefetchw(p + 1);
		__ClearPageReserved(p);
		set_page_count(p, 0);
	}
	__ClearPageReserved(p);
	set_page_count(p, 0);

	atomic_long_add(nr_pages, &page_zone(page)->managed_pages);
	set_page_refcounted(page);
	__free_pages(page, order);
}

void __init memblock_free_pages(struct page *page, u64 pfn,
							unsigned int order)
{
	return __free_pages_boot_core(page, order);
}

u64 free_reserved_area(void *start, void *end, int poison, const char *s)
{
	void *pos;
	u64 pages = 0;

	start = (void *)PAGE_ALIGN((u64)start);
	end = (void *)((u64)end & PAGE_MASK);
	for (pos = start; pos < end; pos += PAGE_SIZE, pages++) {
		struct page *page = virt_to_page(pos);
		void *direct_map_addr;

		/*
		 * 'direct_map_addr' might be different from 'pos'
		 * because some architectures' virt_to_page()
		 * work with aliases.  Getting the direct map
		 * address ensures that we get a _writeable_
		 * alias for the memset().
		 */
		direct_map_addr = page_address(page);
		if ((unsigned int)poison <= 0xFF)
			memset(direct_map_addr, poison, PAGE_SIZE);

		free_reserved_page(page);
	}

	if (pages && s)
		pr_info("Freeing %s memory: %lldK\n",
			s, pages << (PAGE_SHIFT - 10));

	return pages;
}

/* Find the lowest pfn for a node */
static u64 __init find_min_pfn_for_node(void)
{
	phys_addr_t min_pfn = ULONG_MAX;
	phys_addr_t start_pfn;
	u64 i;

	for_each_mem_pfn_range(i, &start_pfn, NULL)
		min_pfn = min(min_pfn, start_pfn);

	if (min_pfn == ULONG_MAX) {
		pr_warn("Could not find start_pfn\n");
		return 0;
	}

	return min_pfn;
}

/**
 * find_min_pfn_with_active_regions - Find the minimum PFN registered
 *
 */
u64 __init find_min_pfn_with_active_regions(void)
{
	return find_min_pfn_for_node();
}

static void __init find_zone_movable_pfns_for_nodes(void)
{

}

/*
 * Zero all valid struct pages in range [spfn, epfn), return number of struct
 * pages zeroed
 */
static u64 zero_pfn_range(u64 spfn, u64 epfn)
{
	u64 pfn;
	u64 pgcnt = 0;

	for (pfn = spfn; pfn < epfn; pfn++) {
		if (!pfn_valid(ALIGN_DOWN(pfn, pageblock_nr_pages))) {
			pfn = ALIGN_DOWN(pfn, pageblock_nr_pages)
				+ pageblock_nr_pages - 1;
			continue;
		}
		mm_zero_struct_page(pfn_to_page(pfn));
		pgcnt++;
	}

	return pgcnt;
}

void __init zero_resv_unavail(void)
{
	phys_addr_t start, end;
	u64 i, pgcnt;
	phys_addr_t next = 0;

	/*
	 * Loop through unavailable ranges not covered by memblock.memory.
	 */
	pgcnt = 0;
	for_each_mem_range(i, &memblock.memory, NULL,
			MEMBLOCK_NONE, &start, &end) {
		if (next < start)
			pgcnt += zero_pfn_range(PFN_DOWN(next), PFN_UP(start));
		next = end;
	}
	pgcnt += zero_pfn_range(PFN_DOWN(next), max_pfn);

	/*
	 * Struct pages that do not have backing memory. This could be because
	 * firmware is using some of this memory, or for some other reasons.
	 */
	if (pgcnt)
		pr_info("Zeroed struct page in unavailable ranges: %lld pages", pgcnt);
}

void __init get_pfn_range(u64 *start_pfn, u64 *end_pfn)
{
	u64 this_start_pfn, this_end_pfn;
	u64 i;

	*start_pfn = -1UL;
	*end_pfn = 0;

	for_each_mem_pfn_range(i, &this_start_pfn, &this_end_pfn) {
		*start_pfn = min(*start_pfn, this_start_pfn);
		*end_pfn = max(*end_pfn, this_end_pfn);
	}

	if (*start_pfn == -1UL)
		*start_pfn = 0;
}

static void __init adjust_zone_range_for_zone_movable(u64 zone_type,
					u64 node_start_pfn,
					u64 node_end_pfn,
					u64 *zone_start_pfn,
					u64 *zone_end_pfn)
{

}

/*
 * Return the number of pages a zone spans in a node, including holes
 * present_pages = zone_spanned_pages_in_node() - zone_absent_pages_in_node()
 */
static u64 __init zone_spanned_pages_in_node(u64 zone_type,
					u64 node_start_pfn,
					u64 node_end_pfn,
					u64 *zone_start_pfn,
					u64 *zone_end_pfn,
					u64 *ignored)
{
	/* When hotadd a new node from cpu_up(), the node should be empty */
	if (!node_start_pfn && !node_end_pfn)
		return 0;

	/* Get the start and end of the zone */
	*zone_start_pfn = arch_zone_lowest_possible_pfn[zone_type];
	*zone_end_pfn = arch_zone_highest_possible_pfn[zone_type];
	adjust_zone_range_for_zone_movable(zone_type,
				node_start_pfn, node_end_pfn,
				zone_start_pfn, zone_end_pfn);

	/* Check that this node has pages within the zone's required range */
	if (*zone_end_pfn < node_start_pfn || *zone_start_pfn > node_end_pfn)
		return 0;

	/* Move the zone boundaries inside the node if necessary */
	*zone_end_pfn = min(*zone_end_pfn, node_end_pfn);
	*zone_start_pfn = max(*zone_start_pfn, node_start_pfn);

	/* Return the spanned pages */
	return *zone_end_pfn - *zone_start_pfn;
}

/*
 * Return the number of holes in a range on a node. If nid is MAX_NUMNODES,
 * then all holes in the requested range will be accounted for.
 */
u64 __init __absent_pages_in_range(u64 range_start_pfn,
				u64 range_end_pfn)
{
	u64 nr_absent = range_end_pfn - range_start_pfn;
	u64 start_pfn, end_pfn;
	u64 i;

	for_each_mem_pfn_range(i, &start_pfn, &end_pfn) {
		start_pfn = clamp(start_pfn, range_start_pfn, range_end_pfn);
		end_pfn = clamp(end_pfn, range_start_pfn, range_end_pfn);
		nr_absent -= end_pfn - start_pfn;
	}
	return nr_absent;
}

/* Return the number of page frames in holes in a zone on a node */
static u64 __init zone_absent_pages_in_node(u64 zone_type,
					u64 node_start_pfn,
					u64 node_end_pfn,
					u64 *ignored)
{
	u64 zone_low = arch_zone_lowest_possible_pfn[zone_type];
	u64 zone_high = arch_zone_highest_possible_pfn[zone_type];
	u64 zone_start_pfn, zone_end_pfn;
	u64 nr_absent;

	/* When hotadd a new node from cpu_up(), the node should be empty */
	if (!node_start_pfn && !node_end_pfn)
		return 0;

	zone_start_pfn = clamp(node_start_pfn, zone_low, zone_high);
	zone_end_pfn = clamp(node_end_pfn, zone_low, zone_high);

	adjust_zone_range_for_zone_movable(zone_type,
			node_start_pfn, node_end_pfn,
			&zone_start_pfn, &zone_end_pfn);
	nr_absent = __absent_pages_in_range(zone_start_pfn, zone_end_pfn);

	return nr_absent;
}

static void __init calculate_node_totalpages(struct pglist_data *pgdat,
						u64 node_start_pfn,
						u64 node_end_pfn,
						u64 *zones_size,
						u64 *zholes_size)
{
	u64 realtotalpages = 0, totalpages = 0;
	enum zone_type i;

	for (i = 0; i < MAX_NR_ZONES; i++) {
		struct zone *zone = pgdat->node_zones + i;
		u64 zone_start_pfn, zone_end_pfn;
		u64 size, real_size;

		size = zone_spanned_pages_in_node(i,
						  node_start_pfn,
						  node_end_pfn,
						  &zone_start_pfn,
						  &zone_end_pfn,
						  zones_size);
		real_size = size - zone_absent_pages_in_node(i,
						  node_start_pfn, node_end_pfn,
						  zholes_size);
		if (size)
			zone->zone_start_pfn = zone_start_pfn;
		else
			zone->zone_start_pfn = 0;
		zone->spanned_pages = size;
		zone->present_pages = real_size;

		totalpages += size;
		realtotalpages += real_size;
	}

	pgdat->node_spanned_pages = totalpages;
	pgdat->node_present_pages = realtotalpages;

	printk(KERN_DEBUG "On node totalpages: %llu\n", realtotalpages);
}

static void __meminit pgdat_init_internals(struct pglist_data *pgdat)
{
}

static u64 __init calc_memmap_size(u64 spanned_pages,
						u64 present_pages)
{
	u64 pages = spanned_pages;

	/*
	 * Provide a more accurate estimation if there are holes within
	 * the zone and SPARSEMEM is in use. If there are holes within the
	 * zone, each populated memory region may cost us one or two extra
	 * memmap pages due to alignment because memmap pages for each
	 * populated regions may not be naturally aligned on page boundary.
	 * So the (present_pages >> 4) heuristic is a tradeoff for that.
	 */
	if (spanned_pages > present_pages + (present_pages >> 4))
		pages = present_pages;

	return PAGE_ALIGN(pages * sizeof(struct page)) >> PAGE_SHIFT;
}

static int zone_batchsize(struct zone *zone)
{
	int batch;

	/*
	 * The per-cpu-pages pools are set to around 1000th of the
	 * size of the zone.
	 */
	batch = zone_managed_pages(zone) / 1024;
	/* But no more than a meg. */
	if (batch * PAGE_SIZE > 1024 * 1024)
		batch = (1024 * 1024) / PAGE_SIZE;
	batch /= 4;		/* We effectively *= 4 below */
	if (batch < 1)
		batch = 1;

	/*
	 * Clamp the batch to a 2^n - 1 value. Having a power
	 * of 2 value was found to be more likely to have
	 * suboptimal cache aliasing properties in some cases.
	 *
	 * For example if 2 tasks are alternately allocating
	 * batches of pages, one task can end up with a lot
	 * of pages of one half of the possible page colors
	 * and the other with pages of the other colors.
	 */
	batch = rounddown_pow_of_two(batch + batch/2) - 1;

	return batch;
}

static void pageset_init(struct per_cpu_pageset *p)
{
	struct per_cpu_pages *pcp;

	memset(p, 0, sizeof(*p));

	pcp = &p->pcp;
	INIT_LIST_HEAD(&pcp->lists);
}

static void pageset_update(struct per_cpu_pages *pcp, u64 high,
		u64 batch)
{
       /* start with a fail safe value for batch */
	pcp->batch = 1;
	smp_wmb();

       /* Update high, then batch, in order */
	pcp->high = high;
	smp_wmb();

	pcp->batch = batch;
}

/* a companion to pageset_set_high() */
static void pageset_set_batch(struct per_cpu_pageset *p, u64 batch)
{
	pageset_update(&p->pcp, 6 * batch, max(1ULL, 1 * batch));
}

static void pageset_set_high_and_batch(struct zone *zone,
				       struct per_cpu_pageset *pcp)
{
	pageset_set_batch(pcp, zone_batchsize(zone));
}

static void __meminit zone_pageset_init(struct zone *zone, int cpu)
{
	struct per_cpu_pageset *pcp = per_cpu_ptr(zone->pageset, cpu);

	pageset_init(pcp);
	pageset_set_high_and_batch(zone, pcp);
}

static void __meminit setup_zone_pageset(struct zone *zone)
{
	int cpu;
	zone->pageset = alloc_percpu(struct per_cpu_pageset);
	for_each_possible_cpu(cpu)
		zone_pageset_init(zone, cpu);
}

/*
 * Allocate per cpu pagesets and initialize them.
 * Before this call only boot pagesets were available.
 */
void __init setup_per_cpu_pageset(void)
{
	struct zone *zone;

	for_each_populated_zone(zone)
		setup_zone_pageset(zone);
}

static __meminit void zone_pcp_init(struct zone *zone)
{
	/*
	 * per cpu subsystem is not up at this point. The following code
	 * relies on the ability of the linker to provide the
	 * offset of a (static) per cpu variable into the per cpu area.
	 */
	zone->pageset = &boot_pageset;

	if (populated_zone(zone))
		printk(KERN_DEBUG "  %s zone: %llu pages, LIFO batch:%u\n",
			zone->name, zone->present_pages,
					 zone_batchsize(zone));
}

static void __meminit zone_init_internals(struct zone *zone, enum zone_type idx,
							u64 remaining_pages)
{
	atomic_long_set(&zone->managed_pages, remaining_pages);
	zone->name = zone_names[idx];
	zone->zone_pgdat = NODE_DATA();
	spin_lock_init(&zone->lock);
	zone_pcp_init(zone);
}

static void __meminit zone_init_free_lists(struct zone *zone)
{
	u32 order;

	for_each_order(order) {
		INIT_LIST_HEAD(&zone->free_area[order].free_list);
		zone->free_area[order].nr_free = 0;
	}
}

void __meminit init_currently_empty_zone(struct zone *zone,
					u64 zone_start_pfn,
					u64 size)
{
	struct pglist_data *pgdat = zone->zone_pgdat;
	int zone_idx = zone_idx(zone) + 1;

	if (zone_idx > pgdat->nr_zones)
		pgdat->nr_zones = zone_idx;

	zone->zone_start_pfn = zone_start_pfn;

	pr_info("Initialising map zone %llu pfns %llu -> %llu\n",
			(u64)zone_idx(zone),
			zone_start_pfn, (zone_start_pfn + size));

	zone_init_free_lists(zone);
	zone->initialized = 1;
}

static void __meminit __init_single_page(struct page *page, u64 pfn,
				u64 zone)
{
	mm_zero_struct_page(page);
	set_page_links(page, zone, pfn);
	init_page_count(page);

	INIT_LIST_HEAD(&page->lru);
}

static void setup_pageset(struct per_cpu_pageset *p, u64 batch)
{
	pageset_init(p);
	pageset_set_batch(p, batch);
}

void build_all_zone(pg_data_t *pgdat)
{
	int cpu;

	for_each_possible_cpu(cpu)
		setup_pageset(&per_cpu(boot_pageset, cpu), 0);
}

/*
 * Initially all pages are reserved - free ones are freed
 * up by memblock_free_all() once the early boot process is
 * done. Non-atomic initialization, single-pass.
 */
void __meminit memmap_init_zone(u64 size, u64 zone,
		u64 start_pfn)
{
	u64 pfn, end_pfn = start_pfn + size;
	struct page *page;

	if (highest_memmap_pfn < end_pfn - 1)
		highest_memmap_pfn = end_pfn - 1;

	for (pfn = start_pfn; pfn < end_pfn; pfn++) {
		page = pfn_to_page(pfn);
		__init_single_page(page, pfn, zone);
	}
}

void __meminit memmap_init(u64 size,
				  u64 zone, u64 start_pfn)
{
	memmap_init_zone(size, zone, start_pfn);
}

/*
 * Set up the zone data structures:
 *   - mark all pages reserved
 *   - mark all memory queues empty
 *   - clear the memory bitmaps
 *
 * NOTE: pgdat should get zeroed by caller.
 * NOTE: this function is only called during early init.
 */
static void __init free_area_init_core(struct pglist_data *pgdat)
{
	enum zone_type j;

	pgdat_init_internals(pgdat);

	for (j = 0; j < MAX_NR_ZONES; j++) {
		struct zone *zone = pgdat->node_zones + j;
		u64 size, freesize, memmap_pages;
		u64 zone_start_pfn = zone->zone_start_pfn;

		size = zone->spanned_pages;
		freesize = zone->present_pages;

		/*
		 * Adjust freesize so that it accounts for how much memory
		 * is used by this zone for memmap. This affects the watermark
		 * and per-cpu initialisations
		 */
		memmap_pages = calc_memmap_size(size, freesize);
		if (freesize >= memmap_pages) {
			freesize -= memmap_pages;
			if (memmap_pages)
				printk(KERN_DEBUG
						"  %s zone: %llu pages used for memmap\n",
						zone_names[j], memmap_pages);
		} else
			pr_warn("  %s zone: %llu pages exceeds freesize %llu\n",
				zone_names[j], memmap_pages, freesize);

		/* Account for reserved pages */
		if (j == 0 && freesize > dma_reserve) {
			freesize -= dma_reserve;
			printk(KERN_DEBUG "  %s zone: %llu pages reserved\n",
					zone_names[0], dma_reserve);
		}

		nr_kernel_pages += freesize;
		nr_all_pages += freesize;

		/*
		 * Set an approximate value for lowmem here, it will be adjusted
		 * when the bootmem allocator frees pages into the buddy system.
		 * And all highmem pages will be managed by the buddy system.
		 */
		zone_init_internals(zone, j, freesize);

		if (!size)
			continue;

		init_currently_empty_zone(zone, zone_start_pfn, size);
		memmap_init(size, j, zone_start_pfn);
	}
}

void __init free_area_init_node(u64 *zones_size,
				   u64 node_start_pfn,
				   u64 *zholes_size)
{
	pg_data_t *pgdat = NODE_DATA();
	u64 start_pfn = 0;
	u64 end_pfn = 0;

	pgdat->node_start_pfn = node_start_pfn;

	get_pfn_range(&start_pfn, &end_pfn);
	pr_info("Initmem setup [mem %#018Lx-%#018Lx]\n",
		(u64)start_pfn << PAGE_SHIFT,
		end_pfn ? ((u64)end_pfn << PAGE_SHIFT) - 1 : 0);

	calculate_node_totalpages(pgdat, start_pfn, end_pfn,
				  zones_size, zholes_size);

	free_area_init_core(pgdat);
}

/**
 * free_area_init_nodes - Initialise all pg_data_t and zone data
 * @max_zone_pfn: an array of max PFNs for each zone
 *
 * This will call free_area_init_node() for each active node in the system.
 * Using the page ranges provided by memblock_set_node(), the size of each
 * zone in each node and their holes is calculated. If the maximum PFN
 * between two adjacent zones match, it is assumed that the zone is empty.
 * For example, if arch_max_dma_pfn == arch_max_dma32_pfn, it is assumed
 * that arch_max_dma32_pfn has no pages. It is also assumed that a zone
 * starts where the previous one ended. For example, ZONE_DMA32 starts
 * at arch_max_dma_pfn.
 */
void __init free_area_init_nodes(u64 *max_zone_pfn)
{
	u64 start_pfn, end_pfn;
	u64 i;

	/* Record where the zone boundaries are */
	memset(arch_zone_lowest_possible_pfn, 0,
				sizeof(arch_zone_lowest_possible_pfn));
	memset(arch_zone_highest_possible_pfn, 0,
				sizeof(arch_zone_highest_possible_pfn));

	start_pfn = find_min_pfn_with_active_regions();

	for (i = 0; i < MAX_NR_ZONES; i++) {
		if (i == ZONE_MOVABLE)
			continue;

		end_pfn = max(max_zone_pfn[i], start_pfn);
		arch_zone_lowest_possible_pfn[i] = start_pfn;
		arch_zone_highest_possible_pfn[i] = end_pfn;

		start_pfn = end_pfn;
	}

	find_zone_movable_pfns_for_nodes();

	/* Print out the zone ranges */
	pr_info("Zone ranges:\n");
	for (i = 0; i < MAX_NR_ZONES; i++) {
		if (i == ZONE_MOVABLE)
			continue;
		
		pr_info("  %-8s ", zone_names[i]);
		if (arch_zone_lowest_possible_pfn[i] ==
				arch_zone_highest_possible_pfn[i])
			pr_cont("empty\n");
		else
			pr_cont("[mem %#018Lx-%#018Lx]\n",
				(u64)arch_zone_lowest_possible_pfn[i]
					<< PAGE_SHIFT,
				((u64)arch_zone_highest_possible_pfn[i]
					<< PAGE_SHIFT) - 1);
	}

	/* Print out the PFNs ZONE_MOVABLE begins at in each node */
	pr_info("Movable zone start for each node\n");
	pr_cont("empty\n");

	/* Print out the early node map */
	pr_info("Early memory node ranges\n");
	for_each_mem_pfn_range(i, &start_pfn, &end_pfn)
		pr_info("  node: [mem %#018Lx-%#018Lx]\n",
			(u64)start_pfn << PAGE_SHIFT,
			((u64)end_pfn << PAGE_SHIFT) - 1);

	zero_resv_unavail();

	free_area_init_node(NULL, find_min_pfn_for_node(), NULL);
}

void zone_pcp_reset(struct zone *zone)
{
	u64 flags;

	/* avoid races with drain_pages()  */
	local_irq_save(flags);
	if (zone->pageset != &boot_pageset) {
		free_percpu(zone->pageset);
		zone->pageset = &boot_pageset;
	}
	local_irq_restore(flags);
}

bool is_free_buddy_page(struct page *page)
{
	struct zone *zone = page_zone(page);
	u64 pfn = page_to_pfn(page);
	u64 flags;
	unsigned int order;

	spin_lock_irqsave(&zone->lock, flags);
	for (order = 0; order < MAX_ORDER; order++) {
		struct page *page_head = page - (pfn & ((1 << order) - 1));

		if (PageBuddy(page_head) && page_order(page_head) >= order)
			break;
	}
	spin_unlock_irqrestore(&zone->lock, flags);

	return order < MAX_ORDER;
}
