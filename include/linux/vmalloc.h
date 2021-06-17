/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_VMALLOC_H_
#define __LINUX_VMALLOC_H_

#include <linux/mm_types.h>
#include <linux/list.h>
#include <linux/types.h>

#include <asm/page.h>

/* bits in flags of vmalloc's vm_struct below */
#define VM_IOREMAP		0x00000001	/* ioremap() and friends */
#define VM_ALLOC		0x00000002	/* vmalloc() */
#define VM_MAP			0x00000004	/* vmap()ed pages */
#define VM_USERMAP		0x00000008	/* suitable for remap_vmalloc_range */
#define VM_UNINITIALIZED	0x00000020	/* vm_struct is not fully initialized */
#define VM_NO_GUARD		0x00000040      /* don't add guard page */

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

/*
 *	Internals.  Dont't use..
 */
extern struct list_head vmap_area_list;
extern __init void vm_area_add_early(struct vm_struct *vm);

#endif /* !__LINUX_VMALLOC_H_ */
