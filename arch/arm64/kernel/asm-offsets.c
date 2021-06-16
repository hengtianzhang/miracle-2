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
#include <linux/sched.h>
#include <linux/mm_types.h>
#include <linux/dma-direction.h>

#include <asm/smp.h>

int main(void)
{
#ifdef CONFIG_STACKPROTECTOR
	DEFINE(TSK_STACK_CANARY,	offsetof(struct task_struct, stack_canary));
#endif
	BLANK();
	DEFINE(VMA_VM_MM,		offsetof(struct vm_area_struct, vm_mm));
	BLANK();
	DEFINE(MM_CONTEXT_ID,		offsetof(struct mm_struct, context.id.counter));
	BLANK();
	DEFINE(DMA_BIDIRECTIONAL,	DMA_BIDIRECTIONAL);
	DEFINE(DMA_TO_DEVICE,		DMA_TO_DEVICE);
	DEFINE(DMA_FROM_DEVICE,	DMA_FROM_DEVICE);
	BLANK();
	DEFINE(CPU_BOOT_STACK,	offsetof(struct secondary_data, stack));
	DEFINE(CPU_BOOT_TASK,		offsetof(struct secondary_data, task));

	return 0;
}
