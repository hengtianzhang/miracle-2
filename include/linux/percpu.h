/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_PERCPU_H_
#define __LINUX_PERCPU_H_

#include <linux/threads.h>
#include <linux/cache.h>

#include <asm/percpu.h>

#define PERCPU_DYNAMIC_EARLY_SLOTS	128
#define PERCPU_DYNAMIC_EARLY_SIZE	(12 << 10)

#define alloc_percpu(type) NULL
#define free_percpu(data)

#define __alloc_percpu(size, align) NULL

#endif /* !__LINUX_PERCPU_H_ */
