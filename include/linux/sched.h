/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_SCHED_H_
#define __LINUX_SCHED_H_

#include <linux/atomic.h>

#include <asm/thread_info.h>

/*
 * Define 'struct task_struct' and provide the main scheduler
 * APIs (schedule(), wakeup variants, etc.)
 */

/* Task command name length: */
#define TASK_COMM_LEN			16

struct task_struct {
	/*
	 * For reasons of header soup (see current_thread_info()), this
	 * must be the first element of task_struct.
	 */
	struct thread_info		thread_info;

	/* -1 unrunnable, 0 runnable, >0 stopped: */
	volatile s64			state;

	/*
	 * This begins the randomizable portion of task_struct. Only
	 * scheduling-critical items should be added above here.
	 */
	randomized_struct_fields_start

	void				*stack;
	atomic_t			usage;

	/*
	 * executable name, excluding path.
	 *
	 * - normally initialized setup_new_exec()
	 * - access it with [gs]et_task_comm()
	 * - lock it with task_lock()
	 */
	char				comm[TASK_COMM_LEN];

	/* A live task holds one reference: */
	atomic_t			stack_refcount;

	/*
	 * New fields for task_struct should be added above here, so that
	 * they are included in the randomized portion of task_struct.
	 */
	randomized_struct_fields_end

	/* CPU-specific state of this task: */
	struct thread_struct		thread;
};

extern u64 init_stack[THREAD_SIZE / sizeof(u64)];

#endif /* !__LINUX_SCHED_H_ */
