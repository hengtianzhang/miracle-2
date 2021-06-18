/* internal.h: mm/ internal definitions
 *
 * Copyright (C) 2004 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#ifndef __MM_INTERNAL_H_
#define __MM_INTERNAL_H_

static inline u64
__find_buddy_pfn(u64 page_pfn, unsigned int order)
{
	return page_pfn ^ (1 << order);
}

/*
 * Turn a non-refcounted page (->_refcount == 0) into refcounted with
 * a count of one.
 */
static inline void set_page_refcounted(struct page *page)
{
	BUG_ON(PageTail(page));
	BUG_ON(page_ref_count(page));
	set_page_count(page, 1);
}

#endif /* !__MM_INTERNAL_H_ */
