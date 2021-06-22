// SPDX-License-Identifier: GPL-2.0
/*
 * SLUB: A slab allocator that limits cache line use instead of queuing
 * objects in per cpu and per node lists.
 *
 * The allocator synchronizes using per slab locks or atomic operatios
 * and only uses a centralized lock to manage a pool of partial slabs.
 *
 * (C) 2007 SGI, Christoph Lameter
 * (C) 2011 Linux Foundation, Christoph Lameter
 */

#include <linux/mm.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/cpumask.h>
#include <linux/bit_spinlock.h>
#include <linux/page.h>
#include <linux/smp.h>

#include "slab.h"

static unsigned int slub_min_order;
static unsigned int slub_max_order = PAGE_ALLOC_COSTLY_ORDER;
static unsigned int slub_min_objects;

static int __init setup_slub_min_order(char *str)
{
	//get_option(&str, (int *)&slub_min_order);

	return 1;
}
early_param("slub_min_order", setup_slub_min_order);

static int __init setup_slub_max_order(char *str)
{
	//get_option(&str, (int *)&slub_max_order);
	//slub_max_order = min(slub_max_order, (unsigned int)MAX_ORDER - 1);

	return 1;
}
early_param("slub_max_order", setup_slub_max_order);

static int __init setup_slub_min_objects(char *str)
{
	return 1;
}
early_param("slub_min_objects", setup_slub_min_objects);

#define MIN_PARTIAL 5

#define MAX_PARTIAL 10

#define SLAB_NO_CMPXCHG (SLAB_CONSISTENCY_CHECKS | SLAB_STORE_USER)

#define OO_SHIFT	16
#define OO_MASK		((1 << OO_SHIFT) - 1)
#define MAX_OBJS_PER_PAGE	32767 /* since page.objects is u15 */

/* Internal SLUB flags */
/* Poison object */
#define __OBJECT_POISON		((slab_flags_t __force)0x80000000U)
/* Use cmpxchg_double */
#define __CMPXCHG_DOUBLE	((slab_flags_t __force)0x40000000U)

static inline unsigned int order_objects(unsigned int order, unsigned int size)
{
	return ((unsigned int)PAGE_SIZE << order) / size;
}

static inline struct kmem_cache_order_objects oo_make(unsigned int order,
		unsigned int size)
{
	struct kmem_cache_order_objects x = {
		(order << OO_SHIFT) + order_objects(order, size)
	};

	return x;
}

static inline unsigned int oo_order(struct kmem_cache_order_objects x)
{
	return x.x >> OO_SHIFT;
}

static inline unsigned int oo_objects(struct kmem_cache_order_objects x)
{
	return x.x & OO_MASK;
}

static __always_inline void slab_lock(struct page *page)
{
	BUG_ON(PageTail(page));
	bit_spin_lock(PG_locked, &page->flags);
}

static __always_inline void slab_unlock(struct page *page)
{
	BUG_ON(PageTail(page));
	__bit_spin_unlock(PG_locked, &page->flags);
}

static inline unsigned int slab_order(unsigned int size,
		unsigned int min_objects, unsigned int max_order,
		unsigned int fract_leftover)
{
	unsigned int min_order = slub_min_order;
	unsigned int order;

	if (order_objects(min_order, size) > MAX_OBJS_PER_PAGE)
		return get_order(size * MAX_OBJS_PER_PAGE) - 1;

	for (order = max(min_order, (unsigned int)get_order(min_objects * size));
			order <= max_order; order++) {

		unsigned int slab_size = (unsigned int)PAGE_SIZE << order;
		unsigned int rem;

		rem = slab_size % size;

		if (rem <= slab_size / fract_leftover)
			break;
	}

	return order;
}

static inline int calculate_order(unsigned int size)
{
	unsigned int order;
	unsigned int min_objects;
	unsigned int max_objects;

	/*
	 * Attempt to find best configuration for a slab. This
	 * works by first attempting to generate a layout with
	 * the best configuration and backing off gradually.
	 *
	 * First we increase the acceptable waste in a slab. Then
	 * we reduce the minimum objects required in a slab.
	 */
	min_objects = slub_min_objects;
	if (!min_objects)
		min_objects = 4 * (fls(nr_possible_cpu_ids) + 1);
	max_objects = order_objects(slub_max_order, size);
	min_objects = min(min_objects, max_objects);

	while (min_objects > 1) {
		unsigned int fraction;

		fraction = 16;
		while (fraction >= 4) {
			order = slab_order(size, min_objects,
					slub_max_order, fraction);
			if (order <= slub_max_order)
				return order;
			fraction /= 2;
		}
		min_objects--;
	}

	/*
	 * We were unable to place multiple objects in a slab. Now
	 * lets see if we can place a single object there.
	 */
	order = slab_order(size, 1, slub_max_order, 1);
	if (order <= slub_max_order)
		return order;

	/*
	 * Doh this slab cannot be placed using slub_max_order.
	 */
	order = slab_order(size, 1, MAX_ORDER, 1);
	if (order < MAX_ORDER)
		return order;
	return -ENOSYS;
}

/*
 * calculate_sizes() determines the order and the distribution of data within
 * a slab object.
 */
static int calculate_sizes(struct kmem_cache *s, int forced_order)
{
	slab_flags_t flags = s->flags;
	unsigned int size = s->object_size;
	unsigned int order;

	/*
	 * Round up object size to the next word boundary. We can only
	 * place the free pointer at word boundaries and this determines
	 * the possible location of the free pointer.
	 */
	size = ALIGN(size, sizeof(void *));

	/*
	 * With that we have determined the number of bytes in actual use
	 * by the object. This is the potential offset to the free pointer.
	 */
	s->inuse = size;

	if (((flags & SLAB_POISON) ||
		s->ctor)) {
		/*
		 * Relocate free pointer after the object if it is not
		 * permitted to overwrite the first word of the object on
		 * kmem_cache_free.
		 *
		 * This is the case if we do RCU, have a constructor or
		 * destructor or are poisoning the objects.
		 */
		s->offset = size;
		size += sizeof(void *);
	}

	/*
	 * SLUB stores one object immediately after another beginning from
	 * offset 0. In order to align the objects we have to simply size
	 * each object to conform to the alignment.
	 */
	size = ALIGN(size, s->align);
	s->size = size;
	if (forced_order >= 0)
		order = forced_order;
	else
		order = calculate_order(size);

	if ((int)order < 0)
		return 0;

	s->allocflags = 0;

	if (s->flags & SLAB_CACHE_DMA)
		s->allocflags |= GFP_DMA;

	/*
	 * Determine the number of objects per slab
	 */
	s->oo = oo_make(order, size);
	s->min = oo_make(get_order(size), size);
	if (oo_objects(s->oo) > oo_objects(s->max))
		s->max = s->oo;

	return !!oo_objects(s->oo);
}

static void set_min_partial(struct kmem_cache *s, u64 min)
{
	if (min < MIN_PARTIAL)
		min = MIN_PARTIAL;
	else if (min > MAX_PARTIAL)
		min = MAX_PARTIAL;
	s->min_partial = min;
}

static void set_cpu_partial(struct kmem_cache *s)
{
}

static inline struct page *alloc_slab_page(struct kmem_cache *s,
		gfp_t flags, struct kmem_cache_order_objects oo)
{
	struct page *page;
	unsigned int order = oo_order(oo);

	page = alloc_pages(flags, order);

	return page;
}

void *fixup_red_left(struct kmem_cache *s, void *p)
{
	return p;
}

static void *setup_object(struct kmem_cache *s, struct page *page,
				void *object)
{
	if (unlikely(s->ctor)) {
		s->ctor(object);
	}

	return object;
}

static inline void *freelist_ptr(const struct kmem_cache *s, void *ptr,
				 u64 ptr_addr)
{
	return ptr;
}

static inline void set_freepointer(struct kmem_cache *s, void *object, void *fp)
{
	u64 freeptr_addr = (u64)object + s->offset;

	*(void **)freeptr_addr = freelist_ptr(s, fp, freeptr_addr);
}

/* Loop over all objects in a slab */
#define for_each_object(__p, __s, __addr, __objects) \
	for (__p = fixup_red_left(__s, __addr); \
		__p < (__addr) + (__objects) * (__s)->size; \
		__p += (__s)->size)

static inline void *freelist_dereference(const struct kmem_cache *s,
					 void *ptr_addr)
{
	return freelist_ptr(s, (void *)*(u64 *)(ptr_addr),
			    (u64)ptr_addr);
}

static inline void *get_freepointer(struct kmem_cache *s, void *object)
{
	return freelist_dereference(s, object + s->offset);
}

static void prefetch_freepointer(const struct kmem_cache *s, void *object)
{
	prefetch(object + s->offset);
}

static inline void *get_freepointer_safe(struct kmem_cache *s, void *object)
{
	return get_freepointer(s, object);
}

static struct page *allocate_slab(struct kmem_cache *s, gfp_t flags)
{
	struct page *page;
	struct kmem_cache_order_objects oo = s->oo;
	gfp_t alloc_gfp;
	void *start, *p, *next;
	int idx, order;
	bool shuffle;

	flags |= s->allocflags;

	/*
	 * Let the initial higher-order allocation fail under memory pressure
	 * so we fall-back to the minimum order allocation.
	 */
	alloc_gfp = (flags | __GFP_NOWARN);

	page = alloc_slab_page(s, alloc_gfp, oo);
	if (unlikely(!page)) {
		oo = s->min;
		alloc_gfp = flags;
		/*
		 * Allocation may have failed due to fragmentation.
		 * Try a lower order alloc if possible
		 */
		page = alloc_slab_page(s, alloc_gfp, oo);
		if (unlikely(!page))
			goto out;
	}

	page->objects = oo_objects(oo);

	order = compound_order(page);
	page->slab_cache = s;
	__SetPageSlab(page);

	start = page_address(page);

	shuffle = false;

	if (!shuffle) {
		start = fixup_red_left(s, start);
		start = setup_object(s, page, start);
		page->freelist = start;
		for (idx = 0, p = start; idx < page->objects - 1; idx++) {
			next = p + s->size;
			next = setup_object(s, page, next);
			set_freepointer(s, p, next);
			p = next;
		}
		set_freepointer(s, p, NULL);
	}

	page->inuse = page->objects;
	page->frozen = 1;

out:
	return page;
}

static struct page *new_slab(struct kmem_cache *s, gfp_t flags)
{
	return allocate_slab(s, flags);
}

static struct kmem_cache *kmem_cache_node;

static void
init_kmem_cache_node(struct kmem_cache_node *n)
{
	n->nr_partial = 0;
	spin_lock_init(&n->list_lock);
	INIT_LIST_HEAD(&n->partial);
}

static inline void
__add_partial(struct kmem_cache_node *n, struct page *page, int tail)
{
	n->nr_partial++;
	if (tail == DEACTIVATE_TO_TAIL)
		list_add_tail(&page->lru, &n->partial);
	else
		list_add(&page->lru, &n->partial);
}

static inline void add_partial(struct kmem_cache_node *n,
				struct page *page, int tail)
{
	__add_partial(n, page, tail);
}

static inline void remove_partial(struct kmem_cache_node *n,
					struct page *page)
{
	list_del(&page->lru);
	n->nr_partial--;
}

static void early_kmem_cache_node_alloc(void)
{
	struct page *page;
	struct kmem_cache_node *n;

	BUG_ON(kmem_cache_node->size < sizeof(struct kmem_cache_node));

	page = new_slab(kmem_cache_node, GFP_KERNEL);

	BUG_ON(!page);

	n = page->freelist;
	BUG_ON(!n);
	page->freelist = get_freepointer(kmem_cache_node, n);
	page->inuse = 1;
	page->frozen = 0;
	kmem_cache_node->node = n;
	init_kmem_cache_node(n);

	/*
	 * No locks need to be taken here as it has just been
	 * initialized and there is no concurrent access.
	 */
	__add_partial(n, page, DEACTIVATE_TO_HEAD);
}

/* Interrupts must be disabled (for the fallback code to work right) */
static inline bool __cmpxchg_double_slab(struct kmem_cache *s, struct page *page,
		void *freelist_old, u64 counters_old,
		void *freelist_new, u64 counters_new,
		const char *n)
{
	BUG_ON(!irqs_disabled());
#if defined(CONFIG_HAVE_CMPXCHG_DOUBLE) && \
    defined(CONFIG_HAVE_ALIGNED_STRUCT_PAGE)
	if (s->flags & __CMPXCHG_DOUBLE) {
		if (cmpxchg_double(&page->freelist, &page->counters,
				   freelist_old, counters_old,
				   freelist_new, counters_new))
			return true;
	} else
#endif
	{
		slab_lock(page);
		if (page->freelist == freelist_old &&
					page->counters == counters_old) {
			page->freelist = freelist_new;
			page->counters = counters_new;
			slab_unlock(page);
			return true;
		}
		slab_unlock(page);
	}

	cpu_relax();

	return false;
}

static inline void *get_freelist(struct kmem_cache *s, struct page *page)
{
	struct page new;
	u64 counters;
	void *freelist;

	do {
		freelist = page->freelist;
		counters = page->counters;

		new.counters = counters;
		BUG_ON(!new.frozen);

		new.inuse = page->objects;
		new.frozen = freelist != NULL;

	} while (!__cmpxchg_double_slab(s, page,
		freelist, counters,
		NULL, new.counters,
		"get_freelist"));

	return freelist;
}

#define TID_STEP  roundup_pow_of_two(CONFIG_NR_CPUS)

static inline u64 next_tid(u64 tid)
{
	return tid + TID_STEP;
}

static inline unsigned int init_tid(int cpu)
{
	return cpu;
}

static inline void *acquire_slab(struct kmem_cache *s,
		struct kmem_cache_node *n, struct page *page,
		int mode, int *objects)
{
	void *freelist;
	u64 counters;
	struct page new;

	/*
	 * Zap the freelist and set the frozen bit.
	 * The old freelist is the list of objects for the
	 * per cpu allocation list.
	 */
	freelist = page->freelist;
	counters = page->counters;
	new.counters = counters;
	*objects = new.objects - new.inuse;
	if (mode) {
		new.inuse = page->objects;
		new.freelist = NULL;
	} else {
		new.freelist = freelist;
	}

	BUG_ON(new.frozen);
	new.frozen = 1;

	if (!__cmpxchg_double_slab(s, page,
			freelist, counters,
			new.freelist, new.counters,
			"acquire_slab"))
		return NULL;

	remove_partial(n, page);
	WARN_ON(!freelist);
	return freelist;
}

static void *get_partial_node(struct kmem_cache *s, struct kmem_cache_node *n,
				struct kmem_cache_cpu *c, gfp_t flags)
{
	struct page *page, *page2;
	void *object = NULL;
	unsigned int available = 0;
	int objects;

	/*
	 * Racy check. If we mistakenly see no partial slabs then we
	 * just allocate an empty slab. If we mistakenly try to get a
	 * partial slab and there is none available then get_partials()
	 * will return NULL.
	 */
	if (!n || !n->nr_partial)
		return NULL;

	spin_lock(&n->list_lock);
	list_for_each_entry_safe(page, page2, &n->partial, lru) {
		void *t;

		t = acquire_slab(s, n, page, object == NULL, &objects);
		if (!t)
			break;

		available += objects;
		if (!object) {
			c->page = page;
			object = t;
		}
		if (available > slub_cpu_partial(s) / 2)
			break;

	}
	spin_unlock(&n->list_lock);
	return object;
}

static void *get_partial(struct kmem_cache *s, gfp_t flags,
		struct kmem_cache_cpu *c)
{
	void *object;

	object = get_partial_node(s, s->node, c, flags);

	return object;
}

static void __free_slab(struct kmem_cache *s, struct page *page)
{
	int order = compound_order(page);

	if (s->flags & SLAB_CONSISTENCY_CHECKS) {
		void *p;

		for_each_object(p, s, page_address(page),
						page->objects)
			;
	}

	__ClearPageSlab(page);

	__free_pages(page, order);
}

static void free_slab(struct kmem_cache *s, struct page *page)
{
	__free_slab(s, page);
}

static void discard_slab(struct kmem_cache *s, struct page *page)
{
	free_slab(s, page);
}

static void deactivate_slab(struct kmem_cache *s, struct page *page,
				void *freelist, struct kmem_cache_cpu *c)
{
	enum slab_modes { M_NONE, M_PARTIAL, M_FULL, M_FREE };
	struct kmem_cache_node *n = s->node;
	int lock = 0;
	enum slab_modes l = M_NONE, m = M_NONE;
	void *nextfree;
	int tail = DEACTIVATE_TO_HEAD;
	struct page new;
	struct page old;

	if (page->freelist)
		tail = DEACTIVATE_TO_TAIL;

	/*
	 * Stage one: Free all available per cpu objects back
	 * to the page freelist while it is still frozen. Leave the
	 * last one.
	 *
	 * There is no need to take the list->lock because the page
	 * is still frozen.
	 */
	while (freelist && (nextfree = get_freepointer(s, freelist))) {
		void *prior;
		u64 counters;

		do {
			prior = page->freelist;
			counters = page->counters;
			set_freepointer(s, freelist, prior);
			new.counters = counters;
			new.inuse--;
			BUG_ON(!new.frozen);

		} while (!__cmpxchg_double_slab(s, page,
			prior, counters,
			freelist, new.counters,
			"drain percpu freelist"));

		freelist = nextfree;
	}

	/*
	 * Stage two: Ensure that the page is unfrozen while the
	 * list presence reflects the actual number of objects
	 * during unfreeze.
	 *
	 * We setup the list membership and then perform a cmpxchg
	 * with the count. If there is a mismatch then the page
	 * is not unfrozen but the page is on the wrong list.
	 *
	 * Then we restart the process which may have to remove
	 * the page from the list that we just put it on again
	 * because the number of objects in the slab may have
	 * changed.
	 */
redo:

	old.freelist = page->freelist;
	old.counters = page->counters;
	BUG_ON(!old.frozen);

	/* Determine target state of the slab */
	new.counters = old.counters;
	if (freelist) {
		new.inuse--;
		set_freepointer(s, freelist, old.freelist);
		new.freelist = freelist;
	} else
		new.freelist = old.freelist;

	new.frozen = 0;

	if (!new.inuse && n->nr_partial >= s->min_partial)
		m = M_FREE;
	else if (new.freelist) {
		m = M_PARTIAL;
		if (!lock) {
			lock = 1;
			/*
			 * Taking the spinlock removes the possiblity
			 * that acquire_slab() will see a slab page that
			 * is frozen
			 */
			spin_lock(&n->list_lock);
		}
	} else {
		m = M_FULL;
	}

	if (l != m) {
		if (l == M_PARTIAL)
			remove_partial(n, page);

		if (m == M_PARTIAL)
			add_partial(n, page, tail);
	}

	l = m;
	if (!__cmpxchg_double_slab(s, page,
				old.freelist, old.counters,
				new.freelist, new.counters,
				"unfreezing slab"))
		goto redo;

	if (lock)
		spin_unlock(&n->list_lock);

	if (m == M_FREE)
		discard_slab(s, page);

	c->page = NULL;
	c->freelist = NULL;
}

static inline void flush_slab(struct kmem_cache *s, struct kmem_cache_cpu *c)
{
	deactivate_slab(s, c->page, c->freelist, c);

	c->tid = next_tid(c->tid);
}

static inline void *new_slab_objects(struct kmem_cache *s, gfp_t flags,
			struct kmem_cache_cpu **pc)
{
	void *freelist;
	struct kmem_cache_cpu *c = *pc;
	struct page *page;

	WARN_ON_ONCE(s->ctor && (flags & __GFP_ZERO));

	freelist = get_partial(s, flags, c);

	if (freelist)
		return freelist;

	page = new_slab(s, flags);
	if (page) {
		c = this_cpu_ptr(s->cpu_slab);
		if (c->page)
			flush_slab(s, c);

		/*
		 * No other reference to the page yet so we can
		 * muck around with it freely without cmpxchg
		 */
		freelist = page->freelist;
		page->freelist = NULL;

		c->page = page;
		*pc = c;
	} else
		freelist = NULL;

	return freelist;
}

static void *___slab_alloc(struct kmem_cache *s, gfp_t gfpflags,
			  u64 addr, struct kmem_cache_cpu *c)
{
	void *freelist;
	struct page *page;

	page = c->page;
	if (!page)
		goto new_slab;

	/* must check again c->freelist in case of cpu migration or IRQ */
	freelist = c->freelist;
	if (freelist)
		goto load_freelist;

	freelist = get_freelist(s, page);

	if (!freelist) {
		c->page = NULL;
		goto new_slab;
	}

load_freelist:
	/*
	 * freelist is pointing to the list of objects to be used.
	 * page is pointing to the page from which the objects are obtained.
	 * That page must be frozen for per cpu allocations to work.
	 */
	BUG_ON(!c->page->frozen);
	c->freelist = get_freepointer(s, freelist);
	c->tid = next_tid(c->tid);
	return freelist;

new_slab:

	freelist = new_slab_objects(s, gfpflags, &c);

	if (unlikely(!freelist))
		return NULL;

	page = c->page;

	goto load_freelist;
}

static void *__slab_alloc(struct kmem_cache *s, gfp_t gfpflags,
			  u64 addr, struct kmem_cache_cpu *c)
{
	void *p;
	u64 flags;

	local_irq_save(flags);
	/*
	 * We may have been preempted and rescheduled on a different
	 * cpu before disabling interrupts. Need to reload cpu area
	 * pointer.
	 */
	c = this_cpu_ptr(s->cpu_slab);

	p = ___slab_alloc(s, gfpflags, addr, c);
	local_irq_restore(flags);

	return p;
}

static __always_inline void *slab_alloc_node(struct kmem_cache *s,
		gfp_t gfpflags, u64 addr)
{
	void *object;
	struct kmem_cache_cpu *c;
	struct page *page;
	u64 tid;

	s = slab_pre_alloc_hook(s, gfpflags);
	if (!s)
		return NULL;
redo:
	/*
	 * Must read kmem_cache cpu data via this cpu ptr. Preemption is
	 * enabled. We may switch back and forth between cpus while
	 * reading from one cpu area. That does not matter as u64
	 * as we end up on the original cpu again when doing the cmpxchg.
	 *
	 * We should guarantee that tid and kmem_cache are retrieved on
	 * the same cpu. It could be different if CONFIG_PREEMPT so we need
	 * to check if it is matched or not.
	 */
	do {
		tid = this_cpu_read(s->cpu_slab->tid);
		c = this_cpu_ptr(s->cpu_slab);
	} while (unlikely(tid != READ_ONCE(c->tid)));

	/*
	 * Irqless object alloc/free algorithm used here depends on sequence
	 * of fetching cpu_slab's data. tid should be fetched before anything
	 * on c to guarantee that object and page associated with previous tid
	 * won't be used with current tid. If we fetch tid first, object and
	 * page could be one associated with next tid and our alloc/free
	 * request will be failed. In this case, we will retry. So, no problem.
	 */
	barrier();

	/*
	 * The transaction ids are globally unique per cpu and per operation on
	 * a per cpu queue. Thus they can be guarantee that the cmpxchg_double
	 * occurs on the right processor and that there was no operation on the
	 * linked list in between.
	 */

	object = c->freelist;
	page = c->page;
	if (unlikely(!object)) {
		object = __slab_alloc(s, gfpflags, addr, c);
	} else {
		void *next_object = get_freepointer_safe(s, object);

		/*
		 * The cmpxchg will only match if there was no additional
		 * operation and if we are on the right processor.
		 *
		 * The cmpxchg does the following atomically (without lock
		 * semantics!)
		 * 1. Relocate first pointer to the current per cpu area.
		 * 2. Verify that tid and freelist have not been changed
		 * 3. If they were not changed replace tid and freelist
		 *
		 * Since this is without lock semantics the protection is only
		 * against code executing on this cpu *not* from access by
		 * other cpus.
		 */
		if (unlikely(!this_cpu_cmpxchg_double(
				s->cpu_slab->freelist, s->cpu_slab->tid,
				object, tid,
				next_object, next_tid(tid)))) {

			goto redo;
		}
		prefetch_freepointer(s, next_object);
	}

	if (unlikely(gfpflags & __GFP_ZERO) && object)
		memset(object, 0, s->object_size);

	return object;
}

static __always_inline void *slab_alloc(struct kmem_cache *s,
		gfp_t gfpflags, u64 addr)
{
	return slab_alloc_node(s, gfpflags, addr);
}

void *kmem_cache_alloc(struct kmem_cache *s, gfp_t gfpflags)
{
	void *ret = slab_alloc(s, gfpflags, _RET_IP_);

	return ret;
}

static inline bool cmpxchg_double_slab(struct kmem_cache *s, struct page *page,
		void *freelist_old, u64 counters_old,
		void *freelist_new, u64 counters_new,
		const char *n)
{
#if defined(CONFIG_HAVE_CMPXCHG_DOUBLE) && \
    defined(CONFIG_HAVE_ALIGNED_STRUCT_PAGE)
	if (s->flags & __CMPXCHG_DOUBLE) {
		if (cmpxchg_double(&page->freelist, &page->counters,
				   freelist_old, counters_old,
				   freelist_new, counters_new))
			return true;
	} else
#endif
	{
		u64 flags;

		local_irq_save(flags);
		slab_lock(page);
		if (page->freelist == freelist_old &&
					page->counters == counters_old) {
			page->freelist = freelist_new;
			page->counters = counters_new;
			slab_unlock(page);
			local_irq_restore(flags);
			return true;
		}
		slab_unlock(page);
		local_irq_restore(flags);
	}

	cpu_relax();

	return false;
}

static void __slab_free(struct kmem_cache *s, struct page *page,
			void *head, void *tail, int cnt,
			u64 addr)

{
	void *prior;
	int was_frozen;
	struct page new;
	u64 counters;
	struct kmem_cache_node *n = NULL;
	u64 uninitialized_var(flags);

	do {
		if (unlikely(n)) {
			spin_unlock_irqrestore(&n->list_lock, flags);
			n = NULL;
		}
		prior = page->freelist;
		counters = page->counters;
		set_freepointer(s, tail, prior);
		new.counters = counters;
		was_frozen = new.frozen;
		new.inuse -= cnt;
		if ((!new.inuse || !prior) && !was_frozen) {
			n = s->node;
			/*
				* Speculatively acquire the list_lock.
				* If the cmpxchg does not succeed then we may
				* drop the list_lock without any processing.
				*
				* Otherwise the list_lock will synchronize with
				* other processors updating the list of slabs.
				*/
			spin_lock_irqsave(&n->list_lock, flags);
		}

	} while (!cmpxchg_double_slab(s, page,
		prior, counters,
		head, new.counters,
		"__slab_free"));

	if (likely(!n))
		return;

	if (unlikely(!new.inuse && n->nr_partial >= s->min_partial))
		goto slab_empty;

	/*
	 * Objects left in the slab. If it was not on the partial list before
	 * then add it.
	 */
	if (unlikely(!prior))
		add_partial(n, page, DEACTIVATE_TO_TAIL);

	spin_unlock_irqrestore(&n->list_lock, flags);
	return;

slab_empty:
	if (prior) {
		/*
		 * Slab on the partial list.
		 */
		remove_partial(n, page);
	}

	spin_unlock_irqrestore(&n->list_lock, flags);

	discard_slab(s, page);
}

static __always_inline void do_slab_free(struct kmem_cache *s,
				struct page *page, void *head, void *tail,
				int cnt, u64 addr)
{
	void *tail_obj = tail ? : head;
	struct kmem_cache_cpu *c;
	u64 tid;
redo:
	/*
	 * Determine the currently cpus per cpu slab.
	 * The cpu may change afterward. However that does not matter since
	 * data is retrieved via this pointer. If we are on the same cpu
	 * during the cmpxchg then the free will succeed.
	 */
	do {
		tid = this_cpu_read(s->cpu_slab->tid);
		c = this_cpu_ptr(s->cpu_slab);
	} while (unlikely(tid != READ_ONCE(c->tid)));

	/* Same with comment on barrier() in slab_alloc_node() */
	barrier();

	if (likely(page == c->page)) {
		set_freepointer(s, tail_obj, c->freelist);

		if (unlikely(!this_cpu_cmpxchg_double(
				s->cpu_slab->freelist, s->cpu_slab->tid,
				c->freelist, tid,
				head, next_tid(tid)))) {

			goto redo;
		}
	} else
		__slab_free(s, page, head, tail_obj, cnt, addr);
}

static __always_inline void slab_free(struct kmem_cache *s, struct page *page,
				      void *head, void *tail, int cnt,
				      u64 addr)
{
	do_slab_free(s, page, head, tail, cnt, addr);
}

void kmem_cache_free(struct kmem_cache *s, void *x)
{
	s = cache_from_obj(s, x);
	if (!s)
		return;
	slab_free(s, virt_to_head_page(x), x, NULL, 1, _RET_IP_);
}

static void free_kmem_cache_nodes(struct kmem_cache *s)
{
	struct kmem_cache_node *n = s->node;

	kmem_cache_free(kmem_cache_node, n);
	s->node = NULL;
}

static int init_kmem_cache_nodes(struct kmem_cache *s)
{
	struct kmem_cache_node *n;

	if (slab_state == DOWN) {
		early_kmem_cache_node_alloc();
		goto done;
	}
	n = kmem_cache_alloc(kmem_cache_node, GFP_KERNEL);
	if (!n) {
		free_kmem_cache_nodes(s);
		return 0;
	}

	init_kmem_cache_node(n);
	s->node = n;

done:
	return 1;
}

static void init_kmem_cache_cpus(struct kmem_cache *s)
{
	int cpu;

	for_each_possible_cpu(cpu)
		per_cpu_ptr(s->cpu_slab, cpu)->tid = init_tid(cpu);
}

static inline int alloc_kmem_cache_cpus(struct kmem_cache *s)
{
	BUILD_BUG_ON(PERCPU_DYNAMIC_EARLY_SIZE <
			KMALLOC_SHIFT_HIGH * sizeof(struct kmem_cache_cpu));

	/*
	 * Must align to double word boundary for the double cmpxchg
	 * instructions to work; see __pcpu_double_call_return_bool().
	 */
	s->cpu_slab = __alloc_percpu(sizeof(struct kmem_cache_cpu),
				     2 * sizeof(void *));

	if (!s->cpu_slab)
		return 0;

	init_kmem_cache_cpus(s);

	return 1;
}

static int kmem_cache_open(struct kmem_cache *s, slab_flags_t flags)
{
	s->flags = flags;

	if (!calculate_sizes(s, -1))
		goto error;

#if defined(CONFIG_HAVE_CMPXCHG_DOUBLE) && \
    defined(CONFIG_HAVE_ALIGNED_STRUCT_PAGE)
	if (system_has_cmpxchg_double() && (s->flags & SLAB_NO_CMPXCHG) == 0)
		/* Enable fast mode */
		s->flags |= __CMPXCHG_DOUBLE;
#endif

	/*
	 * The larger the object size is, the more pages we want on the partial
	 * list to avoid pounding the page allocator excessively.
	 */
	set_min_partial(s, ilog2(s->size) / 2);

	set_cpu_partial(s);

	if (!init_kmem_cache_nodes(s))
		goto error;

	if (alloc_kmem_cache_cpus(s))
		return 0;

	free_kmem_cache_nodes(s);
error:
	if (flags & SLAB_PANIC)
		panic("Cannot create slab %s size=%u realsize=%u order=%u offset=%u flags=%llx\n",
		      s->name, s->size, s->size,
		      oo_order(s->oo), s->offset, (u64)flags);
	return -EINVAL;
}

static inline void __flush_cpu_slab(struct kmem_cache *s, int cpu)
{
	struct kmem_cache_cpu *c = per_cpu_ptr(s->cpu_slab, cpu);

	if (c->page)
		flush_slab(s, c);
}

static struct kmem_cache * __init bootstrap(struct kmem_cache *static_cache)
{
	struct kmem_cache *s = kmem_cache_zalloc(kmem_cache, GFP_KERNEL);
	struct kmem_cache_node *n = s->node;
	struct page *p;

	memcpy(s, static_cache, kmem_cache->object_size);

	/*
	 * This runs very early, and only the boot processor is supposed to be
	 * up.  Even if it weren't true, IRQs are not up so we couldn't fire
	 * IPIs around.
	 */
	__flush_cpu_slab(s, smp_processor_id());

	list_for_each_entry(p, &n->partial, lru)
		p->slab_cache = s;

	list_add(&s->list, &slab_caches);

	return s;
}

void *__kmalloc(size_t size, gfp_t flags)
{
	struct kmem_cache *s;
	void *ret;

	if (unlikely(size > KMALLOC_MAX_CACHE_SIZE))
		return kmalloc_large(size, flags);

	s = kmalloc_slab(size, flags);

	if (unlikely(ZERO_OR_NULL_PTR(s)))
		return s;

	ret = slab_alloc(s, flags, _RET_IP_);

	return ret;
}

static size_t __ksize(const void *object)
{
	struct page *page;

	if (unlikely(object == ZERO_SIZE_PTR))
		return 0;

	page = virt_to_head_page(object);

	if (unlikely(!PageSlab(page))) {
		WARN_ON(!PageCompound(page));
		return PAGE_SIZE << compound_order(page);
	}

	return slab_ksize(page->slab_cache);
}

size_t ksize(const void *object)
{
	size_t size = __ksize(object);

	return size;
}

void kfree(const void *x)
{
	struct page *page;
	void *object = (void *)x;

	if (unlikely(ZERO_OR_NULL_PTR(x)))
		return;

	page = virt_to_head_page(x);
	if (unlikely(!PageSlab(page))) {
		BUG_ON(!PageCompound(page));
		__free_pages(page, compound_order(page));

		return;
	}
	slab_free(page->slab_cache, page, object, NULL, 1, _RET_IP_);
}

void __kmem_cache_release(struct kmem_cache *s)
{
	free_percpu(s->cpu_slab);
	free_kmem_cache_nodes(s);
}

bool __kmem_cache_empty(struct kmem_cache *s)
{
	struct kmem_cache_node *n = s->node;

	if (n->nr_partial)
		return false;

	return true;
}

void __init kmem_cache_init(void)
{
	static __initdata struct kmem_cache boot_kmem_cache,
		boot_kmem_cache_node;

	kmem_cache_node = &boot_kmem_cache_node;
	kmem_cache = &boot_kmem_cache;

	create_boot_cache(kmem_cache_node, "kmem_cache_node",
		sizeof(struct kmem_cache_node), SLAB_HWCACHE_ALIGN, 0, 0);

	/* Able to allocate the per node structures */
	slab_state = PARTIAL;

	create_boot_cache(kmem_cache, "kmem_cache",
			offsetof(struct kmem_cache, node) +
			   sizeof(struct kmem_cache_node *),
		       SLAB_HWCACHE_ALIGN, 0, 0);

	kmem_cache = bootstrap(&boot_kmem_cache);
	kmem_cache_node = bootstrap(&boot_kmem_cache_node);

	/* Now we can use the kmem_cache to allocate kmalloc slabs */
	setup_kmalloc_cache_index_table();
	create_kmalloc_caches(0);

	pr_info("SLUB: HWalign=%d, Order=%u-%u, MinObjects=%u, CPUs=%u\n",
		cache_line_size(),
		slub_min_order, slub_max_order, slub_min_objects,
		nr_possible_cpu_ids);
}

struct kmem_cache *
__kmem_cache_alias(const char *name, unsigned int size, unsigned int align,
		   slab_flags_t flags, void (*ctor)(void *))
{
	struct kmem_cache *s, *c;

	s = find_mergeable(size, align, flags, name, ctor);
	if (s) {
		s->refcount++;

		/*
		 * Adjust the object sizes so that we clear
		 * the complete object on kzalloc.
		 */
		s->object_size = max(s->object_size, size);
		s->inuse = max(s->inuse, ALIGN(size, sizeof(void *)));

		for ((void)(c), (void)(s); 0; ) {
			c->object_size = s->object_size;
			c->inuse = max(c->inuse, ALIGN(size, sizeof(void *)));
		}
	}

	return s;
}

int __kmem_cache_create(struct kmem_cache *s, slab_flags_t flags)
{
	int err;

	err = kmem_cache_open(s, flags);

	return err;
}

void *__kmalloc_track_caller(size_t size, gfp_t gfpflags, u64 caller)
{
	struct kmem_cache *s;
	void *ret;

	if (unlikely(size > KMALLOC_MAX_CACHE_SIZE))
		return kmalloc_large(size, gfpflags);

	s = kmalloc_slab(size, gfpflags);

	if (unlikely(ZERO_OR_NULL_PTR(s)))
		return s;

	ret = slab_alloc(s, gfpflags, caller);

	return ret;
}
