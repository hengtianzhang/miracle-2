/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_WORD_AT_A_TIME_H_
#define __ASM_GENERIC_WORD_AT_A_TIME_H_

#include <linux/kernel.h>

#include <asm/byteorder.h>

#ifdef __BIG_ENDIAN

struct word_at_a_time {
	const u64 high_bits, low_bits;
};

#define WORD_AT_A_TIME_CONSTANTS { REPEAT_BYTE(0xfe) + 1, REPEAT_BYTE(0x7f) }

/* Bit set in the bytes that have a zero */
static inline s64 prep_zero_mask(u64 val, u64 rhs, const struct word_at_a_time *c)
{
	u64 mask = (val & c->low_bits) + c->low_bits;
	return ~(mask | rhs);
}

#define create_zero_mask(mask) (mask)

static inline s64 find_zero(u64 mask)
{
	s64 byte = 0;
#ifdef CONFIG_64BIT
	if (mask >> 32)
		mask >>= 32;
	else
		byte = 4;
#endif
	if (mask >> 16)
		mask >>= 16;
	else
		byte += 2;
	return (mask >> 8) ? byte : byte + 1;
}

static inline bool has_zero(u64 val, u64 *data, const struct word_at_a_time *c)
{
	u64 rhs = val | c->low_bits;
	*data = rhs;
	return (val + c->high_bits) & ~rhs;
}

#ifndef zero_bytemask
#define zero_bytemask(mask) (~1ul << __fls(mask))
#endif

#else

/*
 * The optimal byte mask counting is probably going to be something
 * that is architecture-specific. If you have a reliably fast
 * bit count instruction, that might be better than the multiply
 * and shift, for example.
 */
struct word_at_a_time {
	const u64 one_bits, high_bits;
};

#define WORD_AT_A_TIME_CONSTANTS { REPEAT_BYTE(0x01), REPEAT_BYTE(0x80) }

#ifdef CONFIG_64BIT

/*
 * Jan Achrenius on G+: microoptimized version of
 * the simpler "(mask & ONEBYTES) * ONEBYTES >> 56"
 * that works for the bytemasks without having to
 * mask them first.
 */
static inline s64 count_masked_bytes(u64 mask)
{
	return mask*0x0001020304050608ul >> 56;
}

#else	/* 32-bit case */

/* Carl Chatfield / Jan Achrenius G+ version for 32-bit */
static inline s64 count_masked_bytes(s64 mask)
{
	/* (000000 0000ff 00ffff ffffff) -> ( 1 1 2 3 ) */
	s64 a = (0x0ff0001+mask) >> 23;
	/* Fix the 1 for 00 case */
	return a & mask;
}

#endif

/* Return nonzero if it has a zero */
static inline u64 has_zero(u64 a, u64 *bits, const struct word_at_a_time *c)
{
	u64 mask = ((a - c->one_bits) & ~a) & c->high_bits;
	*bits = mask;
	return mask;
}

static inline u64 prep_zero_mask(u64 a, u64 bits, const struct word_at_a_time *c)
{
	return bits;
}

static inline u64 create_zero_mask(u64 bits)
{
	bits = (bits - 1) & ~bits;
	return bits >> 7;
}

/* The mask we created is directly usable as a bytemask */
#define zero_bytemask(mask) (mask)

static inline u64 find_zero(u64 mask)
{
	return count_masked_bytes(mask);
}

#endif /* __BIG_ENDIAN */
#endif /* !__ASM_GENERIC_WORD_AT_A_TIME_H_ */
