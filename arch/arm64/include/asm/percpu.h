/*
 * Copyright (C) 2013 ARM Ltd.
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
#ifndef __ASM_PERCPU_H_
#define __ASM_PERCPU_H_

#include <linux/compiler.h>
#include <linux/types.h>

#include <asm/stack_pointer.h>

static inline void set_my_cpu_offset(u64 off)
{
	asm volatile("msr tpidr_el1, %0"
			:: "r" (off) : "memory");
}

static inline u64 __my_cpu_offset(void)
{
	u64 off;

	/*
	 * We want to allow caching the value, so avoid using volatile and
	 * instead use a fake stack read to hazard against barrier().
	 */
	asm("mrs %0, tpidr_el1"
		: "=r" (off) :
		"Q" (*(const u64 *)current_stack_pointer));

	return off;
}

#define __my_cpu_offset __my_cpu_offset()

#define PERCPU_RW_OPS(sz)				\
static inline u64 __percpu_read_##sz(void *ptr)	\
{										\
	return READ_ONCE(*(u##sz *)ptr);			\
}											\
												\
static inline void __percpu_write_##sz(void *ptr, u64 val)	\
{										\
	WRITE_ONCE(*(u##sz *)ptr, (u##sz)val);				\
}

#define __PERCPU_OP_CASE(w, sfx, name, sz, op_llsc)		\
static inline void							\
__percpu_##name##_case_##sz(void *ptr, u64 val)		\
{									\
	unsigned int loop;						\
	u##sz tmp;							\
									\
	asm volatile (								\
	/* LL/SC */							\
	"1:	ldxr" #sfx "\t%" #w "[tmp], %[ptr]\n"			\
		#op_llsc "\t%" #w "[tmp], %" #w "[tmp], %" #w "[val]\n"	\
	"	stxr" #sfx "\t%w[loop], %" #w "[tmp], %[ptr]\n"		\
	"	cbnz	%w[loop], 1b"					\
	: [loop] "=&r" (loop), [tmp] "=&r" (tmp),			\
	  [ptr] "+Q"(*(u##sz *)ptr)					\
	: [val] "r" ((u##sz)(val)));					\
}

#define __PERCPU_RET_OP_CASE(w, sfx, name, sz, op_llsc)		\
static inline u##sz							\
__percpu_##name##_return_case_##sz(void *ptr, u64 val)	\
{									\
	unsigned int loop;						\
	u##sz ret;							\
									\
	asm volatile (								\
	/* LL/SC */							\
	"1:	ldxr" #sfx "\t%" #w "[ret], %[ptr]\n"			\
		#op_llsc "\t%" #w "[ret], %" #w "[ret], %" #w "[val]\n"	\
	"	stxr" #sfx "\t%w[loop], %" #w "[ret], %[ptr]\n"		\
	"	cbnz	%w[loop], 1b"					\
	: [loop] "=&r" (loop), [ret] "=&r" (ret),			\
	  [ptr] "+Q"(*(u##sz *)ptr)					\
	: [val] "r" ((u##sz)(val)));					\
									\
	return ret;							\
}

#define PERCPU_OP(name, op_llsc)				\
	__PERCPU_OP_CASE(w, b, name,  8, op_llsc)		\
	__PERCPU_OP_CASE(w, h, name, 16, op_llsc)		\
	__PERCPU_OP_CASE(w,  , name, 32, op_llsc)		\
	__PERCPU_OP_CASE( ,  , name, 64, op_llsc)

#define PERCPU_RET_OP(name, op_llsc)				\
	__PERCPU_RET_OP_CASE(w, b, name,  8, op_llsc)		\
	__PERCPU_RET_OP_CASE(w, h, name, 16, op_llsc)		\
	__PERCPU_RET_OP_CASE(w,  , name, 32, op_llsc)		\
	__PERCPU_RET_OP_CASE( ,  , name, 64, op_llsc)

PERCPU_RW_OPS(8)
PERCPU_RW_OPS(16)
PERCPU_RW_OPS(32)
PERCPU_RW_OPS(64)
PERCPU_OP(add, add)
PERCPU_OP(andnot, bic)
PERCPU_OP(or, orr)
PERCPU_RET_OP(add, add)

#undef PERCPU_RW_OPS
#undef __PERCPU_OP_CASE
#undef __PERCPU_RET_OP_CASE
#undef PERCPU_OP
#undef PERCPU_RET_OP

#define this_cpu_cmpxchg_double_8(ptr1, ptr2, o1, o2, n1, n2)		\
({									\
	int __ret;							\
	__ret = cmpxchg_double_local(	this_cpu_ptr(&(ptr1)),		\
					this_cpu_ptr(&(ptr2)),		\
					o1, o2, n1, n2);		\
	__ret;								\
})

#define _pcp_protect(op, pcp, ...)					\
({									\
	op(this_cpu_ptr(&(pcp)), __VA_ARGS__);				\
})

#define _pcp_protect_return(op, pcp, args...)				\
({									\
	typeof(pcp) __retval;						\
	__retval = (typeof(pcp))op(this_cpu_ptr(&(pcp)), ##args);	\
	__retval;							\
})

#define this_cpu_read_1(pcp)		\
	_pcp_protect_return(__percpu_read_8, pcp)
#define this_cpu_read_2(pcp)		\
	_pcp_protect_return(__percpu_read_16, pcp)
#define this_cpu_read_4(pcp)		\
	_pcp_protect_return(__percpu_read_32, pcp)
#define this_cpu_read_8(pcp)		\
	_pcp_protect_return(__percpu_read_64, pcp)

#define this_cpu_write_1(pcp, val)	\
	_pcp_protect(__percpu_write_8, pcp, (u64)val)
#define this_cpu_write_2(pcp, val)	\
	_pcp_protect(__percpu_write_16, pcp, (u64)val)
#define this_cpu_write_4(pcp, val)	\
	_pcp_protect(__percpu_write_32, pcp, (u64)val)
#define this_cpu_write_8(pcp, val)	\
	_pcp_protect(__percpu_write_64, pcp, (u64)val)

#define this_cpu_add_1(pcp, val)	\
	_pcp_protect(__percpu_add_case_8, pcp, val)
#define this_cpu_add_2(pcp, val)	\
	_pcp_protect(__percpu_add_case_16, pcp, val)
#define this_cpu_add_4(pcp, val)	\
	_pcp_protect(__percpu_add_case_32, pcp, val)
#define this_cpu_add_8(pcp, val)	\
	_pcp_protect(__percpu_add_case_64, pcp, val)

#define this_cpu_add_return_1(pcp, val)	\
	_pcp_protect_return(__percpu_add_return_case_8, pcp, val)
#define this_cpu_add_return_2(pcp, val)	\
	_pcp_protect_return(__percpu_add_return_case_16, pcp, val)
#define this_cpu_add_return_4(pcp, val)	\
	_pcp_protect_return(__percpu_add_return_case_32, pcp, val)
#define this_cpu_add_return_8(pcp, val)	\
	_pcp_protect_return(__percpu_add_return_case_64, pcp, val)

#define this_cpu_and_1(pcp, val)	\
	_pcp_protect(__percpu_andnot_case_8, pcp, ~val)
#define this_cpu_and_2(pcp, val)	\
	_pcp_protect(__percpu_andnot_case_16, pcp, ~val)
#define this_cpu_and_4(pcp, val)	\
	_pcp_protect(__percpu_andnot_case_32, pcp, ~val)
#define this_cpu_and_8(pcp, val)	\
	_pcp_protect(__percpu_andnot_case_64, pcp, ~val)

#define this_cpu_or_1(pcp, val)		\
	_pcp_protect(__percpu_or_case_8, pcp, val)
#define this_cpu_or_2(pcp, val)		\
	_pcp_protect(__percpu_or_case_16, pcp, val)
#define this_cpu_or_4(pcp, val)		\
	_pcp_protect(__percpu_or_case_32, pcp, val)
#define this_cpu_or_8(pcp, val)		\
	_pcp_protect(__percpu_or_case_64, pcp, val)

#define this_cpu_xchg_1(pcp, val)	\
	_pcp_protect_return(xchg_relaxed, pcp, val)
#define this_cpu_xchg_2(pcp, val)	\
	_pcp_protect_return(xchg_relaxed, pcp, val)
#define this_cpu_xchg_4(pcp, val)	\
	_pcp_protect_return(xchg_relaxed, pcp, val)
#define this_cpu_xchg_8(pcp, val)	\
	_pcp_protect_return(xchg_relaxed, pcp, val)

#define this_cpu_cmpxchg_1(pcp, o, n)	\
	_pcp_protect_return(cmpxchg_relaxed, pcp, o, n)
#define this_cpu_cmpxchg_2(pcp, o, n)	\
	_pcp_protect_return(cmpxchg_relaxed, pcp, o, n)
#define this_cpu_cmpxchg_4(pcp, o, n)	\
	_pcp_protect_return(cmpxchg_relaxed, pcp, o, n)
#define this_cpu_cmpxchg_8(pcp, o, n)	\
	_pcp_protect_return(cmpxchg_relaxed, pcp, o, n)

#include <asm-generic/percpu-defs.h>

#endif /* !__ASM_PERCPU_H_ */
