/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_TYPES_H_
#define __LINUX_TYPES_H_

#include <linux/stddef.h>

#define __EXPORTED_HEADERS__
#include <uapi/linux/types.h>

#ifndef __ASSEMBLY__

typedef s64		pid_t;
typedef s64 	suseconds_t;
typedef s64		timer_t;
typedef s32		clockid_t;

typedef _Bool	bool;

typedef u64		uintptr_t;
typedef s64		ptrdiff_t;

typedef s64		loff_t;
typedef s64		time_t;
typedef s64		clock_t;

/* this is a special 64bit data type that is 8-byte aligned */
#define aligned_u64		__aligned_u64
#define aligned_be64		__aligned_be64
#define aligned_le64		__aligned_le64

/*
 * The type of an index into the pagecache.
 */
#define pgoff_t u64

#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
typedef u64 dma_addr_t;
#else
typedef u32 dma_addr_t;
#endif

#ifdef CONFIG_PHYS_ADDR_T_64BIT
typedef u64 phys_addr_t;
#else
typedef u32 phys_addr_t;
#endif

typedef u64 __bitwise gfp_t;

typedef phys_addr_t resource_size_t;

/*
 * This type is the placeholder for a hardware interrupt number. It has to be
 * big enough to enclose whatever representation is used by a given platform.
 */
typedef u64 irq_hw_number_t;

#endif /* !__ASSEMBLY__ */
#endif /* !__LINUX_TYPES_H_ */
