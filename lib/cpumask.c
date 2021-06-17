// SPDX-License-Identifier: GPL-2.0
//#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/bitops.h>
#include <linux/cpumask.h>
#include <linux/memblock.h>

/**
 * alloc_cpumask_var - allocate a struct cpumask
 * @mask: pointer to cpumask_var_t where the cpumask is returned
 * @flags: GFP_ flags
 *
 * Only defined when CONFIG_CPUMASK_OFFSTACK=y, otherwise is
 * a nop returning a constant 1 (in <linux/cpumask.h>)
 * Returns TRUE if memory allocation succeeded, FALSE otherwise.
 *
 * In addition, mask will be NULL if this fails.  Note that gcc is
 * usually smart enough to know that mask can never be NULL if
 * CONFIG_CPUMASK_OFFSTACK=n, so does code elimination in that case
 * too.
 */
bool alloc_cpumask_var(cpumask_t **mask, gfp_t flags)
{
#if 0
	*mask = kmalloc_node(cpumask_size(), flags, node);

	return *mask != NULL;
#endif
	return NULL;
}

bool zalloc_cpumask_var(cpumask_t **mask, gfp_t flags)
{
	//return alloc_cpumask_var(mask, flags | __GFP_ZERO);
	return false;
}

/**
 * free_cpumask_var - frees memory allocated for a struct cpumask.
 * @mask: cpumask to free
 *
 * This is safe on a NULL mask.
 */
void free_cpumask_var(cpumask_t *mask)
{
#if 0
	kfree(mask);
#endif
}
