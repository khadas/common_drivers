#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/amlogic/cpu_version.h>
#include <linux/syscalls.h>
#include <linux/times.h>
#include <linux/time.h>
#include <linux/time64.h>
#include <linux/vmalloc.h>
#include <trace/events/meson_atrace.h>
#include "media_sync_core.h"
#include "media_sync_policy.h"

static u32 media_sync_policy_debug_level = 0;

static u32 first_frame_no_sync = 0;

static u32 slow_sync_avdiff_min_threshold = 800;
static u32 slow_sync_avdiff_max_threshold = 4000;
static u32 slow_sync_expect_av_sync_done = 5000;


#define mediasync_pr_info(dbg_level,inst,fmt,args...) if (dbg_level <= media_sync_policy_debug_level) {pr_info("[MS_Policy:%d][%d] " fmt,__LINE__, inst->sSyncInsId,##args);}


#define MEDIASYNC_NO_AUDIO               0x0001
#define MEDIASYNC_NO_VIDEO               0x0002
#define MEDIASYNC_AUDIO_COME_NORAML      0x0004
#define MEDIASYNC_VIDEO_COME_NORAML      0x0008
#define MEDIASYNC_AUDIO_COME_LATER       0x0010
#define MEDIASYNC_VIDEO_COME_LATER       0x0020
#define MEDIASYNC_AUDIO_COME_TOOLATE     0x0040
#define MEDIASYNC_VIDEO_COME_TOOLATE     0x0080



struct mediasync_policy_manager *m_mgr;


ulong mediasync_policy_inst_create(void);
int mediasync_policy_inst_release(ulong handle);

int mediasync_get_start_slow_sync_enable(mediasync_policy_instance *policyInst);



int mediasync_policy_create(ulong* handle)
{
	ulong inst = 0;
	inst = mediasync_policy_inst_create();
	*handle = inst;
	return 0;
}



int mediasync_policy_release(ulong handle)
{
	int ret = 0;
	mediasync_policy_instance *policyInst =
		(mediasync_policy_instance *) handle;
	if (handle == 0) {
		return ret;
	}
	if (policyInst->sSyncInsId > 0) {
		mediasync_ins_unbinder(policyInst->mMediasyncIns, policyInst->stream_type);
	}
	ret = mediasync_policy_inst_release(handle);
	return ret;
}


int mediasync_policy_bind_instance(ulong handle,s32 SyncInsId,sync_stream_type stream_type)
{
	int ret = 0;
	mediasync_policy_instance *policyInst =
		(mediasync_policy_instance *) handle;

	if (handle == 0) {
		return ret;
	}

	policyInst->sSyncInsId = SyncInsId;
	policyInst->stream_type = stream_type;
	mediasync_pr_info(0,policyInst,"bind_instance\n");
	if (mediasync_ins_binder(policyInst->sSyncInsId,&policyInst->mMediasyncIns)) {
		mediasync_pr_info(0,policyInst,"bind_instance error \n");
	}

	mediasync_pr_info(0,policyInst,"bind_instance(%px) \n", policyInst->mMediasyncIns);

	mediasync_get_start_slow_sync_enable(policyInst);
	if (policyInst->mStartSlowSyncInfo.mSlowSyncEnable == false) {
		policyInst->mShowFirstFrameNosync = false;
	}
	return ret;
}


s64 mediasync_get_system_time_us(void) {
	s64 TimeUs;
	struct timespec64 ts_monotonic;
	ktime_get_ts64(&ts_monotonic);
	TimeUs = ts_monotonic.tv_sec * 1000000LL + div_u64(ts_monotonic.tv_nsec , 1000);
	//pr_debug("get_system_time_us %lld\n", TimeUs);
	return TimeUs;
}


bool checkStreamPtsValid(s64 apts, s64 vpts, s64 demuxpcr, sync_stream_type* invalidstream) {
    s64 pa_diff = demuxpcr - apts;
    s64 pv_diff = demuxpcr - vpts;
    s64 av_diff = apts - vpts;

    if (demuxpcr < 0) {
        //MS_LOGE(mLogHead,"exception: [a:%" PRIx64 ", v:%" PRIx64 ", dmx:%" PRIx64 "],dmx pts < 0.", apts, vpts, demuxpcr);
        *invalidstream = MEDIA_TYPE_MAX;
        return false;
    } else if (apts < 0 || vpts < 0) {
        if (apts < 0 && vpts > 0) {
            if (ABSSUB(pv_diff, 0) > DTV_PTS_CORRECTION_THRESHOLD) {
                //MS_LOGE(mLogHead,"exception:video only or audio late, dmx_pcr is abnormal.");
                *invalidstream = MEDIA_DMXPCR;
            }
        } else if (vpts < 0 && apts > 0) {
            if (ABSSUB(pa_diff, 0) > DTV_PTS_CORRECTION_THRESHOLD) {
                //MS_LOGE(mLogHead,"exception:audio only or video late, dmx_pcr is abnormal.");
                *invalidstream = MEDIA_DMXPCR;
            }
        } else {
             //MS_LOGE(mLogHead,"exception: [a:%" PRIx64 ", v:%" PRIx64 ", dmx:%" PRIx64 "], pts < 0.", apts, vpts, demuxpcr);
             *invalidstream = MEDIA_TYPE_MAX;
             return false;
        }
    } else {
        if (ABSSUB(pa_diff, 0) > DTV_PTS_CORRECTION_THRESHOLD && ABSSUB(pv_diff, 0) > DTV_PTS_CORRECTION_THRESHOLD) {
            //MS_LOGE(mLogHead,"exception: dmx_pcr is abnormal.");
            *invalidstream = MEDIA_DMXPCR;
        } else if (ABSSUB(av_diff, 0) > DTV_PTS_CORRECTION_THRESHOLD) {
            if (ABSSUB(pv_diff, 0) > ABSSUB(pa_diff, 0)) {
                *invalidstream = MEDIA_VIDEO;
                //MS_LOGE(mLogHead,"exception: video pts is abnormal.");
            } else {
                *invalidstream = MEDIA_AUDIO;
                //MS_LOGE(mLogHead,"exception: audio pts is abnormal.");
            }
        }
    }

    //MS_LOGI(MSYNC_DEBUG_LEVEL_0,mLogHead,"pa_diff:%" PRId64 " ms, pv_diff:%" PRId64 " ms, av_diff:%" PRId64 " ms, invalidstream:%s.",
    //        pa_diff / 90, pv_diff / 90, av_diff / 90, streamType2Str(*invalidstream));

    return true;
}

bool checkDmxPcrValid(mediasync_policy_instance *policyInst,sync_stream_type* invalid_stream)
{
	mediasync_frameinfo curQueueAudioInfo;
	mediasync_frameinfo curQueueVideoInfo;
	if (policyInst->firstDmxPcrInfo.framePts == -1) {
		mediasync_pr_info(0,policyInst,"exception: cannot get dmx_pcr, dmx_pcr is invalid.");
		return false;
	}

	mediasync_ins_get_queue_audio_info(policyInst->mMediasyncIns,&curQueueAudioInfo);
	mediasync_ins_get_queue_video_info(policyInst->mMediasyncIns,&curQueueVideoInfo);
	if (checkStreamPtsValid(curQueueAudioInfo.framePts,
							curQueueVideoInfo.framePts,
							policyInst->firstDmxPcrInfo.framePts,
							invalid_stream) &&
							*invalid_stream == MEDIA_DMXPCR) {
		return false;
	}

	if (*invalid_stream == MEDIA_TYPE_MAX) {
		if (checkStreamPtsValid(policyInst->firstAFrameInfo.framePts,
									policyInst->firstVFrameInfo.framePts,
									policyInst->firstDmxPcrInfo.framePts,
									invalid_stream) &&
									*invalid_stream == MEDIA_DMXPCR) {
			return false;
		}
	}

	return true;
}


int  calculateStartPtsOffset(mediasync_policy_instance *policyInst,int* startPlayOffset,sync_stream_type invalid_stream) {
	int avdiffAbs = 0;
	int audioExpectcache = policyInst->mStartPlayThreshold * 90;
	int videoExpectcache = policyInst->mStartPlayThreshold * 90;
	int startPlayVideoOffset = 0;
	int startPlayAudioOffset = 0;
	int64_t startpts = 0;
	int64_t startpts_a = 0;
	int64_t startpts_v = 0;
	int avdiff = 0;
	mediasync_audioinfo* mAudioInfo = &(policyInst->updatInfo.mAudioInfo);
	mediasync_videoinfo* mVideoInfo = &(policyInst->updatInfo.mVideoInfo);
	if (policyInst->clockType == PCR_CLOCK) {
		/*
		if (mIsAmDolbyAudio) {
			if (policyInst->mStartPlayThreshold < 800) {
				audioExpectcache = 72000;//800 * 90;
			}
		} else {
			audioExpectcache = policyInst->mStartPlayThreshold * 90;
		}
		*/
		if (mVideoInfo->specialSizeCount >= 8) {
		//MS_LOGI(MSYNC_DEBUG_LEVEL_0,mLogHead, "special frame size, need enlarge cache thread.");
			videoExpectcache = policyInst->mStartPlayThreshold * 2 * 90;
		} else {
			videoExpectcache = policyInst->mStartPlayThreshold * 90;
		}
	}

	if (policyInst->firstVFrameInfo.framePts != -1 &&
		policyInst->firstAFrameInfo.framePts != -1) {
		avdiff = policyInst->firstVFrameInfo.framePts - policyInst->firstAFrameInfo.framePts;
		avdiffAbs = ABSSUB(avdiff, 0);
		//policyInst->updatInfo.mVideoInfo
		if (avdiff < 0) {
			/*
								 apts > vpts
				/----------(vpts)---------------------------------------->/
				/----------------|<--avdiffAbs-->|(apts)------------------->/
			*/
			if (videoExpectcache > mVideoInfo->cacheDuration) {
				/*
				   |<---videoExpectcache------------->|---
				   |<----videocache---->|<--Offset?-->|---
				*/

				startPlayVideoOffset = videoExpectcache - mVideoInfo->cacheDuration;

			} else {
				/*
					|<---videoExpectcache--->|---------
					<-------videocache---------->|----
				*/
				startPlayVideoOffset = 0;
			}

			if (mAudioInfo->cacheDuration + avdiffAbs < audioExpectcache) {
				/*
					|<------------audioExpectcache-------------->|------
					|<--avdiffAbs-->|<-audiocache->|<--Offset?-->|------
				*/
				startPlayAudioOffset = audioExpectcache - (mAudioInfo->cacheDuration + avdiffAbs);
			} else {
				 /*
					/|<---audioExpectcache--->|--------->/
					/|<--avdiffAbs-->|<-videocache->|--->/
				*/
				startPlayAudioOffset = 0;
			}
			if (policyInst->clockType == PCR_CLOCK) {
				*startPlayOffset = policyInst->firstDmxPcrInfo.framePts - policyInst->firstVFrameInfo.framePts;
			}
		} else {
			/*
								 vpts > apts
				/----------(apts)------------------------------------------>/
				/----------------|<--avdiffAbs-->|(vpts)------------------->/
			*/
			if (mAudioInfo->cacheDuration < audioExpectcache) {
				/*
				   |<---audioExpectcache------------->|---
				   |<----audiocache---->|<--Offset?-->|---
				*/
				startPlayAudioOffset = audioExpectcache - mAudioInfo->cacheDuration;
			} else {
				/*
				   |<---audioExpectcache--->|---------
				   |<-------audiocache---------->|----
				*/
				startPlayAudioOffset = 0;
			}

			if (videoExpectcache > mVideoInfo->cacheDuration + avdiffAbs) {
				/*
					|<--------------videoExpectcache------------>|-------
					|<--avdiffAbs-->|<-videocache->|<--Offset?-->|-------
				*/
				startPlayVideoOffset = videoExpectcache - (mVideoInfo->cacheDuration + avdiffAbs);
			} else {
				/*
					|<---videoExpectcache--->|----------
					|<--avdiffAbs-->|<-videocache->|----
				*/
				startPlayVideoOffset = 0;
			}
			if (policyInst->clockType == PCR_CLOCK) {
				*startPlayOffset = policyInst->firstDmxPcrInfo.framePts - policyInst->firstAFrameInfo.framePts;
			}
		}
		*startPlayOffset += startPlayVideoOffset > startPlayAudioOffset ? startPlayVideoOffset : startPlayAudioOffset;
		startpts = policyInst->anchorFrameInfo.framePts - *startPlayOffset;
	} else {

		if (policyInst->clockType == PCR_CLOCK) {
			if (policyInst->firstAFrameInfo.framePts != -1 &&
					invalid_stream != MEDIA_AUDIO) {
				startpts = policyInst->firstAFrameInfo.framePts;
				if (mAudioInfo->cacheDuration < audioExpectcache) {
					startpts -= (audioExpectcache - mAudioInfo->cacheDuration);
				}
			} else if (policyInst->firstVFrameInfo.framePts != -1 &&
						invalid_stream != MEDIA_VIDEO) {
				startpts = policyInst->firstVFrameInfo.framePts;
				if (mVideoInfo->cacheDuration < videoExpectcache) {
					startpts -= (videoExpectcache - mVideoInfo->cacheDuration);
				}
			}
			*startPlayOffset = policyInst->firstDmxPcrInfo.framePts - startpts;
		} else {
			startpts = policyInst->anchorFrameInfo.framePts;
			if (policyInst->firstAFrameInfo.framePts != -1) {
				startpts_a = policyInst->firstAFrameInfo.framePts + mAudioInfo->cacheDuration - audioExpectcache;
				startpts = startpts < startpts_a ? startpts : startpts_a;
				//startpts = startpts_a;
			} else if (policyInst->firstVFrameInfo.framePts != -1) {

				startpts_v = policyInst->firstVFrameInfo.framePts + mVideoInfo->cacheDuration - videoExpectcache;
				startpts = startpts < startpts_v ? startpts : startpts_v;
				//startpts = startpts_v;
			}
			*startPlayOffset = policyInst->anchorFrameInfo.framePts - startpts;
		}
	}

	if (policyInst->firstAFrameInfo.framePts != -1) {
		mediasync_pr_info(0,policyInst,"init a-cache:[%lld ms, %lld ms]\n",
				div_u64(mAudioInfo->cacheDuration , 90),
				div_u64(mAudioInfo->cacheDuration + policyInst->firstAFrameInfo.framePts - startpts, 90));
	} else {
		mediasync_pr_info(0,policyInst,"init a-cache:no audio \n");
	}

	if (policyInst->firstVFrameInfo.framePts != -1) {
		mediasync_pr_info(0,policyInst,"init v-cache:[%lld ms, %lld ms] \n",
		div_u64(mVideoInfo->cacheDuration , 90),
		div_u64(mVideoInfo->cacheDuration + policyInst->firstVFrameInfo.framePts - startpts, 90));
	} else {
		mediasync_pr_info(0,policyInst,"init a-cache:no video \n");
	}

	mediasync_pr_info(0,policyInst,"init startpts:0x%llx startPlayOffset:%d ",startpts,*startPlayOffset);
	return 0;
}

avsync_state videoInitPcr(mediasync_policy_instance *policyInst) {

	//s32 startflag = policyInst->mStartFlag;
	//s64 curSystime = 0;

	//s32 avdiff = 0;
	//s64 startpts = 0;
	//s64 startpts_a = 0;
	//s64 startpts_v = 0;

	s32 startPlayOffset = 0;

	sync_stream_type invalid_stream = MEDIA_TYPE_MAX;
	bool DmxPcrInvalid = true;

	//curSystime = get_system_time_us();
	mediasync_ins_get_firstaudioframeinfo(policyInst->mMediasyncIns,&(policyInst->firstAFrameInfo));
	mediasync_ins_get_syncmode(policyInst->mMediasyncIns,(s32*)&(policyInst->mSyncMode));

	if (policyInst->mSyncMode == MEDIA_SYNC_PCRMASTER) {
		mediasync_ins_get_firstdmxpcrinfo(policyInst->mMediasyncIns,&(policyInst->firstDmxPcrInfo));
		if (checkDmxPcrValid(policyInst,&invalid_stream)) {
			policyInst->anchorFrameInfo.framePts = policyInst->firstDmxPcrInfo.framePts;
			policyInst->anchorFrameInfo.frameSystemTime = policyInst->firstDmxPcrInfo.frameSystemTime;
			policyInst->clockType = PCR_CLOCK;
			mediasync_ins_set_clocktype(policyInst->mMediasyncIns,PCR_CLOCK);
			DmxPcrInvalid = false;
			mediasync_pr_info(0,policyInst,"init clocktype = PCR_CLOCK \n");
		}
	}

	if (DmxPcrInvalid) {
		policyInst->anchorFrameInfo.framePts = policyInst->firstVFrameInfo.framePts;
		policyInst->anchorFrameInfo.frameSystemTime = mediasync_get_system_time_us();
		policyInst->clockType = VIDEO_CLOCK;
		mediasync_ins_set_clocktype(policyInst->mMediasyncIns,VIDEO_CLOCK);
		mediasync_pr_info(0,policyInst,"init clocktype = VIDEO_CLOCK \n");
	}
	mediasync_ins_set_refclockinfo(policyInst->mMediasyncIns,policyInst->anchorFrameInfo);

	calculateStartPtsOffset(policyInst,&startPlayOffset,invalid_stream);

	mediasync_ins_set_ptsadjust(policyInst->mMediasyncIns,0);
	mediasync_ins_set_startthreshold(policyInst->mMediasyncIns,startPlayOffset);
	mediasync_ins_set_clockstate(policyInst->mMediasyncIns,CLOCK_PROVIDER_NORMAL);
	policyInst->clockState = CLOCK_PROVIDER_NORMAL;

	return MEDIASYNC_RUNNING;
}

s64 get_stc(mediasync_policy_instance *policyInst,bool freerun) {

	u64 mCurPcr = 0;
	u64 Interval = 0;
	u32 k = policyInst->mSpeed.mNumerator * policyInst->mPcrSlope.mNumerator ;
	k = div_u64(k, policyInst->mSpeed.mDenominator);

	if (!freerun) {
		Interval = policyInst->updatInfo.mCurrentSystemtime -
					policyInst->anchorFrameInfo.frameSystemTime;
	} else {
		Interval = policyInst->updatInfo.mCurrentSystemtime -
				policyInst->freerunFrameInfo.frameSystemTime;
	}
	Interval = Interval * k;
	Interval = div_u64(Interval, policyInst->mPcrSlope.mDenominator);
	mCurPcr = div_u64(Interval * 9, 100);
	if (!freerun) {
		mCurPcr = mCurPcr + policyInst->anchorFrameInfo.framePts;
	} else {
		mCurPcr = mCurPcr + policyInst->freerunFrameInfo.framePts;
	}
	mCurPcr = mCurPcr - policyInst->mPtsAdjust - policyInst->mStartThreshold;
	policyInst->mCurPcr = mCurPcr;
	return 0;
}

const char* clockType2Str(mediasync_clocktype type)
{
	const char* str = NULL;

	switch (type) {
		case UNKNOWN_CLOCK:
			str = "UNW";
		break;

		case AUDIO_CLOCK:
			str = "AUD";
		break;

		case VIDEO_CLOCK:
			str = "VID";
		break;

		case PCR_CLOCK:
			str = "PCR";
		break;

		case REF_CLOCK:
			str = "REF";
		break;
	}

	return str;
}

const char* streamType2Str(sync_stream_type type)
{
    const char* str = NULL;

	switch (type) {
		case MEDIA_VIDEO:
			str = "V";
		break;

		case MEDIA_AUDIO:
			str = "A";
		break;

		case MEDIA_DMXPCR:
			str = "D";
		break;

		case MEDIA_SUBTITLE:
			str = "S";
		break;

		case MEDIA_COMMON:
			str = "C";
		break;

		case MEDIA_TYPE_MAX:
			str = "M";
		break;
	}

	return str;
}

const char* videoPolicy2Str(video_policy policy) {
    const char* str = NULL;

    switch (policy) {
    case MEDIASYNC_VIDEO_UNKNOWN:
        str = "UNKNOWN";
        break;

    case MEDIASYNC_VIDEO_NORMAL_OUTPUT:
        str = "OUTPUT";
        break;

    case MEDIASYNC_VIDEO_HOLD:
        str = "HOLD  ";
        break;

    case MEDIASYNC_VIDEO_DROP:
        str = "DROP  ";
        break;

    case MEDIASYNC_VIDEO_EXIT:
        str = "EXIT  ";
        break;

    }

    return str;
}

int videoDiscontinueProcess(mediasync_policy_instance *policyInst,bool mDiscontinueTimeOut) {
	//mediasync_frameinfo videoframeInfo;
	mediasync_frameinfo audioframeInfo;
	mediasync_frameinfo curQueueAudioInfo;
	mediasync_frameinfo curQueueVideoInfo;
	mediasync_frameinfo dmxPcrInfo;
	//mediasync_frameinfo info;
	sync_stream_type invalidstream = MEDIA_TYPE_MAX;
	s32 offset  = 0;
	s64 startpts = 0;
	s32 av_diff = 0;
	s32 audioExpectCache = 0;
	s32 videoExpectCache = 0;
	s64 audioStartPts = 0;
	s64 videoStartPts = 0;
	//s64 curSystime = 0;
	s64 anchor = 0;
	bool ret;
	s32 Threshold90K = policyInst->mDiscontinueCacheThreshold * 90;

	mediasync_ins_get_queue_audio_info(policyInst->mMediasyncIns,&curQueueAudioInfo);
	mediasync_ins_get_queue_video_info(policyInst->mMediasyncIns,&curQueueVideoInfo);
	/*
	if (mIsAmDolbyAudio) {
		if (policyInst->mDiscontinueCacheThreshold < 800) {
			audioExpectCache = 800 * 90;
		}
		} else {
			audioExpectCache = Threshold90K;
		}
	}
	*/
	if (policyInst->clockType == PCR_CLOCK) {
		//there are 3 cases:
		//audio + video
		//audio only: need further complete
		//video only: need further complete

		if ((policyInst->mStartFlag & MEDIASYNC_NO_AUDIO) == MEDIASYNC_NO_AUDIO) {
			// video only
			return 0;
		} else {
			// audio + video
			mediasync_ins_get_curdmxpcrinfo(policyInst->mMediasyncIns,&dmxPcrInfo);
			mediasync_ins_get_curaudioframeinfo(policyInst->mMediasyncIns,&audioframeInfo);
			ret = checkStreamPtsValid(audioframeInfo.framePts,policyInst->mCurVideoFrameInfo.framePts, dmxPcrInfo.framePts, &invalidstream);
			if (ret) {
				av_diff = (audioframeInfo.framePts - policyInst->mCurVideoFrameInfo.framePts) -
							div_u64((audioframeInfo.frameSystemTime - policyInst->mCurVideoFrameInfo.frameSystemTime) * 9, 100);

				if (invalidstream == MEDIA_TYPE_MAX || invalidstream == MEDIA_DMXPCR) {

					if (invalidstream == MEDIA_TYPE_MAX) {
						anchor = dmxPcrInfo.framePts;
					} else if (invalidstream == MEDIA_DMXPCR) {
						if (audioframeInfo.framePts <= policyInst->mCurVideoFrameInfo.framePts) {
						anchor = audioframeInfo.framePts;
						policyInst->clockType = AUDIO_CLOCK;
						mediasync_ins_set_clocktype(policyInst->mMediasyncIns,AUDIO_CLOCK);
					} else {
						anchor = policyInst->mCurVideoFrameInfo.framePts;
						policyInst->clockType = VIDEO_CLOCK;
						mediasync_ins_set_clocktype(policyInst->mMediasyncIns,VIDEO_CLOCK);
					}
				}
						/*
						 videoExpectCache: video expect cache is decided by mDiscontinueCacheThreshold
						 audioExpectCache: audio expect cache is decided by mDiscontinueCacheThreshold generally,
						 but in some special cases, mDiscontinueCacheThreshold is not enough, so need to consider
						 av distribute in the stream.
						*/
					videoExpectCache = Threshold90K;
					#if 0
					if (av_diff < 0 && mConsiderDistribute) {
						audioExpectCache = Threshold90K - av_diff;

						if (audioExpectCache > mExpectMaxCache * 90) {
							audioExpectCache = mExpectMaxCache * 90;
						}
					}
					/* else {
					   audioExpectCache = Threshold90K;
					}*/
					#endif
						/*
					 audioStartPts: the refclock pcr start from audioStartPts that can ensure audio has audioExpectCache
					 videoStartPts: the refclock pcr start from videoStartPts that can ensure video has videoExpectCache
					*/
					if (policyInst->updatInfo.mAudioInfo.cacheDuration >= audioExpectCache) {
						audioStartPts = audioframeInfo.framePts;
					} else {
						audioStartPts = audioframeInfo.framePts +
							policyInst->updatInfo.mAudioInfo.cacheDuration - audioExpectCache;
					}

					if (policyInst->updatInfo.mVideoInfo.cacheDuration >= videoExpectCache) {
						videoStartPts = policyInst->mCurVideoFrameInfo.framePts;
					} else {
						videoStartPts = policyInst->mCurVideoFrameInfo.framePts + policyInst->updatInfo.mVideoInfo.cacheDuration - videoExpectCache;
					}
					/* startpts: the refclock pcr start from startpts that can ensure both audio and video has expect cache */
					//startpts = fmin(audioStartPts, videoStartPts);
					startpts = audioStartPts < videoStartPts ? audioStartPts : videoStartPts;
					mediasync_pr_info(0,policyInst,"[type:%s, invalid:%s],"
						"cache[A:%lld,V:%lld]ms, expect_cache[A:%lld,V:%lld]ms,av-diff:%lld ms,"
						"apts:[%lld, %lld],"
						"vpts:[%lld, %lld],"
						"dmx:%lld.",
						clockType2Str(policyInst->clockType),
						streamType2Str(invalidstream),
						div_u64(policyInst->updatInfo.mAudioInfo.cacheDuration,90),
						div_u64(policyInst->updatInfo.mVideoInfo.cacheDuration,90),
						div_u64(audioExpectCache,90),
						div_u64(videoExpectCache,90),
						div_u64(av_diff,90),
						audioframeInfo.framePts,
						curQueueAudioInfo.framePts,
						policyInst->mCurVideoFrameInfo.framePts,
						curQueueVideoInfo.framePts,
						dmxPcrInfo.framePts);
				} else if (invalidstream == MEDIA_AUDIO) {
					anchor = dmxPcrInfo.framePts;

					videoExpectCache = Threshold90K;
					if (policyInst->updatInfo.mVideoInfo.cacheDuration >= videoExpectCache) {
						startpts = policyInst->mCurVideoFrameInfo.framePts;
					} else {
						startpts = policyInst->mCurVideoFrameInfo.framePts + policyInst->updatInfo.mVideoInfo.cacheDuration - videoExpectCache;
					}
					mediasync_pr_info(0,policyInst,"[type:%s, invalid:%s],"
							"cacheV:%lld ms, expect_cache:%lld ms,"
							"vpts:[%lld, %lld],"
							"dmx:%lld.",\
							clockType2Str(policyInst->clockType),
							streamType2Str(invalidstream),
							div_u64(policyInst->updatInfo.mVideoInfo.cacheDuration, 90),
							div_u64(videoExpectCache,90),
							policyInst->mCurVideoFrameInfo.framePts,
							curQueueVideoInfo.framePts, dmxPcrInfo.framePts);
				} else if (invalidstream == MEDIA_VIDEO) {
					anchor = dmxPcrInfo.framePts;
					//audioExpectCache = Threshold90K;
					if (policyInst->updatInfo.mAudioInfo.cacheDuration >= audioExpectCache) {
						startpts = audioframeInfo.framePts;
					} else {
						startpts = audioframeInfo.framePts + policyInst->updatInfo.mAudioInfo.cacheDuration - audioExpectCache;
					}
					mediasync_pr_info(0,policyInst,"[type:%s, invalid:%s],"
							"cacheV:%lld ms, expect_cache:%lld ms,"
							"apts:[%lld, %lld],"
							"dmx:%lld.",\
							clockType2Str(policyInst->clockType), streamType2Str(invalidstream),
							div_u64(policyInst->updatInfo.mAudioInfo.cacheDuration,90),
							div_u64(audioExpectCache,90),
							audioframeInfo.framePts,
							curQueueVideoInfo.framePts,
							dmxPcrInfo.framePts);
				}
			} else {
				mediasync_pr_info(0,policyInst,"exception: AV FREE RUN!");
				return 0;
			}
		}
	} else if ((policyInst->clockType == AUDIO_CLOCK ||
		policyInst->clockType == VIDEO_CLOCK)) {

		mediasync_ins_get_curaudioframeinfo(policyInst->mMediasyncIns,&audioframeInfo);
		if (policyInst->clockType == AUDIO_CLOCK) {
			anchor = audioframeInfo.framePts;
		} else if (policyInst->clockType == VIDEO_CLOCK) {
			anchor = policyInst->mCurVideoFrameInfo.framePts;
		}

		if (mDiscontinueTimeOut) {
			if (policyInst->clockType == AUDIO_CLOCK) {
				if (policyInst->updatInfo.mAudioInfo.cacheDuration >= Threshold90K)
					startpts = audioframeInfo.framePts;
				else
					startpts = audioframeInfo.framePts + policyInst->updatInfo.mAudioInfo.cacheDuration - Threshold90K;

			} else if (policyInst->clockType == VIDEO_CLOCK) {
				if (policyInst->updatInfo.mVideoInfo.cacheDuration >= Threshold90K)
					startpts = policyInst->mCurVideoFrameInfo.framePts;
				else
					startpts = policyInst->mCurVideoFrameInfo.framePts +
						policyInst->updatInfo.mVideoInfo.cacheDuration - Threshold90K;

			}
			mDiscontinueTimeOut = false;
			mediasync_pr_info(0,policyInst,"type:%s, discontinue time out.",
				clockType2Str(policyInst->clockType));
		} else {
			av_diff = (audioframeInfo.framePts - policyInst->mCurVideoFrameInfo.framePts) -
						div_u64((audioframeInfo.frameSystemTime -
									policyInst->mCurVideoFrameInfo.frameSystemTime) * 9,100);

			videoExpectCache = Threshold90K;
#if 0
			if (av_diff < 0 && mConsiderDistribute) {
			audioExpectCache = Threshold90K - av_diff;
			if (audioExpectCache > mExpectMaxCache * 90) {
			audioExpectCache = mExpectMaxCache * 90;
			}
			}
			/*
			else {
			audioExpectCache = Threshold90K;
			}
			*/
#endif
			if (policyInst->updatInfo.mAudioInfo.cacheDuration >= audioExpectCache) {
				audioStartPts = audioframeInfo.framePts;
			} else {
				audioStartPts = audioframeInfo.framePts +
					policyInst->updatInfo.mAudioInfo.cacheDuration - audioExpectCache;
			}
			if (policyInst->updatInfo.mVideoInfo.cacheDuration >= videoExpectCache) {
				videoStartPts = policyInst->mCurVideoFrameInfo.framePts;
			} else {
				videoStartPts = policyInst->mCurVideoFrameInfo.framePts +
					policyInst->updatInfo.mVideoInfo.cacheDuration - videoExpectCache;
			}
			startpts = audioStartPts < videoStartPts ? audioStartPts : videoStartPts;
		}
		mediasync_pr_info(0,policyInst,"type:%s,"
			"cache[A:%lld,V:%lld]ms, expect_cache[A:%lld,V:%lld]ms,av-diff:%lld ms,"
			"apts:[%lld, %lld],"
			"vpts:[%lld, %lld].",
			clockType2Str(policyInst->clockType),
			div_u64(policyInst->updatInfo.mAudioInfo.cacheDuration,90),
			div_u64(policyInst->updatInfo.mVideoInfo.cacheDuration,90),
			div_u64(audioExpectCache,90),
			div_u64(videoExpectCache,90),
			div_u64(av_diff,90),
			audioframeInfo.framePts,
			curQueueAudioInfo.framePts,
			policyInst->mCurVideoFrameInfo.framePts,
			curQueueVideoInfo.framePts);
	}

		offset = anchor - startpts;


		policyInst->anchorFrameInfo.framePts = anchor;
		policyInst->anchorFrameInfo.frameSystemTime = policyInst->updatInfo.mCurrentSystemtime;
		mediasync_ins_set_refclockinfo(policyInst->mMediasyncIns,policyInst->anchorFrameInfo);


		mediasync_ins_set_ptsadjust(policyInst->mMediasyncIns,0);
		mediasync_ins_set_startthreshold(policyInst->mMediasyncIns,offset);


	return 0;
}


int videoCheckDiscontinue(mediasync_policy_instance *policyInst,s64 vpts) {

	s64 sourcePts = -1;
	s64 diff = 0;
	s64 diffpv = 0;
	bool videoCheckDiscontinue = false;
	bool mDiscontinueTimeOut = false;
	mediasync_frameinfo mDmxPcrInfo;
	mediasync_frameinfo audioframeInfo;
	mediasync_frameinfo audiofirstframeInfo;
	if (policyInst->clockType == UNKNOWN_CLOCK) {
		mediasync_ins_get_clocktype(policyInst->mMediasyncIns,&(policyInst->clockType));
	}
	policyInst->clockState = policyInst->updatInfo.mSourceClockState;

	if (policyInst->clockState != CLOCK_PROVIDER_DISCONTINUE) {
		//video-only case

		//video-only + VMASTER situation, no need to consider
		//video-only + PCRMASTER situation
		//av + VMASTER situation
		if (policyInst->clockType == VIDEO_CLOCK) {
			if ((policyInst->mStartFlag & MEDIASYNC_NO_AUDIO) == MEDIASYNC_NO_AUDIO) {
				//only video
				return 0;
			} else {
				videoCheckDiscontinue = true;
			}
		} else {
			mediasync_ins_get_firstaudioframeinfo(policyInst->mMediasyncIns,&audiofirstframeInfo);
			mediasync_ins_get_curaudioframeinfo(policyInst->mMediasyncIns,&audioframeInfo);
			if (audioframeInfo.frameSystemTime == -1 &&
				audiofirstframeInfo.frameSystemTime == -1) {
				videoCheckDiscontinue = true;
			} else {
				return 0;
			}
		}

		if (videoCheckDiscontinue) {
			diffpv = ABSSUB(policyInst->mCurPcr, policyInst->mCurVideoFrameInfo.framePts);
			if (policyInst->clockType == PCR_CLOCK) {

				mediasync_ins_get_curdmxpcrinfo(policyInst->mMediasyncIns,&mDmxPcrInfo);
				//PCRMASTER, refPcr calculate is different
				diff = ABSSUB(policyInst->mCurPcr, mDmxPcrInfo.framePts);
				if (diffpv > DEFAULT_TRIGGER_DISCONTINUE_THRESHOLD &&
					diff > DEFAULT_TRIGGER_DISCONTINUE_THRESHOLD) {
					policyInst->clockState = CLOCK_PROVIDER_DISCONTINUE;
					mediasync_ins_set_clockstate(policyInst->mMediasyncIns,CLOCK_PROVIDER_DISCONTINUE);
					policyInst->mEnterDiscontinueTime = policyInst->updatInfo.mCurrentSystemtime;
					mediasync_pr_info(0,policyInst,"av + VMASTER discontinue,"
						"[ref:%lld,demuxpts:%lld, vpts:%lld, pvdiff:%lld ms]!",
						policyInst->mCurPcr,mDmxPcrInfo.framePts,
						policyInst->mCurVideoFrameInfo.framePts, div_u64(diffpv,90));
				}

			} else {
				if (diffpv > DEFAULT_TRIGGER_DISCONTINUE_THRESHOLD) {
					policyInst->clockState = CLOCK_PROVIDER_DISCONTINUE;
					mediasync_ins_set_clockstate(policyInst->mMediasyncIns,CLOCK_PROVIDER_DISCONTINUE);
					policyInst->mEnterDiscontinueTime = mediasync_get_system_time_us();
					mediasync_pr_info(0,policyInst,"av + VMASTER discontinue,"
						"[ref:%lld, source:%lld, pvdiff:%lld ms]!",
						policyInst->mCurPcr, sourcePts, div_u64(diffpv,90));
				}

			}
		}

	} else {
		//check whether remove discontinue
		//mDiscontinueCount = 0;
		//no discontinue case in video-only + VMASTER situation, consider PCR_CLOCK enough
		//video-only + PCRMASTER situation
		mediasync_ins_get_curaudioframeinfo(policyInst->mMediasyncIns,&audioframeInfo);
		if (audioframeInfo.frameSystemTime != -1) {
			return 0;
		}
		if (policyInst->clockType == PCR_CLOCK) {
			mediasync_ins_get_curdmxpcrinfo(policyInst->mMediasyncIns,&mDmxPcrInfo);
			sourcePts = mDmxPcrInfo.framePts - policyInst->mPtsAdjust - policyInst->mStartThreshold;//- mRefClock.getPcrAdjust() - mRefClock.getStartPlayThreshold();

			diffpv = ABSSUB(sourcePts,policyInst->mCurVideoFrameInfo.framePts);
			if (diffpv < DEFAULT_REMOVE_DISCONTINUE_THRESHOLD) {

				policyInst->clockState = CLOCK_PROVIDER_NORMAL;
				mediasync_ins_set_clockstate(policyInst->mMediasyncIns,CLOCK_PROVIDER_NORMAL);

				mediasync_pr_info(0,policyInst,
				"v-only + PCRMASTER discontinue end, [source:%lld, v:%lld, diff:%lld ms]!",\
						mDmxPcrInfo.framePts, policyInst->mCurVideoFrameInfo.framePts,div_u64(diffpv,90));
				policyInst->mEnterDiscontinueTime = -1;
			} else if (policyInst->updatInfo.mCurrentSystemtime -
						policyInst->mEnterDiscontinueTime >
							TIME_DISCONTINUE_DURATION) {
				policyInst->clockState = CLOCK_PROVIDER_NORMAL;
				mediasync_ins_set_clockstate(policyInst->mMediasyncIns,CLOCK_PROVIDER_NORMAL);
				mediasync_pr_info(0,policyInst,"v-only + PCRMASTER discontinue timeout!");
				mDiscontinueTimeOut = true;
				policyInst->mEnterDiscontinueTime = -1;
			}

			if (policyInst->clockState == CLOCK_PROVIDER_NORMAL) {
				videoDiscontinueProcess(policyInst,mDiscontinueTimeOut);
			}

		} else {

			diff = ABSSUB(vpts,policyInst->mCurVideoFrameInfo.framePts);
			if (diff < DEFAULT_REMOVE_DISCONTINUE_THRESHOLD) {
				policyInst->clockState = CLOCK_PROVIDER_NORMAL;
				mediasync_ins_set_clockstate(policyInst->mMediasyncIns,CLOCK_PROVIDER_NORMAL);
				mediasync_pr_info(0,policyInst,
						"v-only + VMASTER discontinue end, [source:%lld, v:%lld, diff:%lld ms time:%lld]!",\
						policyInst->mCurVideoFrameInfo.framePts, vpts,
						div_u64(diff,90),
						policyInst->updatInfo.mCurrentSystemtime -
						policyInst->mCurVideoFrameInfo.frameSystemTime);
				policyInst->mEnterDiscontinueTime = -1;
			} else if (policyInst->updatInfo.mCurrentSystemtime -
						policyInst->mEnterDiscontinueTime >
							TIME_DISCONTINUE_DURATION) {
				policyInst->clockState = CLOCK_PROVIDER_NORMAL;
				mediasync_ins_set_clockstate(policyInst->mMediasyncIns,CLOCK_PROVIDER_NORMAL);
				mediasync_pr_info(0,policyInst,"v-only + PCRMASTER discontinue timeout!");
				policyInst->mEnterDiscontinueTime = -1;
			}
			if (policyInst->clockState == CLOCK_PROVIDER_NORMAL) {
				//caution: refclok first frame pts need to calculate
				//dmxPcrInfo.framePts = dmxPcrInfo.framePts + mRefClock.getPcrAdjust() + mRefClock.getStartPlayThreshold();
				//setRefClockInfo(dmxPcrInfo);
				policyInst->anchorFrameInfo.framePts = vpts;
				policyInst->anchorFrameInfo.frameSystemTime =
					policyInst->updatInfo.mCurrentSystemtime;
				mediasync_ins_set_refclockinfo(policyInst->mMediasyncIns,policyInst->anchorFrameInfo);
				mediasync_ins_set_ptsadjust(policyInst->mMediasyncIns,0);
				mediasync_ins_set_startthreshold(policyInst->mMediasyncIns,0);
			}
		}
	}

	return 0;
}

int checkVideoFreeRun(mediasync_policy_instance *policyInst,s64 vpts,bool* isVideoFreeRun)
{
	//video free run case: 1.video-only media, VMASTER; 2. refclock discontinue; 3. pv diff is too large
	//if refclock diff with vpts to much, need to use vpts to adjust.
	// check whether video frame comes too late


	bool videoComesLate = false;
	s32 hasaudio = -1;
	s64 tmppts = 0;
	s64 videoArriveDiff = policyInst->updatInfo.mCurrentSystemtime -
								policyInst->mCurVideoFrameInfo.frameSystemTime;
	*isVideoFreeRun = false;
	//videoArriveDiff > 100ms
	if (videoArriveDiff > 100000 && policyInst->videoLastPolicy != MEDIASYNC_VIDEO_HOLD) {
		mediasync_pr_info(0,policyInst,
		"video frame comes later:%lld us",videoArriveDiff);
		videoComesLate = true;
	}

	mediasync_ins_get_hasaudio(policyInst->mMediasyncIns,&hasaudio);


	//mediasync_pr_info(0,policyInst,"clockType:%d mStartFlag:0x%x hasaudio:%d",
	//	policyInst->clockType,policyInst->mStartFlag,hasaudio);
	//mediasync_pr_info(0,policyInst,"clockState:%d P:%lld v:%lld diff:%lld mVideoSyncThreshold:%d",
	//	policyInst->clockState,
	//	policyInst->mCurPcr,vpts,
	//	ABSSUB(policyInst->mCurPcr, vpts),
	//	policyInst->mVideoSyncThreshold);

	if (((policyInst->clockType == VIDEO_CLOCK || videoComesLate) &&
		(policyInst->mStartFlag & MEDIASYNC_NO_AUDIO) == MEDIASYNC_NO_AUDIO) &&
		hasaudio != 1  &&
		policyInst->mVideoTrickMode == VIDEO_TRICK_MODE_NONE) {
		// > 200ms --> (200000 * 9 / 100)
		if (ABSSUB(policyInst->mCurPcr, vpts) > 18000) {
			tmppts = vpts + policyInst->mPtsAdjust + policyInst->mStartThreshold;

			policyInst->anchorFrameInfo.framePts = tmppts;
			policyInst->anchorFrameInfo.frameSystemTime = policyInst->updatInfo.mCurrentSystemtime ;

			mediasync_ins_set_refclockinfo(policyInst->mMediasyncIns,policyInst->anchorFrameInfo);

			get_stc(policyInst,false);

			if (videoComesLate) {
				mediasync_pr_info(1,policyInst,"video only free run. ArriveDiff:%lld us",videoArriveDiff);
			}
		}
	} else if (policyInst->clockState == CLOCK_PROVIDER_DISCONTINUE ||
				(/*mVideoDuringSlowSyncEnable == 0 &&*/
				ABSSUB(policyInst->mCurPcr, vpts) > policyInst->mVideoSyncThreshold)||
				/*(mVideoDuringSlowSyncEnable == 1 &&
				ABSSUB(mCurPcr, actualVpts) > mVideoSyncThreshold && mCurPcr > actualVpts) ||*/
				policyInst->mVideoFreeRun) {

		if (policyInst->freerunFrameInfo.framePts == -1) {
			policyInst->freerunFrameInfo.framePts = vpts;
			policyInst->freerunFrameInfo.frameSystemTime = policyInst->updatInfo.mCurrentSystemtime;
			//anchor of video freerun clock update
			//mVideoFreerunAnchorUpdate = true;
		}
		get_stc(policyInst,true);

		//mCurPcr = mVideoClock.getStartFreerunPts() +
		//         (curSystime - mVideoClock.getStartFreerunTime()) * 9 * mSlopeMultRate / 100000;
		//18000 = 200ms*90
		if (ABSSUB(policyInst->mCurPcr, vpts) > 18000) {
			policyInst->freerunFrameInfo.framePts = vpts;
			policyInst->freerunFrameInfo.frameSystemTime = policyInst->updatInfo.mCurrentSystemtime;

			policyInst->mCurPcr = vpts;
			//anchor of video freerun clock update
			// mVideoFreerunAnchorUpdate = true;
		}
		*isVideoFreeRun = true;
		//if (logPrintable) {
		//mediasync_pr_info(0,policyInst,"video free run. mVideoFreeRun:%d",policyInst->mVideoFreeRun);
		//}
	}
	//remove discontinue, need update mRefClock
	if (policyInst->mLastClockProviderState == CLOCK_PROVIDER_DISCONTINUE &&
		policyInst->clockState != CLOCK_PROVIDER_DISCONTINUE) {

		mediasync_ins_get_refclockinfo(policyInst->mMediasyncIns,&policyInst->anchorFrameInfo);
		if (!policyInst->mVideoFreeRun) {
			policyInst->freerunFrameInfo.framePts = -1;
			policyInst->freerunFrameInfo.frameSystemTime = -1;
			*isVideoFreeRun = false;
		}
		get_stc(policyInst,true);
	} else if (!policyInst->mVideoFreeRun &&
				!*isVideoFreeRun &&
				policyInst->mLastClockProviderState == CLOCK_PROVIDER_NORMAL &&
				policyInst->clockState == CLOCK_PROVIDER_NORMAL) {
		if (policyInst->freerunFrameInfo.framePts != -1) {
			mediasync_pr_info(0,policyInst,"mVideoFreeRun 1-->0 ");
			policyInst->freerunFrameInfo.framePts = -1;
			policyInst->freerunFrameInfo.frameSystemTime = -1;
		}
	}
	policyInst->mLastClockProviderState = policyInst->clockState;

	return 0;
}


int videoUpdateRefClock(mediasync_policy_instance *policyInst,s64 pts,int* pauseResumeStatus,int* holdTime) {

	mediasync_frameinfo frameInfo;
	mediasync_frameinfo pauseFrameInfo;
	int adjust  = 0;
	int hasaudio = -1;
	bool resetRefClockInfo = false;
	(void) holdTime;
	if (policyInst->clockType == UNKNOWN_CLOCK) {
		mediasync_ins_get_clocktype(policyInst->mMediasyncIns,&policyInst->clockType);
	}

	//case1: pause --> resume
	//MS_LOGI(streamType2Str(mStreamtype), mSyncInsId, mPlayerInstanceNo,"pause->resume,type:%s. [%" PRIx64 ", %" PRIx64 "] ",
	//                clockType2Str(sourceClock),pts, systemTime);
	if (policyInst->clockType != PCR_CLOCK) {
		mediasync_ins_get_firstaudioframeinfo(policyInst->mMediasyncIns,&frameInfo);
		mediasync_ins_get_pause_audio_info(policyInst->mMediasyncIns,&pauseFrameInfo);

		if (policyInst->clockType == VIDEO_CLOCK) {
			frameInfo.framePts = pts;
			frameInfo.frameSystemTime = policyInst->updatInfo.mCurrentSystemtime;
			adjust = 0;
			*pauseResumeStatus = 0;
			mediasync_pr_info(0,policyInst,
				"VIDEO_CLOCK vpts update refclock:[%lld, %lld].",
					pts, policyInst->updatInfo.mCurrentSystemtime);
		} else if ((policyInst->clockType == AUDIO_CLOCK && frameInfo.framePts < 0) /*||
		(sourceClock == AUDIO_CLOCK && pauseFrameInfo.framePts != -1)*/) {
			frameInfo.framePts = pts;
			frameInfo.frameSystemTime = policyInst->updatInfo.mCurrentSystemtime;

			adjust = 0;
			*pauseResumeStatus = 0;
			mediasync_pr_info(0,policyInst,
				"AUDIO_CLOCK vpts update refclock:[%lld, %lld]. diff:%lld us",
				pts, policyInst->updatInfo.mCurrentSystemtime,
				div_u64((pts - frameInfo.framePts) * 100 ,9 ));
		} else {
			pauseFrameInfo.framePts = pts;
			pauseFrameInfo.frameSystemTime = policyInst->updatInfo.mCurrentSystemtime;
			//setPauseVideoInfo(pauseFrameInfo);
			mediasync_ins_set_pause_video_info(policyInst->mMediasyncIns,pauseFrameInfo);
			//MS_LOGI(MSYNC_DEBUG_LEVEL_0,mLogHead,
			//    "setPauseVideoInfo vpts:%" PRIx64 " audioPausePts:%" PRIx64 "",pts,pauseFrameInfo.framePts);
			return 0;
		}
		if (policyInst->mCurPcr - policyInst->anchorFrameInfo.framePts >
			policyInst->mStartThreshold ||
			policyInst->mVideoStarted) {
			resetRefClockInfo = true;
		}
	} else {
		mediasync_ins_get_hasaudio(policyInst->mMediasyncIns,&hasaudio);
		//getHasAudio(&hasaudio);
		//apts has the higher priority
		if (hasaudio == 0) {
			//getCurDmxPcrInfo(&frameInfo);

			mediasync_ins_get_curdmxpcrinfo(policyInst->mMediasyncIns,&frameInfo);
			adjust = frameInfo.framePts - pts;
			resetRefClockInfo = true;
			*pauseResumeStatus = 0;
			mediasync_pr_info(0,policyInst,"video mediasync update refclock.");
		}
	}

	if (resetRefClockInfo) {
		policyInst->anchorFrameInfo.framePts = frameInfo.framePts;
		policyInst->anchorFrameInfo.frameSystemTime = mediasync_get_system_time_us();
		mediasync_ins_set_refclockinfo(policyInst->mMediasyncIns,policyInst->anchorFrameInfo);
		mediasync_ins_set_ptsadjust(policyInst->mMediasyncIns,adjust);
		mediasync_ins_set_startthreshold(policyInst->mMediasyncIns,0);
		policyInst->mPtsAdjust = adjust;
		policyInst->mStartThreshold = 0;
	}
	mediasync_ins_set_pauseresume(policyInst->mMediasyncIns,0);

    return 0;
}

int doVideoHandleInitState(mediasync_policy_instance *policyInst,avsync_state* syncState) {

	s32 hasaudio = 0;
	mediasync_ins_get_hasaudio(policyInst->mMediasyncIns,&hasaudio);
	if (1 != hasaudio) {
		*syncState = MEDIASYNC_AV_ARRIVED;
		mediasync_ins_set_avsyncstate(policyInst->mMediasyncIns,MEDIASYNC_AV_ARRIVED);
		policyInst->mStartFlag = MEDIASYNC_NO_AUDIO;
	} else if (1 == hasaudio) {
		*syncState = MEDIASYNC_VIDEO_ARRIVED;
		mediasync_ins_set_avsyncstate(policyInst->mMediasyncIns,MEDIASYNC_VIDEO_ARRIVED);
	}
	return 0;
}

int doVideoHandleAudioArriveState(mediasync_policy_instance *policyInst,avsync_state* syncState) {

	s32 hasaudio = 0;
	mediasync_ins_get_hasaudio(policyInst->mMediasyncIns,&hasaudio);

	if (hasaudio != 1) {
		policyInst->mStartFlag = MEDIASYNC_NO_AUDIO;
		*syncState = MEDIASYNC_AV_SYNCED;
		mediasync_ins_set_avsyncstate(policyInst->mMediasyncIns,MEDIASYNC_AV_SYNCED);
	}

	return 0;
}


int doVideoHandleVideoArriveState(mediasync_policy_instance *policyInst,avsync_state* syncState) {

	int64_t waitTimeOutThresholdUs = 800000;
	int muteFlag = 0;
	aml_Source_Type sourceType = TS_DEMOD;
	s64 curtime = 0;
	mediasync_ins_get_audiomute(policyInst->mMediasyncIns, &muteFlag);
	mediasync_ins_get_source_type(policyInst->mMediasyncIns,&sourceType);

	if (sourceType == TS_MEMORY &&
		muteFlag == 1 &&
		policyInst->mVideoTrickMode != VIDEO_TRICK_MODE_PAUSE_NEXT) {
		waitTimeOutThresholdUs = 0; // default wait 50ms
		mediasync_pr_info(0,policyInst,"waitTimeOutThresholdUs = 0");
	}
#if 1
    if (policyInst->updatInfo.mPauseResumeFlag) {
        mediasync_pr_info(0,policyInst,"update systemtime");
       // mVideoClock.setFirstFrameArriveTime(mUpdateInfo.mCurrentSystemtime);

		//policyInst->firstVFrameInfo.framePts = vpts;
		policyInst->firstVFrameInfo.frameSystemTime= mediasync_get_system_time_us();
		mediasync_ins_set_firstvideoframeinfo(policyInst->mMediasyncIns,
		policyInst->firstVFrameInfo);

		policyInst->mCurVideoFrameInfo.framePts = policyInst->firstVFrameInfo.framePts;
		policyInst->mCurVideoFrameInfo.frameSystemTime = policyInst->firstVFrameInfo.frameSystemTime;
		mediasync_ins_set_curvideoframeinfo(policyInst->mMediasyncIns,
				policyInst->mCurVideoFrameInfo);
		//getFccEnable();
		//if (mStartSlowSyncInfo.mSlowSyncEnable) {
		//    mStartSlowSyncInfo.mSlowSyncFrameShowTime = mUpdateInfo.mCurrentSystemtime;
		//}
		//setPauseResumeFlag(0);
		mediasync_ins_set_pauseresume(policyInst->mMediasyncIns,0);
		policyInst->updatInfo.mPauseResumeFlag = 0;
    }
#endif
	curtime = mediasync_get_system_time_us();

	if (curtime - policyInst->firstVFrameInfo.frameSystemTime > waitTimeOutThresholdUs) {
		//mFirstEnterAVarriveTime = -1;
		//setStartPlayStrategy(-1,mVideoClock.getFirstFramePts());

		policyInst->mStartFlag = MEDIASYNC_VIDEO_COME_NORAML | MEDIASYNC_AUDIO_COME_LATER;

		mediasync_ins_set_avsyncstate(policyInst->mMediasyncIns,MEDIASYNC_AV_ARRIVED);
		*syncState = MEDIASYNC_AV_ARRIVED;

		mediasync_pr_info(0,policyInst,"exception: VIDEO_ARRIVE state, audio come later.muteFlag=%d,sourceType=%d, waitTimeOutThresholdUs=%lld ",
			muteFlag, sourceType, waitTimeOutThresholdUs);

	}

	return 0;
}

int doVideoHandleAVArriveState(mediasync_policy_instance *policyInst,avsync_state* syncState) {
		//MS_LOGE(mLogHead, "mStartFlag = 0x%x",mStartFlag);

	if ((policyInst->mStartFlag & MEDIASYNC_NO_AUDIO) == MEDIASYNC_NO_AUDIO ||
		(policyInst->mStartFlag  == (MEDIASYNC_VIDEO_COME_NORAML | MEDIASYNC_AUDIO_COME_LATER))) {

		mediasync_ins_set_avsyncstate(policyInst->mMediasyncIns,MEDIASYNC_AV_SYNCED);

		*syncState = MEDIASYNC_AV_SYNCED;
	} else {
		s32 hasaudio = 0;
		mediasync_ins_get_hasaudio(policyInst->mMediasyncIns,&hasaudio);
		if (hasaudio != 1) {
			mediasync_pr_info(0,policyInst,"audio reset,hasaudio:%d",hasaudio);
			policyInst->mStartFlag  = MEDIASYNC_NO_AUDIO;

			mediasync_ins_set_avsyncstate(policyInst->mMediasyncIns,MEDIASYNC_AV_SYNCED);
			*syncState = MEDIASYNC_AV_SYNCED;
		}
	}

	return 0;
}

int doVideoHandleAVSyncedState(mediasync_policy_instance *policyInst,avsync_state* syncState) {
//    mediasync_frameinfo tmpinfo;
//mediasync-video object handle no audio case
//mediasync-audio object handle no video case and contain a/v case
	avsync_state avstatus = MEDIASYNC_INIT;
	if ((policyInst->mStartFlag & MEDIASYNC_NO_AUDIO) == MEDIASYNC_NO_AUDIO ||
		(policyInst->mStartFlag  == (MEDIASYNC_VIDEO_COME_NORAML | MEDIASYNC_AUDIO_COME_LATER))) {
		avstatus = videoInitPcr(policyInst);
		mediasync_ins_set_avsyncstate(policyInst->mMediasyncIns,avstatus);
		*syncState = avstatus;
	}
	return 0;
}

int doVideoHandleAvLostSync(mediasync_policy_instance *policyInst,avsync_state* syncState,int64_t setLostSyncTimeUs) {

#if 0
	mediasync_frameinfo refInfo;
	getRefClockInfo(&refInfo);
	if (!mVideoStarted) {
	setAVSyncState(MEDIASYNC_INIT);
	*syncState = MEDIASYNC_INIT;
	mFirstLostSyncTimeUs = -1;
	return AM_MEDIASYNC_OK;
	}
#endif
	s64 nowTime;
	if (*syncState == MEDIASYNC_AUDIO_LOST_SYNC) {
		nowTime = mediasync_get_system_time_us();

		if (nowTime - setLostSyncTimeUs >= 1000000) {
			mediasync_pr_info(0,policyInst,
				"more than 1s need revert running nowUs(%lld)-(lasttime:%lld)=%lld",
				nowTime,
				setLostSyncTimeUs,
				nowTime-setLostSyncTimeUs);
			//setAVSyncState(MEDIASYNC_RUNNING);
			mediasync_ins_set_avsyncstate(policyInst->mMediasyncIns,MEDIASYNC_RUNNING);
			*syncState = MEDIASYNC_RUNNING;
			///mFirstLostSyncTimeUs = -1;
		}
	}
	return 0;
}

bool mediasync_video_state_process(mediasync_policy_instance *policyInst,avsync_state* syncState) {

	bool ret = true;
	//mediasync_avsync_state_cur_time_us avsync_state;
	//mediasync_ins_get_avsync_state_cur_time_us(policyInst->mMediasyncIns,&avsync_state);

	*syncState = policyInst->updatInfo.mAvSyncState;
	if (*syncState == MEDIASYNC_RUNNING) {
		return true;
	} else if (*syncState == MEDIASYNC_EXIT) {
		return false;
	}

	if (*syncState == MEDIASYNC_INIT) {
		doVideoHandleInitState(policyInst,syncState);
		ret = false;
	}

	if (*syncState == MEDIASYNC_AUDIO_ARRIVED) {
		doVideoHandleAudioArriveState(policyInst,syncState);
		ret = false;
	}

	if (*syncState == MEDIASYNC_VIDEO_ARRIVED) {
		doVideoHandleVideoArriveState(policyInst,syncState);
		ret = false;
	}

	if (*syncState == MEDIASYNC_AV_ARRIVED) {
		doVideoHandleAVArriveState(policyInst,syncState);
		ret = false;
	}

	if (*syncState == MEDIASYNC_AV_SYNCED) {
		doVideoHandleAVSyncedState(policyInst,syncState);
		ret = false;
	}

	if (*syncState == MEDIASYNC_VIDEO_LOST_SYNC ||
		*syncState == MEDIASYNC_AUDIO_LOST_SYNC) {
		doVideoHandleAvLostSync(policyInst,syncState,policyInst->updatInfo.mSetStateCurTimeUs);
		ret = true;
	}
	return ret;

}

int mediasync_get_update_info(mediasync_policy_instance *policyInst)
{
	mediasync_control mediasyncControl;
	u32 StcParmUpdateCount = 0;
	mediasyncControl.cmd = GET_UPDATE_INFO;
	mediasyncControl.size = sizeof(mediasync_update_info);
	mediasyncControl.ptr = (unsigned long)(&policyInst->updatInfo);
	StcParmUpdateCount = policyInst->updatInfo.mStcParmUpdateCount;
	mediasync_ins_ext_ctrls(policyInst->mMediasyncIns,&mediasyncControl);
	if (StcParmUpdateCount != policyInst->updatInfo.mStcParmUpdateCount) {
		mediasync_ins_get_refclockinfo(policyInst->mMediasyncIns,&policyInst->anchorFrameInfo);
		mediasync_ins_get_mediatime_speed(policyInst->mMediasyncIns,&policyInst->mSpeed);
		mediasync_ins_get_pcrslope(policyInst->mMediasyncIns,&policyInst->mPcrSlope);
		mediasync_ins_get_startthreshold(policyInst->mMediasyncIns,&policyInst->mStartThreshold);
		mediasync_ins_get_ptsadjust(policyInst->mMediasyncIns, &policyInst->mPtsAdjust);
		mediasync_ins_get_clocktype(policyInst->mMediasyncIns,&policyInst->clockType);
		mediasync_ins_get_paused(policyInst->mMediasyncIns,&policyInst->isPause);
		mediasyncControl.cmd = GET_TRICK_MODE;
		mediasyncControl.size = sizeof(mediasync_update_info);
		mediasyncControl.ptr = 0;
		mediasync_ins_ext_ctrls(policyInst->mMediasyncIns,&mediasyncControl);
		policyInst->mVideoTrickMode = mediasyncControl.value;
		mediasync_pr_info(0,policyInst,"update pts:%lld mSpeed:%d StartThreshold:%d",
			policyInst->anchorFrameInfo.framePts,policyInst->mSpeed.mNumerator,
			policyInst->mStartThreshold);
		mediasync_pr_info(0,policyInst,"mVideoTrickMode : %d ",policyInst->mVideoTrickMode);
	}
	return 0;
}

int mediasync_get_start_slow_sync_enable(mediasync_policy_instance *policyInst)
{
	mediasync_control mediasyncControl;
	u32 StcParmUpdateCount = 0;
	mediasyncControl.cmd = GET_SLOW_SYNC_ENABLE;
	mediasyncControl.size = sizeof(mediasync_update_info);
	mediasyncControl.ptr = (unsigned long)(&policyInst->updatInfo);
	StcParmUpdateCount = policyInst->updatInfo.mStcParmUpdateCount;
	mediasync_ins_ext_ctrls(policyInst->mMediasyncIns,&mediasyncControl);
	policyInst->mStartSlowSyncInfo.mSlowSyncEnable = mediasyncControl.value;
	mediasync_pr_info(0,policyInst,"mSlowSyncEnable:%d",policyInst->mStartSlowSyncInfo.mSlowSyncEnable);
	return 0;
}
int mediasync_get_start_slow_sync_init(mediasync_policy_instance *policyInst) {
	s64 mSlowSyncRealPVdiffUs = 0;
	s64 ExpectAvSyncDoneTimeUS = 0;
	s64 AvSyncDurationUs = 0;

	mediasync_get_start_slow_sync_enable(policyInst);
	if (policyInst->mStartSlowSyncInfo.mSlowSyncEnable) {
		if (policyInst->firstVFrameInfo.framePts > policyInst->mCurPcr) {
			policyInst->mStartSlowSyncInfo.mSlowSyncRealPVdiff =
				policyInst->firstVFrameInfo.framePts - policyInst->mCurPcr;
		} else {
			policyInst->mStartSlowSyncInfo.mSlowSyncRealPVdiff =
				policyInst->firstVFrameInfo.framePts - policyInst->anchorFrameInfo.framePts;
		}
		if ((policyInst->mStartSlowSyncInfo.mSlowSyncRealPVdiff >
			policyInst->mStartSlowSyncInfo.mSlowSyncPVdiffThreshold) &&
			(policyInst->mStartSlowSyncInfo.mSlowSyncRealPVdiff <
			policyInst->mStartSlowSyncInfo.mSlowSyncMaxPVdiffThreshold)) {
			policyInst->mStartSlowSyncInfo.mSlowSyncFrameShowTime = mediasync_get_system_time_us();
			policyInst->mStartSlowSyncInfo.mSlowSyncStartSystemTime =
				policyInst->mStartSlowSyncInfo.mSlowSyncFrameShowTime;

			mSlowSyncRealPVdiffUs = div_u64(policyInst->mStartSlowSyncInfo.mSlowSyncRealPVdiff * 100 , 9);

			ExpectAvSyncDoneTimeUS = policyInst->mStartSlowSyncInfo.mSlowSyncExpectAvSyncDoneTime * 1000;

			AvSyncDurationUs = ExpectAvSyncDoneTimeUS - mSlowSyncRealPVdiffUs;

			policyInst->mStartSlowSyncInfo.mSlowSyncSpeed =
				div_u64(AvSyncDurationUs * 100 , ExpectAvSyncDoneTimeUS);

			mediasync_pr_info(0,policyInst,
			"SlowSyncThreshold:%lld ms Expect:%lld us AvSyncDurationUs:%lld us RealPVdiffUs:%lld us Speed:%lld",
						div_u64(policyInst->mStartSlowSyncInfo.mSlowSyncPVdiffThreshold,90),
						ExpectAvSyncDoneTimeUS,
						AvSyncDurationUs,
						mSlowSyncRealPVdiffUs,
						policyInst->mStartSlowSyncInfo.mSlowSyncSpeed);
			return 1;
		} else {
			policyInst->mStartSlowSyncInfo.mSlowSyncEnable = false;
		}
	}
	return 0;

}

s32 VideoStartPlaybackSlowSync(mediasync_policy_instance *policyInst,s64 vpts,s64 frameDuration,struct mediasync_video_policy* vsyncPolicy) {

	s32 ret = 0;

	s64 duration = div_u64(frameDuration * 100 , policyInst->mStartSlowSyncInfo.mSlowSyncSpeed);


	s64 curSystime = mediasync_get_system_time_us();

	//int64_t duration = 100000000 / (mVideoFrameRate * mStartSlowSyncInfo.mSlowSyncSpeed);
	s64 outDuration = curSystime - policyInst->mStartSlowSyncInfo.mSlowSyncFrameShowTime;
	mediasync_pr_info(1,policyInst,
	"[vpts:%lld, curPcr:%lld, curSystime:%lld, mSlowSyncFrameShowTime:%lld, [pv_diff:%lld, time_diff:%lld us, SlowSpeed=%lld, frameDuration:%lld us duration=%lld us]",
		vpts, policyInst->mCurPcr, curSystime,
		policyInst->mStartSlowSyncInfo.mSlowSyncFrameShowTime,
		policyInst->mCurPcr - vpts,
		outDuration,
		policyInst->mStartSlowSyncInfo.mSlowSyncSpeed,
		frameDuration,
		duration);


	if (vpts - policyInst->mCurPcr > 1499) {
		if (outDuration < duration) {
			vsyncPolicy->videopolicy = MEDIASYNC_VIDEO_HOLD;
			vsyncPolicy->param2 = duration - outDuration;
		} else {
			vsyncPolicy->videopolicy = MEDIASYNC_VIDEO_NORMAL_OUTPUT;

			policyInst->mStartSlowSyncInfo.mSlowSyncFrameShowTime = curSystime;

			policyInst->mCurVideoFrameInfo.framePts = vpts;
			policyInst->mCurVideoFrameInfo.frameSystemTime = curSystime;

			mediasync_ins_set_curvideoframeinfo(policyInst->mMediasyncIns,
							policyInst->mCurVideoFrameInfo);
			ret = 1;
		}
	} else {
		vsyncPolicy->videopolicy = MEDIASYNC_VIDEO_HOLD;
		vsyncPolicy->param2 = 8000;

		policyInst->mStartSlowSyncInfo.mSlowSyncFrameShowTime  = -1;
		policyInst->mStartSlowSyncInfo.mSlowSyncFinished = true;
		mediasync_pr_info(0,policyInst,
		"Done [vpts:%lld, curPcr:%lld Systime:%lld [pvDiff:%lld,totalTime:%lld us, Speed=%lld, duration=%lld us]",
					vpts, policyInst->mCurPcr, curSystime,
					policyInst->mCurPcr - vpts,
					(curSystime -policyInst->mStartSlowSyncInfo.mSlowSyncStartSystemTime),
					policyInst->mStartSlowSyncInfo.mSlowSyncSpeed,
					duration);
		//mediasync_pr_info(0,policyInst,
		//"[pv_diff:%lld ms, time_diff:%lld us, Speed=%lld, duration=%lld us]",
		//		div_u64((policyInst->mCurPcr - vpts),90),
		//		(curSystime -policyInst->mStartSlowSyncInfo.mSlowSyncStartSystemTime),
		//		policyInst->mStartSlowSyncInfo.mSlowSyncSpeed, duration);
	}


	mediasync_pr_info(1,policyInst,
	"mSlowSyncFrameShowTime:%lld , mSlowSyncFinished:%d, policy:%s",
		policyInst->mStartSlowSyncInfo.mSlowSyncFrameShowTime,
		policyInst->mStartSlowSyncInfo.mSlowSyncFinished,
		videoPolicy2Str(vsyncPolicy->videopolicy));
	return ret;
}


int mediasync_video_process(ulong handle,s64 vpts,struct mediasync_video_policy* vsyncPolicy)
{
	//mediasync_policy_instance *inst = handle;
	int ret = 0;
	bool isVideoFreeRun = false;
	int64_t RecordVpts = vpts;
	mediasync_policy_instance *policyInst =
		(mediasync_policy_instance *) handle;

	avsync_state syncState = 0;
	//s64 mPcr = 0;
	s32 holdTime = 8000;
	s32 VideoTrickMode = 0;
	s64 pvdiff = 0;
	s64 lastSystemTime = 0;
	s64 lastVpts = 0;
	s64 frame_duration = vsyncPolicy->param1;
	if (handle == 0 || vsyncPolicy == NULL) {
		return ret;
	}
	vpts =  div_u64(vpts * 9 ,100);
	vsyncPolicy->videopolicy = MEDIASYNC_VIDEO_HOLD;
	vsyncPolicy->param1 = -1;
	vsyncPolicy->param2 = -1;

	if (policyInst->videoLastPolicy == MEDIASYNC_VIDEO_HOLD &&
		policyInst->mHoldVideoPts == vpts) {
		if (mediasync_get_system_time_us() - policyInst->mStartHoldVideoTime <
			policyInst->mHoldVideoTime) {
			vsyncPolicy->videopolicy = MEDIASYNC_VIDEO_HOLD;
			vsyncPolicy->param1 = -1;
			vsyncPolicy->param2 = -1;
			mediasync_pr_info(3,policyInst,"HOLD holdtime:%lld us duration:%lld us ",
				policyInst->mHoldVideoTime,
				mediasync_get_system_time_us() - policyInst->mStartHoldVideoTime);
			return ret;
		}
	}

	lastSystemTime = policyInst->updatInfo.mCurrentSystemtime;
	//mutex_lock(&(policyInst->mMediasyncIns->m_lock));
	//mutex_unlock(&(policyInst->mMediasyncIns->m_lock));
	VideoTrickMode = policyInst->mVideoTrickMode;
	mediasync_get_update_info(policyInst);

	if (policyInst->mVideoTrickMode == VIDEO_TRICK_MODE_PAUSE_NEXT) {
		if (policyInst->firstVFrameInfo.framePts == -1 &&
			policyInst->firstVFrameInfo.frameSystemTime == -1) {
			vsyncPolicy->videopolicy = MEDIASYNC_VIDEO_NORMAL_OUTPUT;
			policyInst->firstVFrameInfo.framePts = vpts;
			//policyInst->firstVFrameInfo.frameSystemTime= get_system_time_us();
			mediasync_pr_info(0,policyInst,"--->TrickMod");
		} else {
			vsyncPolicy->videopolicy = MEDIASYNC_VIDEO_HOLD;
		}
		return ret;
	} else if (VideoTrickMode == VIDEO_TRICK_MODE_PAUSE_NEXT &&
				policyInst->mVideoTrickMode == VIDEO_TRICK_MODE_NONE) {
		policyInst->firstVFrameInfo.frameSystemTime= mediasync_get_system_time_us();
		mediasync_ins_set_firstvideoframeinfo(policyInst->mMediasyncIns,
						policyInst->firstVFrameInfo);
	}

	if (policyInst->isPause) {
		vsyncPolicy->videopolicy = MEDIASYNC_VIDEO_HOLD;
		return ret;
	}
	if (policyInst->firstVFrameInfo.framePts == -1 &&
		policyInst->firstVFrameInfo.frameSystemTime == -1) {
		if (RecordVpts != -1) {
			policyInst->firstVFrameInfo.framePts = vpts;
			policyInst->firstVFrameInfo.frameSystemTime= mediasync_get_system_time_us();
			mediasync_ins_set_firstvideoframeinfo(policyInst->mMediasyncIns,
							policyInst->firstVFrameInfo);

			policyInst->mCurVideoFrameInfo.framePts = vpts;
			policyInst->mCurVideoFrameInfo.frameSystemTime = policyInst->firstVFrameInfo.frameSystemTime;
			mediasync_ins_set_curvideoframeinfo(policyInst->mMediasyncIns,
							policyInst->mCurVideoFrameInfo);

			if (policyInst->mShowFirstFrameNosync && policyInst->invalidVptsCount == 0) {
				vsyncPolicy->videopolicy = MEDIASYNC_VIDEO_NORMAL_OUTPUT;
				mediasync_video_state_process(policyInst,&syncState);
				mediasync_pr_info(0,policyInst,"no sync display first vpts:0x%llx.",vpts);
				return ret;
			}
		} else {
			if (policyInst->invalidVptsCount == 0) {
				vsyncPolicy->videopolicy = MEDIASYNC_VIDEO_NORMAL_OUTPUT;
				mediasync_pr_info(0,policyInst,"first vpts:0x%llx invalid ,render",vpts);
			} else {	//todo add drop function
				vsyncPolicy->videopolicy = MEDIASYNC_VIDEO_NORMAL_OUTPUT;
				mediasync_pr_info(0,policyInst,"first vpts:0x%llx invalid ,drop",vpts);
			}
			policyInst->invalidVptsCount++;
			return ret;
		}
	}


	if (mediasync_video_state_process(policyInst,&syncState) == false) {
		if (syncState == MEDIASYNC_EXIT) {
			mediasync_pr_info(0,policyInst,"EXIT PLAYBACK.");
			vsyncPolicy->videopolicy = MEDIASYNC_VIDEO_EXIT;
		} else {
			//video keep holding before MEDIASYNC_RUNNING state
			//mediasync_pr_info(0,policyInst,"VIDEO_HOLD.");
			vsyncPolicy->videopolicy = MEDIASYNC_VIDEO_HOLD;
		}
		return ret;
	}

	//int holdTime = 8000;
	if (policyInst->updatInfo.mPauseResumeFlag) {
		int pauseResumeFlag = 1;
		int64_t pausePts = 0;
		if (policyInst->mCurVideoFrameInfo.framePts == -1) {
			pausePts = vpts;
		} else {
			pausePts = policyInst->mCurVideoFrameInfo.framePts;
		}
		videoUpdateRefClock(policyInst,pausePts, &pauseResumeFlag,&holdTime);
		if (pauseResumeFlag) {
		//pause state, policy is always MEDIASYNC_VIDEO_HOLD
		//pauseResumeFlag is true means resume action begins;
		//pauseResumeFlag is false means resume action ends;
			vsyncPolicy->videopolicy = MEDIASYNC_VIDEO_HOLD;
			vsyncPolicy->param2 = holdTime;
			/*
			policyInst->mHoldVideoTime = holdTime;
			policyInst->mHoldVideoPts = vpts;
			policyInst->mStartHoldVideoTime = mediasync_get_system_time_us();
			policyInst->videoLastPolicy = MEDIASYNC_VIDEO_HOLD;
			*/
			return 0;
		}
	}

	get_stc(policyInst,false);
	videoCheckDiscontinue(policyInst,vpts);
	checkVideoFreeRun(policyInst,vpts,&isVideoFreeRun);
	lastVpts = policyInst->mCurVideoFrameInfo.framePts;
	pvdiff = policyInst->mCurPcr - vpts;
	if (policyInst->mVideoStarted == false) {
		if (mediasync_get_start_slow_sync_init(policyInst)) {
			policyInst->mVideoStarted = true;
		} else if (pvdiff > 0) {
			policyInst->mVideoStarted = true;
		}
	}
	if (pvdiff < 0) {

		if (policyInst->mStartSlowSyncInfo.mSlowSyncEnable &&
			!policyInst->mStartSlowSyncInfo.mSlowSyncFinished) {
			if (!VideoStartPlaybackSlowSync(policyInst,vpts,frame_duration,vsyncPolicy)) {
				policyInst->mHoldVideoPts = vpts;
				policyInst->mStartHoldVideoTime = vsyncPolicy->param2;
			}

		} else {
			vsyncPolicy->videopolicy = MEDIASYNC_VIDEO_HOLD;
			policyInst->mHoldVideoTime = div_u64((0 - pvdiff) * 100,9);
			if (policyInst->mHoldVideoTime > 1000000) {
				policyInst->mHoldVideoTime = 8000;
			}
			policyInst->mHoldVideoPts = vpts;
			vsyncPolicy->param2 = (s32)policyInst->mHoldVideoTime;
			policyInst->mStartHoldVideoTime = mediasync_get_system_time_us();
		}
	} else {
		policyInst->mHoldVideoTime = -1;
		policyInst->mHoldVideoPts = -1;
		vsyncPolicy->videopolicy = MEDIASYNC_VIDEO_NORMAL_OUTPUT;
		policyInst->mCurVideoFrameInfo.framePts = vpts;
		policyInst->mCurVideoFrameInfo.frameSystemTime = mediasync_get_system_time_us();
		mediasync_ins_set_curvideoframeinfo(policyInst->mMediasyncIns,
							policyInst->mCurVideoFrameInfo);
	}
	policyInst->videoLastPolicy = vsyncPolicy->videopolicy;
#if 1
	if (media_sync_policy_debug_level >= 1)
		mediasync_pr_info(0,policyInst,
			"[P:%s] mPcr:0x%llx vpts:0x%llx pvdiff:%lld hold:%d us vcache:%lld ms vdiff:%lld sdiff:%lld us\n",
									videoPolicy2Str(vsyncPolicy->videopolicy),
									policyInst->mCurPcr,
									vpts,
									pvdiff,
									vsyncPolicy->param2,
									div_u64(policyInst->updatInfo.mVideoInfo.cacheDuration,90),
									vpts - lastVpts,
									policyInst->updatInfo.mCurrentSystemtime - lastSystemTime);
#endif
	return ret;
}

static void mediasync_policy_destroy(struct kref *kref)
{
	struct mediasync_policy_manager *policy_mgr;

	policy_mgr = container_of(kref, struct mediasync_policy_manager, ref);

	//pr_info("VTP(%px) destroy.\n", vtp_mgr);

	vfree(policy_mgr);
}

int mediasync_policy_parameter_init(mediasync_policy_instance *policyInst) {
	policyInst->sSyncInsId = -1;
	policyInst->firstVFrameInfo.framePts = -1;
	policyInst->firstVFrameInfo.frameSystemTime= -1;
	policyInst->firstAFrameInfo.framePts = -1;
	policyInst->firstAFrameInfo.frameSystemTime= -1;
	policyInst->firstDmxPcrInfo.framePts = -1;
	policyInst->firstDmxPcrInfo.frameSystemTime= -1;
	policyInst->anchorFrameInfo.framePts = -1;
	policyInst->anchorFrameInfo.frameSystemTime= -1;
	policyInst->mCurVideoFrameInfo.framePts = -1;
	policyInst->mCurVideoFrameInfo.frameSystemTime= -1;
	policyInst->freerunFrameInfo.framePts = -1;
	policyInst->freerunFrameInfo.frameSystemTime = -1;
	policyInst->mShowFirstFrameNosync = first_frame_no_sync;
	policyInst->mVideoFreeRun = false;
	policyInst->stream_type = MEDIA_TYPE_MAX;
	policyInst->mStartFlag = 0;
	policyInst->mMediasyncIns = NULL;
	policyInst->mSyncMode = MEDIA_SYNC_PCRMASTER;
	policyInst->mStartPlayThreshold = 300;
	policyInst->clockType = UNKNOWN_CLOCK;
	policyInst->clockState = CLOCK_PROVIDER_NONE;
	policyInst->mSpeed.mNumerator = 100;
	policyInst->mSpeed.mDenominator = 100;
	policyInst->mPcrSlope.mNumerator = 100;
	policyInst->mPcrSlope.mDenominator = 100;
	policyInst->mStartThreshold = 0;
	policyInst->mPtsAdjust = 0;
	policyInst->mVideoSyncThreshold = 1800000;
	policyInst->mDiscontinueCacheThreshold = 300;
	policyInst->mVideoStarted = false;
	policyInst->mVideoTrickMode = 0;
	policyInst->videoLastPolicy = MEDIASYNC_VIDEO_UNKNOWN;
	policyInst->mHoldVideoTime = -1;
	policyInst->mHoldVideoPts = -1;
	policyInst->mStartHoldVideoTime = -1;
	/**start slow sync Parameter**/

	policyInst->mStartSlowSyncInfo.mSlowSyncEnable = true;               // slowsync enable flag, default is false
	policyInst->mStartSlowSyncInfo.mSlowSyncFinished = false;            // slowsync finish flag, default is false
	policyInst->mStartSlowSyncInfo.mSlowSyncSpeed = 30;              // slowsync speed, default is 0.5
	policyInst->mStartSlowSyncInfo.mSlowSyncPVdiffThreshold = slow_sync_avdiff_min_threshold*90;      // slowsync threshold, default is 800ms
	policyInst->mStartSlowSyncInfo.mSlowSyncMaxPVdiffThreshold = slow_sync_avdiff_max_threshold*90;   // slowsync max threshold, default is 4000ms
	policyInst->mStartSlowSyncInfo.mSlowSyncFrameShowTime = -1;    // slowsync video frame show time
	policyInst->mStartSlowSyncInfo.mSlowSyncRealPVdiff = 0;       // first vpts and ref pts diff
	policyInst->mStartSlowSyncInfo.mSlowSyncExpectAvSyncDoneTime = slow_sync_expect_av_sync_done; //Expect AvSync Done Time 5000ms
	policyInst->mStartSlowSyncInfo.mSlowSyncStartSystemTime = -1;
	/***************************/
	policyInst->mVideoSyncIntervalUs = 16666;

	policyInst->invalidVptsCount = 0;
	return 0;
}


ulong mediasync_policy_inst_create(void)
{

	mediasync_policy_instance *policyInst = NULL;
	//int ret;

	if (kref_read(&m_mgr->ref) > MAX_INSTANCE_NUM) {
		return 0;
	}

	policyInst = vzalloc(sizeof(mediasync_policy_instance));
	if (policyInst == NULL) {
		return 0;
	}

	mutex_lock(&m_mgr->mutex);
	//inst->sSyncInsId = sSyncInsId;
	mediasync_policy_parameter_init(policyInst);
	m_mgr->inst_count++;

	list_add(&policyInst->node, &m_mgr->mediasync_policy_list);

	kref_get(&m_mgr->ref);

	mutex_unlock(&m_mgr->mutex);

	return (ulong)policyInst;
}

int mediasync_policy_inst_release(ulong handle)
{
	mediasync_policy_instance*inst =
		(mediasync_policy_instance *)handle;

	mutex_lock(&m_mgr->mutex);


	list_del(&inst->node);

	mutex_unlock(&m_mgr->mutex);

	kref_put(&m_mgr->ref, mediasync_policy_destroy);

	vfree(inst);

	return 0;
}


int mediasync_policy_manager_init(void)
{
	m_mgr = vzalloc(sizeof(struct mediasync_policy_manager));
	if (m_mgr == NULL)
		return -ENOMEM;

	INIT_LIST_HEAD(&m_mgr->mediasync_policy_list);
	mutex_init(&m_mgr->mutex);
	kref_init(&m_mgr->ref);
	m_mgr->inst_count = 0;

	pr_info("mediasync_policy_manager_init(%px) initiation success.\n", m_mgr);

	return 0;
}

void mediasync_policy_manager_exit(void)
{
	pr_info("mediasync_policy_manager_exit(%px) exit.\n", m_mgr);

	kref_put(&m_mgr->ref, mediasync_policy_destroy);
}


module_param(media_sync_policy_debug_level, uint, 0664);
MODULE_PARM_DESC(media_sync_policy_debug_level, "\n media sync policy debug level\n");

module_param(first_frame_no_sync, uint, 0664);
MODULE_PARM_DESC(first_frame_no_sync, "\n media sync policy first frmae no sync\n");

module_param(slow_sync_avdiff_min_threshold, uint, 0664);
MODULE_PARM_DESC(slow_sync_avdiff_min_threshold, "\n media sync policy slow sync avdiff min threshold\n");

module_param(slow_sync_avdiff_max_threshold, uint, 0664);
MODULE_PARM_DESC(slow_sync_avdiff_max_threshold, "\n media sync policy slow sync avdiff max threshold\n");

module_param(slow_sync_expect_av_sync_done, uint, 0664);
MODULE_PARM_DESC(slow_sync_expect_av_sync_done, "\n media sync policy slow sync expect av sync done\n");
