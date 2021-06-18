/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_PERCPU_H_
#define __LINUX_PERCPU_H_

#include <linux/threads.h>
#include <linux/cache.h>

#include <asm/percpu.h>

#define alloc_percpu(type) NULL
#define free_percpu(data)

#endif /* !__LINUX_PERCPU_H_ */
