#include <linux/compiler.h>
#include <linux/types.h>
__u64 a;
__u64 b;

__u64 test(void)
{
	return a+b;
}