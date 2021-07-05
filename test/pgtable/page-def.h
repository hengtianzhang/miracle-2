/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Based on arch/arm/include/asm/page.h
 *
 * Copyright (C) 1995-2003 Russell King
 * Copyright (C) 2017 ARM Ltd.
 */
#ifndef __ASM_PAGE_DEF_H_
#define __ASM_PAGE_DEF_H_

#include <linux/const.h>

/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT		CONFIG_ARM64_PAGE_SHIFT
#define PAGE_SIZE		(_AC(1, ULL) << PAGE_SHIFT)
#define PAGE_MASK		(~(PAGE_SIZE - 1))

#endif /* !__ASM_PAGE_DEF_H_ */
