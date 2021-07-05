


	__HEAD
_head:
	b	stext				// branch to kernel start, magic
	.long	0
	.quad	0				// reserved
	.quad	0				// reserved
	.quad	0				// reserved
	.ascii	ARM64_IMAGE_MAGIC		// Magic number
	.long	0				// reserved

	__INIT
	/*
	 * The following callee saved general purpose registers are used on the
	 * primary lowlevel boot path:
	 *
	 *  Register   Scope                      Purpose
	 *  x21        stext() .. start_kernel()  FDT pointer passed at boot in x0
	 *  x23        stext() .. start_kernel()  physical misalignment/KASLR offset
	 *  x28        __create_page_tables()     callee preserved temp register
	 *  x19/x20    __primary_switch()         callee preserved temp registers
	 */
ENTRY(stext)
	mov		x7, x4
	mov		x8, x5

    /* Make sure interrupts are disable */
    msr daifset, #DAIFSET_MASK

    /* Initialise sctlr_el1 register */
    msr     spsel, #1
	msr		x4, sctlr_el1

	/* tpidr_el1 has the logic ID of the core, starting from 0 */
    mrs     x6, tpidr_el1
	ldr		x5, =THREAD_SIZE
	mul		x5, x5, x6
	ldr		x4, =kernel_stack_alloc + THREAD_SIZE
	add		x4, x4, x5
	mov		sp, x4
    /* the kernel stack must be 4-KiB aligned since we use the
       lowest 12 bits to store the logical core ID. */
	orr		x6, x6, x4
    msr     tpidr_el1, x6

    /* Attempt to workaround any known ARM errata. */
    stp     x0, x1, [sp, #-16]!
    stp     x2, x3, [sp, #-16]!
    stp     x7, x8, [sp, #-16]!
    bl arm64_errata
    ldp     x4, x5, [sp], #16
    ldp     x2, x3, [sp], #16
    ldp     x0, x1, [sp], #16

    /* Call bootstrapping implemented in C */
    bl      init_kernel

    /* Restore the initial thread */
    b restore_user_context
ENDPROC(stext)
