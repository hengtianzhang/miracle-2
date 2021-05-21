/* bit search implementation
 *
 * Copyright (C) 2004 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * Copyright (C) 2008 IBM Corporation
 * 'find_last_bit' is written by Rusty Russell <rusty@rustcorp.com.au>
 * (Inspired by David Howell's find_next_bit implementation)
 *
 * Rewritten by Yury Norov <yury.norov@gmail.com> to decrease
 * size and improve performance, 2015.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <linux/bitops.h>
#include <linux/bitmap.h>
#include <linux/kernel.h>

#if !defined(find_next_bit) || !defined(find_next_zero_bit) || \
		!defined(find_next_and_bit)
/*
 * This is a common helper function for find_next_bit, find_next_zero_bit, and
 * find_next_and_bit. The differences are:
 *  - The "invert" argument, which is XORed with each fetched word before
 *    searching it for one bits.
 *  - The optional "addr2", which is anded with "addr1" if present.
 */
static inline u64 _find_next_bit(const u64 *addr1,
		const u64 *addr2, u64 nbits,
		u64 start, u64 invert)
{
	u64 tmp;

	if (unlikely(start >= nbits))
		return nbits;

	tmp = addr1[start / BITS_PER_LONG];
	if (addr2)
		tmp &= addr2[start / BITS_PER_LONG];
	tmp ^= invert;

	/* Handle 1st word. */
	tmp &= BITMAP_FIRST_WORD_MASK(start);
	start = round_down(start, BITS_PER_LONG);

	while (!tmp) {
		start += BITS_PER_LONG;
		if (start >= nbits)
			return nbits;

		tmp = addr1[start / BITS_PER_LONG];
		if (addr2)
			tmp &= addr2[start / BITS_PER_LONG];
		tmp ^= invert;
	}

	return min(start + __ffs(tmp), nbits);
}
#endif

#ifndef find_next_bit
/*
 * Find the next set bit in a memory region.
 */
u64 find_next_bit(const u64 *addr, u64 size,
			    u64 offset)
{
	return _find_next_bit(addr, NULL, size, offset, 0ULL);
}
#endif

#ifndef find_next_zero_bit
u64 find_next_zero_bit(const u64 *addr, u64 size,
				 u64 offset)
{
	return _find_next_bit(addr, NULL, size, offset, ~0UL);
}
#endif

#if !defined(find_next_and_bit)
u64 find_next_and_bit(const u64 *addr1,
		const u64 *addr2, u64 size,
		u64 offset)
{
	return _find_next_bit(addr1, addr2, size, offset, 0UL);
}
#endif

#ifndef find_first_bit
/*
 * Find the first set bit in a memory region.
 */
u64 find_first_bit(const u64 *addr, u64 size)
{
	u64 idx;

	for (idx = 0; idx * BITS_PER_LONG < size; idx++) {
		if (addr[idx])
			return min(idx * BITS_PER_LONG + __ffs(addr[idx]), size);
	}

	return size;
}
#endif

#ifndef find_first_zero_bit
/*
 * Find the first cleared bit in a memory region.
 */
u64 find_first_zero_bit(const u64 *addr, u64 size)
{
	u64 idx;

	for (idx = 0; idx * BITS_PER_LONG < size; idx++) {
		if (addr[idx] != ~0UL)
			return min(idx * BITS_PER_LONG + ffz(addr[idx]), size);
	}

	return size;
}
#endif

#ifndef find_last_bit
u64 find_last_bit(const u64 *addr, u64 size)
{
	if (size) {
		u64 val = BITMAP_LAST_WORD_MASK(size);
		u64 idx = (size-1) / BITS_PER_LONG;

		do {
			val &= addr[idx];
			if (val)
				return idx * BITS_PER_LONG + __fls(val);

			val = ~0ul;
		} while (idx--);
	}
	return size;
}
#endif

#ifdef __BIG_ENDIAN

/* include/linux/byteorder does not support "u64" type */
static inline u64 ext2_swab(const u64 y)
{
#if BITS_PER_LONG == 64
	return (u64) __swab64((u64) y);
#elif BITS_PER_LONG == 32
	return (u64) __swab32((u32) y);
#else
#error BITS_PER_LONG not defined
#endif
}

#if !defined(find_next_bit_le) || !defined(find_next_zero_bit_le)
static inline u64 _find_next_bit_le(const u64 *addr1,
		const u64 *addr2, u64 nbits,
		u64 start, u64 invert)
{
	u64 tmp;

	if (unlikely(start >= nbits))
		return nbits;

	tmp = addr1[start / BITS_PER_LONG];
	if (addr2)
		tmp &= addr2[start / BITS_PER_LONG];
	tmp ^= invert;

	/* Handle 1st word. */
	tmp &= ext2_swab(BITMAP_FIRST_WORD_MASK(start));
	start = round_down(start, BITS_PER_LONG);

	while (!tmp) {
		start += BITS_PER_LONG;
		if (start >= nbits)
			return nbits;

		tmp = addr1[start / BITS_PER_LONG];
		if (addr2)
			tmp &= addr2[start / BITS_PER_LONG];
		tmp ^= invert;
	}

	return min(start + __ffs(ext2_swab(tmp)), nbits);
}
#endif

#ifndef find_next_zero_bit_le
u64 find_next_zero_bit_le(const void *addr, u64 size, u64 offset)
{
	return _find_next_bit_le(addr, NULL, size, offset, ~0UL);
}
#endif

#ifndef find_next_bit_le
u64 find_next_bit_le(const void *addr, u64 size, u64 offset)
{
	return _find_next_bit_le(addr, NULL, size, offset, 0UL);
}
#endif

#endif /* __BIG_ENDIAN */
