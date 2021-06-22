#include <linux/gfp.h>
#include <linux/kernel.h>
#include <linux/slab.h>

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
