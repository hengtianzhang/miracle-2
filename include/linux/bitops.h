/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_BITOPS_H_
#define __LINUX_BITOPS_H_

#include <linux/types.h>
#include <linux/bits.h>

#define BSTS_DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define BITS_PER_TYPE(type) (sizeof(type) * BITS_PER_BYTE)
#define BITS_TO_LONGS(nr)	BSTS_DIV_ROUND_UP(nr, BITS_PER_TYPE(s64))

extern unsigned int __sw_hweight8(unsigned int w);
extern unsigned int __sw_hweight16(unsigned int w);
extern unsigned int __sw_hweight32(unsigned int w);
extern u64 __sw_hweight64(__u64 w);

/*
 * Include this here because some architectures need generic_ffs/fls in
 * scope
 */
#include <asm/bitops.h>

#endif /* !__LINUX_BITOPS_H_ */
