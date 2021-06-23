/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_PERCPU_H_
#define __LINUX_PERCPU_H_

#include <linux/threads.h>
#include <linux/cache.h>
#include <linux/types.h>

#include <asm/percpu.h>
#include <asm/page.h>

#define PERCPU_MODULE_RESERVE		0

/* minimum unit size, also is the maximum supported allocation size */
#define PCPU_MIN_UNIT_SIZE		PFN_ALIGN(32 << 10)

/* minimum allocation size and shift in bytes */
#define PCPU_MIN_ALLOC_SHIFT		2
#define PCPU_MIN_ALLOC_SIZE		(1 << PCPU_MIN_ALLOC_SHIFT)

/* number of bits per page, used to trigger a scan if blocks are > PAGE_SIZE */
#define PCPU_BITS_PER_PAGE		(PAGE_SIZE >> PCPU_MIN_ALLOC_SHIFT)

/*
 * This determines the size of each metadata block.  There are several subtle
 * constraints around this constant.  The reserved region must be a multiple of
 * PCPU_BITMAP_BLOCK_SIZE.  Additionally, PCPU_BITMAP_BLOCK_SIZE must be a
 * multiple of PAGE_SIZE or PAGE_SIZE must be a multiple of
 * PCPU_BITMAP_BLOCK_SIZE to align with the populated page map. The unit_size
 * also has to be a multiple of PCPU_BITMAP_BLOCK_SIZE to ensure full blocks.
 */
#define PCPU_BITMAP_BLOCK_SIZE		PAGE_SIZE
#define PCPU_BITMAP_BLOCK_BITS		(PCPU_BITMAP_BLOCK_SIZE >>	\
					 PCPU_MIN_ALLOC_SHIFT)

/*
 * Percpu allocator can serve percpu allocations before slab is
 * initialized which allows slab to depend on the percpu allocator.
 * The following two parameters decide how much resource to
 * preallocate for this.  Keep PERCPU_DYNAMIC_RESERVE equal to or
 * larger than PERCPU_DYNAMIC_EARLY_SIZE.
 */
#define PERCPU_DYNAMIC_EARLY_SLOTS	128
#define PERCPU_DYNAMIC_EARLY_SIZE	(12 << 10)

#if BITS_PER_LONG > 32
#define PERCPU_DYNAMIC_RESERVE		(28 << 10)
#else
#define PERCPU_DYNAMIC_RESERVE		(20 << 10)
#endif

#ifndef LOCAL_DISTANCE
#define LOCAL_DISTANCE 10
#endif

#ifndef REMOTE_DISTANCE
#define REMOTE_DISTANCE 20
#endif

extern void *pcpu_base_addr;
extern const u64 *pcpu_unit_offsets;

struct pcpu_group_info {
	int			nr_units;	/* aligned # of units */
	u64		base_offset;	/* base address offset */
	unsigned int		*cpu_map;	/* unit->cpu map, empty
						 * entries contain NR_CPUS */
};

struct pcpu_alloc_info {
	size_t			static_size;
	size_t			reserved_size;
	size_t			dyn_size;
	size_t			unit_size;
	size_t			atom_size;
	size_t			alloc_size;
	size_t			__ai_size;	/* internal, don't use */
	int			nr_groups;	/* 0 if grouping unnecessary */
	struct pcpu_group_info	groups[];
};

enum pcpu_fc {
	PCPU_FC_AUTO,
	PCPU_FC_EMBED,

	PCPU_FC_NR,
};

typedef void * (*pcpu_fc_alloc_fn_t)(unsigned int cpu, size_t size,
				     size_t align);
typedef void (*pcpu_fc_free_fn_t)(void *ptr, size_t size);
typedef int (pcpu_fc_cpu_distance_fn_t)(unsigned int from, unsigned int to);

extern const char * const pcpu_fc_names[PCPU_FC_NR];

extern enum pcpu_fc pcpu_chosen_fc;

extern struct pcpu_alloc_info * __init pcpu_alloc_alloc_info(int nr_groups,
							     int nr_units);
extern void __init pcpu_free_alloc_info(struct pcpu_alloc_info *ai);

extern int __init pcpu_setup_first_chunk(const struct pcpu_alloc_info *ai,
					 void *base_addr);

extern int __init pcpu_embed_first_chunk(size_t reserved_size, size_t dyn_size,
				size_t atom_size,
				pcpu_fc_cpu_distance_fn_t cpu_distance_fn,
				pcpu_fc_alloc_fn_t alloc_fn,
				pcpu_fc_free_fn_t free_fn);

extern void __percpu *__alloc_reserved_percpu(size_t size, size_t align);
extern bool __is_kernel_percpu_address(u64 addr, u64 *can_addr);
extern bool is_kernel_percpu_address(u64 addr);

extern void __init setup_per_cpu_areas(void);

extern void __percpu *__alloc_percpu_gfp(size_t size, size_t align, gfp_t gfp);
extern void __percpu *__alloc_percpu(size_t size, size_t align);
extern void free_percpu(void __percpu *__pdata);
extern phys_addr_t per_cpu_ptr_to_phys(void *addr);

#define alloc_percpu_gfp(type, gfp)					\
	(typeof(type) __percpu *)__alloc_percpu_gfp(sizeof(type),	\
						__alignof__(type), gfp)
#define alloc_percpu(type)						\
	(typeof(type) __percpu *)__alloc_percpu(sizeof(type),		\
						__alignof__(type))

extern u64 pcpu_nr_pages(void);

#endif /* !__LINUX_PERCPU_H_ */
