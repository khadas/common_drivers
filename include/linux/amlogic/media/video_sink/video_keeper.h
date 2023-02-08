/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * include/linux/amlogic/media/video_sink/video_keeper.h
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef VIDEO_KEEPER_HEADER___
#define VIDEO_KEEPER_HEADER___

#include <linux/amlogic/media/vfm/vframe.h>

void video_keeper_new_frame_notify(u8 layer_id);
void try_free_keep_video(int flags);
void try_free_keep_vdx(int flags, u8 layer_id);
int video_keeper_init(void);
void video_keeper_exit(void);
unsigned int vf_keep_current
	(struct vframe_s *cur_dispbuf,
	struct vframe_s *cur_dispbuf2);
unsigned int vf_keep_current_locked
	(u8 layer_id,
	struct vframe_s *cur_dispbuf,
	struct vframe_s *cur_dispbuf_el);
#endif
