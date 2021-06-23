/* SPDX-License-Identifier: GPL-2.0 */
/*
 * workqueue.h --- work queue handling for Linux.
 */

#ifndef __LINUX_WORKQUEUE_H_
#define __LINUX_WORKQUEUE_H_

#include <linux/atomic.h>
#include <linux/types.h>
#include <linux/list.h>

struct work_struct;
typedef void (*work_func_t)(struct work_struct *work);

struct work_struct {
	atomic_long_t data;
	struct list_head entry;
	work_func_t func;
};

/**
 * schedule_work - put work task in global workqueue
 * @work: job to be done
 *
 * Returns %false if @work was already on the kernel-global workqueue and
 * %true otherwise.
 *
 * This puts a job in the kernel-global workqueue if it was not already
 * queued and leaves it in the same position on the kernel-global
 * workqueue otherwise.
 */
static inline bool schedule_work(struct work_struct *work)
{
	//return queue_work(system_wq, work);
	WARN_ON(1);

	return true;
}

#define WORK_DATA_INIT()	ATOMIC_LONG_INIT((u64)0)

static inline void __init_work(struct work_struct *work, int onstack) { }

#define __INIT_WORK(_work, _func, _onstack)				\
	do {								\
		__init_work((_work), _onstack);				\
		(_work)->data = (atomic_long_t) WORK_DATA_INIT();	\
		INIT_LIST_HEAD(&(_work)->entry);			\
		(_work)->func = (_func);				\
	} while (0)

#define INIT_WORK(_work, _func)						\
	__INIT_WORK((_work), (_func), 0)

#endif /* !__LINUX_WORKQUEUE_H_ */
