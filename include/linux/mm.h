/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_MM_H_
#define __LINUX_MM_H_

#include <linux/errno.h>

#ifdef __KERNEL__

#include <asm/page.h>

/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr) ALIGN(addr, PAGE_SIZE)

#endif /* __KERNEL__ */
#endif /* !__LINUX_MM_H_ */
