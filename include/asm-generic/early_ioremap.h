/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_EARLY_IOREMAP_H_
#define __ASM_EARLY_IOREMAP_H_

#include <linux/types.h>

extern pgprot_t early_memremap_pgprot_adjust(resource_size_t phys_addr,
						    u64 size,
						    pgprot_t prot);

extern void early_ioremap_shutdown(void);

extern void early_ioremap_reset(void);

extern void early_ioremap_setup(void);

extern void early_ioremap_init(void);

extern void early_iounmap(void __iomem *addr, u64 size);
/* Remap an IO device */
extern void __iomem *
early_ioremap(resource_size_t phys_addr, u64 size);

#endif /* !__ASM_EARLY_IOREMAP_H_ */
