/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_BITOPS_BUILTIN___FLS_H_
#define __ASM_GENERIC_BITOPS_BUILTIN___FLS_H_

/**
 * __fls - find last (most-significant) set bit in a long word
 * @word: the word to search
 *
 * Undefined if no set bit exists, so code should check against 0 first.
 */
static __always_inline u64 __fls(u64 word)
{
	return (sizeof(word) * 8) - 1 - __builtin_clzl(word);
}

#endif /* !__ASM_GENERIC_BITOPS_BUILTIN___FLS_H_ */
