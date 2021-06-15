/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX__INIT_TASK_H_
#define __LINUX__INIT_TASK_H_

#include <asm/thread_info.h>

#define INIT_TASK_COMM "swapper"

/* Attach to the init_task data structure for proper alignment */
#define __init_task_data __attribute__((__section__(".data..init_task")))

/* Attach to the thread_info data structure for proper alignment */
#define __init_thread_info __attribute__((__section__(".data..init_thread_info")))

#endif /* !__LINUX__INIT_TASK_H_ */
