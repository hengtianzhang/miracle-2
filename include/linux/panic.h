#ifndef __LINUX_PANIC_H_
#define __LINUX_PANIC_H_

#include <linux/compiler.h>

__printf(1, 2)
void panic(const char *fmt, ...) __noreturn __cold;

void do_exit(long error_code) __noreturn;

#endif /* !__LINUX_PANIC_H_ */
