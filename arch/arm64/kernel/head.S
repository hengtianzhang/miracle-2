#include <linux/init.h>
#include <linux/linkage.h>

	__HEAD
_head:
	b	stext
	.long	0

	__INIT
ENTRY(stext)
	b test
ENDPROC(stext)
