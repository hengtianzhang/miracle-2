/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Mutexes: blocking mutual exclusion locks
 *
 * started by Ingo Molnar:
 *
 *  Copyright (C) 2004, 2005, 2006 Red Hat, Inc., Ingo Molnar <mingo@redhat.com>
 *
 * This file contains the main data structure and API definitions.
 */
#ifndef __LINUX_MUTEX_H_
#define __LINUX_MUTEX_H_

#include <linux/atomic.h>
#include <linux/spinlock.h>

struct mutex {
	atomic_long_t		owner;
	spinlock_t		wait_lock;

	struct list_head	wait_list;
};

#define mutex_init(mutex)						\
do {									\
	mutex;								\
} while (0)

#define __MUTEX_INITIALIZER(lockname) \
		{ .owner = ATOMIC_LONG_INIT(0) \
		, .wait_lock = __SPIN_LOCK_UNLOCKED(lockname.wait_lock) \
		, .wait_list = LIST_HEAD_INIT(lockname.wait_list) }

#define DEFINE_MUTEX(mutexname) \
	struct mutex mutexname = __MUTEX_INITIALIZER(mutexname)

static inline void mutex_lock(struct mutex *lock)
{
}

static inline int mutex_trylock(struct mutex *lock)
{
	return 1;
}

static inline void mutex_unlock(struct mutex *lock)
{
}

#endif /* !__LINUX_MUTEX_H_ */
