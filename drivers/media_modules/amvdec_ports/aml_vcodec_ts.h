/*
* Copyright (C) 2020 Amlogic, Inc. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Description:
*/
#ifndef _AML_VCODEC_TS_H_
#define _AML_VCODEC_TS_H_

#include "../media_sync/pts_server/pts_server_core.h"

int aml_vcodec_pts_checkout(s32 ptsserver_id, u64 offset, struct checkoutptsoffset *pts);
int aml_vcodec_pts_offset(s32 ptsserver_id, u64 offset, struct checkoutptsoffset *pts);
int aml_vcodec_pts_checkin(s32 ptsserver_id, u32 pkt_size, u64 pts_val);
int aml_vcodec_pts_first_checkin(u32 format, s32 ptsserver_id, u32 wp, u32 buf_start);

struct pts_server_ops {
	int (*checkout) (s32 ptsserver_id, u64 offset, struct checkoutptsoffset *pts);
	int (*cal_offset) (s32 ptsserver_id, u64 offset, struct checkoutptsoffset *pts);
	int (*checkin) (s32 ptsserver_id, u32 pkt_size, u64 pts_val);
	int (*first_checkin) (u32 format, s32 ptsserver_id, u32 wp, u32 buf_start);
};
struct pts_server_ops *get_pts_server_ops(void);

#endif

