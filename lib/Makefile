# SPDX-License-Identifier: GPL-2.0
#
# Makefile for some libs needed in the kernel.
#
obj-y := ulib/
obj-$(CONFIG_GENERIC_ATOMIC64) += atomic64.o
obj-$(CONFIG_SMP) += cpumask.o

obj-y += dec_and_lock.o dump_stack.o kasprintf.o llist.o	\
		xarray.o radix-tree.o idr.o hexdump.o vsprintf.o	\
		string_helpers.o lcm.o gcd.o
