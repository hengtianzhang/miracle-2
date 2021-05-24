#include <linux/percpu.h>
#include <linux/cache.h>

u64 __per_cpu_offset[NR_CPUS] __read_mostly;
