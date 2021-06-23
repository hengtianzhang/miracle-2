/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_SMP_H_
#define __LINUX_SMP_H_

#include <asm/smp.h>

#define smp_processor_id() 0

void smp_setup_processor_id(void);

extern int __boot_cpu_id;

static inline int get_boot_cpu_id(void)
{
	return __boot_cpu_id;
}

#endif /* !__LINUX_SMP_H_ */
