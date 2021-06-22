/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Written by Mark Hemment, 1996 (markhe@nextd.demon.co.uk).
 *
 * (C) SGI 2006, Christoph Lameter
 * 	Cleaned up and restructured to ease the addition of alternative
 * 	implementations of SLAB allocators.
 * (C) Linux Foundation 2008-2013
 *      Unified interface for all slab allocators
 */

#ifndef __LINUX_SLAB_H_
#define	__LINUX_SLAB_H_

#include <linux/gfp.h>
#include <linux/types.h>
#include <linux/page.h>
#include <linux/overflow.h>

struct kmem_cache;

/* DEBUG: Perform (expensive) checks on alloc/free */
#define SLAB_CONSISTENCY_CHECKS	((slab_flags_t __force)0x00000100U)

#define SLAB_POISON		((slab_flags_t __force)0x00000800U)
#define SLAB_HWCACHE_ALIGN	((slab_flags_t __force)0x00002000U)
#define SLAB_CACHE_DMA		((slab_flags_t __force)0x00004000U)
#define SLAB_STORE_USER		((slab_flags_t __force)0x00010000U)
#define SLAB_PANIC		((slab_flags_t __force)0x00040000U)

#define ZERO_SIZE_PTR ((void *)16)

#define ZERO_OR_NULL_PTR(x) ((u64)(x) <= \
				(u64)ZERO_SIZE_PTR)

#ifndef ARCH_SLAB_MINALIGN
#define ARCH_SLAB_MINALIGN __alignof__(u64)
#endif

#if defined(ARCH_DMA_MINALIGN) && ARCH_DMA_MINALIGN > 8
#define ARCH_KMALLOC_MINALIGN ARCH_DMA_MINALIGN
#define KMALLOC_MIN_SIZE ARCH_DMA_MINALIGN
#define KMALLOC_SHIFT_LOW ilog2(ARCH_DMA_MINALIGN)
#else
#define ARCH_KMALLOC_MINALIGN __alignof__(u64)
#endif

/*
 * SLUB directly allocates requests fitting in to an order-1 page
 * (PAGE_SIZE*2).  Larger requests are passed to the page allocator.
 */
#define KMALLOC_SHIFT_HIGH	(PAGE_SHIFT + 1)
#define KMALLOC_SHIFT_MAX	(MAX_ORDER + PAGE_SHIFT - 1)
#ifndef KMALLOC_SHIFT_LOW
#define KMALLOC_SHIFT_LOW	3
#endif

#define __assume_kmalloc_alignment __assume_aligned(ARCH_KMALLOC_MINALIGN)
#define __assume_slab_alignment __assume_aligned(ARCH_SLAB_MINALIGN)
#define __assume_page_alignment __assume_aligned(PAGE_SIZE)

/* Maximum allocatable size */
#define KMALLOC_MAX_SIZE	(1UL << KMALLOC_SHIFT_MAX)
/* Maximum size for which we actually use a slab cache */
#define KMALLOC_MAX_CACHE_SIZE	(1UL << KMALLOC_SHIFT_HIGH)
/* Maximum order allocatable via the slab allocagtor */
#define KMALLOC_MAX_ORDER	(KMALLOC_SHIFT_MAX - PAGE_SHIFT)

/*
 * Kmalloc subsystem.
 */
#ifndef KMALLOC_MIN_SIZE
#define KMALLOC_MIN_SIZE (1 << KMALLOC_SHIFT_LOW)
#endif

enum kmalloc_cache_type {
	KMALLOC_NORMAL = 0,
	KMALLOC_DMA,
	NR_KMALLOC_TYPES
};

extern struct kmem_cache *
kmalloc_caches[NR_KMALLOC_TYPES][KMALLOC_SHIFT_HIGH + 1];

static __always_inline unsigned int kmalloc_size(unsigned int n)
{
	if (n > 2)
		return 1U << n;

	if (n == 1 && KMALLOC_MIN_SIZE <= 32)
		return 96;

	if (n == 2 && KMALLOC_MIN_SIZE <= 64)
		return 192;

	return 0;
}

static __always_inline enum kmalloc_cache_type kmalloc_type(gfp_t flags)
{
	/*
	 * The most common case is KMALLOC_NORMAL, so test for it
	 * with a single branch for both flags.
	 */
	if (likely((flags & (__GFP_DMA)) == 0))
		return KMALLOC_NORMAL;

	/*
	 * At least one of the flags has to be set. If both are, __GFP_DMA
	 * is more important.
	 */
	return flags & __GFP_DMA ? KMALLOC_DMA : KMALLOC_NORMAL;
}

/*
 * Figure out which kmalloc slab an allocation of a certain size
 * belongs to.
 * 0 = zero alloc
 * 1 =  65 .. 96 bytes
 * 2 = 129 .. 192 bytes
 * n = 2^(n-1)+1 .. 2^n
 */
static __always_inline unsigned int kmalloc_index(size_t size)
{
	if (!size)
		return 0;

	if (size <= KMALLOC_MIN_SIZE)
		return KMALLOC_SHIFT_LOW;

	if (KMALLOC_MIN_SIZE <= 32 && size > 64 && size <= 96)
		return 1;
	if (KMALLOC_MIN_SIZE <= 64 && size > 128 && size <= 192)
		return 2;
	if (size <=          8) return 3;
	if (size <=         16) return 4;
	if (size <=         32) return 5;
	if (size <=         64) return 6;
	if (size <=        128) return 7;
	if (size <=        256) return 8;
	if (size <=        512) return 9;
	if (size <=       1024) return 10;
	if (size <=   2 * 1024) return 11;
	if (size <=   4 * 1024) return 12;
	if (size <=   8 * 1024) return 13;
	if (size <=  16 * 1024) return 14;
	if (size <=  32 * 1024) return 15;
	if (size <=  64 * 1024) return 16;
	if (size <= 128 * 1024) return 17;
	if (size <= 256 * 1024) return 18;
	if (size <= 512 * 1024) return 19;
	if (size <= 1024 * 1024) return 20;
	if (size <=  2 * 1024 * 1024) return 21;
	if (size <=  4 * 1024 * 1024) return 22;
	if (size <=  8 * 1024 * 1024) return 23;
	if (size <=  16 * 1024 * 1024) return 24;
	if (size <=  32 * 1024 * 1024) return 25;
	if (size <=  64 * 1024 * 1024) return 26;
	BUG();

	/* Will never be reached. Needed because the compiler may complain */
	return -1;
}

bool slab_is_available(void);
void __init kmem_cache_init(void);

struct kmem_cache *kmem_cache_create(const char *name, unsigned int size,
			unsigned int align, slab_flags_t flags,
			void (*ctor)(void *));
struct kmem_cache *kmem_cache_create_usercopy(const char *name,
			unsigned int size, unsigned int align,
			slab_flags_t flags,
			unsigned int useroffset, unsigned int usersize,
			void (*ctor)(void *));

void *__kmalloc(size_t size, gfp_t flags) __assume_kmalloc_alignment __malloc;
void *kmem_cache_alloc(struct kmem_cache *, gfp_t flags) __assume_slab_alignment __malloc;
void kmem_cache_free(struct kmem_cache *, void *);

static inline void *kmem_cache_zalloc(struct kmem_cache *k, gfp_t flags)
{
	return kmem_cache_alloc(k, flags | __GFP_ZERO);
}

extern void *kmalloc_order(size_t size, gfp_t flags, unsigned int order) __assume_page_alignment __malloc;

static __always_inline void *kmalloc_large(size_t size, gfp_t flags)
{
	unsigned int order = get_order(size);

	return kmalloc_order(size, flags, order);
}

void kfree(const void *);
size_t ksize(const void *);

static __always_inline void *kmalloc(size_t size, gfp_t flags)
{
	if (__builtin_constant_p(size)) {
		unsigned int index;

		if (size > KMALLOC_MAX_CACHE_SIZE)
			return kmalloc_large(size, flags);

		index = kmalloc_index(size);

		if (!index)
			return ZERO_SIZE_PTR;

		return kmem_cache_alloc(kmalloc_caches[kmalloc_type(flags)][index],
				flags);
	}

	return __kmalloc(size, flags);
}

static inline void *kzalloc(size_t size, gfp_t flags)
{
	return kmalloc(size, flags | __GFP_ZERO);
}

static inline void *kmalloc_array(size_t n, size_t size, gfp_t flags)
{
	size_t bytes;

	if (unlikely(check_mul_overflow(n, size, &bytes)))
		return NULL;
	if (__builtin_constant_p(n) && __builtin_constant_p(size))
		return kmalloc(bytes, flags);
	return __kmalloc(bytes, flags);
}

static inline void *kcalloc(size_t n, size_t size, gfp_t flags)
{
	return kmalloc_array(n, size, flags | __GFP_ZERO);
}

extern void *__kmalloc_track_caller(size_t, gfp_t, u64);
#define kmalloc_track_caller(size, flags) \
	__kmalloc_track_caller(size, flags, _RET_IP_)

#endif /* !__LINUX_SLAB_H_ */
