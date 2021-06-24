/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_MODULE_PARAMS_H_
#define __LINUX_MODULE_PARAMS_H_
/* (C) Copyright 2001, 2002 Rusty Russell IBM Corporation */
#include <linux/init.h>
#include <linux/stringify.h>
#include <linux/kernel.h>

struct kernel_param {

};

/**
 * parameq - checks if two parameter names match
 * @name1: parameter name 1
 * @name2: parameter name 2
 *
 * Returns true if the two parameter names are equal.
 * Dashes (-) are considered equal to underscores (_).
 */
extern bool parameq(const char *name1, const char *name2);

extern char *parse_args(const char *name,
		      char *args,
		      const struct kernel_param *params,
		      unsigned num,
		      s16 level_min,
		      s16 level_max,
		      void *arg,
		      int (*unknown)(char *param, char *val,
				     const char *doing, void *arg));
#endif /* !__LINUX_MODULE_PARAMS_H_ */
