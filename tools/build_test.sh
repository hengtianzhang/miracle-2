#!/bin/sh

make defconfig
make 
make C=2
make includecheck

make CC=clang LD=ld.lld HOSTCC=clang HOSTLD=ld.lld defconfig
make CC=clang LD=ld.lld HOSTCC=clang HOSTLD=ld.lld
make CC=clang LD=ld.lld HOSTCC=clang HOSTLD=ld.lld C=2
make CC=clang LD=ld.lld HOSTCC=clang HOSTLD=ld.lld includecheck

make allyesconfig
make 
make C=2
make includecheck

make CC=clang LD=ld.lld HOSTCC=clang HOSTLD=ld.lld allyesconfig
make CC=clang LD=ld.lld HOSTCC=clang HOSTLD=ld.lld
make CC=clang LD=ld.lld HOSTCC=clang HOSTLD=ld.lld C=2
make CC=clang LD=ld.lld HOSTCC=clang HOSTLD=ld.lld includecheck

make allnoconfig
make
make C=2
make includecheck

make CC=clang LD=ld.lld HOSTCC=clang HOSTLD=ld.lld allnoconfig
make CC=clang LD=ld.lld HOSTCC=clang HOSTLD=ld.lld
make CC=clang LD=ld.lld HOSTCC=clang HOSTLD=ld.lld C=2
make CC=clang LD=ld.lld HOSTCC=clang HOSTLD=ld.lld includecheck
