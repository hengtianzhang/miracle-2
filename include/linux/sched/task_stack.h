/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_SCHED_TASK_STACK_H_
#define __LINUX_SCHED_TASK_STACK_H_

/*
 * task->stack (kernel stack) handling interfaces:
 */

#include <linux/sched.h>
#include <linux/magic.h>


static inline u64 *end_of_stack(const struct task_struct *task)
{
	return task->stack;
}

extern void set_task_stack_end_magic(struct task_struct *tsk);

#endif /* !__LINUX_SCHED_TASK_STACK_H_ */
