/* CPU control.
 * (C) 2001, 2002, 2003, 2004 Rusty Russell
 *
 * This code is licenced under the GPL.
 */

#include <linux/cpumask.h>
#include <linux/cache.h>

struct cpumask __cpu_possible_mask __read_mostly;
struct cpumask __cpu_online_mask __read_mostly;
struct cpumask __cpu_present_mask __read_mostly;
struct cpumask __cpu_active_mask __read_mostly;

/* Setup number of possible processor ids */
unsigned int nr_possible_cpu_ids __read_mostly = NR_CPUS;
unsigned int nr_online_cpu_ids;
unsigned int nr_present_cpu_ids;
unsigned int nr_active_cpu_ids;

const DECLARE_BITMAP(cpu_all_bits, NR_CPUS) = CPU_BITS_ALL;
