/* SPDX-License-Identifier: GPL-2.0 */
/* Atomic operations usable in machine independent code */
#ifndef __LINUX_ATOMIC_H_
#define __LINUX_ATOMIC_H_

typedef struct {
	s32 counter;
} atomic_t;

#ifdef CONFIG_64BIT
typedef struct {
	s64 counter;
} atomic64_t;
#endif

#endif /* !__LINUX_ATOMIC_H_ */
