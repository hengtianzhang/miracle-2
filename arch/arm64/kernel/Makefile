# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the linux kernel.
#

CPPFLAGS_vmlinux.lds	:= -DTEXT_OFFSET=$(TEXT_OFFSET)
AFLAGS_head.o		:= -DTEXT_OFFSET=$(TEXT_OFFSET)

# Object file lists.
obj-y		:= setup.o entry.o smp.o process.o

head-y					:= head.o
extra-y					+= $(head-y) vmlinux.lds
