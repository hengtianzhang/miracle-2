/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_STACKPROTECTOR_H_
#define __LINUX_STACKPROTECTOR_H_

#include <linux/compiler.h>

#ifdef CONFIG_STACKPROTECTOR
# include <asm/stackprotector.h>
#else
static inline void boot_init_stack_canary(void)
{
}
#endif

#endif /* ~__LINUX_STACKPROTECTOR_H_ */
