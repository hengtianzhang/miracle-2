
void init_kernel_core()
{
	create_uptyped();

	init_hrtimer();
	init_sched();

	create_ipc_buffer();
	create_cnode();

	create_init_services();
	create_idle_services();
}
