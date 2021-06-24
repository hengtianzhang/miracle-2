/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __LINUX_OF_H_
#define __LINUX_OF_H_

#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/errno.h>
#include <linux/list.h>

#include <asm/byteorder.h>

typedef u32 phandle;
typedef u32 ihandle;

struct property {
	char 	*name;
	int length;
	void *value;
	struct property *next;
};

struct device_node {
	const char *name;
	phandle	phandle;
	const char *full_name;

	struct	property *properties;
	struct	property *deadprops;	/* removed properties */
	struct	device_node *parent;
	struct	device_node *child;
	struct	device_node *sibling;

	u64 _flags;
	void	*data;
};

#define MAX_PHANDLE_ARGS 16
struct of_phandle_args {
	struct device_node *np;
	int args_count;
	uint32_t args[MAX_PHANDLE_ARGS];
};

#endif /* !__LINUX_OF_H_ */
