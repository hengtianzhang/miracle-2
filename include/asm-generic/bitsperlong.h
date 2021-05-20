/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_BITSPERLONG_H_
#define __ASM_GENERIC_BITSPERLONG_H_

#include <uapi/asm-generic/bitsperlong.h>

#ifdef CONFIG_64BIT
#define BITS_PER_LONG 64
#else
#define BITS_PER_LONG 32
#endif

#if BITS_PER_LONG != __BITS_PER_LONG
#error Inconsistent word size. Check asm/bitsperlong.h
#endif

#ifndef BITS_PER_LONG_LONG
#define BITS_PER_LONG_LONG 64
#endif

#endif /* !__ASM_GENERIC_BITSPERLONG_H_ */
