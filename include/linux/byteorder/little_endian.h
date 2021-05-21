/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_BYTEORDER_LITTLE_ENDIAN_H_
#define __LINUX_BYTEORDER_LITTLE_ENDIAN_H_

#include <uapi/linux/byteorder/little_endian.h>

#ifdef CONFIG_CPU_BIG_ENDIAN
#warning inconsistent configuration, CONFIG_CPU_BIG_ENDIAN is set
#endif

#include <linux/byteorder/generic.h>
#endif /* !__LINUX_BYTEORDER_LITTLE_ENDIAN_H_ */
