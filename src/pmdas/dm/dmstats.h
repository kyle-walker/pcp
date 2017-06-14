/*
 * Device Mapper PMDA - Cache (dm-cache) Stats
 *
 * Copyright (c) 2017 Fumiya Shigemitsu.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */
#define HAVE_DMSTATS 1

#ifdef HAVE_DMSTATS

#ifndef DM_H
#define DM_H

struct pm_dm_stats_counter {
	uint64_t pm_reads;		    /* Num reads completed */
	uint64_t pm_reads_merged;	    /* Num reads merged */
	uint64_t pm_read_sectors;	    /* Num sectors read */
	uint64_t pm_read_nsecs;	    /* Num milliseconds spent reading */
	uint64_t pm_writes;	    /* Num writes completed */
	uint64_t pm_writes_merged;	    /* Num writes merged */
	uint64_t pm_write_sectors;	    /* Num sectors written */
	uint64_t pm_write_nsecs;	    /* Num milliseconds spent writing */
	uint64_t pm_io_in_progress;    /* Num I/Os currently in progress */
	uint64_t pm_io_nsecs;	    /* Num milliseconds spent doing I/Os */
	uint64_t pm_weighted_io_nsecs; /* Weighted num milliseconds doing I/Os */
	uint64_t pm_total_read_nsecs;  /* Total time spent reading in milliseconds */
	uint64_t pm_total_write_nsecs; /* Total time spent writing in milliseconds */
	struct dm_histogram *histogram; /* Histogram. */
};

/*
struct dm_stats_metric {
};
*/

extern int dm_stats_fetch(int, struct pm_dm_stats_counter *, pmAtomValue *);
extern int dm_refresh_stats_counter(const char *, struct pm_dm_stats_counter *);
extern int dm_stats_instance_refresh(void);
extern void dm_stats_setup(void);

#endif
#endif
