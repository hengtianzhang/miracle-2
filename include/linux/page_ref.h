/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_PAGE_REF_H_
#define __LINUX_PAGE_REF_H_

#include <linux/atomic.h>
#include <linux/mm_types.h>
#include <linux/bug.h>

static inline int page_ref_count(struct page *page)
{
	return atomic_read(&page->_refcount);
}

static inline int page_count(struct page *page)
{
	return atomic_read(&compound_head(page)->_refcount);
}

static inline void set_page_count(struct page *page, int v)
{
	atomic_set(&page->_refcount, v);
}

/*
 * Setup the page count before being freed into the page allocator for
 * the first time (boot or memory hotplug)
 */
static inline void init_page_count(struct page *page)
{
	set_page_count(page, 1);
}

static inline void page_ref_add(struct page *page, int nr)
{
	atomic_add(nr, &page->_refcount);
}

static inline void page_ref_sub(struct page *page, int nr)
{
	atomic_sub(nr, &page->_refcount);
}

static inline void page_ref_inc(struct page *page)
{
	atomic_inc(&page->_refcount);
}

static inline void page_ref_dec(struct page *page)
{
	atomic_dec(&page->_refcount);
}

static inline int page_ref_sub_and_test(struct page *page, int nr)
{
	int ret = atomic_sub_and_test(nr, &page->_refcount);

	return ret;
}

static inline int page_ref_inc_return(struct page *page)
{
	int ret = atomic_inc_return(&page->_refcount);

	return ret;
}

static inline int page_ref_dec_and_test(struct page *page)
{
	int ret = atomic_dec_and_test(&page->_refcount);

	return ret;
}

static inline int page_ref_dec_return(struct page *page)
{
	int ret = atomic_dec_return(&page->_refcount);

	return ret;
}

static inline int page_ref_add_unless(struct page *page, int nr, int u)
{
	int ret = atomic_add_unless(&page->_refcount, nr, u);

	return ret;
}

static inline int page_ref_freeze(struct page *page, int count)
{
	int ret = likely(atomic_cmpxchg(&page->_refcount, count, 0) == count);

	return ret;
}

static inline void page_ref_unfreeze(struct page *page, int count)
{
	BUG_ON(page_count(page) != 0);
	BUG_ON(count == 0);

	atomic_set_release(&page->_refcount, count);
}

#endif /* !__LINUX_PAGE_REF_H_ */
