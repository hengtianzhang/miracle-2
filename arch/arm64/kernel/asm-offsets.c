/*
 * Based on arch/arm/kernel/asm-offsets.c
 *
 * Copyright (C) 1995-2003 Russell King
 *               2001-2002 Keith Owens
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/kbuild.h>

#define TSK_STACK_CANARY 1
#define VMA_VM_MM 3
#define MM_CONTEXT_ID 4

#define DMA_TO_DEVICE 0
#define DMA_FROM_DEVICE 1

int main(void)
{
#ifdef CONFIG_STACKPROTECTOR
	DEFINE(TSK_STACK_CANARY,	TSK_STACK_CANARY);
#endif
	DEFINE(VMA_VM_MM,		VMA_VM_MM);
	DEFINE(MM_CONTEXT_ID,		MM_CONTEXT_ID);
	DEFINE(DMA_TO_DEVICE,		DMA_TO_DEVICE);
	DEFINE(DMA_FROM_DEVICE,	DMA_FROM_DEVICE);
	return 0;
}
