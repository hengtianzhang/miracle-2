/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_BITOPS_BUILTIN___FFS_H_
#define __ASM_GENERIC_BITOPS_BUILTIN___FFS_H_

/**
 * __ffs - find first bit in word.
 * @word: The word to search
 *
 * Undefined if no bit exists, so code should check against 0 first.
 */
static __always_inline u64 __ffs(u64 word)
{
	return __builtin_ctzl(word);
}

#endif /* !__ASM_GENERIC_BITOPS_BUILTIN___FFS_H_ */
