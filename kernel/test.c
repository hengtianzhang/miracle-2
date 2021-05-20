
#include <linux/atomic.h>

__u64 a;
__u64 b;

atomic_t c;

__u64 test(void)
{
	atomic_read_acquire(&c);
	return a+b;
}