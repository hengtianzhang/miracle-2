/*
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
#ifndef __ASM_MMU_H_
#define __ASM_MMU_H_

#include <linux/atomic.h>
#include <linux/types.h>

#ifndef __ASSEMBLY__

struct mm_struct;

typedef struct {
	atomic64_t id;
	u64		flags;
} mm_context_t;

/*
 * This macro is only used by the TLBI code, which cannot race with an
 * ASID change and therefore doesn't need to reload the counter using
 * atomic64_read.
 */
#define ASID(mm)	((mm)->context.id.counter & 0xffff)

#define INIT_MM_CONTEXT(name)	\
	.pgd = init_pg_dir,

extern void paging_init(void);
extern void mark_rodata_ro(void);
extern void mark_linear_text_alias_ro(void);
extern void create_pgd_mapping(struct mm_struct *mm, phys_addr_t phys,
			       u64 virt, phys_addr_t size,
			       pgprot_t prot, bool page_mappings_only);

#endif /* !__ASSEMBLY__ */
#endif /* !__ASM_MMU_H_ */
