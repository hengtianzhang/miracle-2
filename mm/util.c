#include <linux/gfp.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>

#include <asm/sections.h>

/**
 * kstrdup - allocate space for and copy an existing string
 * @s: the string to duplicate
 * @gfp: the GFP mask used in the kmalloc() call when allocating memory
 */
char *kstrdup(const char *s, gfp_t gfp)
{
	size_t len;
	char *buf;

	if (!s)
		return NULL;

	len = strlen(s) + 1;
	buf = kmalloc_track_caller(len, gfp);
	if (buf)
		memcpy(buf, s, len);
	return buf;
}

const char *kstrdup_const(const char *s, gfp_t gfp)
{
	if (is_kernel_rodata((u64)s))
		return s;

	return kstrdup(s, gfp);
}

void kfree_const(const void *x)
{
	if (!is_kernel_rodata((u64)x))
		kfree(x);
}

void kvfree(const void *addr)
{
	if (is_vmalloc_addr(addr))
		vfree(addr);
	else
		kfree(addr);
}

void *kvmalloc(size_t size, gfp_t flags)
{
	gfp_t kmalloc_flags = flags;
	void *ret;

	/*
	 * vmalloc uses GFP_KERNEL for some internal allocations (e.g page tables)
	 * so the given set of flags has to be compatible.
	 */
	if ((flags & GFP_KERNEL) != GFP_KERNEL)
		return kmalloc(size, flags);

	/*
	 * We want to attempt a large physically contiguous block first because
	 * it is less likely to fragment multiple larger blocks and therefore
	 * contribute to a s64 term fragmentation less than vmalloc fallback.
	 * However make sure that larger requests are not too disruptive - no
	 * OOM killer and no allocation failure warnings as we have a fallback.
	 */
	if (size > PAGE_SIZE)
		kmalloc_flags |= __GFP_NOWARN;

	ret = kmalloc(size, kmalloc_flags);

	/*
	 * It doesn't really make sense to fallback to vmalloc for sub page
	 * requests
	 */
	if (ret || size <= PAGE_SIZE)
		return ret;

	return __vmalloc_node_flags_caller(size, flags,
			__builtin_return_address(0));
}
