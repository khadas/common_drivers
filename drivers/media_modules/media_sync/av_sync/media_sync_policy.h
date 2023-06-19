/* Copyright (C) 2017 Amlogic, Inc. All rights reserved.
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
#ifndef MEDIA_SYNC_POLICY_HEAD_HH
#define MEDIA_SYNC_POLICY_HEAD_HH

#include <linux/kernel.h>
#include "media_sync_core.h"

#define MAX_INSTANCE_NUM 10

#define DTV_PTS_CORRECTION_THRESHOLD 900000 // (10s * TIME_PTS_UNIT_S(90000))


typedef enum {
    MEDIASYNC_VIDEO_UNKNOWN = 0,
    MEDIASYNC_VIDEO_NORMAL_OUTPUT,
    MEDIASYNC_VIDEO_HOLD,
    MEDIASYNC_VIDEO_DROP,
    MEDIASYNC_VIDEO_EXIT,
} video_policy;

struct mediasync_video_policy {
    video_policy videopolicy;
    int64_t  param1;
    int32_t  param2;
};

struct mediasync_start_slow_sync_info {
    s32 mSlowSyncEnable;               // slowsync enable flag, default is false
    bool mSlowSyncFinished;            // slowsync finish flag, default is false
    s64 mSlowSyncSpeed;              // slowsync speed, default is 0.5
    s32 mSlowSyncPVdiffThreshold;      // slowsync threshold, default is 200ms
    s32 mSlowSyncMaxPVdiffThreshold;   // slowsync max threshold, default is 4000ms
    s64 mSlowSyncFrameShowTime;    // slowsync video frame show time
    s64 mSlowSyncRealPVdiff;       // first vpts and ref pts diff
    s32 mSlowSyncExpectAvSyncDoneTime; //Expect AvSync Done Time
    s64 mSlowSyncStartSystemTime;
};

typedef struct policy_instance {
	struct list_head node;
	s32 sSyncInsId;
	sync_stream_type stream_type;
	bool mShowFirstFrameNosync;
	bool mVideoFreeRun;
	bool mVideoStarted;
	s32 mStartFlag;
	s32 mStartPlayThreshold;
	s64 mEnterDiscontinueTime;
	s64 mCurPcr;
	s32 mStartThreshold;
	s32 mPtsAdjust;
	s32 mVideoSyncThreshold;
	s32 mDiscontinueCacheThreshold;
	s32 isPause;
	s32 mVideoTrickMode;
	s64 mHoldVideoPts;
	s64 mHoldVideoTime;
	s64 mStartHoldVideoTime;
	/**start slow sync Parameter**/
	struct mediasync_start_slow_sync_info mStartSlowSyncInfo;
	/***************************/
	s64 mVideoSyncIntervalUs;
	s32 invalidVptsCount;
	video_policy videoLastPolicy;
	mediasync_speed mSpeed;
	mediasync_speed mPcrSlope;
	sync_mode mSyncMode;
	mediasync_clocktype clockType;
	mediasync_clockprovider_state clockState;
	mediasync_clockprovider_state mLastClockProviderState;
	mediasync_frameinfo firstDmxPcrInfo;
	mediasync_frameinfo firstVFrameInfo;
	mediasync_frameinfo firstAFrameInfo;
	mediasync_frameinfo anchorFrameInfo;
	mediasync_frameinfo mCurVideoFrameInfo;
	mediasync_frameinfo freerunFrameInfo;
	mediasync_update_info updatInfo;
	MediaSyncManager* mMediasyncIns;
} mediasync_policy_instance;

struct mediasync_policy_manager {
	struct kref		ref;
	struct mutex		mutex;
	struct list_head	mediasync_policy_list;
	int			inst_count;
};

s64 mediasync_get_system_time_us(void);


int mediasync_policy_create(ulong* handle);
int mediasync_policy_release(ulong handle);

int mediasync_policy_bind_instance(ulong handle,s32 SyncInsId,sync_stream_type stream_type);

int mediasync_video_process(ulong handle,s64 vpts,struct mediasync_video_policy* vsyncPolicy);


int mediasync_policy_manager_init(void);
void mediasync_policy_manager_exit(void);


#endif

