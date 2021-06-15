// SPDX-License-Identifier: GPL-2.0
#include <linux/sched.h>
#include <linux/init_task.h>
#include <linux/sched/task.h>

#include <asm/thread_info.h>

/*
 * Set up the first task table, touch at your own risk!. Base=0,
 * limit=0x1fffff (=2MB)
 */
struct task_struct init_task
__init_task_data
= {
	.thread_info	= INIT_THREAD_INFO(init_task),
	.stack_refcount	= ATOMIC_INIT(1),
	.state		= 0,
	.stack		= init_stack,
	.usage		= ATOMIC_INIT(2),
};
