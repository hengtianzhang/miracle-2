#ifndef __ASM_PAGE_H_
#define __ASM_PAGE_H_

#include <asm/page-def.h>

#ifndef __ASSEMBLY__

extern void copy_page(void *to, const void *from);
extern void clear_page(void *to);

extern int pfn_valid(u64 pfn);

#include <asm/memory.h>

#endif /* !__ASSEMBLY__ */

#include <asm-generic/getorder.h>

#endif /* !__ASM_PAGE_H_ */
