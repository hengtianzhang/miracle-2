
static void release_secondary_cpus(void)
{

    /* release the cpus at the same time */
    secondary_cpu_boot_lock = 1;
}

static bool try_init_kernel(u64 v_initrd_entry,
					 u64 p_initrd_start,
					 u64 p_initrd_end,
					 u64 p_dtb_start,
					 u64 v_early_serial_compiable_offset,
					 u64 pv_offset)
{
	init_early_serial(v_early_serial_compiable_offset);

	prase_dtb(p_dtb_start); /* Get gic, uart, timer map info and memory info */

	map_kernel();
	map_device();
	map_memory();

	init_serial(); /* fixmap init */

	cpu_replace_ttbr1();
	cpu_replace_ttbr0();

	 /* initialise the platform */
    init_platform();

	init_cpu();

	init_gic();

	init_irq();
	init_time();

	init_kernel_core();

	release_secondary_cpus();

	return true;
}



volatile int secondary_cpu_boot_lock = 0;

static bool try_init_kernel_secondary_core(void)
{
	u32 i;

	while (!secondary_cpu_boot_lock)
		cpu_relax();

	/* Perform cpu init */
    init_cpu();

	init_irq();
	init_timer();
}




asmlinkage __visible void __init init_kernel(
				u64 v_initrd_entry,
				u64 p_initrd_start,
				u64 p_initrd_end,
				u64 p_dtb_start,
				u64 v_early_serial_compiable_offset,
				u64 pv_offset)
{
	bool result;

	if (smp_processor_id() == MAIN_CPU) {
		result = try_init_kernel(v_initrd_entry,
								 p_initrd_start,
								 p_initrd_end,
								 p_dtb_start,
								 v_early_serial_compiable_offset,
								 pv_offset);
	} else {
		result = try_init_kernel_secondary_core();
	}

	if (!result)
		panic("Kernel init failed for some reason :(\n");

	schedule();
}



