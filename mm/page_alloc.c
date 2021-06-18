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
