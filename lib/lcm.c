#include <linux/compiler.h>
#include <linux/gcd.h>
#include <linux/lcm.h>

/* Lowest common multiple */
u64 lcm(u64 a, u64 b)
{
	if (a && b)
		return (a / gcd(a, b)) * b;
	else
		return 0;
}

u64 lcm_not_zero(u64 a, u64 b)
{
	u64 l = lcm(a, b);

	if (l)
		return l;

	return (b ? : a);
}
