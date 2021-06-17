/*
 * Generic helpers for smp ipi calls
 *
 * (C) Jens Axboe <jens.axboe@oracle.com> 2008
 */

#define pr_fmt(fmt) KBUILD_BASENAME ": " fmt

#include <linux/cache.h>
#include <linux/threads.h>
