#ifndef __LINUX_PRINTK_H_
#define __LINUX_PRINTK_H_

#include <linux/compiler.h>
#include <linux/linkage.h>

extern const char linux_banner[];

asmlinkage __printf(1, 2) __cold
int printk(const char *fmt, ...);

#endif /* !__LINUX_PRINTK_H_ */
