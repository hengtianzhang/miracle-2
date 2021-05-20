/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_LIST_H_
#define __LINUX_LIST_H_

struct list_head {
	struct list_head *next, *prev;
};

struct hlist_head {
	struct hlist_node *first;
};

struct hlist_node {
	struct hlist_node *next, **pprev;
};

#endif /* !__LINUX_LIST_H_ */
