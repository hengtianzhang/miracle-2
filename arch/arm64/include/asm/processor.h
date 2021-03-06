/*
 * Based on arch/arm/include/asm/processor.h
 *
 * Copyright (C) 1995-1999 Russell King
 * Copyright (C) 2012 ARM Ltd.
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
#ifndef __ASM_PROCESSOR_H_
#define __ASM_PROCESSOR_H_

#ifndef __ASSEMBLY__
#ifdef __KERNEL__

#include <linux/types.h>

static inline void cpu_relax(void)
{
	asm volatile("yield" ::: "memory");
}

/*
 * Prefetching support
 */
#define ARCH_HAS_PREFETCH
static inline void prefetch(const void *ptr)
{
	asm volatile("prfm pldl1keep, %a0\n" : : "p" (ptr));
}

#define ARCH_HAS_PREFETCHW
static inline void prefetchw(const void *ptr)
{
	asm volatile("prfm pstl1keep, %a0\n" : : "p" (ptr));
}

#define ARCH_HAS_SPINLOCK_PREFETCH
static inline void spin_lock_prefetch(const void *ptr)
{
	asm volatile("prfm pstl1strm, %a0"
		    : : "p" (ptr));
}

struct thread_struct {
	u64		fault_address;	/* fault info */
};

#endif /* __KERNEL__ */
#endif /* !__ASSEMBLY__ */
#endif /* !__ASM_PROCESSOR_H_ */
