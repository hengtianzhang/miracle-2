/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_VMALLOC_H_
#define __LINUX_VMALLOC_H_

#include <linux/mm_types.h>
#include <linux/list.h>
#include <linux/llist.h>
#include <linux/types.h>
#include <linux/rbtree.h>

#include <asm/page.h>

/* bits in flags of vmalloc's vm_struct below */
#define VM_IOREMAP		0x00000001	/* ioremap() and friends */
#define VM_ALLOC		0x00000002	/* vmalloc() */
#define VM_MAP			0x00000004	/* vmap()ed pages */
#define VM_USERMAP		0x00000008	/* suitable for remap_vmalloc_range */
#define VM_UNINITIALIZED	0x00000020	/* vm_struct is not fully initialized */

struct vm_struct {
	struct vm_struct	*next;
	void			*addr;
	u64 			size;
	u64				flags;
	struct page		**pages;
	unsigned int		nr_pages;
	phys_addr_t		phys_addr;
	const void		*caller;
};

struct vmap_area {
	u64 va_start;
	u64 va_end;
	u64 flags;
	struct rb_node rb_node;         /* address sorted rbtree */
	struct list_head list;          /* address sorted list */
	struct llist_node purge_list;    /* "lazy purge" list */
	struct vm_struct *vm;
};

/*
 *	Internals.  Dont't use..
 */
extern struct list_head vmap_area_list;
extern __init void vm_area_add_early(struct vm_struct *vm);

#endif /* !__LINUX_VMALLOC_H_ */
