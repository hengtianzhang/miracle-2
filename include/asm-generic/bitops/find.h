/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_BITOPS_FIND_H_
#define __ASM_GENERIC_BITOPS_FIND_H_

#ifndef find_next_bit
/**
 * find_next_bit - find the next set bit in a memory region
 * @addr: The address to base the search on
 * @offset: The bitnumber to start searching at
 * @size: The bitmap size in bits
 *
 * Returns the bit number for the next set bit
 * If no bits are set, returns @size.
 */
extern u64 find_next_bit(const u64 *addr, u64
		size, u64 offset);
#endif

#ifndef find_next_and_bit
/**
 * find_next_and_bit - find the next set bit in both memory regions
 * @addr1: The first address to base the search on
 * @addr2: The second address to base the search on
 * @offset: The bitnumber to start searching at
 * @size: The bitmap size in bits
 *
 * Returns the bit number for the next set bit
 * If no bits are set, returns @size.
 */
extern u64 find_next_and_bit(const u64 *addr1,
		const u64 *addr2, u64 size,
		u64 offset);
#endif

#ifndef find_next_zero_bit
/**
 * find_next_zero_bit - find the next cleared bit in a memory region
 * @addr: The address to base the search on
 * @offset: The bitnumber to start searching at
 * @size: The bitmap size in bits
 *
 * Returns the bit number of the next zero bit
 * If no bits are zero, returns @size.
 */
extern u64 find_next_zero_bit(const u64 *addr, u64 size, u64 offset);
#endif

#ifdef CONFIG_GENERIC_FIND_FIRST_BIT

/**
 * find_first_bit - find the first set bit in a memory region
 * @addr: The address to start the search at
 * @size: The maximum number of bits to search
 *
 * Returns the bit number of the first set bit.
 * If no bits are set, returns @size.
 */
extern u64 find_first_bit(const u64 *addr,
				    u64 size);

/**
 * find_first_zero_bit - find the first cleared bit in a memory region
 * @addr: The address to start the search at
 * @size: The maximum number of bits to search
 *
 * Returns the bit number of the first cleared bit.
 * If no bits are zero, returns @size.
 */
extern u64 find_first_zero_bit(const u64 *addr,
					 u64 size);
#else /* CONFIG_GENERIC_FIND_FIRST_BIT */

#ifndef find_first_bit
#define find_first_bit(addr, size) find_next_bit((addr), (size), 0)
#endif
#ifndef find_first_zero_bit
#define find_first_zero_bit(addr, size) find_next_zero_bit((addr), (size), 0)
#endif

#endif /* CONFIG_GENERIC_FIND_FIRST_BIT */

#endif /* !__ASM_GENERIC_BITOPS_FIND_H_ */
