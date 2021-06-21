/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_SLUB_DEF_H_
#define __LINUX_SLUB_DEF_H_

#include <linux/types.h>
#include <linux/mm_types.h>
#include <linux/percpu.h>
#include <linux/list.h>

/*
 * SLUB : A Slab allocator without object queues.
 *
 * (C) 2007 SGI, Christoph Lameter
 */
enum stat_item {
	DEACTIVATE_TO_HEAD,	/* Cpu slab was moved to the head of partials */
	DEACTIVATE_TO_TAIL,	/* Cpu slab was moved to the tail of partials */
	NR_SLUB_STAT_ITEMS
};

struct kmem_cache_cpu {
	void **freelist;
	u64 tid;
	struct page *page;
};

struct kmem_cache_order_objects {
	unsigned int x;
};

struct kmem_cache_node;
struct kmem_cache {
	struct kmem_cache_cpu __percpu *cpu_slab;
	/* Used for retriving partial slabs etc */
	slab_flags_t flags;
	u64 min_partial;
	unsigned int size;	/* The size of an object including meta data */
	unsigned int object_size;/* The size of an object without meta data */
	unsigned int offset;	/* Free pointer offset. */

	struct kmem_cache_order_objects oo;

	/* Allocation and freeing of slabs */
	struct kmem_cache_order_objects max;
	struct kmem_cache_order_objects min;
	gfp_t allocflags;	/* gfp flags to use on each alloc */
	int refcount;		/* Refcount for slab cache destroy */
	void (*ctor)(void *);
	unsigned int inuse;		/* Offset to metadata */
	unsigned int align;		/* Alignment */
	unsigned int red_left_pad;	/* Left redzone padding size */
	const char *name;	/* Name (only for display!) */
	struct list_head list;	/* List of slab caches */

	unsigned int useroffset;	/* Usercopy region offset */
	unsigned int usersize;		/* Usercopy region size */

	struct kmem_cache_node *node;
};

#define slub_cpu_partial(s)		(0)

void *fixup_red_left(struct kmem_cache *s, void *p);

#endif /* !__LINUX_SLUB_DEF_H_ */
