#ifndef __ASM_PAGE_H_
#define __ASM_PAGE_H_

#include <asm/page-def.h>

#ifndef __ASSEMBLY__

extern void copy_page(void *to, const void *from);
extern void clear_page(void *to);

#endif /* !__ASSEMBLY__ */

#include <asm-generic/getorder.h>

#endif /* !__ASM_PAGE_H_ */
