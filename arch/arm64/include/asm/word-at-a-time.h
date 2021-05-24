/*
 * Copyright (C) 2013 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __ASM_WORD_AT_A_TIME_H_
#define __ASM_WORD_AT_A_TIME_H_

#ifndef __AARCH64EB__

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/bitops.h>

struct word_at_a_time {
	const u64 one_bits, high_bits;
};

#define WORD_AT_A_TIME_CONSTANTS { REPEAT_BYTE(0x01), REPEAT_BYTE(0x80) }

static inline u64 has_zero(u64 a, u64 *bits,
				     const struct word_at_a_time *c)
{
	u64 mask = ((a - c->one_bits) & ~a) & c->high_bits;
	*bits = mask;
	return mask;
}

#define prep_zero_mask(a, bits, c) (bits)

static inline u64 create_zero_mask(u64 bits)
{
	bits = (bits - 1) & ~bits;
	return bits >> 7;
}

static inline u64 find_zero(u64 mask)
{
	return fls64(mask) >> 3;
}

#define zero_bytemask(mask) (mask)

#else	/* __AARCH64EB__ */
#include <asm-generic/word-at-a-time.h>
#endif

#endif /* !__ASM_WORD_AT_A_TIME_H_ */
