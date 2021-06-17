/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_SMP_H_
#define __LINUX_SMP_H_

#include <asm/smp.h>

#define smp_processor_id() 0

void smp_setup_processor_id(void);

#endif /* !__LINUX_SMP_H_ */
