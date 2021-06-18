// SPDX-License-Identifier: GPL-2.0
/*
 * linux/mm/mmzone.c
 *
 * management codes for pgdats, zones and page flags
 */

#include <linux/stddef.h>
#include <linux/mm.h>
#include <linux/mmzone.h>

struct pglist_data *first_online_pgdat(void)
{
	return NODE_DATA();
}

struct pglist_data *next_online_pgdat(struct pglist_data *pgdat)
{
	return NULL;
}

/*
 * next_zone - helper magic for for_each_zone()
 */
struct zone *next_zone(struct zone *zone)
{
	pg_data_t *pgdat = zone->zone_pgdat;

	if (zone < pgdat->node_zones + MAX_NR_ZONES - 1)
		zone++;
	else {
		pgdat = next_online_pgdat(pgdat);
		if (pgdat)
			zone = pgdat->node_zones;
		else
			zone = NULL;
	}
	return zone;
}
