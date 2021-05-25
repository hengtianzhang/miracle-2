# SPDX-License-Identifier: GPL-2.0
#
# Makefile for some libs needed in the kernel.
#
lib-y := rbtree.o hweight.o find_bit.o ctype.o string.o	\
			bitmap.o

lib-$(CONFIG_CRC32)	+= crc32.o
lib-$(CONFIG_BITREVERSE) += bitrev.o

hostprogs-y	:= gen_crc32table
clean-files	:= crc32table.h

$(obj)/crc32.o: $(obj)/crc32table.h

quiet_cmd_crc32 = GEN     $@
      cmd_crc32 = $< > $@

$(obj)/crc32table.h: $(obj)/gen_crc32table
	$(call cmd,crc32)
