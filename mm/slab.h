/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __MM_SLAB_H_
#define __MM_SLAB_H_
/*
 * Internal slab definitions
 */
#include <linux/slub_def.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/page.h>

/*
 * State of the slab allocator.
 */
enum slab_state {
	DOWN,			/* No slab functionality yet */
	PARTIAL,		/* SLUB: kmem_cache_node available */
	PARTIAL_NODE,		/* SLAB: kmalloc size for node struct available */
	UP,			/* Slab caches usable but not all extras yet */
	FULL			/* Everything is working */
};

extern enum slab_state slab_state;

/* The slab cache mutex protects the management structures during changes */
extern struct mutex slab_mutex;

extern struct list_head slab_caches;

extern struct kmem_cache *kmem_cache;

extern const struct kmalloc_info_struct {
	const char *name;
	unsigned int size;
} kmalloc_info[];

#define slab_root_caches	slab_caches
#define root_caches_node	list

struct kmem_cache_node {
	spinlock_t list_lock;

	u64 nr_partial;
	struct list_head partial;
};

/* Legal flag mask for kmem_cache_create(), for various configurations */
#define SLAB_CORE_FLAGS (SLAB_HWCACHE_ALIGN | SLAB_CACHE_DMA | SLAB_PANIC)

/* Common flags available with current configuration */
#define CACHE_CREATE_MASK (SLAB_CORE_FLAGS)

#define SLAB_FLAGS_PERMITTED (SLAB_CORE_FLAGS | \
			      SLAB_POISON | \
			      SLAB_STORE_USER | \
			      SLAB_CONSISTENCY_CHECKS)

static inline struct kmem_cache *slab_pre_alloc_hook(struct kmem_cache *s,
						     gfp_t flags)
{
	return s;
}

static inline struct kmem_cache *cache_from_obj(struct kmem_cache *s, void *x)
{
	struct kmem_cache *cachep;
	struct page *page;

	/*
	 * When kmemcg is not being used, both assignments should return the
	 * same value. but we don't want to pay the assignment price in that
	 * case. If it is not compiled in, the compiler should be smart enough
	 * to not do even the assignment. In that case, slab_equal_or_root
	 * will also be a constant.
	 */
	if (!unlikely(s->flags & SLAB_CONSISTENCY_CHECKS))
		return s;

	page = virt_to_head_page(x);
	cachep = page->slab_cache;

	return cachep;
}

static inline size_t slab_ksize(const struct kmem_cache *s)
{
	/*
	 * If we have the need to store the freelist pointer
	 * back there or track user information then we can
	 * only use the space before that information.
	 */
	if (s->flags & (SLAB_STORE_USER))
		return s->inuse;
	/*
	 * Else we can use all the padding etc for the allocation
	 */
	return s->size;
}

int slab_unmergeable(struct kmem_cache *s);

struct kmem_cache *find_mergeable(unsigned size, unsigned align,
		slab_flags_t flags, const char *name, void (*ctor)(void *));

void setup_kmalloc_cache_index_table(void);
void create_kmalloc_caches(slab_flags_t);

bool __kmem_cache_empty(struct kmem_cache *);

struct kmem_cache *
__kmem_cache_alias(const char *name, unsigned int size, unsigned int align,
		   slab_flags_t flags, void (*ctor)(void *));

/* Find the kmalloc slab corresponding for a certain size */
struct kmem_cache *kmalloc_slab(size_t, gfp_t);

struct kmem_cache *create_kmalloc_cache(const char *name, unsigned int size,
			slab_flags_t flags, unsigned int useroffset,
			unsigned int usersize);
extern void create_boot_cache(struct kmem_cache *, const char *name,
			unsigned int size, slab_flags_t flags,
			unsigned int useroffset, unsigned int usersize);

int __kmem_cache_create(struct kmem_cache *, slab_flags_t flags);

void __kmem_cache_release(struct kmem_cache *);

#endif /* !__MM_SLAB_H_ */
