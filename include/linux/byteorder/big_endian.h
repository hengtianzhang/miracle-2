/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_BYTEORDER_BIG_ENDIAN_H_
#define __LINUX_BYTEORDER_BIG_ENDIAN_H_

#include <uapi/linux/byteorder/big_endian.h>

#ifndef CONFIG_CPU_BIG_ENDIAN
#warning inconsistent configuration, needs CONFIG_CPU_BIG_ENDIAN
#endif

#include <linux/byteorder/generic.h>
#endif /* !__LINUX_BYTEORDER_BIG_ENDIAN_H_ */
