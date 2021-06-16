# SPDX-License-Identifier: GPL-2.0
#
# Makefile for some libs needed in the kernel.
#
lib-y := rbtree.o hweight.o find_bit.o ctype.o string.o	\
			bitmap.o div64.o kstrtox.o

lib-$(CONFIG_CRC32)	+= crc32.o
lib-$(CONFIG_BITREVERSE) += bitrev.o

libfdt_files = fdt.o fdt_ro.o fdt_wip.o fdt_rw.o fdt_sw.o fdt_strerror.o \
	       fdt_empty_tree.o
$(foreach file, $(libfdt_files), \
	$(eval CFLAGS_$(file) = -I$(src)/../../scripts/dtc/libfdt))
lib-$(CONFIG_LIBFDT) += $(libfdt_files)

hostprogs-y	:= gen_crc32table
clean-files	:= crc32table.h

$(obj)/crc32.o: $(obj)/crc32table.h

quiet_cmd_crc32 = GEN     $@
      cmd_crc32 = $< > $@

$(obj)/crc32table.h: $(obj)/gen_crc32table
	$(call cmd,crc32)
