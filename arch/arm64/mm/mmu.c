/*
 * Based on arch/arm/mm/mmu.c
 *
 * Copyright (C) 1995-2005 Russell King
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

#include <linux/types.h>
#include <linux/cache.h>
#include <linux/page_aligned.h>

#include <asm/tlbflush.h>
#include <asm/pgtable.h>
#include <asm/kernel-pgtable.h>
#include <asm/page.h>
#include <asm/mmu_context.h>

#define NO_BLOCK_MAPPINGS	BIT(0)
#define NO_CONT_MAPPINGS	BIT(1)

u64 idmap_t0sz = TCR_T0SZ(VA_BITS);
u64 idmap_ptrs_per_pgd = PTRS_PER_PGD;
u64 vabits_user __ro_after_init;

u64 kimage_voffset __ro_after_init;

/*
 * Empty_zero_page is a special page that is used for zero-initialized data
 * and COW.
 */
u64 empty_zero_page[PAGE_SIZE / sizeof(u64)] __page_aligned_bss;
