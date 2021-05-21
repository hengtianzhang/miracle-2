
#include <asm/daifflags.h>


int test(void)
{
	int flags;
	flags = local_daif_save();
	return 0;
}
