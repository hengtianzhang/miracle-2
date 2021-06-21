#ifndef __LINUX_MEMBLOCK_H_
#define __LINUX_MEMBLOCK_H_

#ifdef __KERNEL__

/*
 * Logical memory blocks.
 *
 * Copyright (C) 2001 Peter Bergner, IBM Corp.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/types.h>
#include <linux/pfn.h>

/**
 * enum memblock_flags - definition of memory region attributes
 * @MEMBLOCK_NONE: no special request
 * @MEMBLOCK_NOMAP: don't add to kernel direct mapping
 */
enum memblock_flags {
	MEMBLOCK_NONE		= 0x0,	/* No special request */
	MEMBLOCK_NOMAP		= 0x1,	/* don't add to kernel direct mapping */
	MEMBLOCK_DMA		= 0x2,
	MEMBLOCK_MOVABLE	= 0x3,
};

/**
 * struct memblock_region - represents a memory region
 * @base: physical address of the region
 * @size: size of the region
 * @flags: memory region attributes
 * @nid: NUMA node id
 */
struct memblock_region {
	phys_addr_t base;
	phys_addr_t size;
	enum memblock_flags flags;
};

/**
 * struct memblock_type - collection of memory regions of certain type
 * @cnt: number of regions
 * @max: size of the allocated array
 * @total_size: size of all regions
 * @regions: array of regions
 * @name: the memory type symbolic name
 */
struct memblock_type {
	u64 cnt;
	u64 max;
	phys_addr_t total_size;
	struct memblock_region *regions;
	char *name;
};

/**
 * struct memblock - memblock allocator metadata
 * @bottom_up: is bottom up direction?
 * @current_limit: physical address of the current allocation limit
 * @memory: usabe memory regions
 * @reserved: reserved memory regions
 * @physmem: all physical memory
 */
struct memblock {
	bool bottom_up;  /* is bottom up direction? */
	phys_addr_t current_limit;
	struct memblock_type memory;
	struct memblock_type reserved;
};

extern struct memblock memblock;
extern int memblock_debug;

#define __init_memblock
#define __initdata_memblock

#define memblock_dbg(fmt, ...) \
	if (memblock_debug) printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)

/* Flags for memblock allocation APIs */
#define MEMBLOCK_ALLOC_ANYWHERE	(~(phys_addr_t)0)
#define MEMBLOCK_ALLOC_ACCESSIBLE	0

static inline bool memblock_is_nomap(struct memblock_region *m)
{
	return m->flags & MEMBLOCK_NOMAP;
}

static inline bool memblock_is_dma(struct memblock_region *m)
{
	return m->flags & MEMBLOCK_DMA;
}

static inline bool memblock_is_movable(struct memblock_region *m)
{
	return m->flags & MEMBLOCK_MOVABLE;
}

static inline void memblock_set_region_flags(struct memblock_region *r,
					     enum memblock_flags flags)
{
	r->flags |= flags;
}

static inline void memblock_clear_region_flags(struct memblock_region *r,
					       enum memblock_flags flags)
{
	r->flags &= ~flags;
}

/*
 * Set the allocation direction to bottom-up or top-down.
 */
static inline void __init memblock_set_bottom_up(bool enable)
{
	memblock.bottom_up = enable;
}

/*
 * Check if the allocation direction is bottom-up or not.
 * if this is true, that said, memblock will allocate memory
 * in bottom-up direction.
 */
static inline bool memblock_bottom_up(void)
{
	return memblock.bottom_up;
}

/**
 * memblock_region_memory_base_pfn - get the lowest pfn of the memory region
 * @reg: memblock_region structure
 *
 * Return: the lowest pfn intersecting with the memory region
 */
static inline u64 memblock_region_memory_base_pfn(const struct memblock_region *reg)
{
	return PFN_UP(reg->base);
}

/**
 * memblock_region_memory_end_pfn - get the end pfn of the memory region
 * @reg: memblock_region structure
 *
 * Return: the end_pfn of the reserved region
 */
static inline u64 memblock_region_memory_end_pfn(const struct memblock_region *reg)
{
	return PFN_DOWN(reg->base + reg->size);
}

/**
 * memblock_region_reserved_base_pfn - get the lowest pfn of the reserved region
 * @reg: memblock_region structure
 *
 * Return: the lowest pfn intersecting with the reserved region
 */
static inline u64 memblock_region_reserved_base_pfn(const struct memblock_region *reg)
{
	return PFN_DOWN(reg->base);
}

/**
 * memblock_region_reserved_end_pfn - get the end pfn of the reserved region
 * @reg: memblock_region structure
 *
 * Return: the end_pfn of the reserved region
 */
static inline u64 memblock_region_reserved_end_pfn(const struct memblock_region *reg)
{
	return PFN_UP(reg->base + reg->size);
}

void __next_mem_range(u64 *idx, enum memblock_flags flags,
		      struct memblock_type *type_a,
		      struct memblock_type *type_b, phys_addr_t *out_start,
		      phys_addr_t *out_end);

/**
 * for_each_mem_range - iterate through memblock areas from type_a and not
 * included in type_b. Or just type_a if type_b is NULL.
 * @i: u64 used as loop variable
 * @type_a: ptr to memblock_type to iterate
 * @type_b: ptr to memblock_type which excludes from the iteration
 * @flags: pick from blocks based on memory attributes
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 */
#define for_each_mem_range(i, type_a, type_b, flags,		\
			   p_start, p_end)			\
	for (i = 0, __next_mem_range(&i, flags, type_a, type_b,	\
				     p_start, p_end);		\
	     i != (u64)ULLONG_MAX;					\
	     __next_mem_range(&i, flags, type_a, type_b,		\
			      p_start, p_end))

/**
 * for_each_free_mem_range - iterate through free memblock areas
 * @i: u64 used as loop variable
 * @flags: pick from blocks based on memory attributes
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 *
 * Walks over free (memory && !reserved) areas of memblock.  Available as
 * soon as memblock is initialized.
 */
#define for_each_free_mem_range(i, flags, p_start, p_end)	\
	for_each_mem_range(i, &memblock.memory, &memblock.reserved,	\
				flags, p_start, p_end)

void __next_mem_range_rev(u64 *idx, enum memblock_flags flags,
			  struct memblock_type *type_a,
			  struct memblock_type *type_b, phys_addr_t *out_start,
			  phys_addr_t *out_end);

/**
 * for_each_mem_range_rev - reverse iterate through memblock areas from
 * type_a and not included in type_b. Or just type_a if type_b is NULL.
 * @i: u64 used as loop variable
 * @type_a: ptr to memblock_type to iterate
 * @type_b: ptr to memblock_type which excludes from the iteration
 * @flags: pick from blocks based on memory attributes
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 */
#define for_each_mem_range_rev(i, type_a, type_b, flags,		\
			       p_start, p_end)			\
	for (i = (u64)ULLONG_MAX,					\
		     __next_mem_range_rev(&i, flags, type_a, type_b,\
					  p_start, p_end);	\
	     i != (u64)ULLONG_MAX;					\
	     __next_mem_range_rev(&i, flags, type_a, type_b,	\
				  p_start, p_end))

/**
 * for_each_free_mem_range_reverse - rev-iterate through free memblock areas
 * @i: u64 used as loop variable
 * @flags: pick from blocks based on memory attributes
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 *
 * Walks over free (memory && !reserved) areas of memblock in reverse
 * order.  Available as soon as memblock is initialized.
 */
#define for_each_free_mem_range_reverse(i, flags, p_start, p_end)				\
	for_each_mem_range_rev(i, &memblock.memory, &memblock.reserved,	\
			    flags, p_start, p_end)

#define for_each_memblock(memblock_type, region)					\
	for (region = memblock.memblock_type.regions;					\
	     region < (memblock.memblock_type.regions + memblock.memblock_type.cnt);	\
	     region++)

#define for_each_memblock_type(i, memblock_type, rgn)			\
	for (i = 0, rgn = &memblock_type->regions[0];			\
	     i < memblock_type->cnt;					\
	     i++, rgn = &memblock_type->regions[i])

void __next_reserved_mem_region(u64 *idx,
					   phys_addr_t *out_start,
					   phys_addr_t *out_end);

/**
 * for_each_reserved_mem_region - iterate over all reserved memblock areas
 * @i: u64 used as loop variable
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 *
 * Walks over reserved areas of memblock. Available as soon as memblock
 * is initialized.
 */
#define for_each_reserved_mem_region(i, p_start, p_end)			\
	for (i = 0UL, __next_reserved_mem_region(&i, p_start, p_end);	\
	     i != (u64)ULLONG_MAX;					\
	     __next_reserved_mem_region(&i, p_start, p_end))

void __next_mem_pfn_range(u64 *idx, phys_addr_t *out_start_pfn,
			  phys_addr_t *out_end_pfn);

#define for_each_mem_pfn_range(i, p_start, p_end)		\
	for (i = -1, __next_mem_pfn_range(&i, p_start, p_end); \
	     i >= 0; __next_mem_pfn_range(&i, p_start, p_end))

enum memblock_flags choose_memblock_flags(void);
bool memblock_overlaps_region(struct memblock_type *type,
			      phys_addr_t base, phys_addr_t size);
phys_addr_t __memblock_find_in_range(phys_addr_t size,
					phys_addr_t align, phys_addr_t start,
					phys_addr_t end,
					enum memblock_flags flags);
phys_addr_t memblock_find_in_range(phys_addr_t start,
					phys_addr_t end, phys_addr_t size,
					phys_addr_t align);
int memblock_add_range(struct memblock_type *type,
				phys_addr_t base, phys_addr_t size,
				enum memblock_flags flags);
int memblock_add(phys_addr_t base, phys_addr_t size);
int memblock_remove(phys_addr_t base, phys_addr_t size);
int memblock_free(phys_addr_t base, phys_addr_t size);
int memblock_reserve(phys_addr_t base, phys_addr_t size);

int memblock_mark_nomap(phys_addr_t base, phys_addr_t size);
int memblock_clear_nomap(phys_addr_t base, phys_addr_t size);

int memblock_mark_dma(phys_addr_t base, phys_addr_t size);
int memblock_clear_dma(phys_addr_t base, phys_addr_t size);

int memblock_mark_movable(phys_addr_t base, phys_addr_t size);
int memblock_clear_movable(phys_addr_t base, phys_addr_t size);

phys_addr_t memblock_alloc_range(phys_addr_t size, phys_addr_t align,
					phys_addr_t start, phys_addr_t end,
					enum memblock_flags flags);
phys_addr_t memblock_alloc_base(phys_addr_t size,
					phys_addr_t align, phys_addr_t max_addr,
					enum memblock_flags flags);
phys_addr_t memblock_phys_alloc(phys_addr_t size, phys_addr_t align);
void *memblock_alloc_try_raw(
			phys_addr_t size, phys_addr_t align,
			phys_addr_t min_addr, phys_addr_t max_addr);
void *memblock_alloc_try_nopanic(
				phys_addr_t size, phys_addr_t align,
				phys_addr_t min_addr, phys_addr_t max_addr);
void *memblock_alloc_try(
			phys_addr_t size, phys_addr_t align,
			phys_addr_t min_addr, phys_addr_t max_addr);

phys_addr_t memblock_phys_mem_size(void);
phys_addr_t memblock_reserved_size(void);
phys_addr_t memblock_mem_size(u64 limit_pfn);
phys_addr_t memblock_start_of_DRAM(void);
phys_addr_t memblock_end_of_DRAM(void);
void memblock_enforce_memory_limit(phys_addr_t limit);
void memblock_cap_memory_range(phys_addr_t base, phys_addr_t size);
void memblock_mem_limit_remove_map(phys_addr_t limit);
bool memblock_is_reserved(phys_addr_t addr);
bool memblock_is_memory(phys_addr_t addr);
bool memblock_is_map_memory(phys_addr_t addr);
bool memblock_is_region_memory(phys_addr_t base, phys_addr_t size);
bool memblock_is_region_reserved(phys_addr_t base, phys_addr_t size);
void memblock_trim_memory(phys_addr_t align);
void memblock_set_current_limit(phys_addr_t limit);
phys_addr_t memblock_get_current_limit(void);
void __memblock_dump_all(void);
void memblock_allow_resize(void);

/* We are using top down, so it is safe to use 0 here */
#define MEMBLOCK_LOW_LIMIT 0

#ifndef ARCH_LOW_ADDRESS_LIMIT
#define ARCH_LOW_ADDRESS_LIMIT  0xffffffffUL
#endif

static inline void * __init memblock_alloc(phys_addr_t size,  phys_addr_t align)
{
	return memblock_alloc_try(size, align, MEMBLOCK_LOW_LIMIT,
				      MEMBLOCK_ALLOC_ACCESSIBLE);
}

static inline void * __init memblock_alloc_raw(phys_addr_t size,
					       phys_addr_t align)
{
	return memblock_alloc_try_raw(size, align, MEMBLOCK_LOW_LIMIT,
					  MEMBLOCK_ALLOC_ACCESSIBLE);
}

static inline void * __init memblock_alloc_from(phys_addr_t size,
						phys_addr_t align,
						phys_addr_t min_addr)
{
	return memblock_alloc_try(size, align, min_addr,
				      MEMBLOCK_ALLOC_ACCESSIBLE);
}

static inline void * __init memblock_alloc_nopanic(phys_addr_t size,
						   phys_addr_t align)
{
	return memblock_alloc_try_nopanic(size, align, MEMBLOCK_LOW_LIMIT,
					      MEMBLOCK_ALLOC_ACCESSIBLE);
}

static inline void * __init memblock_alloc_low(phys_addr_t size,
					       phys_addr_t align)
{
	return memblock_alloc_try(size, align, MEMBLOCK_LOW_LIMIT,
				      ARCH_LOW_ADDRESS_LIMIT);
}

static inline void * __init memblock_alloc_low_nopanic(phys_addr_t size,
						       phys_addr_t align)
{
	return memblock_alloc_try_nopanic(size, align, MEMBLOCK_LOW_LIMIT,
					      ARCH_LOW_ADDRESS_LIMIT);
}

static inline void * __init memblock_alloc_from_nopanic(phys_addr_t size,
							phys_addr_t align,
							phys_addr_t min_addr)
{
	return memblock_alloc_try_nopanic(size, align, min_addr,
					      MEMBLOCK_ALLOC_ACCESSIBLE);
}

static inline void memblock_dump_all(void)
{
	if (memblock_debug)
		__memblock_dump_all();
}

#ifdef CONFIG_MEMTEST
extern void early_memtest(phys_addr_t start, phys_addr_t end);
#else
static inline void early_memtest(phys_addr_t start, phys_addr_t end)
{
}
#endif

#endif /* __KERNEL__ */
#endif /* !__LINUX_MEMBLOCK_H_ */
