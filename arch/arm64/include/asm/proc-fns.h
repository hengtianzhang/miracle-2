/*
 * Based on arch/arm/include/asm/proc-fns.h
 *
 * Copyright (C) 1997-1999 Russell King
 * Copyright (C) 2000 Deep Blue Solutions Ltd
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
#ifndef __ASM_PROCFNS_H_
#define __ASM_PROCFNS_H_

#ifdef __KERNEL__
#ifndef __ASSEMBLY__

#include <linux/types.h>

#include <asm/page.h>

struct mm_struct;
extern void cpu_do_idle(void);
extern void cpu_do_switch_mm(u64 pgd_phys, struct mm_struct *mm);

#include <asm/memory.h>

#endif /* !__ASSEMBLY__ */
#endif /* __KERNEL__ */
#endif /* !__ASM_PROCFNS_H_ */