/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef __UAPI_LINUX_TYPES_H_
#define __UAPI_LINUX_TYPES_H_

#include <asm/types.h>

#ifndef __ASSEMBLY__
#ifndef	__KERNEL__
#ifndef __EXPORTED_HEADERS__
#warning "Attempt to use kernel headers from user space, see http://kernelnewbies.org/KernelHeaders"
#endif /* !__EXPORTED_HEADERS__ */
#endif

/*
 * Below are truly Linux-specific types that should never collide with
 * any application/library that wants linux/types.h.
 */

#ifdef __CHECKER__
#define __bitwise__ __attribute__((bitwise))
#else
#define __bitwise__
#endif
#define __bitwise __bitwise__

typedef __u16 __bitwise __le16;
typedef __u16 __bitwise __be16;
typedef __u32 __bitwise __le32;
typedef __u32 __bitwise __be32;
typedef __u64 __bitwise __le64;
typedef __u64 __bitwise __be64;

/*
 * aligned_u64 should be used in defining kernel<->userspace ABIs to avoid
 * common 32/64-bit compat problems.
 * 64-bit values align to 4-byte boundaries on x86_32 (and possibly other
 * architectures) and to 8-byte boundaries on 64-bit architectures.  The new
 * aligned_64 type enforces 8-byte alignment so that structs containing
 * aligned_64 values have the same alignment on 32-bit and 64-bit architectures.
 * No conversions are necessary between 32-bit user-space and a 64-bit kernel.
 */
#define __aligned_u64 __u64 __attribute__((aligned(8)))
#define __aligned_be64 __be64 __attribute__((aligned(8)))
#define __aligned_le64 __le64 __attribute__((aligned(8)))

typedef __s8  s8;
typedef __u8  u8;
typedef __s16 s16;
typedef __u16 u16;
typedef __s32 s32;
typedef __u32 u32;
typedef __s64 s64;
typedef __u64 u64;

#if __BITS_PER_LONG != 64
typedef u32		size_t;
typedef s32		ssize_t;
#else
typedef unsigned long		size_t;
typedef __signed__ long		ssize_t;
#endif

#endif /* !__ASSEMBLY__ */

#ifndef __ASSEMBLY__

#define S8_C(x)  x
#define U8_C(x)  x ## U
#define S16_C(x) x
#define U16_C(x) x ## U
#define S32_C(x) x
#define U32_C(x) x ## U
#define S64_C(x) x ## LL
#define U64_C(x) x ## ULL

#else

#define S8_C(x)  x
#define U8_C(x)  x
#define S16_C(x) x
#define U16_C(x) x
#define S32_C(x) x
#define U32_C(x) x
#define S64_C(x) x
#define U64_C(x) x

#endif /* !__ASSEMBLY__ */

#endif /* !__UAPI_LINUX_TYPES_H_ */
