/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_MM_TYPES_H_
#define __LINUX_MM_TYPES_H_

#include <linux/types.h>
#include <linux/rbtree.h>

#include <asm/mmu.h>

#ifdef CONFIG_HAVE_ALIGNED_STRUCT_PAGE
#define _struct_page_alignment	__aligned(2 * sizeof(u64))
#else
#define _struct_page_alignment
#endif

struct page {
	u64 flags;
} _struct_page_alignment;

/*
 * Used for sizing the vmemmap region on some architectures
 */
#define STRUCT_PAGE_MAX_SHIFT	(order_base_2(sizeof(struct page)))

struct vm_area_struct {
	u64 vm_start;
	u64 vm_end;

	struct vm_area_struct *vm_next, *vm_prev;

	struct rb_node vm_rb;

	struct mm_struct *vm_mm;
} __randomize_layout;

struct mm_struct {
	struct {
		struct vm_area_struct *mmap;
		struct rb_root mm_rb;

		/* Architecture-specific MM context */
		mm_context_t context;
	} __randomize_layout;
};

#endif /* !__LINUX_MM_TYPES_H_ */
