/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_PAGE_FLAGS_LAYOUT_H_
#define __LINUX_PAGE_FLAGS_LAYOUT_H_

#include <generated/bounds.h>

/*
 * When a memory allocation must conform to specific limitations (such
 * as being suitable for DMA) the caller will pass in hints to the
 * allocator in the gfp_mask, in the zone modifier bits.  These bits
 * are used to select a priority ordered list of memory zones which
 * match the requested limits. See gfp_zone() in include/linux/gfp.h
 */
#if MAX_NR_ZONES < 2
#define ZONES_SHIFT 0
#elif MAX_NR_ZONES <= 2
#define ZONES_SHIFT 1
#elif MAX_NR_ZONES <= 4
#define ZONES_SHIFT 2
#elif MAX_NR_ZONES <= 8
#define ZONES_SHIFT 3
#else
#error ZONES_SHIFT -- too many zones configured adjust calculation
#endif

#define ZONES_WIDTH		ZONES_SHIFT

#if ZONES_WIDTH > BITS_PER_LONG - NR_PAGEFLAGS
#error "Vmemmap: No space for nodes field in page flags"
#endif

#endif /* !__LINUX_PAGE_FLAGS_LAYOUT_H_ */
