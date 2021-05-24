#ifndef __LINUX_SPINLOCK_TYPES_UP_H_
#define __LINUX_SPINLOCK_TYPES_UP_H_

#ifndef __LINUX_SPINLOCK_TYPES_H_
#error "please don't include this file directly"
#endif

typedef struct { } arch_spinlock_t;

#define __ARCH_SPIN_LOCK_UNLOCKED { }

typedef struct {
	/* no debug version on UP */
} arch_rwlock_t;

#define __ARCH_RW_LOCK_UNLOCKED { }

#endif /* __LINUX_SPINLOCK_TYPES_UP_H_ */
