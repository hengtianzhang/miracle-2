/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_START_KERNEL_H_
#define __LINUX_START_KERNEL_H_

#include <linux/linkage.h>
#include <linux/init.h>

extern bool early_boot_irqs_disabled;

/* Define the prototype for start_kernel here, rather than cluttering
   up something else. */

extern asmlinkage void __init start_kernel(void);

#endif /* !__LINUX_START_KERNEL_H_ */
