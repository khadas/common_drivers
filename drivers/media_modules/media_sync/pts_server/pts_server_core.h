/*
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Description:
 */
#ifndef PTS_SERVER_HEAD_HH
#define PTS_SERVER_HEAD_HH

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/amlogic/cpu_version.h>

typedef struct ptsnode {
	struct list_head node;
	u32 offset;
	u32 pts;
	u64 pts_90k;
	u64 pts_64;
	u32 expired_count;
	u64 duration_count;
	u64 index;
} pts_node;

typedef struct psinstance {
	struct list_head pts_list;
	struct list_head pts_free_list;
	s32 mPtsServerInsId;
	u32 mMaxCount;
	u32 mLookupThreshold;
	u32 mPtsCheckinStarted;
	u32 mPtsCheckoutStarted;
	u32 mPtsCheckoutFailCount;
	u32 mFrameDuration;
	u64 mFrameDuration64;
	u32 mFirstCheckinPts;
	u32 mFirstCheckinOffset;
	u32 mFirstCheckinSize;
	u32 mLastCheckinPts;
	u32 mLastCheckinOffset;
	u32 mLastCheckinSize;
	u32 mAlignmentOffset;
	u32 mLastCheckoutPts;
	u32 mLastPeekPts;
	u32 mLastCheckoutOffset;
	u32 mFirstCheckinPts64;
	u32 mLastCheckinPts64;
	u64 mLastCheckoutPts64;
	u64 mLastPeekPts64;
	u32 mDoubleCheckFrameDuration;
	u64 mDoubleCheckFrameDuration64;
	u32 mDoubleCheckFrameDurationCount;
	u32 mLastDoubleCheckoutPts;
	u64 mLastDoubleCheckoutPts64;
	u32 mDecoderDuration;
	u32 kDoubleCheckThreshold;
	u32 mLastDropIndex;
	u32 mLastIndex;
	struct mutex mPtsListLock;
	u32 mListSize;
	u32 mLastCheckoutCurOffset;
	pts_node* all_free_ptn;
	bool setC2Mode;
	s32 mTrickMode;
	bool mOffsetMode;
	//for piece checkin decode
	u32 mLastCheckinPiecePts;
	u64 mLastCheckinPiecePts64;
	u32 mLastCheckinPieceOffset;
	u32 mLastCheckinPieceSize;
	u32 mLastCheckoutIndex;
	u64 mLastCheckinDurationCount;
	u64 mLastCheckoutDurationCount;
	u32 mLastShotBound;
	u32 mStickyWrapFlag;
	u64 mLastCheckoutPts90k;
	u64 mFirstCheckinPts90k;
	u64 mLastCheckinPts90k;
	spinlock_t mPtsListSlock;
	s32 mRef;
} ptsserver_ins;

typedef struct Pts_Server_Manage {
	ptsserver_ins* pInstance;
	struct mutex mListLock;
	spinlock_t mListSlock;
} PtsServerManage;

typedef struct ps_alloc_para {
	u32 mMaxCount;
	u32 mLookupThreshold;
	u32 kDoubleCheckThreshold;
} ptsserver_alloc_para;

typedef struct checkinptssize {
	u32 size;
	u32 pts;
	u64 pts_64;
} checkin_pts_size;

typedef struct checkinptsoffset {
	u32 offset;
	u32 pts;
	u64 pts_64;
} checkin_pts_offset;

typedef struct checkoutptsoffset {
	u64 offset;
	u32 pts;
	u64 pts_64;
} checkout_pts_offset;

typedef struct checkinaptssize {
	u32 offset;
	u32 size;
	u64 pts_90k;
	u64 pts_64;
} checkin_apts_size;

typedef struct checkoutaptsoffset {
	u64 offset;
	u64 pts_90k;
	u64 pts_64;
} checkout_apts_offset;

typedef struct startoffset {
	u32 mBaseOffset;
	u32 mAlignmentOffset;
} start_offset;

typedef struct lastcheckinpts {
	u32 mLastCheckinPts;
	u64 mLastCheckinPts64;
} last_checkin_pts;

typedef struct lastcheckoutpts {
	u32 mLastCheckoutPts;
	u64 mLastCheckoutPts64;
} last_checkout_pts;

long ptsserver_init(void);
long ptsserver_ins_alloc(s32 *pServerInsId,ptsserver_ins **pIns,ptsserver_alloc_para* allocParm);
void ptsserver_set_mode(s32 pServerInsId, bool set_mode);
long ptsserver_set_first_checkin_offset(s32 pServerInsId,start_offset* mStartOffset);
long ptsserver_checkin_pts_size(s32 pServerInsId,checkin_pts_size* mCheckinPtsSize);
long ptsserver_checkin_pts_offset(s32 pServerInsId, checkin_pts_offset* mCheckinPtsOffset);
long ptsserver_checkout_pts_offset(s32 pServerInsId,checkout_pts_offset* mCheckoutPtsOffset);
long ptsserver_peek_pts_offset(s32 pServerInsId,checkout_pts_offset* mCheckoutPtsOffset);
long ptsserver_get_last_checkin_pts(s32 pServerInsId,last_checkin_pts* mLastCheckinPts);
long ptsserver_get_last_checkout_pts(s32 pServerInsId,last_checkout_pts* mLastCheckOutPts);
long ptsserver_ins_release(s32 pServerInsId);
long ptsserver_set_trick_mode(s32 pServerInsId,s32 mode);
//audio
long ptsserver_ins_reset(s32 pServerInsId);
long ptsserver_static_ins_binder(s32 pServerInsId, ptsserver_ins** pIns, ptsserver_alloc_para allocParm);
long ptsserver_checkin_apts_size(s32 pServerInsId,checkin_apts_size* mCheckinPtsSize);
long ptsserver_checkout_apts_offset(s32 pServerInsId,checkout_apts_offset* mCheckoutPtsOffset);
long ptsserver_get_list_size(s32 pServerInsId, u32* ListSize);

#endif
