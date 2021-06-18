/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_MMZONE_H_
#define __LINUX_MMZONE_H_

#ifndef __ASSEMBLY__
#ifndef __GENERATING_BOUNDS_H

#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/atomic.h>
#include <linux/cache.h>
#include <linux/percpu.h>

#include <generated/bounds.h>

/* Free memory management - zoned buddy allocator.  */
#ifndef CONFIG_FORCE_MAX_ZONEORDER
#define MAX_ORDER 11
#else
#define MAX_ORDER CONFIG_FORCE_MAX_ZONEORDER
#endif
#define MAX_ORDER_NR_PAGES (1 << (MAX_ORDER - 1))

#define pageblock_order (MAX_ORDER - 1)
#define pageblock_nr_pages (1ULL << pageblock_order)

struct free_area {
	struct list_head	free_list;
	u64		nr_free;
};

struct zone_padding {
	char x[0];
} ____cacheline_internodealigned_in_smp;
#define ZONE_PADDING(name)	struct zone_padding name;

struct per_cpu_pages {
	int count;		/* number of pages in the list */
	int high;		/* high watermark, emptying needed */
	int batch;		/* chunk size for buddy add/remove */

	/* Lists of pages, one per migrate type stored on the pcp-lists */
	struct list_head lists;
};

struct per_cpu_pageset {
	struct per_cpu_pages pcp;
};

#endif /* !__GENERATING_BOUNDS_H */

enum zone_type {
	ZONE_DMA,
	ZONE_NORMAL,
	ZONE_MOVABLE,
	__MAX_NR_ZONES
};

#ifndef __GENERATING_BOUNDS_H

struct pglist_data;

struct zone {
	struct pglist_data	*zone_pgdat;
	struct per_cpu_pageset __percpu *pageset;

	/* zone_start_pfn == zone_start_paddr >> PAGE_SHIFT */
	u64		zone_start_pfn;

	atomic_long_t		managed_pages;
	u64		spanned_pages;
	u64		present_pages;

	const char		*name;

	int initialized;

	/* Write-intensive fields used from the page allocator */
	ZONE_PADDING(_pad1_)

	/* free areas of different sizes */
	struct free_area	free_area[MAX_ORDER];

	/* zone flags, see below */
	u64		flags;

	/* Primarily protects free_area */
	spinlock_t		lock;
} ____cacheline_internodealigned_in_smp;

static inline u64 zone_managed_pages(struct zone *zone)
{
	return (u64)atomic_long_read(&zone->managed_pages);
}

static inline u64 zone_end_pfn(const struct zone *zone)
{
	return zone->zone_start_pfn + zone->spanned_pages;
}

static inline bool zone_spans_pfn(const struct zone *zone, u64 pfn)
{
	return zone->zone_start_pfn <= pfn && pfn < zone_end_pfn(zone);
}

static inline bool zone_is_initialized(struct zone *zone)
{
	return zone->initialized;
}

static inline bool zone_is_empty(struct zone *zone)
{
	return zone->spanned_pages == 0;
}

/*
 * Return true if [start_pfn, start_pfn + nr_pages) range has a non-empty
 * intersection with the given zone
 */
static inline bool zone_intersects(struct zone *zone,
		u64 start_pfn, u64 nr_pages)
{
	if (zone_is_empty(zone))
		return false;
	if (start_pfn >= zone_end_pfn(zone) ||
	    start_pfn + nr_pages <= zone->zone_start_pfn)
		return false;

	return true;
}

/* Returns true if a zone has memory */
static inline bool populated_zone(struct zone *zone)
{
	return zone->present_pages;
}

/*
 * zone_idx() returns 0 for the ZONE_DMA zone, 1 for the ZONE_NORMAL zone, etc.
 */
#define zone_idx(zone)		((zone) - (zone)->zone_pgdat->node_zones)

typedef struct pglist_data {
	struct zone node_zones[MAX_NR_ZONES];
	int nr_zones;

	u64 node_start_pfn;
	u64 node_present_pages; /* total number of physical pages */
	u64 node_spanned_pages; /* total size of physical page
					     range, including holes */

	/* Write-intensive fields used by page reclaim */
	ZONE_PADDING(_pad1_)
	u64		flags;
} pg_data_t;

extern struct pglist_data node_data;
#define NODE_DATA()		(&node_data)

#define for_each_order(order) \
	for (order = 0; order < MAX_ORDER; order++)

extern u64 max_pfn;
extern u64 highest_memmap_pfn;

u64 find_min_pfn_with_active_regions(void);
void zero_resv_unavail(void);
void get_pfn_range(u64 *start_pfn, u64 *end_pfn);
u64 __absent_pages_in_range(u64 range_start_pfn, u64 range_end_pfn);
void init_currently_empty_zone(struct zone *zone, u64 zone_start_pfn,
					u64 size);
void memmap_init_zone(u64 size, u64 zone, u64 start_pfn);
void memmap_init(u64 size, u64 zone, u64 start_pfn);
void free_area_init_node(u64 *zones_size, u64 node_start_pfn,
								u64 *zholes_size);

extern struct pglist_data *first_online_pgdat(void);
extern struct pglist_data *next_online_pgdat(struct pglist_data *pgdat);
extern struct zone *next_zone(struct zone *zone);

/**
 * for_each_online_pgdat - helper macro to iterate over all online nodes
 * @pgdat - pointer to a pg_data_t variable
 */
#define for_each_online_pgdat(pgdat)			\
	for (pgdat = first_online_pgdat();		\
	     pgdat;					\
	     pgdat = next_online_pgdat(pgdat))

/**
 * for_each_zone - helper macro to iterate over all memory zones
 * @zone - pointer to struct zone variable
 *
 * The user only needs to declare the zone variable, for_each_zone
 * fills it in.
 */
#define for_each_zone(zone)			        \
	for (zone = (first_online_pgdat())->node_zones; \
	     zone;					\
	     zone = next_zone(zone))

#define for_each_populated_zone(zone)		        \
	for (zone = (first_online_pgdat())->node_zones; \
	     zone;					\
	     zone = next_zone(zone))			\
		if (!populated_zone(zone))		\
			; /* do nothing */		\
		else

void free_area_init_nodes(u64 *max_zone_pfn);
void build_all_zone(pg_data_t *pgdat);
void setup_per_cpu_pageset(void);

#endif /* !__GENERATING_BOUNDS_H */
#endif /* !__ASSEMBLY__ */
#endif /* !__LINUX_MMZONE_H_ */
