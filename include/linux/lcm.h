/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_LCM_H_
#define __LINUX_LCM_H_

#include <linux/compiler.h>

u64 lcm(u64 a, u64 b) __attribute_const__;
u64 lcm_not_zero(u64 a, u64 b) __attribute_const__;

#endif /* !__LINUX_LCM_H_ */
