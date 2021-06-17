/*
 *  linux/init/main.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  GK 2/5/95  -  Changed to support mounting root fs via NFS
 *  Added initrd & change_root: Werner Almesberger & Hans Lermen, Feb '96
 *  Moan early if gcc is old, avoiding bogus kernels - Paul Gortmaker, May '96
 *  Simplified starting of init:  Michael A. Griffith <grif@acm.org>
 */

#include <linux/kernel.h>
#include <linux/start_kernel.h>
#include <linux/linkage.h>
#include <linux/sched/task.h>
#include <linux/sched/task_stack.h>
#include <linux/irqflags.h>
#include <linux/cache.h>
#include <linux/smp.h>

/*
 * Debug helper: via this flag we know that we are in 'early bootup code'
 * where only the boot processor is running with IRQ disabled.  This means
 * two things - IRQ must not be enabled before the flag is cleared and some
 * operations which are not allowed with IRQ disabled are allowed while the
 * flag is set.
 */
bool early_boot_irqs_disabled __read_mostly;

enum system_states system_state __read_mostly;

void __init __weak smp_setup_processor_id(void)
{
}

asmlinkage __visible void __init start_kernel(void)
{
	system_state = SYSTEM_BOOTING;

	set_task_stack_end_magic(&init_task);
	smp_setup_processor_id();

	local_irq_disable();
	early_boot_irqs_disabled = true;

}
