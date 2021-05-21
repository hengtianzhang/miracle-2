/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_BITMAP_H_
#define __LINUX_BITMAP_H_

#include <asm/bitsperlong.h>

#define BITMAP_FIRST_WORD_MASK(start) (~0ULL << ((start) & (BITS_PER_LONG - 1)))
#define BITMAP_LAST_WORD_MASK(nbits) (~0ULL >> (-(nbits) & (BITS_PER_LONG - 1)))

#endif /* !__LINUX_BITMAP_H_ */
