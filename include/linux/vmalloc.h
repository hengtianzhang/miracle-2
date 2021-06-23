/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_VMALLOC_H_
#define __LINUX_VMALLOC_H_

#include <linux/mm_types.h>
#include <linux/list.h>
#include <linux/llist.h>
#include <linux/types.h>
#include <linux/rbtree.h>
#include <linux/rcupdate.h>

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

struct vmap_area {
	u64 va_start;
	u64 va_end;
	u64 flags;
	struct rb_node rb_node;         /* address sorted rbtree */
	struct list_head list;          /* address sorted list */
	struct llist_node purge_list;    /* "lazy purge" list */
	struct vm_struct *vm;
	struct rcu_head rcu_head;
};

static inline size_t get_vm_area_size(const struct vm_struct *area)
{
	if (!(area->flags & VM_NO_GUARD))
		/* return actual size without guard page */
		return area->size - PAGE_SIZE;
	else
		return area->size;

}

static inline bool is_vmalloc_addr(const void *x)
{
	u64 addr = (u64)x;

	return addr >= VMALLOC_START && addr < VMALLOC_END;
}

extern void __init vmalloc_init(void);

/*
 *	Internals.  Dont't use..
 */
extern struct list_head vmap_area_list;
extern __init void vm_area_add_early(struct vm_struct *vm);
extern __init void vm_area_register_early(struct vm_struct *vm, size_t align);

struct vm_struct **pcpu_get_vm_areas(const u64 *offsets,
				     const size_t *sizes, int nr_vms,
				     size_t align);
void pcpu_free_vm_areas(struct vm_struct **vms, int nr_vms);

extern void *vmap(struct page **pages, unsigned int count,
			u64 flags, pgprot_t prot);
extern void vunmap(const void *addr);

void vfree(const void *addr);

extern void *vmalloc(u64 size);
extern void *vzalloc(u64 size);
extern void *__vmalloc(u64 size, gfp_t gfp_mask, pgprot_t prot);
extern void *__vmalloc_node_range(u64 size, u64 align,
			u64 start, u64 end, gfp_t gfp_mask,
			pgprot_t prot, u64 vm_flags,
			const void *caller);

extern void *__vmalloc_node_flags_caller(u64 size,
					gfp_t flags, void *caller);

extern int map_vm_area(struct vm_struct *area, pgprot_t prot,
			struct page **pages);

extern int map_kernel_range_noflush(u64 start, u64 size,
				    pgprot_t prot, struct page **pages);
extern void unmap_kernel_range_noflush(u64 addr, u64 size);
extern void unmap_kernel_range(u64 addr, u64 size);

extern struct vm_struct *get_vm_area(u64 size, u64 flags);
extern struct vm_struct *get_vm_area_caller(u64 size,
					u64 flags, const void *caller);
extern struct vm_struct *__get_vm_area(u64 size, u64 flags,
					u64 start, u64 end);
extern struct vm_struct *__get_vm_area_caller(u64 size,
					u64 flags,
					u64 start, u64 end,
					const void *caller);
extern void free_vm_area(struct vm_struct *area);
extern struct vm_struct *remove_vm_area(const void *addr);
extern struct vm_struct *find_vm_area(const void *addr);

extern void set_iounmap_nonlazy(void);

/* Support for virtually mapped pages */
struct page *vmalloc_to_page(const void *addr);
u64 vmalloc_to_pfn(const void *addr);

#define VMALLOC_TOTAL (VMALLOC_END - VMALLOC_START)

#endif /* !__LINUX_VMALLOC_H_ */
