/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef TSYNC_PCR_H
#define TSYNC_PCR_H
#include <linux/amlogic/media/frame_sync/tsync.h>

#ifdef CONFIG_AMLOGIC_MEDIA_FRAME_SYNC
void tsync_pcr_avevent_locked(enum avevent_e event, u32 param);

int tsync_pcr_start(void);

void tsync_pcr_stop(void);

int tsync_pcr_set_apts(unsigned int pts);

int get_vsync_pts_inc_mode(void);

int tsync_pcr_init(void);
void tsync_pcr_exit(void);
int tsync_pcr_demux_pcr_used(void);
#else
void __weak  tsync_pcr_avevent_locked(enum avevent_e event, u32 param)
{
}

int __weak tsync_pcr_start(void)
{
	return -1;
}

void __weak  tsync_pcr_stop(void);

int __weak tsync_pcr_set_apts(unsigned int pts)
{
	return -1;
}

int __weak tsync_pcr_init(void)
{
	return -1;
}
void __weak  tsync_pcr_exit(void);
int __weak tsync_pcr_demux_pcr_used(void)
{
	return -1;
}
#endif

#endif
