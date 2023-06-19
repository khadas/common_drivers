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
#include <asm/current.h>

#include "media_sync_core.h"
#define KERNEL_ATRACE_TAG KERNEL_ATRACE_TAG_MEDIA_SYNC
#include <trace/events/meson_atrace.h>

#define MAX_DYNAMIC_INSTANCE_NUM 10
#define MAX_INSTANCE_NUM 40
#define MAX_CACHE_TIME_MS (60*1000)


/*
    bit_0: 1:VideoFreeRunBytime 0:disenable VideoFreeRunBytime
    bit_1: 1:AudioFreeRun       0:disenable AudioFreeRun
    bit_4-bit_7:mDebugLevel
    bit_8-bit_11:DeBugVptsOffset Unit: second
*/
static u32 media_sync_user_debug_level = 0;

static u32 media_sync_debug_level = 0;

static u32 media_sync_calculate_cache_enable = 0;

static u32 media_sync_start_slow_sync_enable = 1;


#define mediasync_pr_info(dbg_level,inst,fmt,args...) if (dbg_level <= media_sync_debug_level) {pr_info("[MS_Core:%d][%d_%d_%d] " fmt,__LINE__, inst->mSyncId,inst->mSyncIndex,inst->mUId,##args);}
#define mediasync_pr_error(fmt,args...) {pr_info("[%s:%d] err " fmt,__func__,__LINE__,##args);}
#define valid_pts(pts) ((pts) > 0)
#define PTS_TYPE_VIDEO 0
#define PTS_TYPE_AUDIO 1


static MediaSyncManager vMediaSyncInsList[MAX_INSTANCE_NUM];
static u32 g_inst_uid = 1;
static u64 last_system;
static u64 last_pcr;

#define VIDEO_FRAME_LIST_SIZE  (8192 * 2)
#define AUDIO_FRAME_LIST_SIZE  (8192 * 2)
#ifndef PAGE_SIZE
# define PAGE_SIZE 4096
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_DVB_DMX)
extern int demux_get_pcr(int demux_device_index, int index, u64 *pcr);
#else
int demux_get_pcr(int demux_device_index, int index, u64 *pcr) {return -1;}
#endif

extern int register_mediasync_vpts_set_cb(void* pfunc);
extern int register_mediasync_apts_set_cb(void* pfunc);

static u64 get_llabs(s64 value){
	u64 llvalue;
	if (value > 0) {
		return value;
	} else {
		llvalue = (u64)(0-value);
		return llvalue;
	}
}
#if 0
static long get_index_from_sync_id(s32 sSyncInsId)
{
	long index = -1;
	long temp = -1;

	if (sSyncInsId < MAX_DYNAMIC_INSTANCE_NUM) {
		for (temp = 0; temp < MAX_DYNAMIC_INSTANCE_NUM; temp++) {
			if (vMediaSyncInsList[temp].pInstance != NULL && sSyncInsId == vMediaSyncInsList[temp].pInstance->mSyncId) {
				index = temp;
				break;
			}
		}
	}
	if (index == -1) {
		for (temp = MAX_DYNAMIC_INSTANCE_NUM; temp < MAX_INSTANCE_NUM; temp++) {
				if (vMediaSyncInsList[temp].pInstance != NULL) {
					if (sSyncInsId == vMediaSyncInsList[temp].pInstance->mSyncId) {
							index = temp;
							break;
					}
				}
		}
	}

	return index;
}
#endif
static MediaSyncManager* get_media_sync_manager(s32 sSyncInsId,const char* from,int line) {
	MediaSyncManager* ret = NULL;
	if (sSyncInsId >= 0) {
		s32 index;
		for (index = 0; index < MAX_INSTANCE_NUM; index++) {
			if (vMediaSyncInsList[index].pInstance != NULL &&
				sSyncInsId == vMediaSyncInsList[index].pInstance->mSyncId) {
				ret = &vMediaSyncInsList[index];
				break;
			}
		}
	}
	if (ret == NULL) {
		mediasync_pr_error("Invalid sSyncInsId:%d from %s:%d\n",sSyncInsId,from,line);
	}
	return ret;
}

static void free_frame_list(struct frame_table_s *ptable)
{
#ifdef SIMPLE_ALLOC_LIST
	if (0) {		/*don't free,used a static memory */
		kfree(ptable->pts_recs);
		ptable->pts_recs = NULL;
	}
#else
	unsigned long *p = ptable->pages_list;
	void *onepage = (void *)p[0];

	while (onepage) {
		free_page((unsigned long)onepage);
		p++;
		onepage = (void *)p[0];
	}
	kfree(ptable->pages_list);
	ptable->pages_list = NULL;
#endif
	INIT_LIST_HEAD(&ptable->valid_list);
	INIT_LIST_HEAD(&ptable->free_list);
}

static int alloc_frame_list(struct frame_table_s *ptable)
{
	int i;
	int page_nums;

	INIT_LIST_HEAD(&ptable->valid_list);
	INIT_LIST_HEAD(&ptable->free_list);
#ifdef SIMPLE_ALLOC_LIST
	if (!ptable->pts_recs) {
		ptable->pts_recs = kcalloc(ptable->rec_num,
				sizeof(struct frame_rec_s),
				GFP_KERNEL);
	}
	if (!ptable->pts_recs) {
		ptable->status = 0;
		return -ENOMEM;
	}
	for (i = 0; i < ptable->rec_num; i++)
		list_add_tail(&ptable->pts_recs[i].list, &ptable->free_list);
	return 0;
#else
	page_nums = div_u64(ptable->rec_num * sizeof(struct frame_rec_s), PAGE_SIZE);
	if (div_u64(PAGE_SIZE, sizeof(struct frame_rec_s)) != 0) {
		page_nums = div_u64((ptable->rec_num + page_nums +
			 1) * sizeof(struct frame_rec_s), PAGE_SIZE);
	}
	ptable->pages_list = kzalloc((page_nums + 1) * sizeof(void *),
				GFP_KERNEL);
	if (!ptable->pages_list)
		return -ENOMEM;
	for (i = 0; i < page_nums; i++) {
		int j;
		void *one_page = (void *)__get_free_page(GFP_KERNEL);
		struct frame_rec_s *recs = one_page;

		if (!one_page)
			goto error_alloc_pages;
		for (j = 0; j < div_u64(PAGE_SIZE, sizeof(struct frame_rec_s)); j++)
			list_add_tail(&recs[j].list, &ptable->free_list);
		ptable->pages_list[i] = (unsigned long)one_page;
	}
	ptable->pages_list[page_nums] = 0;
	return 0;
error_alloc_pages:
	free_frame_list(ptable);
#endif
	return -ENOMEM;
}

static u64 get_stc_time_us(mediasync_ins* pInstance)
{

	int ret = -1;
	u64 stc;
	u64 timeus;
	u64 pcr;
	s64 pcr_diff;
	s64 time_diff;

	struct timespec64 ts_monotonic;

	if (pInstance == NULL) {
		return -1;
	}

	if (pInstance->mSyncMode != MEDIA_SYNC_PCRMASTER) {
		return 0;
	}

	ktime_get_ts64(&ts_monotonic);
	timeus = ts_monotonic.tv_sec * 1000000LL + div_u64(ts_monotonic.tv_nsec , 1000);
	if (pInstance->mDemuxId < 0) {
		return timeus;
	}

	ret = demux_get_pcr(pInstance->mDemuxId, 0, &pcr);

	if (ret != 0) {
		stc = timeus;
	} else {
		if (last_pcr == 0) {
			stc = timeus;
			last_pcr = div_u64(pcr * 100 , 9);
			last_system = timeus;
		} else {
			pcr_diff = div_u64(pcr * 100 , 9) - last_pcr;
			time_diff = timeus - last_system;
			if (time_diff && (div_u64(get_llabs(pcr_diff) , (s32)time_diff)
					    > 100)) {
				last_pcr = div_u64(pcr * 100 , 9);
				last_system = timeus;
				stc = timeus;
			} else {
				if (time_diff)
					stc = last_system + pcr_diff;
				else
					stc = timeus;

				last_pcr = div_u64(pcr * 100 , 9);
				last_system = stc;
			}
		}
	}
	pr_debug("get_stc_time_us stc:%lld pcr:%lld system_time:%lld\n", stc,  div_u64(pcr * 100 , 9),  timeus);
	return stc;
}

static s64 get_system_time_us(void) {
	s64 TimeUs;
	struct timespec64 ts_monotonic;
	ktime_get_ts64(&ts_monotonic);
	TimeUs = ts_monotonic.tv_sec * 1000000LL + div_u64(ts_monotonic.tv_nsec , 1000);
	pr_debug("get_system_time_us %lld\n", TimeUs);
	return TimeUs;
}

static mediasync_frameinfo_inner check_apts_valid(mediasync_ins* pInstance,int64_t pts) {
	mediasync_frameinfo_inner ret;
	struct frame_table_s* pTable = &pInstance->frame_table[PTS_TYPE_AUDIO];

	ret.frameid = 0;
	if (!list_empty(&pTable->valid_list) && valid_pts(pts)) {
		if (pts == pTable->mLastCheckedPts && pTable->mLastCheckedFrame.frameid != 0) {
			ret = pTable->mLastCheckedFrame;
		} else {
			struct frame_rec_s *rec = NULL;
			struct frame_rec_s *next = NULL;
			int loop_cnt = 0;
			list_for_each_entry_safe(rec,next, &pTable->valid_list, list) {
				loop_cnt++;
				if (pts >= rec->frame_info.framePts && pts < (rec->frame_info.framePts + rec->frame_info.frameduration)) {
					ret = rec->frame_info;
					mediasync_pr_info(5,pInstance,"found aFrame:[%llx,%lld],diff:%lld, pts:%llx,loop:%d",\
						ret.framePts,ret.frameid,div_u64((ret.framePts-pts),90),pts,loop_cnt);
					pTable->mLastCheckedPts = pts;
					pTable->mLastCheckedFrame = ret;
					break;
				}
			}
		}
	}
	if (ret.frameid == 0) {
		if (pts >= pTable->mLastQueued.framePts && pts < pTable->mLastQueued.framePts + pTable->mLastQueued.frameduration) {
			ret = pTable->mLastQueued;
		}
	}
	if (ret.frameid == 0) {
		mediasync_pr_info(0,pInstance,"apts invalid:%llx, count:%d, min:%llx, in:%llx, out:%llx",\
			pts, pTable->mFrameCount, pTable->mMinPtsFrame.framePts,\
			pTable->mLastQueued.framePts,pInstance->mSyncInfo.curAudioInfo.framePts);
	}
	return ret;
}

static mediasync_frameinfo_inner check_vpts_valid(mediasync_ins* pInstance,int64_t pts) {
	mediasync_frameinfo_inner ret;
	struct frame_table_s* pTable = &pInstance->frame_table[PTS_TYPE_VIDEO];

	ret.frameid = 0;
	if (!list_empty(&pTable->valid_list) && valid_pts(pts)) {
		if (pts == pTable->mLastCheckedPts && pTable->mLastCheckedFrame.frameid != 0) {
			ret = pTable->mLastCheckedFrame;
		} else {
			struct frame_rec_s *rec = NULL;
			struct frame_rec_s *next = NULL;
			int loop_cnt = 0;
			list_for_each_entry_safe_reverse(rec, next, &pTable->valid_list, list) {
				loop_cnt++;
				if (pts >= rec->frame_info.framePts && pts < next->frame_info.framePts) {
					ret = rec->frame_info;
					mediasync_pr_info(5,pInstance,"found vFrame:[%llx,%llx],diff:%lld, pts:%llx,loop:%d",\
						ret.framePts,ret.frameid,div_u64((ret.framePts-pts),90),pts,loop_cnt);
					pTable->mLastCheckedPts = pts;
					pTable->mLastCheckedFrame = ret;
					break;
				}
			}
		}
	}
	if (ret.frameid == 0) {
		mediasync_pr_info(0,pInstance,"vpts invalid:%llx, count:%d, min:%llx, in:%llx, out:%llx",\
			pts, pTable->mFrameCount, pTable->mMinPtsFrame.framePts,\
			pTable->mLastQueued.framePts, pInstance->mSyncInfo.curVideoInfo.framePts);
	}

	return ret;
}

static void clear_frame_list(mediasync_ins* pInstance, struct frame_table_s* pTable) {
	struct frame_rec_s *rec = NULL;
	struct frame_rec_s *next = NULL;
	list_for_each_entry_safe(rec,next, &pTable->valid_list, list) {
		list_move_tail(&rec->list, &pTable->free_list);
	}
	pTable->mCacheInfo.cacheDuration = 0;
	pTable->mCacheInfo.cacheSize = 0;
	pTable->mFrameCount = 0;
	pTable->current_frame_id = 1;
	pTable->mLastQueued.framePts = -1;
	pTable->mLastQueued.remainedduration = -1;
	pTable->mLastQueued.frameid = -1;
	pTable->mLastQueued.frameSystemTime = -1;
	pTable->mLastProcessedPts = -1;
	pTable->mMinPtsFrame = pTable->mLastQueued;
	mediasync_pr_info(0,pInstance,"type:%s\n",(pTable->mTableType == PTS_TYPE_AUDIO) ? "audio":"video");
}

static void update_audio_cache(mediasync_ins* pInstance,int64_t pts, int64_t duration_add) {
	struct frame_table_s* pTable = &pInstance->frame_table[PTS_TYPE_AUDIO];
	if (duration_add > 0) {
		//called from queueAudioFrame
		pTable->mCacheInfo.cacheDuration += duration_add;
	} else {
		//called from AudioProcess,need erase expired frames,but can simple update total cache with erased frames and remained duration in current frame
		if (list_empty(&pTable->valid_list) || !valid_pts(pts)) {
			if (pts >= pTable->mLastQueued.framePts && pts < (pTable->mLastQueued.framePts + pTable->mLastQueued.frameduration)) {
				pTable->mCacheInfo.cacheDuration = pts - pTable->mLastQueued.framePts;
			} else {
				pTable->mCacheInfo.cacheDuration = 0;
			}
		} else {
			struct frame_rec_s *rec = NULL;
			struct frame_rec_s *next = NULL;
			mediasync_frameinfo_inner frameinfo_l;
			frameinfo_l = check_apts_valid(pInstance, pts);
			if (frameinfo_l.frameid != 0) {
				list_for_each_entry_safe(rec,next, &pTable->valid_list, list) {
					if (rec->frame_info.frameid < frameinfo_l.frameid) {
						mediasync_pr_info(1,pInstance,"expired aFrame erased:[%llx,%llx] to [%llx,%llx] time:%lld ms, maxid:%llx",\
							rec->frame_info.framePts, rec->frame_info.frameid, frameinfo_l.framePts,\
							frameinfo_l.frameid, div_u64((rec->frame_info.framePts-frameinfo_l.framePts), 90), pTable->current_frame_id);
						pTable->mCacheInfo.cacheDuration -= rec->frame_info.remainedduration;
						if (next != NULL) {
							pTable->mMinPtsFrame = next->frame_info;
						} else {
							pTable->mMinPtsFrame = rec->frame_info;
						}
						list_move_tail(&rec->list, &pTable->free_list);
						pTable->mFrameCount--;
						break;
					} else if (rec->frame_info.frameid == frameinfo_l.frameid) {
						pTable->mCacheInfo.cacheDuration -= rec->frame_info.remainedduration;
						rec->frame_info.remainedduration = rec->frame_info.frameduration - (pts - rec->frame_info.framePts);
						pTable->mCacheInfo.cacheDuration += rec->frame_info.remainedduration;
						mediasync_pr_info(2,pInstance,"aFrame multi frame es:[%llx,%llx],to pts:%lld ms,duration:%lld/%lld ms",\
							rec->frame_info.framePts, rec->frame_info.frameid, div_u64((pts-rec->frame_info.framePts), 90),\
							div_u64(rec->frame_info.remainedduration, 90), div_u64(rec->frame_info.frameduration, 90));
						pTable->mMinPtsFrame = rec->frame_info;
						break;
					}
				}
			}
		}
	}
	mediasync_pr_info(1,pInstance,"(%s)aCache:%lld ms(%d) pts:[%llx,%llx,%llx], a_add:%lld ms, interval:%lld ms",\
		(duration_add > 0) ? "+":"-", div_u64(pTable->mCacheInfo.cacheDuration, 90),\
		pTable->mFrameCount, pTable->mMinPtsFrame.framePts, pts, pTable->mLastQueued.framePts, div_u64(duration_add, 90),div_u64(pTable->mLastQueued.frameduration, 90));
}

static void update_video_cache(mediasync_ins* pInstance,int64_t pts, int64_t duration_add) {
	mediasync_frameinfo_inner minelement;
	mediasync_frameinfo_inner maxelement;
	int segment_size = 0;
	struct frame_table_s* pTable = &pInstance->frame_table[PTS_TYPE_VIDEO];

	minelement.framePts = -1;
	maxelement.framePts = -1;
	if (!valid_pts(pts)) {
		pTable->mCacheInfo.cacheDuration = 0;
	} else {
		struct frame_rec_s *rec = NULL;
		struct frame_rec_s *next = NULL;
		mediasync_frameinfo_inner frameinfo_l;
		int64_t diff_pts_2_seg_min_pts = 0;//current pts maybe not exactly the minpts in the segment,so cache should remove this diff
		pTable->mCacheInfo.cacheDuration = 0;
		//first erase expired frames
		frameinfo_l = check_vpts_valid(pInstance, pts);
		if (frameinfo_l.frameid != 0) {
			struct frame_rec_s *rec = NULL;
			struct frame_rec_s *next = NULL;
			list_for_each_entry_safe(rec,next, &pTable->valid_list, list) {
				if (rec->frame_info.frameid < frameinfo_l.frameid) {
					mediasync_pr_info(1,pInstance,"vFrame erased:[%llx,%llx]",\
						rec->frame_info.framePts,rec->frame_info.frameid);
					list_move_tail(&rec->list, &pTable->free_list);
					pTable->mFrameCount--;
				}
			}
		}
		//then calculate total video cache
		segment_size++;
		list_for_each_entry_safe(rec, next, &pTable->valid_list, list) {
			if (!valid_pts(minelement.framePts)) {
				minelement = maxelement = rec->frame_info;
			}
			if (!list_entry_is_head(next, &pTable->valid_list, list)) {
				if (rec->frame_info.framePts - next->frame_info.framePts > DEFAULT_FRAME_SEGMENT_THRESHOLD) {
					pInstance->mVideoInfo.cacheDuration += maxelement.framePts - minelement.framePts;
					if (pts >= minelement.framePts && pts <= maxelement.framePts) {
						diff_pts_2_seg_min_pts = pts - minelement.framePts;
					}
					mediasync_pr_info(1,pInstance,"video segment jump back %lldms,%llx->%llx,id:%llx->%llx, seg_cache:%d(%d)/%lldms",\
						div_u64((rec->frame_info.framePts - next->frame_info.framePts), 90), rec->frame_info.framePts, next->frame_info.framePts,\
						rec->frame_info.frameid, next->frame_info.frameid, (int)div_u64((maxelement.framePts - minelement.framePts),90), segment_size,\
						div_u64(pTable->mCacheInfo.cacheDuration, 90));
					if (pTable->mMinPtsFrame.framePts < minelement.framePts) {
						pTable->mMinPtsFrame = minelement;
					}
					maxelement = minelement = next->frame_info;
					segment_size = 0;
				}
				else if (rec->frame_info.framePts > maxelement.framePts) {
					maxelement = rec->frame_info;
				}
				else if (rec->frame_info.framePts < minelement.framePts) {
					minelement = rec->frame_info;
				}
				segment_size++;
			}
		}
		if (pts >= minelement.framePts && pts <= maxelement.framePts) {
			diff_pts_2_seg_min_pts = pts - minelement.framePts;
		}
		if (pTable->mMinPtsFrame.framePts < minelement.framePts) {
			pTable->mMinPtsFrame = minelement;
		}
		pTable->mCacheInfo.cacheDuration += maxelement.framePts - minelement.framePts;
		pTable->mCacheInfo.cacheDuration -= diff_pts_2_seg_min_pts;

		if (pTable->mCacheInfo.cacheDuration < 0)
			pTable->mCacheInfo.cacheDuration = 0;
	}
	mediasync_pr_info(1,pInstance,"(%s)vCache:%lld ms(%d) pts:[%llx,%llx,%llx] v_add:%lld ms",\
		(duration_add != 0) ? "+":"-", div_u64(pTable->mCacheInfo.cacheDuration, 90),\
		pTable->mFrameCount, pTable->mMinPtsFrame.framePts, pts, pTable->mLastQueued.framePts, div_u64(duration_add, 90));
}

static void insert_video_frame_to_list(mediasync_ins* pInstance,mediasync_frameinfo *frame_info,int64_t v_add) {
	struct frame_rec_s *frame = NULL;
	struct frame_table_s* pTable = &pInstance->frame_table[PTS_TYPE_VIDEO];
	if (list_empty(&pTable->free_list)) {
		mediasync_pr_info(0,pInstance,"frame list is full,count:%d",pTable->mFrameCount);
		clear_frame_list(pInstance, pTable);
	}
	frame = list_first_entry(&pTable->free_list,struct frame_rec_s, list);
	list_del(&frame->list);
	if (frame != NULL) {
		frame->frame_info.framePts = frame_info->framePts;
		frame->frame_info.frameSystemTime = frame_info->frameSystemTime;
		frame->frame_info.frameid = pTable->current_frame_id;
		pTable->current_frame_id++;
		mediasync_pr_info(2,pInstance,"frame_info:[%llx,%llx],v_add:%lldms",\
			frame->frame_info.framePts,frame->frame_info.frameid,div_u64(v_add, 90));

		if (list_empty(&pTable->valid_list)) {
			list_add_tail(&frame->list, &pTable->valid_list);
		} else {
			bool done = false;
			struct frame_rec_s *rec = NULL;
			struct frame_rec_s *next = NULL;
			struct frame_rec_s *frame_swapid_head = list_first_entry(&pTable->valid_list, struct frame_rec_s, list);
			list_for_each_entry_safe(rec,
				next, &pTable->valid_list, list) {
				if (!list_is_last(&next->list, &pTable->valid_list) &&
					(rec->frame_info.framePts - next->frame_info.framePts) >
					DEFAULT_TRIGGER_DISCONTINUE_THRESHOLD) {
					frame_swapid_head = next;
				}
				if (frame_info->framePts == rec->frame_info.framePts) {
					//do not insert the same pts
					mediasync_pr_info(5,pInstance,"same pts %llx,%lldd",\
						rec->frame_info.framePts, rec->frame_info.frameid);
					done = true;
					break;
				} else if (frame_info->framePts > rec->frame_info.framePts) {
					if (!list_is_last(&next->list, &pTable->valid_list) &&
						(rec->frame_info.framePts - next->frame_info.framePts) <
						DEFAULT_TRIGGER_DISCONTINUE_THRESHOLD) {
						//short jump back should be B frame,we sort frameid by pts
						uint64_t tmpid = frame_swapid_head->frame_info.frameid;
						struct frame_rec_s* rec1 = frame_swapid_head;
						mediasync_pr_info(5,pInstance,"update frameid %llx:%llx->%llx",\
							frame_swapid_head->frame_info.framePts,frame_swapid_head->frame_info.frameid,frame->frame_info.frameid);
						frame_swapid_head->frame_info.frameid = frame->frame_info.frameid;
						rec1 = list_next_entry(rec1, list);
						while (rec1 != next) {
							uint64_t tmp;
							mediasync_pr_info(5,pInstance,"update frameid %llx:%llx->%llx",\
								rec1->frame_info.framePts,rec1->frame_info.frameid,tmpid);
							tmp = rec1->frame_info.frameid;
							rec1->frame_info.frameid = tmpid;
							tmpid = tmp;
							rec1 = list_next_entry(rec1, list);
						}
						frame->frame_info.frameid = tmpid;
					}
					list_add(&frame->list, rec->list.prev);
					done = true;
					break;
				}
			}
#if 0
			{
				//just for debug,list all frames in list
				int count = 0;
				mediasync_pr_info(pInstance->mSyncIndex,"******dump list start******");
				list_for_each_entry_safe(rec,
					next, &pTable->valid_list, list) {
					mediasync_pr_info(pInstance->mSyncIndex,"frame[%llx,%llx]",\
					rec->frame_info.framePts,rec->frame_info.frameid);
					count++;
					if (count > 10) {
						break;
					}
				}
				mediasync_pr_info(pInstance->mSyncIndex,"******dump list end  ******");
			}
#endif
			if (!done) {
				//this frame jump back,all pts in the list is larger than this pts
				list_add_tail(&frame->list, &pTable->valid_list);
			}
		}
		pTable->mLastQueued = frame->frame_info;
		if (!valid_pts(pTable->mMinPtsFrame.framePts) || pTable->mMinPtsFrame.framePts > pTable->mLastQueued.framePts) {
			pTable->mMinPtsFrame = pTable->mLastQueued;
		}
		pTable->mFrameCount++;
	}
}

static long mediasync_ins_set_queue_audio_info_l(mediasync_ins* pInstance, mediasync_frameinfo *info) {
	mediasync_pr_info(2,pInstance,"queue_audio[%llx,%llx]",\
			info->framePts,info->frameSystemTime);
	pInstance->mSyncInfo.queueAudioInfo.framePts = info->framePts;
	pInstance->mSyncInfo.queueAudioInfo.frameSystemTime = info->frameSystemTime;
	if (pInstance->mCacheFrames) {
		struct frame_table_s* pTable = &pInstance->frame_table[PTS_TYPE_AUDIO];
		int64_t a_add;
		if (!valid_pts(pTable->mLastQueued.framePts)) {
			a_add = 0;
		} else if (ABSSUB(info->framePts, pTable->mLastQueued.framePts) > DEFAULT_TRIGGER_DISCONTINUE_THRESHOLD) {
			//when apts jump,we can not use direct apts diff for frame duration
			//but we can use average diff for the duration to the last queued frame
			a_add = pTable->mLastQueued.frameduration;
		} else {
			a_add = info->framePts - pTable->mLastQueued.framePts;
		}
		if (valid_pts(pTable->mLastQueued.framePts)) {
			struct frame_rec_s *rec = NULL;
			//delay one queue to push_back for calculate the frameduration
			//and when calculating the total duration will consider the pts of mLastQueueFrame
			if (list_empty(&pTable->free_list)) {
				mediasync_pr_info(0,pInstance,"frame list is full,count:%d",pTable->mFrameCount);
				clear_frame_list(pInstance, pTable);
			}
			rec = list_first_entry(&pTable->free_list,struct frame_rec_s, list);
			list_del(&rec->list);
			if (rec != NULL) {
				rec->frame_info.framePts = pTable->mLastQueued.framePts;
				rec->frame_info.frameSystemTime = pTable->mLastQueued.frameSystemTime;
				rec->frame_info.frameid = pTable->mLastQueued.frameid;
				if (a_add < DEFAULT_FRAME_SEGMENT_THRESHOLD) {
					rec->frame_info.remainedduration = rec->frame_info.frameduration = a_add;
				} else {
					rec->frame_info.remainedduration = rec->frame_info.frameduration = pTable->mLastQueued.frameduration;
				}
				list_add_tail(&rec->list, &pTable->valid_list);
				pTable->mFrameCount++;
				update_audio_cache(pInstance,pTable->mLastProcessedPts,a_add);
			}
		}
		if (valid_pts(pTable->mLastQueued.framePts)) {
			if (ABSSUB(a_add,0) < 22500/*250 * 90*/) {
				if (pTable->mLastQueued.frameduration <= 0) {
					pTable->mLastQueued.frameduration = a_add;
				} else {
					pTable->mLastQueued.frameduration = div_u64((pTable->mLastQueued.frameduration + a_add), 2);
				}
			}
		}

		pTable->mLastQueued.framePts = info->framePts;
		pTable->mLastQueued.remainedduration = -1;
		pTable->mLastQueued.frameid = pTable->current_frame_id;
		pTable->mLastQueued.frameSystemTime = info->frameSystemTime;
		if (!valid_pts(pTable->mMinPtsFrame.framePts) || pTable->mMinPtsFrame.framePts > pTable->mLastQueued.framePts) {
			pTable->mMinPtsFrame = pTable->mLastQueued;
		}
		pTable->current_frame_id++;
	}
	return 0;
}

static long mediasync_ins_set_queue_video_info_l(mediasync_ins* pInstance, mediasync_frameinfo* info) {
	struct frame_table_s* pTable;
	int64_t v_add;
	mediasync_pr_info(2,pInstance,"queue_video[%llx,%llx]",\
			info->framePts,info->frameSystemTime);
	pInstance->mSyncInfo.queueVideoInfo.framePts = info->framePts;
	pInstance->mSyncInfo.queueVideoInfo.frameSystemTime = info->frameSystemTime;

	if (pInstance->mCacheFrames) {
		pTable = &pInstance->frame_table[PTS_TYPE_VIDEO];
		if (valid_pts(info->framePts)) {
			v_add = !valid_pts(pTable->mLastQueued.framePts) ? 0:(info->framePts - pTable->mLastQueued.framePts);
			insert_video_frame_to_list(pInstance,info,v_add);
			update_video_cache(pInstance, pTable->mLastProcessedPts, v_add);
		}
	}
	return 0;
}

static long mediasync_ins_delete(MediaSyncManager* pSyncManage) {
	mediasync_ins* pInstance = NULL;
	long ret = -1;
	//MediaSyncManager* pSyncManage = get_media_sync_manager(sSyncInsId,__func__,__LINE__);
	if (pSyncManage == NULL) {
		return -1;
	}

	pInstance = pSyncManage->pInstance;
	if (pInstance != NULL) {
		mediasync_pr_info(0,pInstance,"");
		if (media_sync_calculate_cache_enable) {
			free_frame_list(&pInstance->frame_table[0]);
			free_frame_list(&pInstance->frame_table[1]);
		}
		kfree(pInstance);
		pSyncManage->pInstance = NULL;
		ret = 0;
	}
	return ret;
}

static long mediasync_ins_init_syncinfo(mediasync_ins* pInstance) {
	if (pInstance == NULL)
		return -1;
	pInstance->mSyncInfo.state = MEDIASYNC_INIT;
	pInstance->mSyncInfo.firstAframeInfo.framePts = -1;
	pInstance->mSyncInfo.firstAframeInfo.frameSystemTime = -1;
	pInstance->mSyncInfo.firstVframeInfo.framePts = -1;
	pInstance->mSyncInfo.firstVframeInfo.frameSystemTime = -1;
	pInstance->mSyncInfo.firstDmxPcrInfo.framePts = -1;
	pInstance->mSyncInfo.firstDmxPcrInfo.frameSystemTime = -1;
	pInstance->mSyncInfo.refClockInfo.framePts = -1;
	pInstance->mSyncInfo.refClockInfo.frameSystemTime = -1;
	pInstance->mSyncInfo.curAudioInfo.framePts = -1;
	pInstance->mSyncInfo.curAudioInfo.frameSystemTime = -1;
	pInstance->mSyncInfo.curVideoInfo.framePts = -1;
	pInstance->mSyncInfo.curVideoInfo.frameSystemTime = -1;
	pInstance->mSyncInfo.curDmxPcrInfo.framePts = -1;
	pInstance->mSyncInfo.curDmxPcrInfo.frameSystemTime = -1;
	pInstance->mSyncInfo.queueAudioInfo.framePts = -1;
	pInstance->mSyncInfo.queueAudioInfo.frameSystemTime = -1;
	pInstance->mSyncInfo.queueVideoInfo.framePts = -1;
	pInstance->mSyncInfo.queueVideoInfo.frameSystemTime = -1;

	pInstance->mSyncInfo.videoPacketsInfo.packetsPts = -1;
	pInstance->mSyncInfo.videoPacketsInfo.packetsSize = -1;
	pInstance->mVideoDiscontinueInfo.discontinuePtsBefore = -1;
	pInstance->mVideoDiscontinueInfo.discontinuePtsAfter = -1;
	pInstance->mVideoDiscontinueInfo.lastDiscontinuePtsBefore = -1;
	pInstance->mVideoDiscontinueInfo.lastDiscontinuePtsAfter  = -1;
	pInstance->mVideoDiscontinueInfo.isDiscontinue = 0;
	pInstance->mSyncInfo.firstVideoPacketsInfo.framePts = -1;
	pInstance->mSyncInfo.firstVideoPacketsInfo.frameSystemTime = -1;

	pInstance->mSyncInfo.audioPacketsInfo.packetsSize = -1;
	pInstance->mSyncInfo.audioPacketsInfo.duration= -1;
	pInstance->mSyncInfo.audioPacketsInfo.isworkingchannel = 1;
	pInstance->mSyncInfo.audioPacketsInfo.isneedupdate = 0;
	pInstance->mSyncInfo.audioPacketsInfo.packetsPts = -1;
	pInstance->mSyncInfo.firstAudioPacketsInfo.framePts = -1;
	pInstance->mSyncInfo.firstAudioPacketsInfo.frameSystemTime = -1;
	pInstance->mAudioDiscontinueInfo.discontinuePtsBefore = -1;
	pInstance->mAudioDiscontinueInfo.discontinuePtsAfter = -1;
	pInstance->mAudioDiscontinueInfo.lastDiscontinuePtsBefore = -1;
	pInstance->mAudioDiscontinueInfo.lastDiscontinuePtsAfter  = -1;
	pInstance->mAudioDiscontinueInfo.isDiscontinue = 0;

	pInstance->mAudioInfo.cacheSize = -1;
	pInstance->mAudioInfo.cacheDuration = -1;
	pInstance->mVideoInfo.cacheSize = -1;
	pInstance->mVideoInfo.cacheDuration = -1;
	pInstance->mPauseResumeFlag = 0;
	pInstance->mSpeed.mNumerator = 100;
	pInstance->mSpeed.mDenominator = 100;
	pInstance->mPcrSlope.mNumerator = 100;
	pInstance->mPcrSlope.mDenominator = 100;
	pInstance->mAVRef = 0;
	pInstance->mPlayerInstanceId = -1;

	pInstance->mSyncInfo.pauseVideoInfo.framePts = -1;
	pInstance->mSyncInfo.pauseVideoInfo.frameSystemTime = -1;
	pInstance->mSyncInfo.pauseAudioInfo.framePts = -1;
	pInstance->mSyncInfo.pauseAudioInfo.frameSystemTime = -1;

	pInstance->mAudioFormat.channels = -1;
	pInstance->mAudioFormat.datawidth = -1;
	pInstance->mAudioFormat.format = -1;
	pInstance->mAudioFormat.samplerate = -1;
	pInstance->mCacheFrames = 0;
	return 0;
}

static void mediasync_ins_reset_l(mediasync_ins* pInstance) {
	struct frame_table_s *pTable;
	if (pInstance != NULL) {
		mediasync_ins_init_syncinfo(pInstance);
		pInstance->mHasAudio = 0;
		pInstance->mHasVideo = 0;
		pInstance->mVideoWorkMode = 0;
		pInstance->mFccEnable = 0;
		pInstance->mPaused = 0;
		pInstance->mSourceClockType = UNKNOWN_CLOCK;
		pInstance->mSyncInfo.state = MEDIASYNC_INIT;
		pInstance->mSourceClockState = CLOCK_PROVIDER_NORMAL;
		pInstance->mute_flag = false;
		pInstance->mSourceType = TS_DEMOD;
		pInstance->mUpdateTimeThreshold = MIN_UPDATETIME_THRESHOLD_US;
		pInstance->mStcParmUpdateCount = 0;
		pInstance->isVideoFrameAdvance = 0;
		pInstance->mFreeRunType = 0;
		pInstance->mSlowSyncEnable = media_sync_start_slow_sync_enable;
		if (media_sync_calculate_cache_enable) {
			pTable = &pInstance->frame_table[PTS_TYPE_AUDIO];
			clear_frame_list(pInstance, pTable);
			pTable = &pInstance->frame_table[PTS_TYPE_VIDEO];
			clear_frame_list(pInstance, pTable);
		}
		mediasync_pr_info(0,pInstance,"");
	}
}


long mediasync_init(void) {
	int index = 0;
	for (index = 0; index < MAX_INSTANCE_NUM; index++) {
		vMediaSyncInsList[index].pInstance = NULL;
		spin_lock_init(&(vMediaSyncInsList[index].m_lock));
	}
	register_mediasync_vpts_set_cb(mediasync_ins_set_video_packets_info);
	register_mediasync_apts_set_cb(mediasync_ins_set_audio_packets_info);
	return 0;
}


long mediasync_ins_alloc(s32 sDemuxId,
			s32 sPcrPid,
			s32 *sSyncInsId,
			MediaSyncManager **pSyncManage){
	s32 index = 0;
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;

	pInstance = kmalloc(sizeof(mediasync_ins), GFP_KERNEL|__GFP_ZERO);
	if (pInstance == NULL) {
		return -1;
	}

	for (index = 0; index < MAX_DYNAMIC_INSTANCE_NUM; index++) {
		spin_lock_irqsave(&(vMediaSyncInsList[index].m_lock),flags);
		if (vMediaSyncInsList[index].pInstance == NULL) {
			vMediaSyncInsList[index].pInstance = pInstance;
			pInstance->mSyncIndex = pInstance->mSyncId = index;
			pInstance->mUId = g_inst_uid;
			g_inst_uid++;
			*sSyncInsId = pInstance->mSyncId;
			mediasync_pr_info(0,pInstance,"mediasync_ins_alloc index:%d, demuxid:%d.\n", index, sDemuxId);

			pInstance->mDemuxId = sDemuxId;
			pInstance->mPcrPid = sPcrPid;
			// mediasync_ins_init_syncinfo(pInstance);
			pInstance->mHasAudio = 0;
			pInstance->mHasVideo = 0;
			pInstance->mVideoWorkMode = 0;
			pInstance->mFccEnable = 0;
			pInstance->mSourceClockType = UNKNOWN_CLOCK;
			pInstance->mSyncInfo.state = MEDIASYNC_INIT;
			pInstance->mSourceClockState = CLOCK_PROVIDER_NORMAL;
			pInstance->mute_flag = false;
			pInstance->mSourceType = TS_DEMOD;
			pInstance->mUpdateTimeThreshold = MIN_UPDATETIME_THRESHOLD_US;
			pInstance->mRef++;
			pInstance->mStcParmUpdateCount = 0;
			pInstance->mAudioCacheUpdateCount = 0;
			pInstance->mVideoCacheUpdateCount = 0;
			pInstance->isVideoFrameAdvance = 0;
			pInstance->mVideoTrickMode = 0;
			pInstance->mFreeRunType = 0;
			snprintf(pInstance->atrace_video,
				sizeof(pInstance->atrace_video), "msync_v_%d", *sSyncInsId);
			snprintf(pInstance->atrace_audio,
				sizeof(pInstance->atrace_audio), "msync_a_%d", *sSyncInsId);
			snprintf(pInstance->atrace_pcrscr,
				sizeof(pInstance->atrace_pcrscr), "msync_s_%d", *sSyncInsId);

			if (media_sync_calculate_cache_enable) {
				pInstance->frame_table[PTS_TYPE_AUDIO].rec_num = AUDIO_FRAME_LIST_SIZE;
				pInstance->frame_table[PTS_TYPE_VIDEO].rec_num = VIDEO_FRAME_LIST_SIZE;
				pInstance->frame_table[PTS_TYPE_AUDIO].mTableType = PTS_TYPE_AUDIO;
				pInstance->frame_table[PTS_TYPE_VIDEO].mTableType = PTS_TYPE_VIDEO;
				alloc_frame_list(&pInstance->frame_table[PTS_TYPE_AUDIO]);
				alloc_frame_list(&pInstance->frame_table[PTS_TYPE_VIDEO]);
			}
			for (pInstance->mRcordSlopeCount = 0;
					pInstance->mRcordSlopeCount < RECORD_SLOPE_NUM;
					pInstance->mRcordSlopeCount++) {
				pInstance->mRcordSlope[pInstance->mRcordSlopeCount] = 0;
			}
			pInstance->mRcordSlopeCount = 0;

			mediasync_ins_reset_l(pInstance);
			spin_unlock_irqrestore(&(vMediaSyncInsList[index].m_lock),flags);
			break;
		}
		spin_unlock_irqrestore(&(vMediaSyncInsList[index].m_lock),flags);
	}

	if (index == MAX_DYNAMIC_INSTANCE_NUM) {
		kfree(pInstance);
		return -1;
	}

	*pSyncManage = &vMediaSyncInsList[index];

	return 0;
}

long mediasync_ins_binder(s32 sSyncInsId,
			MediaSyncManager **pSyncManage) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	MediaSyncManager* SyncManage = NULL;

	SyncManage = get_media_sync_manager(sSyncInsId,__func__,__LINE__);
	if (SyncManage == NULL) {
		return -1;
	}
	spin_lock_irqsave(&(SyncManage->m_lock),flags);
	pInstance = SyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(SyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mRef++;
	*pSyncManage = SyncManage;
	mediasync_pr_info(0,pInstance,"mRef:%d,mAVRef:%d",pInstance->mRef,pInstance->mAVRef);
	spin_unlock_irqrestore(&(SyncManage->m_lock),flags);

	return 0;
}

long mediasync_static_ins_binder(s32 sSyncInsId,
			MediaSyncManager **pSyncManage) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	s32 temp_ins;
	long ret = -1;
	MediaSyncManager* SyncManage = NULL;
	if (sSyncInsId < MAX_DYNAMIC_INSTANCE_NUM && sSyncInsId >= 0) {
		ret = mediasync_ins_binder(sSyncInsId, &SyncManage);
	}

	if (ret) {
		SyncManage = get_media_sync_manager(sSyncInsId,__func__,__LINE__);
		if (SyncManage == NULL) {
			pInstance = kzalloc(sizeof(mediasync_ins), GFP_KERNEL);
			if (pInstance == NULL) {
				return -1;
			}

			for (temp_ins = MAX_DYNAMIC_INSTANCE_NUM; temp_ins < MAX_INSTANCE_NUM; temp_ins++) {
				spin_lock_irqsave(&(vMediaSyncInsList[temp_ins].m_lock),flags);
				if (vMediaSyncInsList[temp_ins].pInstance == NULL) {
					vMediaSyncInsList[temp_ins].pInstance = pInstance;
					pInstance->mSyncId = sSyncInsId;
					pInstance->mUId = g_inst_uid;
					g_inst_uid++;
					pInstance->mSyncIndex = temp_ins;
					mediasync_pr_info(0,pInstance,"mediasync_static_ins_binder alloc InsId:%d, index:%d.\n", sSyncInsId, temp_ins);

					//TODO add demuxID and pcr pid
					pInstance->mDemuxId = -1;
					pInstance->mPcrPid = -1;
					pInstance->mHasAudio = 0;
					pInstance->mHasVideo = 0;
					pInstance->mVideoWorkMode = 0;
					pInstance->mFccEnable = 0;
					pInstance->mSourceClockType = UNKNOWN_CLOCK;
					pInstance->mSyncInfo.state = MEDIASYNC_INIT;
					pInstance->mSourceClockState = CLOCK_PROVIDER_NORMAL;
					pInstance->mute_flag = false;
					pInstance->mSourceType = TS_DEMOD;
					pInstance->mUpdateTimeThreshold = MIN_UPDATETIME_THRESHOLD_US;
					pInstance->mRef++;
					snprintf(pInstance->atrace_video,
						sizeof(pInstance->atrace_video), "msync_v_%d", sSyncInsId);
					snprintf(pInstance->atrace_audio,
						sizeof(pInstance->atrace_audio), "msync_a_%d", sSyncInsId);
					snprintf(pInstance->atrace_pcrscr,
						sizeof(pInstance->atrace_pcrscr), "msync_s_%d", sSyncInsId);
					if (media_sync_calculate_cache_enable) {
						pInstance->frame_table[PTS_TYPE_AUDIO].rec_num = AUDIO_FRAME_LIST_SIZE;
						pInstance->frame_table[PTS_TYPE_VIDEO].rec_num = VIDEO_FRAME_LIST_SIZE;
						pInstance->frame_table[PTS_TYPE_AUDIO].mTableType = PTS_TYPE_AUDIO;
						pInstance->frame_table[PTS_TYPE_VIDEO].mTableType = PTS_TYPE_VIDEO;
						alloc_frame_list(&pInstance->frame_table[PTS_TYPE_AUDIO]);
						alloc_frame_list(&pInstance->frame_table[PTS_TYPE_VIDEO]);
					}
					mediasync_ins_reset_l(pInstance);
					//*pIns = pInstance;
					SyncManage = &(vMediaSyncInsList[temp_ins]);
					spin_unlock_irqrestore(&(vMediaSyncInsList[temp_ins].m_lock),flags);
					ret = 0;
					break;
				}
				spin_unlock_irqrestore(&(vMediaSyncInsList[temp_ins].m_lock),flags);
			}
		}else {
			spin_lock_irqsave(&(SyncManage->m_lock),flags);
			pInstance = SyncManage->pInstance;
			if (pInstance != NULL) {
				pInstance->mRef++;
				//*pIns = pInstance;
				ret = 0;
			}
			spin_unlock_irqrestore(&(SyncManage->m_lock),flags);
		}
	}
	*pSyncManage = SyncManage;
	return ret;
}

long mediasync_ins_unbinder(MediaSyncManager* pSyncManage, s32 sStreamType) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}
	spin_lock_irqsave(&(pSyncManage->m_lock),flags);

	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mRef--;

	mediasync_pr_info(0,pInstance,"mRef:%d,mAVRef:%d,streamType:%d",pInstance->mRef,pInstance->mAVRef,sStreamType);
	if (pInstance->mRef > 0 && pInstance->mAVRef == 0)
		mediasync_ins_reset_l(pInstance);

	if (pInstance->mRef <= 0)
		mediasync_ins_delete(pSyncManage);

	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
	return 0;
}

long mediasync_ins_reset(MediaSyncManager* pSyncManage) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}
	spin_lock_irqsave(&(pSyncManage->m_lock),flags);

	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	mediasync_ins_reset_l(pInstance);
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_update_mediatime(MediaSyncManager* pSyncManage,
				s64 lMediaTime,
				s64 lSystemTime, bool forceUpdate) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	u64 current_stc = 0;
	s64 current_systemtime = 0;
	s64 diff_system_time = 0;
	s64 diff_mediatime = 0;
	u32 k = 0;

	if (pSyncManage == NULL) {
		return -1;
	}
	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	current_stc = get_stc_time_us(pInstance);
	current_systemtime = get_system_time_us();
	k = pInstance->mSpeed.mNumerator * pInstance->mPcrSlope.mNumerator;
	k = div_u64(k, pInstance->mSpeed.mDenominator);
#if 0
	pInstance->mSyncMode = MEDIA_SYNC_PCRMASTER;
#endif
	if (pInstance->mSyncMode == MEDIA_SYNC_PCRMASTER) {
		if (lSystemTime == 0) {
			if (current_stc != 0) {
				diff_system_time = div_u64((current_stc - pInstance->mLastStc) * k, pInstance->mSpeed.mDenominator);
				diff_mediatime = lMediaTime - pInstance->mLastMediaTime;
			} else {
				diff_system_time = div_u64((current_systemtime - pInstance->mLastRealTime) * k, pInstance->mSpeed.mDenominator);
				diff_mediatime = lMediaTime - pInstance->mLastMediaTime;
			}
			if (pInstance->mSyncModeChange == 1
				|| diff_mediatime < 0
				|| ((diff_mediatime > 0)
				&& (get_llabs(diff_system_time - diff_mediatime) > pInstance->mUpdateTimeThreshold))) {
				mediasync_pr_info(0,pInstance,"MEDIA_SYNC_PCRMASTER update time\n");
				pInstance->mLastMediaTime = lMediaTime;
				pInstance->mLastRealTime = current_systemtime;
				pInstance->mLastStc = current_stc;
				pInstance->mSyncModeChange = 0;
			}
		} else {
			if (current_stc != 0) {
				diff_system_time = div_u64((lSystemTime - pInstance->mLastRealTime) * k, pInstance->mSpeed.mDenominator);
				diff_mediatime = lMediaTime - pInstance->mLastMediaTime;
			} else {
				diff_system_time = div_u64((lSystemTime - pInstance->mLastRealTime) * k, pInstance->mSpeed.mDenominator);
				diff_mediatime = lMediaTime - pInstance->mLastMediaTime;
			}

			if (pInstance->mSyncModeChange == 1
				|| diff_mediatime < 0
				|| ((diff_mediatime > 0)
				&& (get_llabs(diff_system_time - diff_mediatime) > pInstance->mUpdateTimeThreshold))) {
				pInstance->mLastMediaTime = lMediaTime;
				pInstance->mLastRealTime = lSystemTime;
				pInstance->mLastStc = current_stc + lSystemTime - current_systemtime;
				pInstance->mSyncModeChange = 0;
			}
		}
	} else {
		if (lSystemTime == 0) {
			diff_system_time = div_u64((current_systemtime - pInstance->mLastRealTime) * k, pInstance->mSpeed.mDenominator);
			diff_mediatime = lMediaTime - pInstance->mLastMediaTime;

			if (pInstance->mSyncModeChange == 1
				|| forceUpdate
				|| diff_mediatime < 0
				|| ((diff_mediatime > 0)
				&& (get_llabs(diff_system_time - diff_mediatime) > pInstance->mUpdateTimeThreshold))) {
				mediasync_pr_info(0,pInstance,"mSyncMode:%d update time system diff:%lld media diff:%lld current:%lld\n",
					pInstance->mSyncMode,
					diff_system_time,
					diff_mediatime,
					current_systemtime);
				pInstance->mLastMediaTime = lMediaTime;
				pInstance->mLastRealTime = current_systemtime;
				pInstance->mLastStc = current_stc;
				pInstance->mSyncModeChange = 0;
			}
	} else {
			diff_system_time = div_u64((lSystemTime - pInstance->mLastRealTime) * k, pInstance->mSpeed.mDenominator);
			diff_mediatime = lMediaTime - pInstance->mLastMediaTime;
			if (pInstance->mSyncModeChange == 1
				|| forceUpdate
				|| diff_mediatime < 0
				|| ((diff_mediatime > 0)
				&& (get_llabs(diff_system_time - diff_mediatime) > pInstance->mUpdateTimeThreshold))) {
				mediasync_pr_info(0,pInstance,"mSyncMode:%d update time stc diff:%lld media diff:%lld lSystemTime:%lld lMediaTime:%lld,k:%d, mUpdateTimeThreshold:%lld\n",
					pInstance->mSyncMode,
					diff_system_time,
					diff_mediatime,
					lSystemTime,
					lMediaTime,
					k,
					pInstance->mUpdateTimeThreshold);
				pInstance->mLastMediaTime = lMediaTime;
				pInstance->mLastRealTime = lSystemTime;
				pInstance->mLastStc = current_stc + lSystemTime - current_systemtime;
				pInstance->mSyncModeChange = 0;
			}
		}
	}
	pInstance->mTrackMediaTime = lMediaTime;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
	return 0;
}

long mediasync_ins_set_mediatime_speed(MediaSyncManager* pSyncManage,
					mediasync_speed fSpeed) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;

	if (pSyncManage == NULL) {
		return -1;
	}
	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	if (pInstance->mSpeed.mNumerator == fSpeed.mNumerator &&
		pInstance->mSpeed.mDenominator == fSpeed.mDenominator) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return 0;
	}

	pInstance->mSpeed.mNumerator = fSpeed.mNumerator;
	pInstance->mSpeed.mDenominator = fSpeed.mDenominator;
	pInstance->mStcParmUpdateCount++;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_paused(MediaSyncManager* pSyncManage, s32 sPaused) {

	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	u64 current_stc = 0;
	s64 current_systemtime = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	if (sPaused != 0 && sPaused != 1) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	if (sPaused == pInstance->mPaused) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return 0;
	}

	current_stc = get_stc_time_us(pInstance);
	current_systemtime = get_system_time_us();

	pInstance->mPaused = sPaused;

	if (pInstance->mSyncMode == MEDIA_SYNC_AMASTER)
		pInstance->mLastMediaTime = pInstance->mLastMediaTime +
		(current_systemtime - pInstance->mLastRealTime);

	pInstance->mLastRealTime = current_systemtime;
	pInstance->mLastStc = current_stc;

	pInstance->mStcParmUpdateCount++;

	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_paused(MediaSyncManager* pSyncManage, s32* spPaused) {

	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	*spPaused = pInstance->mPaused ;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_syncmode(MediaSyncManager* pSyncManage, s32 sSyncMode){
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mSyncMode = sSyncMode;
	pInstance->mSyncModeChange = 1;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_syncmode(MediaSyncManager* pSyncManage, s32 *sSyncMode) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	*sSyncMode = pInstance->mSyncMode;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_mediatime_speed(MediaSyncManager* pSyncManage, mediasync_speed *fpSpeed) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	fpSpeed->mNumerator = pInstance->mSpeed.mNumerator;
	fpSpeed->mDenominator = pInstance->mSpeed.mDenominator;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_anchor_time(MediaSyncManager* pSyncManage,
				s64* lpMediaTime,
				s64* lpSTCTime,
				s64* lpSystemTime) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;

	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	*lpMediaTime = pInstance->mLastMediaTime;
	*lpSTCTime = pInstance->mLastStc;
	*lpSystemTime = pInstance->mLastRealTime;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_systemtime(MediaSyncManager* pSyncManage, s64* lpSTC, s64* lpSystemTime){
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	u64 current_stc = 0;
	s64 current_systemtime = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	current_stc = get_stc_time_us(pInstance);
	current_systemtime = get_system_time_us();

	*lpSTC = current_stc;
	*lpSystemTime = current_systemtime;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_nextvsync_systemtime(MediaSyncManager* pSyncManage, s64* lpSystemTime) {

	return 0;
}

long mediasync_ins_set_updatetime_threshold(MediaSyncManager* pSyncManage, s64 lTimeThreshold) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;

	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	pInstance->mUpdateTimeThreshold = lTimeThreshold;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_updatetime_threshold(MediaSyncManager* pSyncManage, s64* lpTimeThreshold) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	*lpTimeThreshold = pInstance->mUpdateTimeThreshold;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_trackmediatime(MediaSyncManager* pSyncManage, s64* lpTrackMediaTime) {

	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	*lpTrackMediaTime = pInstance->mTrackMediaTime;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}
long mediasync_ins_set_clocktype(MediaSyncManager* pSyncManage, mediasync_clocktype type) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mSourceClockType = type;
	pInstance->mStcParmUpdateCount++;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_clocktype(MediaSyncManager* pSyncManage, mediasync_clocktype* type) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	*type = pInstance->mSourceClockType;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_clockstate(MediaSyncManager* pSyncManage, mediasync_clockprovider_state state) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mSourceClockState = state;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_clockstate(MediaSyncManager* pSyncManage, mediasync_clockprovider_state* state) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	*state = pInstance->mSourceClockState;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_hasaudio(MediaSyncManager* pSyncManage, int hasaudio) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mHasAudio = hasaudio;
	pInstance->mStcParmUpdateCount++;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_hasaudio(MediaSyncManager* pSyncManage, int* hasaudio) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	*hasaudio = pInstance->mHasAudio;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}
long mediasync_ins_set_hasvideo(MediaSyncManager* pSyncManage, int hasvideo) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mHasVideo = hasvideo;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_hasvideo(MediaSyncManager* pSyncManage, int* hasvideo) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	*hasvideo = pInstance->mHasVideo;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_firstaudioframeinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mSyncInfo.firstAframeInfo.framePts = info.framePts;
	pInstance->mSyncInfo.firstAframeInfo.frameSystemTime = info.frameSystemTime;
	mediasync_pr_info(0,pInstance,"first audio framePts:0x%llx frameSystemTime:%lld us\n",
							pInstance->mSyncInfo.firstAframeInfo.framePts,
							pInstance->mSyncInfo.firstAframeInfo.frameSystemTime);
	if (media_sync_calculate_cache_enable) {
		if (info.framePts == -1 && info.frameSystemTime == -1) {
			clear_frame_list(pInstance, &pInstance->frame_table[PTS_TYPE_AUDIO]);
		}
	}
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_firstaudioframeinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo* info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	info->framePts = pInstance->mSyncInfo.firstAframeInfo.framePts;
	info->frameSystemTime = pInstance->mSyncInfo.firstAframeInfo.frameSystemTime;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_firstvideoframeinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mSyncInfo.firstVframeInfo.framePts = info.framePts;
	pInstance->mSyncInfo.firstVframeInfo.frameSystemTime = info.frameSystemTime;
	mediasync_pr_info(0,pInstance,"first video framePts:0x%llx frameSystemTime:%lld us\n",
							pInstance->mSyncInfo.firstVframeInfo.framePts,
							pInstance->mSyncInfo.firstVframeInfo.frameSystemTime);

	if (media_sync_calculate_cache_enable) {
		if (info.framePts == -1 && info.frameSystemTime == -1) {
			clear_frame_list(pInstance, &pInstance->frame_table[PTS_TYPE_VIDEO]);
		}
	}
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_firstvideoframeinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo* info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	info->framePts = pInstance->mSyncInfo.firstVframeInfo.framePts;
	info->frameSystemTime = pInstance->mSyncInfo.firstVframeInfo.frameSystemTime;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_firstdmxpcrinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mSyncInfo.firstDmxPcrInfo.framePts = info.framePts;
	pInstance->mSyncInfo.firstDmxPcrInfo.frameSystemTime = info.frameSystemTime;
	mediasync_pr_info(0,pInstance,"first demux framePts:%lld frameSystemTime:%lld\n",
							pInstance->mSyncInfo.firstDmxPcrInfo.framePts,
							pInstance->mSyncInfo.firstDmxPcrInfo.frameSystemTime);
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_firstdmxpcrinfo(MediaSyncManager* pSyncManage,mediasync_frameinfo* info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;

	int64_t pcr = -1;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	if (pInstance->mSyncInfo.firstDmxPcrInfo.framePts == -1) {
		if (pInstance->mDemuxId >= 0) {
			demux_get_pcr(pInstance->mDemuxId, 0, &pcr);
			pInstance->mSyncInfo.firstDmxPcrInfo.framePts = pcr;
			pInstance->mSyncInfo.firstDmxPcrInfo.frameSystemTime = get_system_time_us();
			mediasync_pr_info(2,pInstance,"pcr:%lld frameSystemTime:%lld\n",
					pcr,
					pInstance->mSyncInfo.firstDmxPcrInfo.frameSystemTime);
		} else {
			pInstance->mSyncInfo.firstDmxPcrInfo.framePts = pcr;
			pInstance->mSyncInfo.firstDmxPcrInfo.frameSystemTime = get_system_time_us();
		}
	}

	info->framePts = pInstance->mSyncInfo.firstDmxPcrInfo.framePts;
	info->frameSystemTime = pInstance->mSyncInfo.firstDmxPcrInfo.frameSystemTime;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_refclockinfo(MediaSyncManager* pSyncManage,mediasync_frameinfo info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}


	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	if (pInstance->mSyncInfo.refClockInfo.framePts == info.framePts &&
		pInstance->mSyncInfo.refClockInfo.frameSystemTime == info.frameSystemTime) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return 0;
	}
	pInstance->mSyncInfo.refClockInfo.framePts = info.framePts;
	pInstance->mSyncInfo.refClockInfo.frameSystemTime = info.frameSystemTime;
	pInstance->mStcParmUpdateCount++;
	mediasync_pr_info(0,pInstance,"refclockinfo framePts:%lld frameSystemTime:%lld\n",pInstance->mSyncInfo.refClockInfo.framePts,pInstance->mSyncInfo.refClockInfo.frameSystemTime);
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_refclockinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo* info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	info->framePts = pInstance->mSyncInfo.refClockInfo.framePts;
	info->frameSystemTime = pInstance->mSyncInfo.refClockInfo.frameSystemTime;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_curaudioframeinfo(MediaSyncManager* pSyncManage,mediasync_frameinfo info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	ATRACE_COUNTER(pInstance->atrace_audio, info.framePts);
	pInstance->mSyncInfo.curAudioInfo.framePts = info.framePts;
	pInstance->mSyncInfo.curAudioInfo.frameSystemTime = info.frameSystemTime;
	pInstance->mAudioCacheUpdateCount++;

	if (media_sync_calculate_cache_enable) {
		pInstance->frame_table[PTS_TYPE_AUDIO].mLastProcessedPts = info.framePts;
		if (pInstance->mCacheFrames) {
			if (info.framePts != -1) {
				update_audio_cache(pInstance,info.framePts,0);
			}
		}
	}
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_curaudioframeinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo* info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	info->framePts = pInstance->mSyncInfo.curAudioInfo.framePts;
	info->frameSystemTime = pInstance->mSyncInfo.curAudioInfo.frameSystemTime;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_curvideoframeinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	ATRACE_COUNTER(pInstance->atrace_video, info.framePts);
	pInstance->mSyncInfo.curVideoInfo.framePts = info.framePts;
	pInstance->mSyncInfo.curVideoInfo.frameSystemTime = info.frameSystemTime;
	pInstance->mTrackMediaTime = div_u64(info.framePts * 100 , 9);
	pInstance->mVideoCacheUpdateCount++;
	if (media_sync_calculate_cache_enable) {
		pInstance->frame_table[PTS_TYPE_VIDEO].mLastProcessedPts = info.framePts;
		if (pInstance->mCacheFrames) {
			if (info.framePts != -1) {
				update_video_cache(pInstance,info.framePts,0);
			}
		}
	}
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_curvideoframeinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo* info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	info->framePts = pInstance->mSyncInfo.curVideoInfo.framePts;
	info->frameSystemTime = pInstance->mSyncInfo.curVideoInfo.frameSystemTime;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_curdmxpcrinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mSyncInfo.curDmxPcrInfo.framePts = info.framePts;
	pInstance->mSyncInfo.curDmxPcrInfo.frameSystemTime = info.frameSystemTime;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_curdmxpcrinfo(MediaSyncManager* pSyncManage, mediasync_frameinfo* info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	int64_t pcr = -1;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	if (pInstance->mDemuxId >= 0) {
		demux_get_pcr(pInstance->mDemuxId, 0, &pcr);
		pInstance->mSyncInfo.curDmxPcrInfo.framePts = pcr;
		pInstance->mSyncInfo.curDmxPcrInfo.frameSystemTime = get_system_time_us();
		mediasync_pr_info(2,pInstance,"pcr:%llx frameSystemTime:%llx\n",
				pcr,
				pInstance->mSyncInfo.curDmxPcrInfo.frameSystemTime);
	} else {
		pInstance->mSyncInfo.curDmxPcrInfo.framePts = -1;
		pInstance->mSyncInfo.curDmxPcrInfo.frameSystemTime = -1;
	}

	info->framePts = pInstance->mSyncInfo.curDmxPcrInfo.framePts;
	info->frameSystemTime = pInstance->mSyncInfo.curDmxPcrInfo.frameSystemTime;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_audiomute(MediaSyncManager* pSyncManage, int mute_flag) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mute_flag = mute_flag;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_audiomute(MediaSyncManager* pSyncManage, int* mute_flag) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	*mute_flag = pInstance->mute_flag;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_audioinfo(MediaSyncManager* pSyncManage, mediasync_audioinfo info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mAudioInfo.cacheDuration = info.cacheDuration;
	pInstance->mAudioInfo.cacheSize = info.cacheSize;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_audioinfo(MediaSyncManager* pSyncManage, mediasync_audioinfo* info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	info->cacheDuration = pInstance->mAudioInfo.cacheDuration;
	info->cacheSize = pInstance->mAudioInfo.cacheSize;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_videoinfo(MediaSyncManager* pSyncManage,mediasync_videoinfo info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mVideoInfo.cacheDuration = info.cacheDuration;
	pInstance->mVideoInfo.cacheSize = info.cacheSize;
	pInstance->mVideoInfo.specialSizeCount = info.specialSizeCount;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;

}

long mediasync_ins_get_videoinfo(MediaSyncManager* pSyncManage, mediasync_videoinfo* info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	info->cacheDuration = pInstance->mVideoInfo.cacheDuration;
	info->cacheSize = pInstance->mVideoInfo.cacheSize;
	info->specialSizeCount = pInstance->mVideoInfo.specialSizeCount;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

const char* mediasync_syncstate_to_str(avsync_state state)
{
	const char* str = NULL;
	switch (state) {
		case MEDIASYNC_INIT:
			str = "INIT";
		break;

		case MEDIASYNC_AUDIO_ARRIVED:
			str = "AUDIO_ARRIVED";
		break;

		case MEDIASYNC_VIDEO_ARRIVED:
			str = "VIDEO_ARRIVED";
		break;

		case MEDIASYNC_AV_ARRIVED:
			str = "AV_ARRIVED";
		break;

		case MEDIASYNC_AV_SYNCED:
			str = "AV_SYNCED";
		break;

		case MEDIASYNC_RUNNING:
			str = "RUNNING";
		break;

		case MEDIASYNC_VIDEO_LOST_SYNC:
			str = "VIDEO_LOST_SYNC";
		break;

		case MEDIASYNC_AUDIO_LOST_SYNC:
			str = "AUDIO_LOST_SYNC";
		break;

		case MEDIASYNC_EXIT:
			str = "EXIT";
		break;
	}

	return str;
}

long mediasync_ins_set_avsyncstate(MediaSyncManager* pSyncManage, s32 state) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	mediasync_pr_info(0,pInstance,"state: %s --> %s.",
			mediasync_syncstate_to_str(pInstance->mSyncInfo.state),
			mediasync_syncstate_to_str(state));

	pInstance->mSyncInfo.state = state;
	pInstance->mSyncInfo.setStateCurTimeUs = get_system_time_us();

	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_avsyncstate(MediaSyncManager* pSyncManage, s32* state) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	*state = pInstance->mSyncInfo.state;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_avsync_state_cur_time_us(MediaSyncManager* pSyncManage, mediasync_avsync_state_cur_time_us* avsync_state) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}


	//pr_info("(%s)(pid:%d)(%px) \n",current->comm, current->pid,pSyncManage);

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	avsync_state->state = pInstance->mSyncInfo.state;
	avsync_state->setStateCurTimeUs = pInstance->mSyncInfo.setStateCurTimeUs;

	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}


long mediasync_ins_set_startthreshold(MediaSyncManager* pSyncManage, s32 threshold) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	if (pInstance->mStartThreshold == threshold) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return 0;
	}
	pInstance->mStartThreshold = threshold;
	pInstance->mStcParmUpdateCount++;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_startthreshold(MediaSyncManager* pSyncManage, s32* threshold) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	*threshold = pInstance->mStartThreshold;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_ptsadjust(MediaSyncManager* pSyncManage, s32 adjust_pts) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	if (pInstance->mPtsAdjust == adjust_pts) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return 0;
	}
	pInstance->mPtsAdjust = adjust_pts;
	pInstance->mStcParmUpdateCount++;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_ptsadjust(MediaSyncManager* pSyncManage, s32* adjust_pts) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	*adjust_pts = pInstance->mPtsAdjust;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}


long mediasync_ins_set_videoworkmode(MediaSyncManager* pSyncManage, s32 mode) {

	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mVideoWorkMode = mode;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}


long mediasync_ins_get_videoworkmode(MediaSyncManager* pSyncManage, s32* mode) {

	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	*mode = pInstance->mVideoWorkMode;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}


long mediasync_ins_set_fccenable(MediaSyncManager* pSyncManage, s32 enable) {

	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mFccEnable = enable;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_fccenable(MediaSyncManager* pSyncManage, s32* enable) {

	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	*enable = pInstance->mFccEnable;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}


long mediasync_ins_set_source_type(MediaSyncManager* pSyncManage, aml_Source_Type sourceType) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mSourceType = sourceType;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_source_type(MediaSyncManager* pSyncManage, aml_Source_Type* sourceType) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	*sourceType = pInstance->mSourceType;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_start_media_time(MediaSyncManager* pSyncManage, s64 startime) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	pInstance->mStartMediaTime = startime;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_start_media_time(MediaSyncManager* pSyncManage, s64* starttime) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	*starttime = pInstance->mStartMediaTime;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_audioformat(MediaSyncManager* pSyncManage, mediasync_audio_format format) {

	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	pInstance->mAudioFormat.channels = format.channels;
	pInstance->mAudioFormat.datawidth = format.datawidth;
	pInstance->mAudioFormat.format = format.format;
	pInstance->mAudioFormat.samplerate = format.samplerate;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;

}

long mediasync_ins_get_audioformat(MediaSyncManager* pSyncManage, mediasync_audio_format* format) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	format->channels = pInstance->mAudioFormat.channels;
	format->datawidth = pInstance->mAudioFormat.datawidth;
	format->format = pInstance->mAudioFormat.format;
	format->samplerate = pInstance->mAudioFormat.samplerate;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_pauseresume(MediaSyncManager* pSyncManage, int flag) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mPauseResumeFlag = flag;
	if (flag == 1) {
		pInstance->mSyncInfo.pauseVideoInfo.framePts = -1;
		pInstance->mSyncInfo.pauseVideoInfo.frameSystemTime = -1;
		pInstance->mSyncInfo.pauseAudioInfo.framePts = -1;
		pInstance->mSyncInfo.pauseAudioInfo.frameSystemTime = -1;
	}
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_pauseresume(MediaSyncManager* pSyncManage, int* flag) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	*flag = pInstance->mPauseResumeFlag;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_pcrslope_implementation(mediasync_ins* pInstance, mediasync_speed pcrslope) {

	if (pInstance->mPcrSlope.mNumerator == pcrslope.mNumerator &&
		pInstance->mPcrSlope.mDenominator == pcrslope.mDenominator) {
		return 0;
	}

	pInstance->mPcrSlope.mNumerator = pcrslope.mNumerator;
	pInstance->mPcrSlope.mDenominator = pcrslope.mDenominator;
	pInstance->mStcParmUpdateCount++;


	return 0;
}


long mediasync_ins_set_pcrslope(MediaSyncManager* pSyncManage, mediasync_speed pcrslope) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	mediasync_ins_set_pcrslope_implementation(pInstance,pcrslope);

	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;

}

long mediasync_ins_get_pcrslope(MediaSyncManager* pSyncManage, mediasync_speed *pcrslope){
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pcrslope->mNumerator = pInstance->mPcrSlope.mNumerator;
	pcrslope->mDenominator = pInstance->mPcrSlope.mDenominator;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_update_avref(MediaSyncManager* pSyncManage, int flag) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	if (flag)
		pInstance->mAVRef ++;
	else
		pInstance->mAVRef --;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;

}

long mediasync_ins_get_avref(MediaSyncManager* pSyncManage, int *ref) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	*ref = pInstance->mAVRef;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_queue_audio_info(MediaSyncManager* pSyncManage, mediasync_frameinfo info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}


	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	mediasync_ins_set_queue_audio_info_l(pInstance,&info);
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
	return 0;
}

long mediasync_ins_get_queue_audio_info(MediaSyncManager* pSyncManage, mediasync_frameinfo* info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}


	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	if (pInstance->mCacheFrames) {
		struct frame_table_s* pTable = &pInstance->frame_table[PTS_TYPE_AUDIO];
		info->framePts = pTable->mLastQueued.framePts;
		info->frameSystemTime = pTable->mLastQueued.frameSystemTime;
	} else {
		info->framePts = pInstance->mSyncInfo.queueAudioInfo.framePts;
		info->frameSystemTime = pInstance->mSyncInfo.queueAudioInfo.frameSystemTime;
	}

	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}


long mediasync_ins_set_queue_video_info(MediaSyncManager* pSyncManage, mediasync_frameinfo info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}


	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	mediasync_ins_set_queue_video_info_l(pInstance,&info);
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
	return 0;

}

EXPORT_SYMBOL(mediasync_ins_set_queue_video_info);

long mediasync_ins_get_queue_video_info(MediaSyncManager* pSyncManage, mediasync_frameinfo* info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}


	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	if (pInstance->mCacheFrames) {
		struct frame_table_s* pTable;
		pTable = &pInstance->frame_table[PTS_TYPE_VIDEO];
		info->framePts = pTable->mLastQueued.framePts;
		info->frameSystemTime = pTable->mLastQueued.frameSystemTime;
	} else {
		info->framePts = pInstance->mSyncInfo.queueVideoInfo.framePts;
		info->frameSystemTime = pInstance->mSyncInfo.queueVideoInfo.frameSystemTime;
	}
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}


long mediasync_ins_set_audio_packets_info_implementation(MediaSyncManager* pSyncManage, mediasync_audio_packets_info info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	mediasync_frameinfo frameinfo;
	if (pSyncManage == NULL) {
		return -1;
	}


	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	if (pInstance->mSyncInfo.audioPacketsInfo.packetsPts != -1) {
		int64_t PtsDiff = info.packetsPts - pInstance->mSyncInfo.audioPacketsInfo.packetsPts;
		if (get_llabs(PtsDiff) >= 45000 /*500000 us*/) {
			pInstance->mAudioDiscontinueInfo.lastDiscontinuePtsBefore = pInstance->mAudioDiscontinueInfo.discontinuePtsBefore;
			pInstance->mAudioDiscontinueInfo.lastDiscontinuePtsAfter  = pInstance->mAudioDiscontinueInfo.discontinuePtsAfter;
			pInstance->mAudioDiscontinueInfo.discontinuePtsBefore =
				pInstance->mSyncInfo.audioPacketsInfo.packetsPts;
			pInstance->mAudioDiscontinueInfo.discontinuePtsAfter = info.packetsPts;
			pInstance->mAudioDiscontinueInfo.isDiscontinue = 1;
		}
	}

	pInstance->mSyncInfo.audioPacketsInfo.packetsSize = info.packetsSize;
	pInstance->mSyncInfo.audioPacketsInfo.duration= info.duration;
	pInstance->mSyncInfo.audioPacketsInfo.isworkingchannel = info.isworkingchannel;
	pInstance->mSyncInfo.audioPacketsInfo.isneedupdate = info.isneedupdate;
	pInstance->mSyncInfo.audioPacketsInfo.packetsPts = info.packetsPts;

	frameinfo.framePts = info.packetsPts;
	frameinfo.frameSystemTime = get_system_time_us();

	if (pInstance->mSyncInfo.firstAudioPacketsInfo.framePts == -1) {
		pInstance->mSyncInfo.firstAudioPacketsInfo.framePts = info.packetsPts;
		pInstance->mSyncInfo.firstAudioPacketsInfo.frameSystemTime = frameinfo.frameSystemTime;
	}
	pInstance->mAudioCacheUpdateCount++;
	mediasync_pr_info(2,pInstance,"APts:%llx,Size:%d\n",\
			pInstance->mSyncInfo.audioPacketsInfo.packetsPts,\
			pInstance->mSyncInfo.audioPacketsInfo.packetsSize);
	mediasync_ins_set_queue_audio_info_l(pInstance, &frameinfo);
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
	return 0;
}


/**/
long mediasync_ins_set_audio_packets_info(s32 sSyncInsId, mediasync_audio_packets_info info) {

	int ret = 0;
	MediaSyncManager* pSyncManage = get_media_sync_manager(sSyncInsId,__func__,__LINE__);
	if (pSyncManage == NULL) {
		return -1;
	}
	ret = mediasync_ins_set_audio_packets_info_implementation(pSyncManage,info);
	return ret;
}
EXPORT_SYMBOL(mediasync_ins_set_audio_packets_info);


void mediasync_ins_get_audio_cache_info_implementation(mediasync_ins* pInstance, mediasync_audioinfo* info) {
	int64_t Before_diff = 0;
	int64_t After_diff = 0;
	int64_t lastBefore_diff = 0;
	int64_t lastAfter_diff = 0;
	int isDiscontinueDone = 1;
	int64_t cacheDuration = 0;
	if (pInstance->mCacheFrames) {
		info->cacheDuration = pInstance->frame_table[PTS_TYPE_AUDIO].mCacheInfo.cacheDuration;
		info->cacheSize = pInstance->frame_table[PTS_TYPE_AUDIO].mCacheInfo.cacheSize;
	} else {
		//pr_info("mediasync_ins_get_videoinfo_2 curVideoInfo.framePts :%lld \n",pInstance->mSyncInfo.curVideoInfo.framePts);
		if (pInstance->mAudioCacheUpdateCount == pInstance->mGetAudioCacheUpdateCount) {
			info->cacheSize = pInstance->mAudioInfo.cacheSize;
			info->cacheDuration = pInstance->mAudioInfo.cacheDuration;
			//pr_info("mGetAudioCacheUpdateCount:%d mAudioCacheUpdateCount:%d\n",pInstance->mGetAudioCacheUpdateCount,pInstance->mAudioCacheUpdateCount);
			return;
		}
		pInstance->mGetAudioCacheUpdateCount = pInstance->mAudioCacheUpdateCount;
		if (pInstance->mSyncInfo.curAudioInfo.framePts != -1) {


			cacheDuration = pInstance->mSyncInfo.audioPacketsInfo.packetsPts - pInstance->mSyncInfo.curAudioInfo.framePts;
/*
			mediasync_pr_info(0,pInstance,"isDiscontinue:%d cacheDuration:%lld ms pts:0x%llx curpts:0x%llx PtsBefore:0x%llx PtsAfter:0x%llx \n",
				pInstance->mAudioDiscontinueInfo.isDiscontinue,
				cacheDuration / 90,
				pInstance->mSyncInfo.audioPacketsInfo.packetsPts,
				pInstance->mSyncInfo.curAudioInfo.framePts,
				pInstance->mAudioDiscontinueInfo.discontinuePtsBefore,
				pInstance->mAudioDiscontinueInfo.discontinuePtsAfter);
*/
			if (pInstance->mAudioDiscontinueInfo.discontinuePtsAfter != -1) {
				int64_t diff = pInstance->mSyncInfo.curAudioInfo.framePts - pInstance->mAudioDiscontinueInfo.discontinuePtsAfter;
				if (diff < 0) {
					if (get_llabs(diff) > 45000) {
						isDiscontinueDone = 0;
					}
				}
			}

			if (cacheDuration > -27000 && isDiscontinueDone == 1) {
				if (cacheDuration > 0)
					info->cacheDuration = cacheDuration;
				else {
					info->cacheDuration = 0;
				}

				if (pInstance->mAudioDiscontinueInfo.isDiscontinue == 1) {
					pInstance->mAudioDiscontinueInfo.lastDiscontinuePtsBefore = pInstance->mAudioDiscontinueInfo.discontinuePtsBefore;
					pInstance->mAudioDiscontinueInfo.lastDiscontinuePtsAfter  = pInstance->mAudioDiscontinueInfo.discontinuePtsAfter;
					pInstance->mAudioDiscontinueInfo.discontinuePtsAfter = -1;
					pInstance->mAudioDiscontinueInfo.discontinuePtsBefore = -1;
					pInstance->mAudioDiscontinueInfo.isDiscontinue = 0;
				}
			} else {
				if (pInstance->mAudioDiscontinueInfo.discontinuePtsAfter != -1 &&
					pInstance->mAudioDiscontinueInfo.discontinuePtsBefore != -1) {
					Before_diff = pInstance->mAudioDiscontinueInfo.discontinuePtsBefore - pInstance->mSyncInfo.curAudioInfo.framePts;
					if (Before_diff < 0 && pInstance->mAudioDiscontinueInfo.lastDiscontinuePtsBefore != -1 && pInstance->mAudioDiscontinueInfo.lastDiscontinuePtsAfter != -1) {
						//curframe is at the end of stream, but discontinuePtsBefore at the begin and pts jump
						mediasync_pr_info(2,pInstance,
							"discontinuePtsBefore:%lld, discontinuePtsAfter:%lld, lastDiscontinuePtsBefore:%lld, lastDiscontinuePtsAfter:%lld, framePts:%lld, packetsPts:%lld",
														pInstance->mAudioDiscontinueInfo.discontinuePtsBefore,
														pInstance->mAudioDiscontinueInfo.discontinuePtsAfter,
														pInstance->mAudioDiscontinueInfo.lastDiscontinuePtsBefore,
														pInstance->mAudioDiscontinueInfo.lastDiscontinuePtsAfter,
														pInstance->mSyncInfo.curAudioInfo.framePts,
														pInstance->mSyncInfo.audioPacketsInfo.packetsPts);
						lastBefore_diff = pInstance->mAudioDiscontinueInfo.lastDiscontinuePtsBefore - pInstance->mSyncInfo.curAudioInfo.framePts;
						lastAfter_diff = pInstance->mAudioDiscontinueInfo.discontinuePtsBefore - pInstance->mAudioDiscontinueInfo.lastDiscontinuePtsAfter;
						Before_diff = lastBefore_diff + lastAfter_diff;
					}
					After_diff = pInstance->mSyncInfo.audioPacketsInfo.packetsPts - pInstance->mAudioDiscontinueInfo.discontinuePtsAfter;
					info->cacheDuration = Before_diff + After_diff;
					// sometimes stream not descramble, lead pts jump, cache_duration will have error
					if (pInstance->mAudioDiscontinueInfo.isDiscontinue && (info->cacheDuration < 0 || info->cacheDuration > MAX_CACHE_TIME_MS * 90 * 2)) {
						//mediasync_pr_info(0,pInstance,"get_audio_cache, cache=%lldms, maybe cache cal have problem, need check more\n", div_u64(info->cacheDuration, 90));
						info->cacheDuration = 0;
					}
				} else {
					info->cacheDuration = 0;
				}
			}
		} else {
			if (pInstance->mSyncInfo.audioPacketsInfo.packetsPts >
					pInstance->mSyncInfo.firstAudioPacketsInfo.framePts) {
				info->cacheDuration = pInstance->mSyncInfo.audioPacketsInfo.packetsPts -
										pInstance->mSyncInfo.firstAudioPacketsInfo.framePts;
				if (pInstance->mAudioDiscontinueInfo.isDiscontinue == 1) {
					pInstance->mAudioDiscontinueInfo.lastDiscontinuePtsBefore = pInstance->mAudioDiscontinueInfo.discontinuePtsBefore;
					pInstance->mAudioDiscontinueInfo.lastDiscontinuePtsAfter  = pInstance->mAudioDiscontinueInfo.discontinuePtsAfter;
					pInstance->mAudioDiscontinueInfo.discontinuePtsAfter = -1;
					pInstance->mAudioDiscontinueInfo.discontinuePtsBefore = -1;
					pInstance->mAudioDiscontinueInfo.isDiscontinue = 0;
				}
			} else {
				if (pInstance->mAudioDiscontinueInfo.discontinuePtsAfter != -1 &&
					pInstance->mAudioDiscontinueInfo.discontinuePtsBefore != -1) {

					Before_diff = pInstance->mAudioDiscontinueInfo.discontinuePtsBefore - pInstance->mSyncInfo.firstAudioPacketsInfo.framePts;
					After_diff = pInstance->mSyncInfo.audioPacketsInfo.packetsPts  - pInstance->mAudioDiscontinueInfo.discontinuePtsAfter;
					info->cacheDuration = Before_diff + After_diff;

				} else {
					info->cacheDuration = 0;
				}
			}

		}
		//pr_info("---->audio cacheDuration : %d ms",info->cacheDuration / 90);
		//info->cacheDuration = pInstance->mVideoInfo.cacheDuration;
		//info->cacheSize =  pInstance->mAudioInfo.cacheSize;

		pInstance->mAudioInfo.cacheSize = info->cacheSize;
		pInstance->mAudioInfo.cacheDuration = info->cacheDuration;

	}
	return;
}

long mediasync_ins_get_audio_cache_info(MediaSyncManager* pSyncManage, mediasync_audioinfo* info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}


	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	mediasync_ins_get_audio_cache_info_implementation(pInstance,info);
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}


long mediasync_ins_set_video_packets_info_implementation(MediaSyncManager* pSyncManage, mediasync_video_packets_info info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	mediasync_frameinfo frameinfo;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	if (pInstance->mVideoInfo.specialSizeCount < 8) {
		if (info.packetsSize < 15) {
			pInstance->mVideoInfo.specialSizeCount++;
		} else {
			pInstance->mVideoInfo.specialSizeCount = 0;
		}
	}


	if (pInstance->mSyncInfo.videoPacketsInfo.packetsPts != -1) {
		int64_t PtsDiff = info.packetsPts - pInstance->mSyncInfo.videoPacketsInfo.packetsPts;
		if (get_llabs(PtsDiff) >= 45000 /*500000 us*/) {
			pInstance->mVideoDiscontinueInfo.lastDiscontinuePtsBefore = pInstance->mVideoDiscontinueInfo.discontinuePtsBefore;
			pInstance->mVideoDiscontinueInfo.lastDiscontinuePtsAfter  = pInstance->mVideoDiscontinueInfo.discontinuePtsAfter;
			pInstance->mVideoDiscontinueInfo.discontinuePtsBefore =
				pInstance->mSyncInfo.videoPacketsInfo.packetsPts;
			pInstance->mVideoDiscontinueInfo.discontinuePtsAfter = info.packetsPts;
			pInstance->mVideoDiscontinueInfo.isDiscontinue = 1;
		}
	}

	pInstance->mSyncInfo.videoPacketsInfo.packetsPts = info.packetsPts;
	pInstance->mSyncInfo.videoPacketsInfo.packetsSize = info.packetsSize;

	frameinfo.framePts = info.packetsPts;
	frameinfo.frameSystemTime = get_system_time_us();
	mediasync_pr_info(2,pInstance,"VPts:%llx,Size:%d\n",
			info.packetsPts,
			info.packetsSize);
	if (pInstance->mSyncInfo.firstVideoPacketsInfo.framePts == -1) {
		pInstance->mSyncInfo.firstVideoPacketsInfo.framePts = info.packetsPts;
		pInstance->mSyncInfo.firstVideoPacketsInfo.frameSystemTime = frameinfo.frameSystemTime;
	}
	pInstance->mVideoCacheUpdateCount++;
	mediasync_ins_set_queue_video_info_l(pInstance,&frameinfo);
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
	return 0;

}



long mediasync_ins_set_video_packets_info(s32 sSyncInsId, mediasync_video_packets_info info) {
	int ret = 0;
	MediaSyncManager* pSyncManage = get_media_sync_manager(sSyncInsId,__func__,__LINE__);
	if (pSyncManage == NULL) {
		return -1;
	}
	ret = mediasync_ins_set_video_packets_info_implementation(pSyncManage,info);
	return ret;

	return 0;

}

EXPORT_SYMBOL(mediasync_ins_set_video_packets_info);

void mediasync_ins_get_video_cache_info_implementation(mediasync_ins* pInstance, mediasync_videoinfo* info) {
	//pr_info("mediasync_ins_get_videoinfo_2 curVideoInfo.framePts :%lld \n",pInstance->mSyncInfo.curVideoInfo.framePts);
	int64_t Before_diff = 0;
	int64_t After_diff = 0;
	int64_t lastBefore_diff = 0;
	int64_t lastAfter_diff = 0;
	int isDiscontinueDone = 1;
	int64_t cacheDuration = 0;
	if (pInstance->mCacheFrames) {
		info->cacheDuration = pInstance->frame_table[PTS_TYPE_VIDEO].mCacheInfo.cacheDuration;
		info->cacheSize = pInstance->frame_table[PTS_TYPE_VIDEO].mCacheInfo.cacheSize;
		info->specialSizeCount = pInstance->mVideoInfo.specialSizeCount;
	} else {
		if (pInstance->mVideoCacheUpdateCount == pInstance->mGetVideoCacheUpdateCount) {
			info->cacheSize = pInstance->mVideoInfo.cacheSize;
			info->specialSizeCount = pInstance->mVideoInfo.specialSizeCount;
			info->cacheDuration = pInstance->mVideoInfo.cacheDuration;
			//pr_info("mGetVideoCacheUpdateCount:%d mVideoCacheUpdateCount:%d\n",pInstance->mGetVideoCacheUpdateCount,pInstance->mVideoCacheUpdateCount);
			return;
		}
		pInstance->mGetVideoCacheUpdateCount = pInstance->mVideoCacheUpdateCount;
		if (pInstance->mSyncInfo.curVideoInfo.framePts != -1) {

			cacheDuration = pInstance->mSyncInfo.videoPacketsInfo.packetsPts - pInstance->mSyncInfo.curVideoInfo.framePts;
/*
			mediasync_pr_info(0,pInstance,
				"isDiscontinue:%d cacheDuration:%lld ms pts:0x%llx curpts:0x%llx PtsBefore:0x%llx PtsAfter:0x%llx \n",
							pInstance->mVideoDiscontinueInfo.isDiscontinue,
							cacheDuration / 90,
							pInstance->mSyncInfo.videoPacketsInfo.packetsPts,
							pInstance->mSyncInfo.curVideoInfo.framePts,
							pInstance->mVideoDiscontinueInfo.discontinuePtsBefore,
							pInstance->mVideoDiscontinueInfo.discontinuePtsAfter);
*/
			if (pInstance->mVideoDiscontinueInfo.discontinuePtsAfter != -1) {
				int64_t diff = pInstance->mSyncInfo.curVideoInfo.framePts - pInstance->mVideoDiscontinueInfo.discontinuePtsAfter;
				if (diff < 0) {
					if (get_llabs(diff) > 45000) {
						isDiscontinueDone = 0;
					}
				}
			}

			if (cacheDuration > -45000 && isDiscontinueDone == 1) {
				if (cacheDuration > 0) {
					info->cacheDuration = cacheDuration;
				} else {
					cacheDuration = 0;
				}
				if (pInstance->mVideoDiscontinueInfo.isDiscontinue == 1) {
					pInstance->mVideoDiscontinueInfo.lastDiscontinuePtsBefore = pInstance->mVideoDiscontinueInfo.discontinuePtsBefore;
					pInstance->mVideoDiscontinueInfo.lastDiscontinuePtsAfter  = pInstance->mVideoDiscontinueInfo.discontinuePtsAfter;
					pInstance->mVideoDiscontinueInfo.discontinuePtsAfter = -1;
					pInstance->mVideoDiscontinueInfo.discontinuePtsBefore = -1;
					pInstance->mVideoDiscontinueInfo.isDiscontinue = 0;
				}
			} else {
				if (pInstance->mVideoDiscontinueInfo.discontinuePtsAfter != -1 &&
					pInstance->mVideoDiscontinueInfo.discontinuePtsBefore != -1) {
						Before_diff = pInstance->mVideoDiscontinueInfo.discontinuePtsBefore - pInstance->mSyncInfo.curVideoInfo.framePts;
						if (Before_diff < 0 && pInstance->mVideoDiscontinueInfo.lastDiscontinuePtsBefore != -1 && pInstance->mVideoDiscontinueInfo.lastDiscontinuePtsAfter != -1) {
							//curframe is at the end of stream, but discontinuePtsBefore at the begin and pts jump
							lastBefore_diff = pInstance->mVideoDiscontinueInfo.lastDiscontinuePtsBefore - pInstance->mSyncInfo.curVideoInfo.framePts;
							lastAfter_diff = pInstance->mVideoDiscontinueInfo.discontinuePtsBefore - pInstance->mVideoDiscontinueInfo.lastDiscontinuePtsAfter;
							Before_diff = lastBefore_diff + lastAfter_diff;
						}
						After_diff = pInstance->mSyncInfo.videoPacketsInfo.packetsPts - pInstance->mVideoDiscontinueInfo.discontinuePtsAfter;
						info->cacheDuration = Before_diff + After_diff;

					// sometimes stream not descramble, lead pts jump, cache_duration will have error
					if (pInstance->mVideoDiscontinueInfo.isDiscontinue && (info->cacheDuration < 0 || info->cacheDuration > MAX_CACHE_TIME_MS * 90)) {
						//mediasync_pr_info(0,pInstance,"get_video_cache, cache=%lldms, maybe cache cal have problem, need check more\n", div_u64(info->cacheDuration, 90));
						info->cacheDuration = 0;
					}

				} else {
					info->cacheDuration = 0;
				}
			}
		} else {
			if (pInstance->mSyncInfo.videoPacketsInfo.packetsPts >
					pInstance->mSyncInfo.firstVideoPacketsInfo.framePts) {
				info->cacheDuration = pInstance->mSyncInfo.videoPacketsInfo.packetsPts -
										pInstance->mSyncInfo.firstVideoPacketsInfo.framePts;
				if (pInstance->mVideoDiscontinueInfo.isDiscontinue == 1) {
					pInstance->mVideoDiscontinueInfo.lastDiscontinuePtsBefore = pInstance->mVideoDiscontinueInfo.discontinuePtsBefore;
					pInstance->mVideoDiscontinueInfo.lastDiscontinuePtsAfter  = pInstance->mVideoDiscontinueInfo.discontinuePtsAfter;
					pInstance->mVideoDiscontinueInfo.discontinuePtsAfter = -1;
					pInstance->mVideoDiscontinueInfo.discontinuePtsBefore = -1;
					pInstance->mVideoDiscontinueInfo.isDiscontinue = 0;
				}
			} else {
				if (pInstance->mVideoDiscontinueInfo.discontinuePtsAfter != -1 &&
					pInstance->mVideoDiscontinueInfo.discontinuePtsBefore != -1) {

					Before_diff = pInstance->mVideoDiscontinueInfo.discontinuePtsBefore - pInstance->mSyncInfo.firstVideoPacketsInfo.framePts;
					After_diff = pInstance->mSyncInfo.videoPacketsInfo.packetsPts  - pInstance->mVideoDiscontinueInfo.discontinuePtsAfter;
					info->cacheDuration = Before_diff + After_diff;

				} else {
					info->cacheDuration = 0;
				}
			}

		}
		//pr_info("----> cacheDuration : %d ms",info->cacheDuration / 90);
		//info->cacheDuration = pInstance->mVideoInfo.cacheDuration;
		info->cacheSize = 0;
		info->specialSizeCount = pInstance->mVideoInfo.specialSizeCount;


		pInstance->mVideoInfo.cacheSize = info->cacheSize;
		pInstance->mVideoInfo.specialSizeCount = info->specialSizeCount;
		pInstance->mVideoInfo.cacheDuration = info->cacheDuration;

	}

	return;
}

long mediasync_ins_get_video_cache_info(MediaSyncManager* pSyncManage, mediasync_videoinfo* info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;

	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	mediasync_ins_get_video_cache_info_implementation(pInstance,info);
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}


long mediasync_ins_set_first_queue_audio_info(MediaSyncManager* pSyncManage, mediasync_frameinfo info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}


	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	pInstance->mSyncInfo.firstAudioPacketsInfo.framePts = info.framePts;
	pInstance->mSyncInfo.firstAudioPacketsInfo.frameSystemTime = info.frameSystemTime;
	if (media_sync_calculate_cache_enable) {
		pInstance->frame_table[PTS_TYPE_AUDIO].mFirstPacket.framePts = info.framePts;
		pInstance->frame_table[PTS_TYPE_AUDIO].mFirstPacket.frameSystemTime = info.frameSystemTime;
		if (info.framePts == -1 && info.frameSystemTime == -1) {
			clear_frame_list(pInstance, &pInstance->frame_table[PTS_TYPE_AUDIO]);
		}
	}
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
	return 0;
}

long mediasync_ins_get_first_queue_audio_info(MediaSyncManager* pSyncManage, mediasync_frameinfo* info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}


	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	if (pInstance->mCacheFrames) {
		info->framePts = pInstance->frame_table[PTS_TYPE_AUDIO].mFirstPacket.framePts;
		info->frameSystemTime = pInstance->frame_table[PTS_TYPE_AUDIO].mFirstPacket.frameSystemTime;
	} else {
		info->framePts = pInstance->mSyncInfo.firstAudioPacketsInfo.framePts;
		info->frameSystemTime = pInstance->mSyncInfo.firstAudioPacketsInfo.frameSystemTime;
	}
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_first_queue_video_info(MediaSyncManager* pSyncManage, mediasync_frameinfo info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}


	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	pInstance->mSyncInfo.firstVideoPacketsInfo.framePts = info.framePts;
	pInstance->mSyncInfo.firstVideoPacketsInfo.frameSystemTime = info.frameSystemTime;
	if (media_sync_calculate_cache_enable) {
		pInstance->frame_table[PTS_TYPE_VIDEO].mFirstPacket.framePts = info.framePts;
		pInstance->frame_table[PTS_TYPE_VIDEO].mFirstPacket.frameSystemTime = info.frameSystemTime;
		if (info.framePts == -1 && info.frameSystemTime == -1) {
			clear_frame_list(pInstance, &pInstance->frame_table[PTS_TYPE_VIDEO]);
		}
	}
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
	return 0;

}

long mediasync_ins_get_first_queue_video_info(MediaSyncManager* pSyncManage, mediasync_frameinfo* info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	if (pInstance->mCacheFrames) {
		info->framePts = pInstance->frame_table[PTS_TYPE_VIDEO].mFirstPacket.framePts;
		info->frameSystemTime = pInstance->frame_table[PTS_TYPE_VIDEO].mFirstPacket.frameSystemTime;
	} else {
		info->framePts = pInstance->mSyncInfo.firstVideoPacketsInfo.framePts;
		info->frameSystemTime = pInstance->mSyncInfo.firstVideoPacketsInfo.frameSystemTime;
	}
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_player_instance_id(MediaSyncManager* pSyncManage, s32 PlayerInstanceId) {

	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;

	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mPlayerInstanceId = PlayerInstanceId;

	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_player_instance_id(MediaSyncManager* pSyncManage, s32* PlayerInstanceId) {

	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}


	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	*PlayerInstanceId = pInstance->mPlayerInstanceId ;

	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_pause_video_info(MediaSyncManager* pSyncManage, mediasync_frameinfo info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}


	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mSyncInfo.pauseVideoInfo.framePts = info.framePts;
	pInstance->mSyncInfo.pauseVideoInfo.frameSystemTime = info.frameSystemTime;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
	return 0;

}

long mediasync_ins_get_pause_video_info(MediaSyncManager* pSyncManage, mediasync_frameinfo* info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}


	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	info->framePts = pInstance->mSyncInfo.pauseVideoInfo.framePts;
	info->frameSystemTime = pInstance->mSyncInfo.pauseVideoInfo.frameSystemTime;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_set_pause_audio_info(MediaSyncManager* pSyncManage, mediasync_frameinfo info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}


	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mSyncInfo.pauseAudioInfo.framePts = info.framePts;
	pInstance->mSyncInfo.pauseAudioInfo.frameSystemTime = info.frameSystemTime;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
	return 0;

}

long mediasync_ins_get_pause_audio_info(MediaSyncManager* pSyncManage, mediasync_frameinfo* info) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}


	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	info->framePts = pInstance->mSyncInfo.pauseAudioInfo.framePts;
	info->frameSystemTime = pInstance->mSyncInfo.pauseAudioInfo.frameSystemTime;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}


s64 mediasync_ins_get_stc_time_implementation(mediasync_ins* pInstance,s64 CurTimeUs) {

	u64 mCurPcr = 0;
	u64 Interval = 0;
	u32 k = pInstance->mSpeed.mNumerator * pInstance->mPcrSlope.mNumerator ;
	k = div_u64(k, pInstance->mSpeed.mDenominator);

	Interval = (CurTimeUs - pInstance->mSyncInfo.refClockInfo.frameSystemTime);
	Interval = Interval * k;
	Interval = div_u64(Interval, pInstance->mPcrSlope.mDenominator);
	mCurPcr = div_u64(Interval * 9, 100);
	mCurPcr = mCurPcr + pInstance->mSyncInfo.refClockInfo.framePts;
	mCurPcr = mCurPcr - pInstance->mPtsAdjust - pInstance->mStartThreshold;

	return mCurPcr;
}

s64 mediasync_ins_get_stc_time(MediaSyncManager* pSyncManage) {


	u64 mCurPcr = 0;
	u64 CurTimeUs = 0;
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}
	CurTimeUs = get_system_time_us();
	mCurPcr = mediasync_ins_get_stc_time_implementation(pInstance,CurTimeUs);

	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return mCurPcr;

}



void mediasync_ins_check_pcr_slope(mediasync_ins* pInstance, mediasync_update_info* info) {

	s64 CurTimeUs = 0;
	s64 pcr = -1;
	s64 pcr_ns = -1;
	s64 pcr_diff = 0;
	s64 time_diff = 0;
	u32 slope = 100;
	int mincache = 0;
	int cacheDiffAbs = 0;
	u64 mCurPcr = 0;
	u32 i = 0;
	u32 maxslope = 100;
	u32 minslope = 100;
	u32 avgslope = 0;
	u32 isUpdate = 0;
	s64 pcrCurpcrDiff = 0;
	u32 UpdateSlop = 100;
	mediasync_speed pcrslope;
	u32 referenceAudioCache = 0;
	if (pInstance->mSourceClockType != PCR_CLOCK ||
		pInstance->mDemuxId < 0 ||
		pInstance->mSyncInfo.state != MEDIASYNC_RUNNING ||
		pInstance->mSourceClockState == CLOCK_PROVIDER_DISCONTINUE) {
		return;
	}

	if (pInstance->mLastCheckSlopeSystemtime == 0) {
		CurTimeUs = get_system_time_us();
		pInstance->mLastCheckSlopeSystemtime = CurTimeUs;
		if (pInstance->mDemuxId >= 0) {
			demux_get_pcr(pInstance->mDemuxId, 0, &pcr);
			pcr_ns = div_u64(pcr * 100000 , 9);
		}
		pInstance->mLastCheckSlopeDemuxPts = pcr_ns;
		pInstance->mlastCheckVideocacheDuration = 0;
		return ;
	}

	CurTimeUs = get_system_time_us();
	time_diff = CurTimeUs - pInstance->mLastCheckSlopeSystemtime;

	if (time_diff >= 500000) {
		if (pInstance->mDemuxId >= 0) {
			demux_get_pcr(pInstance->mDemuxId, 0, &pcr);
			mCurPcr = mediasync_ins_get_stc_time_implementation(pInstance,CurTimeUs);
		}
		pcr_ns = div_u64(pcr * 100000 , 9);
		pcr_diff = pcr_ns - pInstance->mLastCheckSlopeDemuxPts;
		if (pcr_diff > 0) {
			slope = div_u64(pcr_diff, time_diff);
			slope = div_u64(slope+5, 10);
		} else {
			pInstance->mLastCheckSlopeSystemtime = CurTimeUs;
			pInstance->mLastCheckSlopeDemuxPts = pcr_ns;
			mediasync_pr_info(0,pInstance,"--->pcr_diff:%lld \n",pcr_diff);
			return;
		}

		if (slope > 80 && slope < 130) {
			pInstance->mRcordSlope[pInstance->mRcordSlopeCount] = slope;
			pInstance->mRcordSlopeCount++;
			if (pInstance->mRcordSlopeCount >= RECORD_SLOPE_NUM) {
				pInstance->mRcordSlopeCount = 0;
			}
		}

		for (i = 0;i < RECORD_SLOPE_NUM;i++) {
			if (pInstance->mRcordSlope[i] == 0) {
				break;
			}
			if (maxslope < pInstance->mRcordSlope[i]) {
				maxslope = pInstance->mRcordSlope[i];
			} else if (minslope > pInstance->mRcordSlope[i]) {
				minslope = pInstance->mRcordSlope[i];
			}
			avgslope+=pInstance->mRcordSlope[i];
		}


		pInstance->mLastCheckSlopeSystemtime = CurTimeUs;
		pInstance->mLastCheckSlopeDemuxPts = pcr_ns;
		if (i < RECORD_SLOPE_NUM) {
			return;
		}

		avgslope = div_u64(avgslope,RECORD_SLOPE_NUM);
		if (pInstance->mHasVideo == 1) {
			mincache = info->mVideoInfo.cacheDuration;
		} else if (pInstance->mHasAudio == 1) {
			mincache = info->mAudioInfo.cacheDuration;
		}

		if (pInstance->mlastCheckVideocacheDuration != 0) {
			if (pInstance->mPcrSlope.mNumerator != avgslope) {
				cacheDiffAbs = ABSSUB(mincache,pInstance->mlastCheckVideocacheDuration);
				if (avgslope > pInstance->mPcrSlope.mNumerator) {
					if (mincache > 90000) {
						if (mincache > pInstance->mlastCheckVideocacheDuration &&
							cacheDiffAbs > 45000) {
							isUpdate = 1;
							UpdateSlop = avgslope;
						}
					} else {
						if (mincache < pInstance->mlastCheckVideocacheDuration &&
							cacheDiffAbs > 45000) {
							isUpdate = 1;
							UpdateSlop = minslope;
						}
					}
				} else {
					//mediasync_pr_info(0,pInstance,
					//	"demux pts diff:%lld ms  cur pts : %lld ms vcache:%lld us achce:%d us",
					//			(pInstance->mSyncInfo.videoPacketsInfo.packetsPts - pInstance->mSyncInfo.audioPacketsInfo.packetsPts)/90,
					//		(pInstance->mSyncInfo.curVideoInfo.framePts - pInstance->mSyncInfo.curAudioInfo.framePts) / 90,
					//			mincache*100/9,
					//			info->mAudioInfo.cacheDuration*100/9);
					referenceAudioCache = 0;
					if (pInstance->mHasAudio == 1 &&
						info->mAudioInfo.cacheDuration > 0 &&
						info->mAudioInfo.cacheDuration < 27000){
						referenceAudioCache = 1;
					}
					if (mincache < 90000 ||
						referenceAudioCache == 1) {
						if ((mincache < pInstance->mlastCheckVideocacheDuration &&
							cacheDiffAbs > 18000) ||
							referenceAudioCache == 1) {
							isUpdate = 1;
							UpdateSlop = avgslope;
						}
					} else {
						if (mincache > pInstance->mlastCheckVideocacheDuration &&
							cacheDiffAbs > 90000) {
							isUpdate = 1;
							UpdateSlop = maxslope;
						}
					}
				}
			}
		} else {
			pInstance->mlastCheckVideocacheDuration = mincache;
		}
		pcrCurpcrDiff = ABSSUB(pcr ,(s64)mCurPcr);
		#if 1
		mediasync_pr_info(1,pInstance,"=======\n");
		mediasync_pr_info(1,pInstance,"offset:%lld ms(%lld) pcr:%lld mCurPcr:%lld",
			div_u64((pcr - (s64)mCurPcr),90),
			pcrCurpcrDiff,
			pcr,mCurPcr);

		mediasync_pr_info(1,pInstance,
		"Slop:%d avg:%d max:%d min:%d cache:%lld us Lcache::%lld us abs:%lld us acache:%lld us",
			pInstance->mPcrSlope.mNumerator,avgslope,maxslope,minslope,
			div_u64((s64)(mincache*100),9),
			div_u64((s64)(pInstance->mlastCheckVideocacheDuration*100),9),
			div_u64((s64)(cacheDiffAbs*100),9),
			div_u64((s64)(info->mAudioInfo.cacheDuration*100),9));
		#endif
		//7000ms * 90
		if (isUpdate && pcrCurpcrDiff < DEFAULT_TRIGGER_DISCONTINUE_THRESHOLD) {
			pcrslope.mNumerator = UpdateSlop;
			pcrslope.mDenominator = 100;
			//mCurPcr = mediasync_ins_get_stc_time(pInstance,CurTimeUs);
			pInstance->mSyncInfo.refClockInfo.framePts = pcr;
			pInstance->mSyncInfo.refClockInfo.frameSystemTime = CurTimeUs;
			pInstance->mPtsAdjust = 0;
			pInstance->mStartThreshold = pcr - mCurPcr;
			mediasync_pr_info(0,pInstance,
				"update nowSlope:%d -> Slope:%d max:%d min:%d offset:%lld",
					pInstance->mPcrSlope.mNumerator,
					UpdateSlop,
					maxslope,minslope,
					pcr - (s64)mCurPcr);
			mediasync_ins_set_pcrslope_implementation(pInstance,pcrslope);
			pInstance->mlastCheckVideocacheDuration = mincache;
		}
	}

	return ;
}



long mediasync_ins_get_update_info(mediasync_ins* pInstance, mediasync_update_info* info) {

	info->debugLevel = media_sync_user_debug_level;

	info->mCurrentSystemtime = get_system_time_us();
	info->mPauseResumeFlag = pInstance->mPauseResumeFlag;
	info->mAvSyncState = pInstance->mSyncInfo.state;
	info->mSetStateCurTimeUs = pInstance->mSyncInfo.setStateCurTimeUs;
	info->mSourceClockState = pInstance->mSourceClockState;
	mediasync_ins_get_audio_cache_info_implementation(pInstance,&(info->mAudioInfo));
	mediasync_ins_get_video_cache_info_implementation(pInstance,&(info->mVideoInfo));
	info->isVideoFrameAdvance = pInstance->isVideoFrameAdvance;
	info->mFreeRunType = pInstance->mFreeRunType;
	mediasync_ins_check_pcr_slope(pInstance,info);
	info->mStcParmUpdateCount = pInstance->mStcParmUpdateCount;
	return 0;
}

long mediasync_ins_ext_ctrls_ioctrl(MediaSyncManager* pSyncManage, ulong arg, unsigned int is_compat_ptr) {
	mediasync_ins* pInstance = NULL;
	s32 minSize = 0;
	long ret = 0;
	mediasync_control mediasyncUserControl;
	mediasync_control mediasyncControl;
	mediasync_update_info info;
	ulong ptr;
	if (pSyncManage == NULL) {
		return -1;
	}

	if (copy_from_user((void *)&mediasyncUserControl,
				(void *)arg,
				sizeof(mediasyncControl))) {
		return -EFAULT;
	}

	switch (mediasyncUserControl.cmd) {
		case GET_UPDATE_INFO:
		{
			mediasyncControl.cmd = GET_UPDATE_INFO;
			mediasyncControl.size = sizeof(mediasync_update_info);
			mediasyncControl.ptr = (ulong)(&info);

			mediasync_ins_ext_ctrls(pSyncManage,&mediasyncControl);

			minSize = mediasyncUserControl.size;
			if (minSize > mediasyncControl.size) {
				minSize = mediasyncControl.size;
			}

			if (is_compat_ptr == 1) {
#ifdef CONFIG_COMPAT
				ptr = (ulong)compat_ptr(mediasyncUserControl.ptr);
#else
				ptr = mediasyncUserControl.ptr;
#endif
			} else {
				ptr = mediasyncUserControl.ptr;
			}

			if (copy_to_user((void *)ptr,(void*)&info,minSize)) {
				mediasync_pr_info(0,pInstance,"copy_to_user ptr -EFAULT \n");
				ret = -EFAULT;
				break;
			}

			mediasyncUserControl.size = minSize;
			if (copy_to_user((void *)arg,&mediasyncUserControl,sizeof(mediasyncControl))) {
				mediasync_pr_info(0,pInstance,"copy_to_user arg -EFAULT \n");
				ret = -EFAULT;
			}

			break;
		}
		case GET_SLOW_SYNC_ENABLE:
		case GET_TRICK_MODE:
		case GET_AUDIO_WORK_MODE:
		{
			mediasync_ins_ext_ctrls(pSyncManage,&mediasyncUserControl);
			if (copy_to_user((void *)arg,&mediasyncUserControl,sizeof(mediasyncControl))) {
				pr_info("copy_to_user arg -EFAULT \n");
				ret = -EFAULT;
			}
			break;
		}

		case SET_VIDEO_FRAME_ADVANCE:
		case SET_SLOW_SYNC_ENABLE:
		case SET_TRICK_MODE:
		case SET_FREE_RUN_TYPE:
		{
			mediasync_ins_ext_ctrls(pSyncManage,&mediasyncUserControl);
			break;
		}

		default:
			break;
	}

	return ret;
}


long mediasync_ins_ext_ctrls(MediaSyncManager* pSyncManage,mediasync_control* mediasyncControl)
{
	long ret = -1;
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	s32 minSize = 0;
	//ulong ptr;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;

	switch (mediasyncControl->cmd) {
		case GET_UPDATE_INFO:
		{
			mediasync_update_info info;
			mediasync_ins_get_update_info(pInstance,&info);
			minSize = sizeof(mediasync_update_info);

			if (minSize > mediasyncControl->size) {
				minSize = mediasyncControl->size;
			}

			memcpy((mediasync_update_info*)mediasyncControl->ptr,&info,minSize);

			mediasyncControl->size = minSize;

			ret = 0;
			break;
		}
		case SET_VIDEO_FRAME_ADVANCE:
		{
			pInstance->isVideoFrameAdvance = mediasyncControl->value;
			ret = 0;
			break;
		}
		case SET_SLOW_SYNC_ENABLE:
		{
			pInstance->mSlowSyncEnable = mediasyncControl->value;
			ret = 0;
			break;
		}
		case GET_SLOW_SYNC_ENABLE:
		{
			mediasyncControl->value = pInstance->mSlowSyncEnable;
			ret = 0;
			break;
		}
		case SET_TRICK_MODE:
		{
			pInstance->mStcParmUpdateCount++;
			pInstance->mVideoTrickMode = mediasyncControl->value;
			ret = 0;
			//mediasync_pr_info(0,pInstance," set mVideoTrickMode : %d \n",pInstance->mVideoTrickMode);
			break;
		}
		case GET_TRICK_MODE:
		{
			mediasyncControl->value = pInstance->mVideoTrickMode;
			ret = 0;
			//mediasync_pr_info(0,pInstance," get mVideoTrickMode : %d \n",pInstance->mVideoTrickMode);
			break;
		}
		case SET_FREE_RUN_TYPE:
		{
			pInstance->mFreeRunType = mediasyncControl->value;
			ret = 0;
			break;
		}
		case GET_AUDIO_WORK_MODE:
			mediasyncControl->value = pInstance->mSyncInfo.audioPacketsInfo.isworkingchannel;
			ret = 0;
			break;
		default:
			break;
	}

	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);


	return ret;
}




long mediasync_ins_set_video_smooth_tag(MediaSyncManager* pSyncManage, s32 sSmooth_tag) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}


	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	pInstance->mVideoSmoothTag = sSmooth_tag;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_get_video_smooth_tag(MediaSyncManager* pSyncManage, s32* spSmooth_tag) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}


	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	*spSmooth_tag = pInstance->mVideoSmoothTag;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return 0;
}

long mediasync_ins_check_apts_valid(MediaSyncManager* pSyncManage, s64 apts) {
	long ret = -1;
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	mediasync_frameinfo_inner frame_in;

	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	frame_in.frameid = 0;
	pInstance = pSyncManage->pInstance;
	if (pInstance != NULL) {
		if (pInstance->mCacheFrames) {
			frame_in = check_apts_valid(pInstance, apts);
			ret = frame_in.frameid == 0;
		} else {
			ret = 0;
		}
	}
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return ret;
}

long mediasync_ins_check_vpts_valid(MediaSyncManager* pSyncManage, s64 vpts) {
	long ret = -1;
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	mediasync_frameinfo_inner frame_in;

	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	frame_in.frameid = 0;
	pInstance = pSyncManage->pInstance;
	if (pInstance != NULL) {
		if (pInstance->mCacheFrames) {
			frame_in = check_vpts_valid(pInstance, vpts);
			ret = frame_in.frameid == 0;
		} else {
			ret = 0;
		}
	}

	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);

	return ret;
}

long mediasync_ins_set_cache_frames(MediaSyncManager* pSyncManage, s64 cache) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}
	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	mediasync_pr_info(0,pInstance,"mCacheFrames=%lld",cache);
	pInstance->mCacheFrames = cache;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
	return 0;
}

long mediasync_ins_set_pcr_and_dmx_id(MediaSyncManager* pSyncManage, s32 sDemuxId, s32 sPcrPid) {
	mediasync_ins* pInstance = NULL;
	unsigned long flags = 0;
	if (pSyncManage == NULL) {
		return -1;
	}

	spin_lock_irqsave(&(pSyncManage->m_lock),flags);
	pInstance = pSyncManage->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
		return -1;
	}

	mediasync_pr_info(1, pInstance, "mDemuxId=%d mPcrPid:%d\n", sDemuxId, sPcrPid);
	pInstance->mDemuxId = sDemuxId;
	pInstance->mPcrPid = sPcrPid;
	spin_unlock_irqrestore(&(pSyncManage->m_lock),flags);
	return 0;
}

module_param(media_sync_debug_level, uint, 0664);
MODULE_PARM_DESC(media_sync_debug_level, "\n mediasync debug level\n");

module_param(media_sync_user_debug_level, uint, 0664);
MODULE_PARM_DESC(media_sync_user_debug_level, "\n mediasync user debug level\n");


module_param(media_sync_calculate_cache_enable, uint, 0664);
MODULE_PARM_DESC(media_sync_calculate_cache_enable, "\n mediasync calculate cache enable\n");

module_param(media_sync_start_slow_sync_enable, uint, 0664);
MODULE_PARM_DESC(media_sync_start_slow_sync_enable, "\n media sync policy  slow sync enable\n");

