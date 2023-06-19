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

#include <linux/types.h>
#include <linux/delay.h>
#include <linux/videodev2.h>

#include "aml_vcodec_drv.h"

#include <linux/sched/clock.h>
#include "aml_vcodec_ts.h"

int aml_vcodec_pts_checkout(s32 ptsserver_id, u64 offset, struct checkoutptsoffset *pts)
{
	checkout_pts_offset mCheckOutPtsOffset = {0};
	int ret;

	mCheckOutPtsOffset.offset = offset;
	ret = ptsserver_checkout_pts_offset(ptsserver_id,&mCheckOutPtsOffset);
	if (ret) {
		pr_err("aml_vcodec_pts_checkout ret = %d\n", ret);
		return ret;
	}

	*pts = mCheckOutPtsOffset;

	pr_debug("%s duration: %lld offset: 0x%llx pts: 0x%x pts64: %llu\n",
		__func__, (offset >> 32) & 0xffffffff,
		offset & 0xffffffff, pts->pts, pts->pts_64);

	return 0;
}
EXPORT_SYMBOL(aml_vcodec_pts_checkout);

int aml_vcodec_pts_offset(s32 ptsserver_id, u64 offset, struct checkoutptsoffset *pts)
{
	checkout_pts_offset mCheckOutPtsOffset = {0};
	int ret;
	mCheckOutPtsOffset.offset = offset;

	ret = ptsserver_peek_pts_offset(ptsserver_id, &mCheckOutPtsOffset);
	if (ret) {
		pr_err("aml_vcodec_pts_offset ret = %d\n", ret);
		return ret;
	}

	*pts = mCheckOutPtsOffset;

	pr_debug("%s duration: %lld offset: 0x%llx pts: 0x%x pts64: %llu\n",
		__func__, (offset >> 32) & 0xffffffff,
		offset & 0xffffffff, pts->pts, pts->pts_64);

	return 0;
}
EXPORT_SYMBOL(aml_vcodec_pts_offset);

int aml_vcodec_pts_checkin(s32 ptsserver_id, u32 pkt_size, u64 pts_val)
{
	checkin_pts_size mCheckinPtsSize;

	mCheckinPtsSize.size =pkt_size;
	mCheckinPtsSize.pts = (u32)div64_u64(pts_val * 9, 100);
	mCheckinPtsSize.pts_64 = pts_val;
	ptsserver_checkin_pts_size(ptsserver_id,&mCheckinPtsSize);

	pr_debug("%s pkt_size:%d chekin pts: %d, pts64: %llu\n",
		__func__, pkt_size, mCheckinPtsSize.pts, pts_val);

	return 0;
}

int aml_vcodec_pts_first_checkin(u32 format, s32 ptsserver_id, u32 wp, u32 buf_start)
{
	uint32_t mBaseffset = 0;
	uint32_t mAlignmentOffset = 0;
	start_offset mSartOffset;

	if ((format == V4L2_PIX_FMT_HEVC) || (format == V4L2_PIX_FMT_VP9) || (format == V4L2_PIX_FMT_AV1)) {
		mAlignmentOffset  = wp % 0x80;
		mBaseffset = 0;
	} else {
		mBaseffset = wp - buf_start;
	}

	mSartOffset.mBaseOffset = mBaseffset;
	mSartOffset.mAlignmentOffset = mAlignmentOffset;
	ptsserver_set_first_checkin_offset(ptsserver_id,&mSartOffset);

	pr_debug("%s format:%d mBaseffset: 0x%d mAlignmentOffset: %d\n",
		__func__, format, mBaseffset, mAlignmentOffset);

	return 0;
}

static struct pts_server_ops pts_server_ops = {
	.checkout	= aml_vcodec_pts_checkout,
	.cal_offset = aml_vcodec_pts_offset,
	.checkin	= aml_vcodec_pts_checkin,
	.first_checkin	= aml_vcodec_pts_first_checkin,
};

struct pts_server_ops *get_pts_server_ops(void)
{
	return &pts_server_ops;
}
EXPORT_SYMBOL(get_pts_server_ops);
