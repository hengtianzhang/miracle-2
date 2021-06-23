/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_DMA_DIRECTION_H_
#define __LINUX_DMA_DIRECTION_H_

#include <asm/page.h>

enum dma_data_direction {
	DMA_BIDIRECTIONAL = 0,
	DMA_TO_DEVICE = 1,
	DMA_FROM_DEVICE = 2,
	DMA_NONE = 3,
};

#ifndef MAX_DMA_ADDRESS
#define MAX_DMA_ADDRESS PAGE_OFFSET
#endif

#endif /* !__LINUX_DMA_DIRECTION_H_ */
