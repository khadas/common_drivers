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
#ifndef MEDIA_SYNC_HEAD_HH
#define MEDIA_SYNC_HEAD_HH

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/amlogic/cpu_version.h>


#define MIN_UPDATETIME_THRESHOLD_US 50000
#define RECORD_SLOPE_NUM 5
#define DEFAULT_TRIGGER_DISCONTINUE_THRESHOLD 630000//(7000 * 90)
#define DEFAULT_REMOVE_DISCONTINUE_THRESHOLD 450000
#define DEFAULT_FRAME_SEGMENT_THRESHOLD 45000//(500 * 90)
#define TIME_DISCONTINUE_DURATION  5000000 // (5s * TIME_UNIT_S)
#define ABSSUB(a, b) (((a) >= (b)) ? ((a) - (b)) : ((b) - (a)))

typedef enum {
	MEDIA_SYNC_VMASTER = 0,
	MEDIA_SYNC_AMASTER = 1,
	MEDIA_SYNC_PCRMASTER = 2,
	MEDIA_SYNC_MODE_MAX = 255,
} sync_mode;

typedef enum {
    MEDIA_VIDEO = 0,
    MEDIA_AUDIO = 1,
    MEDIA_DMXPCR = 2,
    MEDIA_SUBTITLE = 3,
    MEDIA_COMMON = 4,
    MEDIA_TYPE_MAX = 255,
} sync_stream_type;

typedef enum {
	MEDIASYNC_INIT = 0,
	MEDIASYNC_AUDIO_ARRIVED,
	MEDIASYNC_VIDEO_ARRIVED,
	MEDIASYNC_AV_ARRIVED,
	MEDIASYNC_AV_SYNCED,
	MEDIASYNC_RUNNING,
	MEDIASYNC_VIDEO_LOST_SYNC,
	MEDIASYNC_AUDIO_LOST_SYNC,
	MEDIASYNC_EXIT,
} avsync_state;

typedef enum {
	UNKNOWN_CLOCK = 0,
	AUDIO_CLOCK,
	VIDEO_CLOCK,
	PCR_CLOCK,
	REF_CLOCK,
} mediasync_clocktype;

/*Video trick mode*/
typedef enum {
    VIDEO_TRICK_MODE_NONE = 0,          // Disable trick mode
    VIDEO_TRICK_MODE_PAUSE = 1,         // Pause the video decoder
    VIDEO_TRICK_MODE_PAUSE_NEXT = 2,    // Pause the video decoder when a new frame displayed
    VIDEO_TRICK_MODE_IONLY = 3          // Decoding and Out I frame only
} mediasync_video_trick_mode;

typedef enum {
	GET_UPDATE_INFO = 0,
	GET_SLOW_SYNC_ENABLE,
	GET_TRICK_MODE,
	SET_VIDEO_FRAME_ADVANCE = 500,
	SET_SLOW_SYNC_ENABLE,
	SET_TRICK_MODE,
	SET_FREE_RUN_TYPE,
	GET_AUDIO_WORK_MODE,

} mediasync_control_cmd;

typedef struct m_control {
	u32 cmd;
	u32 size;
	u32 reserved[2];
	union {
		s32 value;
		s64 value64;
		ulong ptr;
	};
} mediasync_control;


typedef struct speed{
	u32 mNumerator;
	u32 mDenominator;
} mediasync_speed;

typedef struct mediasync_lock{
	struct mutex m_mutex;
	int Is_init;
} mediasync_lock;

typedef struct frameinfo{
	int64_t framePts;
	int64_t frameSystemTime;
} mediasync_frameinfo;

typedef struct frameinfo_inner{
	int64_t framePts;
	int64_t frameSystemTime;
	uint64_t frameid;
	int frameduration;
	int remainedduration;
}mediasync_frameinfo_inner;

typedef struct video_packets_info{
	int packetsSize;
	int64_t packetsPts;
} mediasync_video_packets_info;

typedef struct audio_packets_info{
	int packetsSize;
	int duration;
	int isworkingchannel;
	int isneedupdate;
	int64_t packetsPts;
} mediasync_audio_packets_info;

typedef struct discontinue_frame_info{
	int64_t discontinuePtsBefore;
	int64_t discontinuePtsAfter;
	int isDiscontinue;
	int64_t lastDiscontinuePtsBefore;
	int64_t lastDiscontinuePtsAfter;
} mediasync_discontinue_frame_info;

typedef struct avsync_state_cur_time_us{
	avsync_state state;
	int64_t setStateCurTimeUs;
} mediasync_avsync_state_cur_time_us;

typedef struct syncinfo {
	avsync_state state;
	int64_t setStateCurTimeUs;
	mediasync_frameinfo firstAframeInfo;
	mediasync_frameinfo firstVframeInfo;
	mediasync_frameinfo firstDmxPcrInfo;
	mediasync_frameinfo refClockInfo;
	mediasync_frameinfo curAudioInfo;
	mediasync_frameinfo curVideoInfo;
	mediasync_frameinfo curDmxPcrInfo;
	mediasync_frameinfo queueAudioInfo;
	mediasync_frameinfo queueVideoInfo;
	mediasync_frameinfo firstAudioPacketsInfo;
	mediasync_frameinfo firstVideoPacketsInfo;
	mediasync_frameinfo pauseVideoInfo;
	mediasync_frameinfo pauseAudioInfo;
	mediasync_video_packets_info videoPacketsInfo;
	mediasync_audio_packets_info audioPacketsInfo;
} mediasync_syncinfo;

typedef struct audioinfo{
	int cacheSize;
	int cacheDuration;
} mediasync_audioinfo;

typedef struct videoinfo{
	int cacheSize;
	int specialSizeCount;
	int cacheDuration;
} mediasync_videoinfo;

typedef struct cacheinfo{
	int cacheSize;
	int cacheDuration;
}mediasync_cacheinfo;

typedef struct audio_format{
	int samplerate;
	int datawidth;
	int channels;
	int format;
} mediasync_audio_format;

typedef enum
{
	TS_DEMOD = 0,                          // TS Data input from demod
	TS_MEMORY = 1,                         // TS Data input from memory
	ES_MEMORY = 2,                         // ES Data input from memory
} aml_Source_Type;

typedef enum {
    CLOCK_PROVIDER_NONE = 0,
    CLOCK_PROVIDER_DISCONTINUE,
    CLOCK_PROVIDER_NORMAL,
    CLOCK_PROVIDER_LOST,
    CLOCK_PROVIDER_RECOVERING,
} mediasync_clockprovider_state;

typedef struct update_info{
	u32 mStcParmUpdateCount;
	u32 debugLevel;
	s64 mCurrentSystemtime;
	int mPauseResumeFlag;
	avsync_state mAvSyncState;
	int64_t mSetStateCurTimeUs;
	mediasync_clockprovider_state mSourceClockState;
	mediasync_audioinfo mAudioInfo;
	mediasync_videoinfo mVideoInfo;
	u32 isVideoFrameAdvance;
	uint32_t mFreeRunType;
} mediasync_update_info;

typedef struct frame_rec_s {
	struct list_head list;
	mediasync_frameinfo_inner frame_info;
} frame_rec_t;


typedef struct frame_table_s {
	int mTableType;
	int rec_num;
	uint64_t current_frame_id;
	struct frame_rec_s *pts_recs;
	unsigned long *pages_list;
	mediasync_frameinfo_inner mLastQueued;
	mediasync_frameinfo_inner mMinPtsFrame;
	mediasync_cacheinfo mCacheInfo;
	mediasync_frameinfo mFirstPacket;
	int64_t mLastProcessedPts;
	int mFrameCount;
	int64_t mLastCheckedPts;
	mediasync_frameinfo_inner mLastCheckedFrame;
	struct list_head valid_list;
	struct list_head free_list;
} frame_table_t;

typedef struct instance{
	s32 mSyncIndex;
	s32 mSyncId;
	u32 mUId;
	s32 mDemuxId;
	s32 mPcrPid;
	s32 mPaused;
	s32 mRef;
	s32 mSyncMode;
	s64 mLastStc;
	s64 mLastRealTime;
	s64 mLastMediaTime;
	s64 mTrackMediaTime;
	s64 mStartMediaTime;
	mediasync_speed mSpeed;
	mediasync_speed mPcrSlope;
	s32 mSyncModeChange;
	s64 mUpdateTimeThreshold;
	s32 mPlayerInstanceId;
	s32 mVideoSmoothTag;
	int mCacheFrames;
	int mHasAudio;
	int mHasVideo;
	int mute_flag;
	int mStartThreshold;
	int mPtsAdjust;
	int mVideoWorkMode;
	int mFccEnable;
	int mPauseResumeFlag;
	int mAVRef;
	u32 mStcParmUpdateCount;
	u32 mAudioCacheUpdateCount;
	u32 mVideoCacheUpdateCount;
	u32 mGetAudioCacheUpdateCount;
	u32 mGetVideoCacheUpdateCount;
	u32 isVideoFrameAdvance;
	s64 mLastCheckSlopeSystemtime;
	s64 mLastCheckSlopeDemuxPts;
	s32 mVideoTrickMode;
	mediasync_clocktype mSourceClockType;
	mediasync_clockprovider_state mSourceClockState;
	mediasync_audioinfo mAudioInfo;
	mediasync_videoinfo mVideoInfo;
	mediasync_syncinfo mSyncInfo;
	mediasync_discontinue_frame_info mVideoDiscontinueInfo;
	mediasync_discontinue_frame_info mAudioDiscontinueInfo;
	aml_Source_Type mSourceType;
	mediasync_audio_format mAudioFormat;
	bool mSlowSyncEnable;
	char atrace_video[32];
	char atrace_audio[32];
	char atrace_pcrscr[32];
	struct frame_table_s frame_table[2];
	u32 mRcordSlope[RECORD_SLOPE_NUM];
	u32 mRcordSlopeCount;
	s32 mlastCheckVideocacheDuration;
	uint32_t mFreeRunType;
}mediasync_ins;

typedef struct Media_Sync_Manage {
	mediasync_ins* pInstance;
	spinlock_t m_lock;
} MediaSyncManager;

long mediasync_init(void);
long mediasync_ins_alloc(s32 sDemuxId,
			s32 sPcrPid,
			s32 *sSyncInsId,
			MediaSyncManager **pSyncManage);

long mediasync_ins_binder(s32 sSyncInsId,
			MediaSyncManager **pSyncManage);
long mediasync_static_ins_binder(s32 sSyncInsId,
			MediaSyncManager **pSyncManage);
long mediasync_ins_unbinder(MediaSyncManager* pSyncManage, s32 sStreamType);
long mediasync_ins_update_mediatime(MediaSyncManager* pSyncManage,
					s64 lMediaTime,
					s64 lSystemTime, bool forceUpdate);
long mediasync_ins_set_mediatime_speed(MediaSyncManager* pSyncManage, mediasync_speed fSpeed);
long mediasync_ins_set_paused(MediaSyncManager* pSyncManage, s32 sPaused);
long mediasync_ins_get_paused(MediaSyncManager* pSyncManage, s32* spPaused);
long mediasync_ins_get_trackmediatime(MediaSyncManager* pSyncManage, s64* lpTrackMediaTime);
long mediasync_ins_set_syncmode(MediaSyncManager* pSyncManage, s32 sSyncMode);
long mediasync_ins_get_syncmode(MediaSyncManager* pSyncManage, s32 *sSyncMode);
long mediasync_ins_get_mediatime_speed(MediaSyncManager* pSyncManage, mediasync_speed *fpSpeed);
long mediasync_ins_get_anchor_time(MediaSyncManager* pSyncManage,
				s64* lpMediaTime,
				s64* lpSTCTime,
				s64* lpSystemTime);
long mediasync_ins_get_systemtime(MediaSyncManager* pSyncManage,
				s64* lpSTC,
				s64* lpSystemTime);
long mediasync_ins_get_nextvsync_systemtime(MediaSyncManager* pSyncManage, s64* lpSystemTime);
long mediasync_ins_set_updatetime_threshold(MediaSyncManager* pSyncManage, s64 lTimeThreshold);
long mediasync_ins_get_updatetime_threshold(MediaSyncManager* pSyncManage, s64* lpTimeThreshold);

long mediasync_ins_set_clocktype(MediaSyncManager* pSyncManage, mediasync_clocktype type);
long mediasync_ins_get_clocktype(MediaSyncManager* pSyncManage, mediasync_clocktype* type);
long mediasync_ins_set_avsyncstate(MediaSyncManager* pSyncManage, s32 state);
long mediasync_ins_get_avsyncstate(MediaSyncManager* pSyncManage, s32* state);
long mediasync_ins_set_hasaudio(MediaSyncManager* pSyncManage, int hasaudio);
long mediasync_ins_get_hasaudio(MediaSyncManager* pSyncManage, int* hasaudio);
long mediasync_ins_set_hasvideo(MediaSyncManager* pSyncManage, int hasvideo);
long mediasync_ins_get_hasvideo(MediaSyncManager* pSyncManage, int* hasvideo);
long mediasync_ins_set_audioinfo(MediaSyncManager* pSyncManage, mediasync_audioinfo info);
long mediasync_ins_get_audioinfo(MediaSyncManager* pSyncManage, mediasync_audioinfo* info);
long mediasync_ins_set_videoinfo(MediaSyncManager* pSyncManage, mediasync_videoinfo info);
long mediasync_ins_set_audiomute(MediaSyncManager* pSyncManage, int mute_flag);
long mediasync_ins_get_audiomute(MediaSyncManager* pSyncManage, int* mute_flag);
long mediasync_ins_get_videoinfo(MediaSyncManager* pSyncManage, mediasync_videoinfo* info);
long mediasync_ins_set_firstaudioframeinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo info);
long mediasync_ins_get_firstaudioframeinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo* info);
long mediasync_ins_set_firstvideoframeinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo info);
long mediasync_ins_get_firstvideoframeinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo* info);
long mediasync_ins_set_firstdmxpcrinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo info);
long mediasync_ins_get_firstdmxpcrinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo* info);
long mediasync_ins_set_refclockinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo info);
long mediasync_ins_get_refclockinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo* info);
long mediasync_ins_set_curaudioframeinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo info);
long mediasync_ins_get_curaudioframeinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo* info);
long mediasync_ins_set_curvideoframeinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo info);
long mediasync_ins_get_curvideoframeinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo* info);
long mediasync_ins_set_curdmxpcrinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo info);
long mediasync_ins_get_curdmxpcrinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo* info);
long mediasync_ins_set_clockstate(MediaSyncManager* pSyncManage, mediasync_clockprovider_state state);
long mediasync_ins_get_clockstate(MediaSyncManager* pSyncManage, mediasync_clockprovider_state* state);
long mediasync_ins_set_startthreshold(MediaSyncManager* pSyncManage, s32 threshold);
long mediasync_ins_get_startthreshold(MediaSyncManager* pSyncManage, s32* threshold);
long mediasync_ins_set_ptsadjust(MediaSyncManager* pSyncManage, s32 adjust_pts);
long mediasync_ins_get_ptsadjust(MediaSyncManager* pSyncManage, s32* adjust_pts);
long mediasync_ins_set_videoworkmode(MediaSyncManager* pSyncManage, s32 mode);
long mediasync_ins_get_videoworkmode(MediaSyncManager* pSyncManage, s32* mode);
long mediasync_ins_set_fccenable(MediaSyncManager* pSyncManage, s32 enable);
long mediasync_ins_get_fccenable(MediaSyncManager* pSyncManage, s32* enable);
long mediasync_ins_set_source_type(MediaSyncManager* pSyncManage, aml_Source_Type sourceType);
long mediasync_ins_get_source_type(MediaSyncManager* pSyncManage, aml_Source_Type* sourceType);
long mediasync_ins_set_start_media_time(MediaSyncManager* pSyncManage, s64 startime);
long mediasync_ins_get_start_media_time(MediaSyncManager* pSyncManage, s64* starttime);
long mediasync_ins_set_audioformat(MediaSyncManager* pSyncManage, mediasync_audio_format format);
long mediasync_ins_get_audioformat(MediaSyncManager* pSyncManage, mediasync_audio_format* format);
long mediasync_ins_set_pauseresume(MediaSyncManager* pSyncManage, int flag);
long mediasync_ins_get_pauseresume(MediaSyncManager* pSyncManage, int* flag);
long mediasync_ins_set_pcrslope(MediaSyncManager* pSyncManage, mediasync_speed pcrslope);
long mediasync_ins_get_pcrslope(MediaSyncManager* pSyncManage, mediasync_speed *pcrslope);
long mediasync_ins_reset(MediaSyncManager* pSyncManage);
long mediasync_ins_update_avref(MediaSyncManager* pSyncManage, int flag);
long mediasync_ins_get_avref(MediaSyncManager* pSyncManage, int *ref);
long mediasync_ins_set_queue_audio_info(MediaSyncManager* pSyncManage, mediasync_frameinfo info);
long mediasync_ins_get_queue_audio_info(MediaSyncManager* pSyncManage, mediasync_frameinfo* info);
long mediasync_ins_set_queue_video_info(MediaSyncManager* pSyncManage, mediasync_frameinfo info);
long mediasync_ins_get_queue_video_info(MediaSyncManager* pSyncManage, mediasync_frameinfo* info);

long mediasync_ins_set_audio_packets_info_implementation(MediaSyncManager* pSyncManage, mediasync_audio_packets_info info);

long mediasync_ins_set_audio_packets_info(s32 sSyncInsId, mediasync_audio_packets_info info);
long mediasync_ins_get_audio_cache_info(MediaSyncManager* pSyncManage, mediasync_audioinfo* info);
long mediasync_ins_set_video_packets_info_implementation(MediaSyncManager* pSyncManage, mediasync_video_packets_info info);
long mediasync_ins_set_video_packets_info(s32 sSyncInsId, mediasync_video_packets_info info);

long mediasync_ins_get_video_cache_info(MediaSyncManager* pSyncManage, mediasync_videoinfo* info);
long mediasync_ins_set_first_queue_audio_info(MediaSyncManager* pSyncManage, mediasync_frameinfo info);
long mediasync_ins_get_first_queue_audio_info(MediaSyncManager* pSyncManage, mediasync_frameinfo* info);
long mediasync_ins_set_first_queue_video_info(MediaSyncManager* pSyncManage, mediasync_frameinfo info);
long mediasync_ins_get_first_queue_video_info(MediaSyncManager* pSyncManage, mediasync_frameinfo* info);
long mediasync_ins_set_player_instance_id(MediaSyncManager* pSyncManage, s32 PlayerInstanceId);
long mediasync_ins_get_player_instance_id(MediaSyncManager* pSyncManage, s32* PlayerInstanceId);
long mediasync_ins_get_avsync_state_cur_time_us(MediaSyncManager* pSyncManage, mediasync_avsync_state_cur_time_us* avsyncstate);
long mediasync_ins_set_pause_video_info(MediaSyncManager* pSyncManage, mediasync_frameinfo info);
long mediasync_ins_get_pause_video_info(MediaSyncManager* pSyncManage, mediasync_frameinfo* info);
long mediasync_ins_set_pause_audio_info(MediaSyncManager* pSyncManage, mediasync_frameinfo info);
long mediasync_ins_get_pause_audio_info(MediaSyncManager* pSyncManage, mediasync_frameinfo* info);
long mediasync_ins_check_apts_valid(MediaSyncManager* pSyncManage, s64 apts);
long mediasync_ins_check_vpts_valid(MediaSyncManager* pSyncManage, s64 vpts);
long mediasync_ins_set_cache_frames(MediaSyncManager* pSyncManage, s64 cache);
long mediasync_ins_ext_ctrls_ioctrl(MediaSyncManager* pSyncManage, ulong arg, unsigned int is_compat_ptr);
long mediasync_ins_ext_ctrls(MediaSyncManager* pSyncManage,mediasync_control* mediasyncControl);
s64 mediasync_ins_get_stc_time(MediaSyncManager* pSyncManage);
void mediasync_ins_check_pcr_slope(mediasync_ins* pInstance, mediasync_update_info* info);
long mediasync_ins_set_pcrslope_implementation(mediasync_ins* pInstance, mediasync_speed pcrslope);
long mediasync_ins_set_video_smooth_tag(MediaSyncManager* pSyncManage, s32 sSmooth_tag);
long mediasync_ins_get_video_smooth_tag(MediaSyncManager* pSyncManage, s32* spSmooth_tag);
long mediasync_ins_set_pcr_and_dmx_id(MediaSyncManager* pSyncManage, s32 sDemuxId, s32 sPcrPid);
#endif
