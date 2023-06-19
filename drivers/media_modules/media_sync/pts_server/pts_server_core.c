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
#include <linux/vmalloc.h>

#include "pts_server_core.h"

#define pts_pr_info(number,fmt,args...) pr_info("multi_ptsserv:[%d] " fmt, number,##args)


static u32 ptsserver_debuglevel = 0;

#define MAX_INSTANCE_NUM (40)
#define MAX_EXPIRED_COUNT (300)
#define MAX_CHECKOUT_RETRY_COUNT (8)
#define MAX_CHECKOUT_MIN_PTS_THRESHOLD (5)
#define MAX_CHECKOUT_BOUND_VALUE (5)
#define MAX_DYNAMIC_INSTANCE_NUM (10)

#define DEFAULT_FREE_LIST_COUNT (500)
#define DEFAULT_LOOKUP_THRESHOLD (2500)
#define DEFAULT_DOULBCHECK_THRESHOLD (5)
#define DEFAULT_REWIND_PTS_THRESHOLD (5000000)
#define DEFAULT_REWIND_INDEX_THRESHOLD (120)

PtsServerManage vPtsServerInsList[MAX_INSTANCE_NUM];

static u64 get_llabs(s64 value){
	u64 llvalue;
	if (value > 0) {
		return value;
	} else {
		llvalue = (u64)(0-value);
		return llvalue;
	}
}

static long get_index_from_ptsserver_id(s32 pServerInsId)
{
	long index = -1;
	long temp = -1;

	for (temp = 0; temp < MAX_INSTANCE_NUM; temp++) {
		if (vPtsServerInsList[temp].pInstance != NULL) {
			if (pServerInsId == vPtsServerInsList[temp].pInstance->mPtsServerInsId) {
				index = temp;
				break;
			}
		}
	}

	return index;
}

long ptsserver_ins_init_syncinfo(ptsserver_ins* pInstance,ptsserver_alloc_para* allocParm) {

	s32 index = 0;
	s32 pts_server_id = pInstance->mPtsServerInsId;
	pts_node* ptn = NULL;

	if (pInstance == NULL) {
		return -1;
	}

	if (allocParm != NULL) {
		pInstance->mMaxCount = allocParm->mMaxCount; //500
		pInstance->mLookupThreshold = allocParm->mLookupThreshold; //2500
		pInstance->kDoubleCheckThreshold = allocParm->kDoubleCheckThreshold; //5
	} else {
		pInstance->mMaxCount = DEFAULT_FREE_LIST_COUNT;
		pInstance->mLookupThreshold = DEFAULT_LOOKUP_THRESHOLD;
		pInstance->kDoubleCheckThreshold = DEFAULT_DOULBCHECK_THRESHOLD;
	}

	pInstance->mPtsCheckinStarted = 0;
	pInstance->mPtsCheckoutStarted = 0;
	pInstance->mPtsCheckoutFailCount = 0;
	pInstance->mFrameDuration = 0;
	pInstance->mFrameDuration64 = 0;
	pInstance->mFirstCheckinPts = 0;
	pInstance->mFirstCheckinOffset = 0;
	pInstance->mFirstCheckinSize = 0;
	pInstance->mLastCheckinPts = 0;
	pInstance->mLastCheckinOffset = 0;
	pInstance->mLastCheckinSize = 0;
	pInstance->mAlignmentOffset = 0;
	pInstance->mLastCheckoutPts = 0;
	pInstance->mLastPeekPts = 0;
	pInstance->mLastCheckoutOffset = 0;
	pInstance->mFirstCheckinPts64 = 0;
	pInstance->mLastCheckinPts64 = 0;
	pInstance->mLastCheckoutPts64 = 0;
	pInstance->mLastPeekPts64 = 0;
	pInstance->mDoubleCheckFrameDuration = 0;
	pInstance->mDoubleCheckFrameDuration64 = 0;
	pInstance->mDoubleCheckFrameDurationCount = 0;
	pInstance->mLastDoubleCheckoutPts = 0;
	pInstance->mLastDoubleCheckoutPts64 = 0;
	pInstance->mDecoderDuration = 0;
	pInstance->mListSize = 0;
	pInstance->mLastCheckoutCurOffset = 0;
	pInstance->mLastCheckinPiecePts = 0;
	pInstance->mLastCheckinPiecePts64 = 0;
	pInstance->mLastCheckinPieceOffset = 0;
	pInstance->mLastCheckinPieceSize = 0;
	pInstance->mLastCheckoutIndex = 0;
	pInstance->mLastCheckinDurationCount = 0;
	pInstance->mLastCheckoutDurationCount = 0;
	pInstance->mLastShotBound = 0;
	pInstance->mTrickMode = 0;
	pInstance->setC2Mode = false;
	pInstance->mOffsetMode = 0;
	pInstance->mStickyWrapFlag = false;
	pInstance->mLastDropIndex = 0;
	pInstance->mLastIndex = 0;
	pInstance->mLastCheckoutPts90k = 0;
	pInstance->mFirstCheckinPts90k = 0;
	pInstance->mLastCheckinPts90k = 0;

	mutex_init(&pInstance->mPtsListLock);
	spin_lock_init(&pInstance->mPtsListSlock);

	INIT_LIST_HEAD(&pInstance->pts_list);
	INIT_LIST_HEAD(&pInstance->pts_free_list);

	pInstance->all_free_ptn = vmalloc(pInstance->mMaxCount * sizeof(struct ptsnode));
	if (pInstance->all_free_ptn == NULL) {
		pr_info("vmalloc all_free_ptn fail \n");
		return -1;
	}

	for (index = 0; index < pInstance->mMaxCount; index++) {
		ptn = &pInstance->all_free_ptn[index];
		if (ptsserver_debuglevel >= 1) {
			pts_pr_info(pts_server_id,"ptn[%d]:%px\n", index,ptn);
		}
		list_add_tail(&ptn->node, &pInstance ->pts_free_list);
	}

	return 0;
}


long ptsserver_init(void) {
	int i = 0;
	for (i = 0;i < MAX_INSTANCE_NUM;i++) {
		vPtsServerInsList[i].pInstance = NULL;
		mutex_init(&(vPtsServerInsList[i].mListLock));
		spin_lock_init(&(vPtsServerInsList[i].mListSlock));
	}
	return 0;
}


long ptsserver_ins_alloc(s32 *pServerInsId,
							ptsserver_ins **pIns,
							ptsserver_alloc_para* allocParm) {

	s32 index = 0;
	ptsserver_ins* pInstance = NULL;
	pInstance = kzalloc(sizeof(ptsserver_ins), GFP_KERNEL);
	if (pInstance == NULL) {
		return -1;
	}
	pr_info("multi_ptsserv: ptsserver_ins_alloc in\n");
	for (index = 0; index < MAX_DYNAMIC_INSTANCE_NUM - 1; index++) {
		mutex_lock(&vPtsServerInsList[index].mListLock);
		if (vPtsServerInsList[index].pInstance == NULL) {
			vPtsServerInsList[index].pInstance = pInstance;
			pInstance->mPtsServerInsId = index;
			pInstance->mRef++;
			*pServerInsId = index;
			ptsserver_ins_init_syncinfo(pInstance,allocParm);
			pts_pr_info(index,"ptsserver_ins_alloc --> index:%d\n", index);
			mutex_unlock(&vPtsServerInsList[index].mListLock);
			break;
		}
		mutex_unlock(&vPtsServerInsList[index].mListLock);
	}

	if (index == MAX_DYNAMIC_INSTANCE_NUM) {
		memset(pInstance, 0, sizeof(ptsserver_ins));
		kfree(pInstance);;
		return -1;
	}
	*pIns = pInstance;
	pts_pr_info(index,"ptsserver_ins_alloc ok\n");
	return 0;
}
EXPORT_SYMBOL(ptsserver_ins_alloc);

void ptsserver_set_mode(s32 pServerInsId, bool set_mode) {
	ptsserver_ins* pInstance = NULL;
	PtsServerManage* vPtsServerIns = NULL;
	s32 index = pServerInsId;
	if (index < 0 || index >= MAX_INSTANCE_NUM)
		return;
	vPtsServerIns = &(vPtsServerInsList[index]);

	pInstance = vPtsServerIns->pInstance;
	if (pInstance == NULL) {
		return;
	}
	pInstance->setC2Mode = set_mode;
	// If set trickmode we set mOffsetMode default
	if (set_mode) {
		pInstance->mOffsetMode = 1;
	}

	return;
}
EXPORT_SYMBOL(ptsserver_set_mode);

long ptsserver_set_first_checkin_offset(s32 pServerInsId,start_offset* mStartOffset) {
	ptsserver_ins* pInstance = NULL;
	PtsServerManage* vPtsServerIns = NULL;
	s32 index = pServerInsId;
	if (index < 0 || index >= MAX_INSTANCE_NUM)
		return -1;
	vPtsServerIns = &(vPtsServerInsList[index]);

	mutex_lock(&vPtsServerIns->mListLock);
	pInstance = vPtsServerIns->pInstance;
	if (pInstance == NULL) {
		mutex_unlock(&vPtsServerIns->mListLock);
		return -1;
	}
	pts_pr_info(index,"mBaseOffset:%d mAlignmentOffset:%d\n",
						mStartOffset->mBaseOffset,
						mStartOffset->mAlignmentOffset);
	pInstance->mLastCheckinOffset = mStartOffset->mBaseOffset;
	pInstance->mLastCheckinPieceOffset = mStartOffset->mBaseOffset;
	pInstance->mAlignmentOffset = mStartOffset->mAlignmentOffset;
	mutex_unlock(&vPtsServerIns->mListLock);
	return 0;
}
EXPORT_SYMBOL(ptsserver_set_first_checkin_offset);

long ptsserver_checkin_pts_size(s32 pServerInsId,checkin_pts_size* mCheckinPtsSize) {
	PtsServerManage* vPtsServerIns = NULL;
	ptsserver_ins* pInstance = NULL;
	pts_node* ptn = NULL;
	pts_node* ptn_cur = NULL;
	pts_node* ptn_tmp = NULL;
	pts_node* del_ptn = NULL;
	s32 index = pServerInsId;
	s32 level = 0;
	bool checkinHeadNode = false;
	bool checkinSpliceNode = false;
	if (index < 0 || index >= MAX_INSTANCE_NUM)
		return -1;

	vPtsServerIns = &(vPtsServerInsList[index]);
	mutex_lock(&vPtsServerIns->mListLock);
	pInstance = vPtsServerIns->pInstance;
	if (pInstance == NULL) {
		mutex_unlock(&vPtsServerIns->mListLock);
		return -1;
	}
	if (pInstance->mListSize >= pInstance->mMaxCount) {
		list_for_each_entry_safe(ptn_cur, ptn_tmp, &pInstance->pts_list, node) {
			if (ptn_cur->index <= pInstance->mLastDropIndex) {
				del_ptn = ptn_cur;
				list_del(&del_ptn->node);
				list_add_tail(&del_ptn->node, &pInstance ->pts_free_list);
				pInstance->mLastDropIndex++;
				pInstance->mListSize--;
				if (ptsserver_debuglevel >= 1) {
				pts_pr_info(index,"Checkin delete node index node del_ptn:%px index:%lld, size:%d pts:0x%x pts_64:%lld\n",
									del_ptn,del_ptn->index,del_ptn->offset,
									del_ptn->pts,
									del_ptn->pts_64);
			}
			}
		}

		if (!del_ptn) {
			del_ptn = list_first_entry(&pInstance->pts_list, struct ptsnode, node);
			list_del(&del_ptn->node);
			list_add_tail(&del_ptn->node, &pInstance ->pts_free_list);	//queue empty buffer
			if (ptsserver_debuglevel >= 1) {
				pts_pr_info(index,"Checkin delete node first node del_ptn:%px size:%d pts:0x%x pts_64:%lld\n",
									del_ptn,del_ptn->offset,
									del_ptn->pts,
									del_ptn->pts_64);
			}
			pInstance->mListSize--;
		}
	}

	//record every checkin offset in mLastCheckinPieceOffset
	pInstance->mLastCheckinPieceOffset = pInstance->mLastCheckinPieceOffset + pInstance->mLastCheckinPieceSize;

	if (mCheckinPtsSize->pts == 0) {//checkin piece node in splice mode
		if (pInstance->mListSize != 0) {
			ptn = list_last_entry(&pInstance->pts_list, struct ptsnode, node);

			if (ptn == NULL) {
				if (ptsserver_debuglevel >= 1) {
					pts_pr_info(index,"ptn is null return \n");
				}
				mutex_unlock(&vPtsServerIns->mListLock);
				return -1;
			}
		} else {
			if (ptsserver_debuglevel >= 1) {
				pts_pr_info(index,"list is empty and head node not exist,just record info\n");
			}
			//for piece checkin record
			pInstance->mLastCheckinPieceSize = mCheckinPtsSize->size;
			pInstance->mLastCheckinPiecePts = mCheckinPtsSize->pts;
			pInstance->mLastCheckinPiecePts64 = mCheckinPtsSize->pts_64;

			if (ptsserver_debuglevel >= 1) {
				pts_pr_info(index,"[SpliceNode] [hasNoHead] Checkin Size(%d) 0x%x pts(32:0x%x 64:%lld)\n",
									pInstance->mLastCheckinPieceSize,
									pInstance->mLastCheckinPieceOffset,
									pInstance->mLastCheckinPiecePts,
									pInstance->mLastCheckinPiecePts64);
			}

			mutex_unlock(&vPtsServerIns->mListLock);
			return 0;
		}
		checkinSpliceNode = true;
	}  else {//checkin head node
		checkinHeadNode = true;
		if (!list_empty(&pInstance->pts_free_list)) {	//dequeue empty buffer
			ptn = list_first_entry(&pInstance->pts_free_list, struct ptsnode, node);
			list_del(&ptn->node);
		}

		if (ptn != NULL) {
			pInstance->mLastCheckinSize = pInstance->mLastCheckinPieceOffset - pInstance->mLastCheckinOffset;
			ptn->offset = pInstance->mLastCheckinOffset + pInstance->mLastCheckinSize;
			ptn->pts = mCheckinPtsSize->pts;
			ptn->pts_64 = mCheckinPtsSize->pts_64;
			ptn->expired_count = MAX_EXPIRED_COUNT;
			ptn->index = pInstance->mLastIndex++;
			if (ptsserver_debuglevel >= 1) {
					pts_pr_info(index,"-->dequeue empty ptn:%px offset:0x%x pts(32:0x%x 64:%lld)\n",
										ptn,ptn->offset,ptn->pts,
										ptn->pts_64);
				}
		} else {
			if (ptsserver_debuglevel >= 1) {
				pts_pr_info(index,"ptn is null return \n");
			}
			mutex_unlock(&vPtsServerIns->mListLock);
			return -1;
		}

		if (pInstance->setC2Mode) {
			list_for_each_entry_safe(ptn_cur,ptn_tmp,&pInstance->pts_list, node) {
				if (ptn_cur != NULL) {
					if (ptn_cur->pts_64 >= ptn->pts_64)
						break;
				}
			}
			list_add_tail(&ptn->node, &ptn_cur->node);
		} else {
			// sort pts_list by pts if pts invalid sort by offset,
			// so pts_list is amlost list by order
			list_for_each_entry_safe(ptn_cur, ptn_tmp, &pInstance->pts_list, node) {
				if (ptn_cur != NULL) {
					if (ptn->pts_64 != -1) {
						if (ptn->pts_64 <= ptn_cur->pts_64 &&
							get_llabs((s64)(ptn->pts_64 - ptn_cur->pts_64)) < DEFAULT_REWIND_PTS_THRESHOLD &&
							abs(ptn->index - ptn_cur->index) < DEFAULT_REWIND_INDEX_THRESHOLD)
							break;
					} else {
						if (ptn->offset <= ptn_cur->offset)
							break;
					}
				}
			}
			list_add_tail(&ptn->node, &ptn_cur->node);
		}

		if (ptn->pts_64 < pInstance->mLastCheckinPts64 &&
			get_llabs(ptn->pts_64 - pInstance->mLastCheckinPts64) > DEFAULT_REWIND_PTS_THRESHOLD) {
			ptn->duration_count = pInstance->mLastCheckinDurationCount++;
		} else {
			ptn->duration_count = pInstance->mLastCheckinDurationCount;
		}

		pInstance->mLastCheckinPts = ptn->pts;
		pInstance->mLastCheckinPts64 = ptn->pts_64;

		pInstance->mListSize++;
	}

	if (!pInstance->mPtsCheckinStarted) {
		pts_pr_info(index,"-->Checkin(First) ListSize:%d CheckinPtsSize(%d)-> offset:0x%x pts(32:0x%x 64:%llu)\n",
							pInstance->mListSize,
							mCheckinPtsSize->size,
							ptn->offset,ptn->pts,
							ptn->pts_64);
		pInstance->mFirstCheckinOffset = 0;
		pInstance->mFirstCheckinSize = mCheckinPtsSize->size;
		pInstance->mFirstCheckinPts = mCheckinPtsSize->pts;
		pInstance->mFirstCheckinPts64 = mCheckinPtsSize->pts_64;
		pInstance->mPtsCheckinStarted = 1;
	}

	if (pInstance->mAlignmentOffset != 0) {
		pInstance->mLastCheckinOffset = pInstance->mAlignmentOffset;
		pInstance->mLastCheckinPieceOffset = pInstance->mAlignmentOffset;
		pInstance->mAlignmentOffset = 0;
	} else {
		pInstance->mLastCheckinOffset = ptn->offset;
	}

	if (ptsserver_debuglevel >= 1 && checkinHeadNode == true) {
		level = ptn->offset - pInstance->mLastCheckoutCurOffset;
		pts_pr_info(index,"[HeadNode] Checkin ListCount:%d LastCheckinOffset:0x%x LastCheckinSize:0x%x\n",
								pInstance->mListSize,
								pInstance->mLastCheckinOffset,
								pInstance->mLastCheckinSize);
		pts_pr_info(index,"[HeadNode] Checkin Size(%d) %s:0x%x (%s:%d) pts(32:0x%x 64:%lld) duration_count:%lld\n",
							mCheckinPtsSize->size,
							ptn->offset < pInstance->mLastCheckinOffset?"rest offset":"offset",
							ptn->offset,
							level >= 5242880 ? ">5M level": "level",
							level,
							ptn->pts,
							ptn->pts_64,
							ptn->duration_count);
	}

	//for piece checkin record
	pInstance->mLastCheckinPieceSize = mCheckinPtsSize->size;
	pInstance->mLastCheckinPiecePts = mCheckinPtsSize->pts;
	pInstance->mLastCheckinPiecePts64 = mCheckinPtsSize->pts_64;

	if (ptsserver_debuglevel >= 1 && checkinSpliceNode == true) {
	pts_pr_info(index,"[SpliceNode] Checkin Size(%d) %s 0x%x pts(32:0x%x 64:%lld) ptn:%px\n",
							pInstance->mLastCheckinPieceSize,
							ptn->offset < pInstance->mLastCheckinOffset?"rest offset":"offset",
							pInstance->mLastCheckinPieceOffset,
							pInstance->mLastCheckinPiecePts,
							pInstance->mLastCheckinPiecePts64,
							ptn);
	}

	mutex_unlock(&vPtsServerIns->mListLock);
	return 0;
}
EXPORT_SYMBOL(ptsserver_checkin_pts_size);

long ptsserver_checkin_pts_offset(s32 pServerInsId, checkin_pts_offset* mCheckinPtsOffset) {
	PtsServerManage* vPtsServerIns = NULL;
	ptsserver_ins* pInstance = NULL;
	pts_node* ptn = NULL;
	pts_node* ptn_cur = NULL;
	pts_node* ptn_tmp = NULL;
	pts_node* del_ptn = NULL;
	s32 index = pServerInsId;
	s32 level = 0;
	bool checkinHeadNode = false;
	bool checkinSpliceNode = false;
	if (index < 0 || index >= MAX_INSTANCE_NUM)
		return -1;

	vPtsServerIns = &(vPtsServerInsList[index]);
	mutex_lock(&vPtsServerIns->mListLock);
	pInstance = vPtsServerIns->pInstance;
	if (pInstance == NULL) {
		mutex_unlock(&vPtsServerIns->mListLock);
		return -1;
	}
	if (pInstance->mListSize >= pInstance->mMaxCount) {
		del_ptn = list_first_entry(&pInstance->pts_list, struct ptsnode, node);
		list_del(&del_ptn->node);
		list_add_tail(&del_ptn->node, &pInstance ->pts_free_list);	//queue empty buffer
		if (ptsserver_debuglevel >= 1) {
			pts_pr_info(index,"Checkin delete node del_ptn:%px size:%d pts:0x%x pts_64:%lld\n",
								del_ptn,del_ptn->offset,
								del_ptn->pts,
								del_ptn->pts_64);
		}
		pInstance->mListSize--;
	}

	//record every checkin offset in mLastCheckinPieceOffset
	pInstance->mLastCheckinPieceOffset = pInstance->mLastCheckinPieceOffset + pInstance->mLastCheckinPieceSize;

	if (mCheckinPtsOffset->pts == 0) {//checkin piece node in splice mode
		if (pInstance->mListSize != 0) {
			ptn = list_last_entry(&pInstance->pts_list, struct ptsnode, node);

			if (ptn == NULL) {
				if (ptsserver_debuglevel >= 1) {
					pts_pr_info(index,"ptn is null return \n");
				}
				mutex_unlock(&vPtsServerIns->mListLock);
				return -1;
			}
		} else {
			if (ptsserver_debuglevel >= 1) {
				pts_pr_info(index,"list is empty and head node not exist,just record info\n");
			}
			//for piece checkin record
			pInstance->mLastCheckinPieceOffset = mCheckinPtsOffset->offset;
			pInstance->mLastCheckinPiecePts = mCheckinPtsOffset->pts;
			pInstance->mLastCheckinPiecePts64 = mCheckinPtsOffset->pts_64;

			if (ptsserver_debuglevel >= 1) {
				pts_pr_info(index,"[SpliceNode] [hasNoHead] Checkin offset:0x%x pts(32:0x%x 64:%lld)\n",
									pInstance->mLastCheckinPieceOffset,
									pInstance->mLastCheckinPiecePts,
									pInstance->mLastCheckinPiecePts64);
			}

			mutex_unlock(&vPtsServerIns->mListLock);
			return 0;
		}
		checkinSpliceNode = true;
	}  else {//checkin head node
		checkinHeadNode = true;
		if (!list_empty(&pInstance->pts_free_list)) {	//dequeue empty buffer
			ptn = list_first_entry(&pInstance->pts_free_list, struct ptsnode, node);
			list_del(&ptn->node);
		}

		if (ptn != NULL) {
			pInstance->mLastCheckinSize = pInstance->mLastCheckinPieceOffset - pInstance->mLastCheckinOffset;
			ptn->offset = mCheckinPtsOffset->offset;
			ptn->pts = mCheckinPtsOffset->pts;
			ptn->pts_64 = mCheckinPtsOffset->pts_64;
			ptn->expired_count = MAX_EXPIRED_COUNT;
			if (ptsserver_debuglevel >= 1) {
					pts_pr_info(index,"-->dequeue empty ptn:%px offset:0x%x pts(32:0x%x 64:%lld)\n",
										ptn,ptn->offset,ptn->pts,
										ptn->pts_64);
				}
		} else {
			if (ptsserver_debuglevel >= 1) {
				pts_pr_info(index,"ptn is null return \n");
			}
			mutex_unlock(&vPtsServerIns->mListLock);
			return -1;
		}

		if (pInstance->setC2Mode) {
			list_for_each_entry_safe(ptn_cur,ptn_tmp,&pInstance->pts_list, node) {
				if (ptn_cur != NULL) {
					if (ptn_cur->pts_64 >= ptn->pts_64)
						break;
				}
			}
			list_add_tail(&ptn->node, &ptn_cur->node);
		} else {
			// sort pts_list by pts if pts invalid sort by offset,
			// so pts_list is amlost list by order
			list_for_each_entry_safe(ptn_cur, ptn_tmp, &pInstance->pts_list, node) {
				if (ptn_cur != NULL) {
					if (ptn->pts_64 != -1) {
						if (ptn->pts_64 <= ptn_cur->pts_64 &&
							get_llabs((s64)(ptn->pts_64 - ptn_cur->pts_64)) < DEFAULT_REWIND_PTS_THRESHOLD &&
							abs(ptn->index - ptn_cur->index) < DEFAULT_REWIND_INDEX_THRESHOLD)
							break;
					} else {
						if (ptn->offset <= ptn_cur->offset)
							break;
					}
				}
			}
			list_add_tail(&ptn->node, &ptn_cur->node);
		}

		if (ptn->pts_64 < pInstance->mLastCheckinPts64 &&
			get_llabs(ptn->pts_64 - pInstance->mLastCheckinPts64) > DEFAULT_REWIND_PTS_THRESHOLD) {
			ptn->duration_count = pInstance->mLastCheckinDurationCount++;
		} else {
			ptn->duration_count = pInstance->mLastCheckinDurationCount;
		}

		pInstance->mLastCheckinPts = ptn->pts;
		pInstance->mLastCheckinPts64 = ptn->pts_64;

		pInstance->mListSize++;
	}

	if (!pInstance->mPtsCheckinStarted) {
		pts_pr_info(index,"-->Checkin(First) ListSize:%d offset:0x%x pts(32:0x%x 64:%llu)\n",
							pInstance->mListSize,
							ptn->offset,ptn->pts,
							ptn->pts_64);
		pInstance->mFirstCheckinOffset = mCheckinPtsOffset->offset;
		pInstance->mFirstCheckinPts = mCheckinPtsOffset->pts;
		pInstance->mFirstCheckinPts64 = mCheckinPtsOffset->pts_64;
		pInstance->mPtsCheckinStarted = 1;
	}

	if (pInstance->mAlignmentOffset != 0) {
		pInstance->mLastCheckinOffset = pInstance->mAlignmentOffset;
		pInstance->mLastCheckinPieceOffset = pInstance->mAlignmentOffset;
		pInstance->mAlignmentOffset = 0;
	} else {
		pInstance->mLastCheckinOffset = ptn->offset;
	}

	if (ptsserver_debuglevel >= 1 && checkinHeadNode == true) {
		level = ptn->offset - pInstance->mLastCheckoutCurOffset;
		pts_pr_info(index,"[HeadNode] Checkin ListCount:%d LastCheckinOffset:0x%x LastCheckinSize:0x%x\n",
								pInstance->mListSize,
								pInstance->mLastCheckinOffset,
								pInstance->mLastCheckinSize);
		pts_pr_info(index,"[HeadNode] Checkin %s:0x%x (%s:%d) pts(32:0x%x 64:%lld) duration_count:%lld\n",
							ptn->offset < pInstance->mLastCheckinOffset?"rest offset":"offset",
							ptn->offset,
							level >= 5242880 ? ">5M level": "level",
							level,
							ptn->pts,
							ptn->pts_64,
							ptn->duration_count);
	}

	//for piece checkin record
	pInstance->mLastCheckinPieceOffset = mCheckinPtsOffset->offset;
	pInstance->mLastCheckinPiecePts = mCheckinPtsOffset->pts;
	pInstance->mLastCheckinPiecePts64 = mCheckinPtsOffset->pts_64;

	if (ptsserver_debuglevel >= 1 && checkinSpliceNode == true) {
	pts_pr_info(index,"[SpliceNode] Checkin %s 0x%x pts(32:0x%x 64:%lld) ptn:%px\n",
							ptn->offset < pInstance->mLastCheckinOffset?"rest offset":"offset",
							pInstance->mLastCheckinPieceOffset,
							pInstance->mLastCheckinPiecePts,
							pInstance->mLastCheckinPiecePts64,
							ptn);
	}

	mutex_unlock(&vPtsServerIns->mListLock);
	return 0;
}
EXPORT_SYMBOL(ptsserver_checkin_pts_offset);

long ptsserver_checkout_pts_offset(s32 pServerInsId, checkout_pts_offset* mCheckoutPtsOffset) {
	PtsServerManage* vPtsServerIns = NULL;
	ptsserver_ins* pInstance = NULL;
	s32 index = pServerInsId;
	pts_node* ptn = NULL;
	pts_node* find_ptn = NULL;
	pts_node* fit_offset_ptn = NULL;
	pts_node* fit_pts_ptn = NULL;
	pts_node* ptn_tmp = NULL;

	u32 cur_offset = 0xFFFFFFFF & mCheckoutPtsOffset->offset;
	s32 cur_duration = (mCheckoutPtsOffset->offset >> 32) & 0x3FFFFFFF;

	u32 expected_offset_diff = 2500;
	u64 expected_pts = 0;
	s32 find_frame_num = -1;
	s32 find = 0;
	s32 invalid_mode = 0;
	u32 offsetAbs = 0;
	u32 FrameDur = 0;
	u64 FrameDur64 = 0;
	s32 find_index = -1;
	s32 fit_offset_number = -1;
	s32 fit_pts_number = -1;
	s32 must_have_fit = 0;
	s32 retryCount = MAX_CHECKOUT_RETRY_COUNT;
	s32 lastCheckoutPts = 0;
	s64 lastCheckoutPtsU64 = 0;
	s32 shot_bound = MAX_CHECKOUT_BOUND_VALUE;// show the credibility of the current pts
	s32 i = 0;

	if (index < 0 || index >= MAX_INSTANCE_NUM) {
		return -1;
	}
	vPtsServerIns = &(vPtsServerInsList[index]);
	mutex_lock(&vPtsServerIns->mListLock);
	pInstance = vPtsServerIns->pInstance;

	if (pInstance == NULL) {
		mutex_unlock(&vPtsServerIns->mListLock);
		return -1;
	}

	expected_offset_diff = pInstance->mLookupThreshold;
	expected_pts = pInstance->mLastCheckoutPts64;

	if (!pInstance->mPtsCheckoutStarted) {
		pts_pr_info(index,"Checkout(First) ListSize:%d offset:0x%x duration:%d\n",
							pInstance->mListSize,
							cur_offset,
							cur_duration);
	}

	if (ptsserver_debuglevel >= 1) {
		pts_pr_info(index,"checkout ListCount:%d offset:%llx cur_offset:0x%x cur_duration:%d\n",
							pInstance->mListSize,
							mCheckoutPtsOffset->offset,
							cur_offset,
							cur_duration);
	}

	// Normal Case: Pull pts from list if decoder send offset with value.
	// 1.stream mode with normal offset
	// 2.v4l2 pileline es mode, pts must checkout from checkin node
	if (cur_offset != 0xFFFFFFFF &&
		!list_empty(&pInstance->pts_list)) {
		find = 0;
		list_for_each_entry_safe(ptn,ptn_tmp,&pInstance->pts_list, node) {

			if (ptn != NULL) {
				offsetAbs = abs(cur_offset - ptn->offset);

				// Only if ptn node is deadly expired, we increase the drop count.
				if (ptn->offset == 0xFFFFFFFF || ptn->offset <= pInstance->mLastCheckoutCurOffset) {
					if ((ptn->pts_64 < pInstance->mLastCheckoutPts64) || (ptn->pts == -1)) {
						ptn->expired_count --;
					}
				}

				// Print all ptn base info
				if (ptsserver_debuglevel > 3) {
					pts_pr_info(index,"Checkout i:%d offset(diff:%d L:0x%x C:0x%x pts:0x%x pts_64:%llu expired_count:%d dur_count:%lld) expected_pts:0x%llu\n",
									i, offsetAbs, ptn->offset, cur_offset, ptn->pts, ptn->pts_64, ptn->expired_count, ptn->duration_count, expected_pts);
				}

				//record the first fit pts minimum ptn node to record,this node is the key one normally even without
				// check offset,this can double check if the key one right or not in some specially case.
				if (fit_pts_ptn == NULL && pInstance->mLastCheckoutPts64 > 0) {
					if (((ptn->pts_64 - pInstance->mLastCheckoutPts64) > 10000) && ((ptn->pts_64 - pInstance->mLastCheckoutPts64) < 500000) &&
							offsetAbs < 50000) {
						fit_pts_ptn = ptn;
						fit_pts_number = i;
						if (ptsserver_debuglevel >= 1) {
							pts_pr_info(index, "record fit in minimum pts case. find_index:%d, pts:0x%x, pts_64:%llu\n",
									fit_pts_number, fit_pts_ptn->pts, fit_pts_ptn->pts_64);
						}
					}
				}

				// If shot the threshold, there must be one ptn node be selected.
				// So trust mLookupThreshold,if not shot in mLookupThreshold, ignore them all.
				// Otherwise mLookupThreshold is not right,need adjust it.
				if (offsetAbs <=  pInstance->mLookupThreshold) {

					// Record the minimum offset as anchors, if any case need to reset pts, use this node fit_offset_ptn.
					// If in mLookupThreshold,any ptn fit in offset, we mark must_have_fit,
					// which means we can pull one ptn in list for sure, no matter where it is.
					if ((offsetAbs <= expected_offset_diff && cur_offset > ptn->offset)) {
						fit_offset_ptn = ptn;
						fit_offset_number = i;
						expected_offset_diff = offsetAbs;
						// cause we consider fit pts as find one, so it is we can find key one even in mLookupThreshold,
						// But if in mLookupThreshold,we can pull one for sure so we mark must_have_fit to find ptn not in mLookupThreshold.
						must_have_fit = 1;
						if (ptsserver_debuglevel >= 1) {
							pts_pr_info(index, "Record fit in minimum offset case. find_index:%d\n", fit_offset_number);
						}
						// Deal with invalid pts == -1 case,
						// If first node is invalid case, set it as found node
						// If there is other node found, we can only compare offset, and set invalid flag
						if (ptn->pts == -1) {
							pInstance->mOffsetMode = 1;
							find = 1;// Here to set find to avoid retryCount count wrong
							if (ptsserver_debuglevel >= 1) {
								pts_pr_info(index, "Found invalid pts. in invalid mode\n");
							}
						}
					}

					// Compare pts as first Strategy, expect the first ptn in mLookupThreshold will be the key one
					// any node in mLookupThreshold has invalid pts we not compare pts, just conmare offset
					if (ptn->pts_64 >= pInstance->mLastCheckoutPts64 &&
											!pInstance->mOffsetMode &&
											cur_offset > ptn->offset) {
											if (ptn->pts_64 <= expected_pts ||
												expected_pts == pInstance->mLastCheckoutPts64) {
												find = 1;
												find_index = i;
												find_ptn = ptn;
												expected_pts = ptn->pts_64;
											}
					}
					if (cur_offset > ptn->offset || (find_frame_num >= 0)) {
						find_frame_num = 0;
					}
				}else if (find_frame_num >= 0) {
					find_frame_num ++;
				}

				// Drop Node Strategy
				// <Out of Buffer Size>
				// <expired count == 0>
				if ((/*cur_offset > ptn->offset && */offsetAbs > 251658240 || ptn->expired_count <= 0) &&
				    (find_ptn != ptn) && (ptn != fit_offset_ptn) && (fit_pts_ptn != ptn)) {
					// > 240M (15M*16)
					if (ptsserver_debuglevel >= 1) {
						pts_pr_info(index,"Checkout delete(%d) diff:%d offset:0x%x pts:0x%x pts_64:%llu,drop reason map:<%d %d>\n",
											i,
											offsetAbs,
											ptn->offset,
											ptn->pts,
											ptn->pts_64,
											offsetAbs > 251658240,
											ptn->expired_count <= 0);
					}
					list_del(&ptn->node);
					list_add_tail(&ptn->node, &pInstance ->pts_free_list);
					pInstance->mListSize--;

				}

				if (find) {
					// Deal with the case pts rewind and insert the first node in pts_list,
					// and offset is also within correct threshhold.But the key on is at the end of pts_list,
					// and we can`t find within just in MAX_CHECKOUT_RETRY_COUNT,we need retry all list
					if (find_index == 0 && pInstance->mLastCheckoutIndex > 0) {
						retryCount = pInstance->mListSize;
					}
				}

				if (find_frame_num >= retryCount) {
					break;
				}
			}
			i++;
		}

		if (pInstance->mOffsetMode) {
			if (fit_offset_ptn) {
				find = 1;
				find_ptn = fit_offset_ptn;
				find_index = fit_offset_number;
				shot_bound --;
				pInstance->mStickyWrapFlag = false;
				if (find_ptn->pts == -1) {
					invalid_mode = 1;
					if (ptsserver_debuglevel >= 1) {
						pts_pr_info(index, "Checkout invalid pts case find_index:%d\n", find_index);
					}
				}
			}
		} else {
			// to see if the key one within offset the same as the minimum pts diff one,
			// if not fit, we choose to trust pts fit.
			if (must_have_fit && fit_offset_ptn) {
				find = 1;
				if (pInstance->mPtsCheckoutStarted
					&& fit_pts_ptn && find_ptn != fit_pts_ptn
					&& fit_pts_ptn->duration_count == fit_offset_ptn->duration_count
					&& !pInstance->mStickyWrapFlag) {
					if (fit_pts_ptn->pts_64 > fit_offset_ptn->pts_64 &&
						fit_offset_ptn->pts_64 - pInstance->mLastCheckoutPts64 > 10000) {
						find_ptn = fit_offset_ptn;
						find_index = fit_offset_number;
						shot_bound --;
						pInstance->mStickyWrapFlag = false;
						if (ptsserver_debuglevel > 1) {
							pts_pr_info(index, "offset fit pts more valuable than pts fit one find_index:%d, bound value:%d\n", find_index, shot_bound);
						}
					} else {
						find_ptn = fit_pts_ptn;
						find_index = fit_pts_number;
						shot_bound --;
						pInstance->mStickyWrapFlag = true;
						if (ptsserver_debuglevel > 1) {
							pts_pr_info(index, "Checkout pts fit not the same as key one case find_index:%d, bound value:%d\n", find_index, shot_bound);
						}
					}
					// if find by pts not found but can be found by offset, need reset by offset,usually seen in pts rewind case
				} else if ((fit_offset_ptn && !find_ptn) || !pInstance->mPtsCheckoutStarted) {
					find_ptn = fit_offset_ptn;
					find_index = fit_offset_number;
					shot_bound --;
					pInstance->mStickyWrapFlag = false;
					if (ptsserver_debuglevel >= 1) {
						pts_pr_info(index, "Not find pts fit node, need reset by offset number:%d\n", find_index);
					}
				} else if (find_ptn) {
					pInstance->mStickyWrapFlag = false;
					if (ptsserver_debuglevel > 1) {
						pts_pr_info(index, "Perfect case %d\n", find_index);
					}
				}
			}
		}

		if (!find && pInstance->mTrickMode == 1) {//i_only, not found pts, need retry!
			i = 0;
			find_frame_num = 0;
			expected_offset_diff = 10000;
			list_for_each_entry_safe(ptn,ptn_tmp,&pInstance->pts_list, node) {
				if (ptn != NULL) {
					offsetAbs = abs(cur_offset - ptn->offset);
				}
				if (offsetAbs <= expected_offset_diff && cur_offset > ptn->offset) {
					expected_offset_diff = offsetAbs;
					find = 1;
					find_index = i;
					find_ptn = ptn;
				} else if (find_frame_num > 5) {
					break;
				}
				if (find) {
					find_frame_num++;
				}
				i++;
			}
		}

		if (!find && pInstance->setC2Mode == 1) {
			if (cur_offset != 0xFFFFFFFF) {
				u32 min_offsetAbs = 0xFFFFFFFF;

				if (pInstance->mPtsCheckoutStarted) {
					mCheckoutPtsOffset->pts = pInstance->mLastCheckoutPts;
					mCheckoutPtsOffset->pts_64 = pInstance->mLastCheckoutPts64;
					min_offsetAbs = abs(cur_offset - pInstance->mLastCheckoutCurOffset);
				}

				list_for_each_entry_safe(ptn,ptn_tmp,&pInstance->pts_list, node) {
					if (ptn != NULL && (cur_offset > ptn->offset) && (ptn->offset >= pInstance->mLastCheckoutCurOffset)) {
						offsetAbs = abs(cur_offset - ptn->offset);

						if (offsetAbs < min_offsetAbs) {
							min_offsetAbs = offsetAbs;
							find_ptn = ptn;
							if (ptsserver_debuglevel >= 1)
								pts_pr_info(index,"Checkout change to i:%d offset(diff:%d L:0x%x C:0x%x)\n",
								i,offsetAbs,find_ptn->offset,cur_offset);
						}
					}
				}
				if (find_ptn) {
					mCheckoutPtsOffset->pts = find_ptn->pts;
					mCheckoutPtsOffset->pts_64 = find_ptn->pts_64;
					cur_offset = find_ptn->offset;
					if (ptsserver_debuglevel >= 1 || !pInstance->mPtsCheckoutStarted)
						pts_pr_info(index,"Checkout second ok ListCount:%d find:%d offset(diff:%d L:0x%x) pts(32:0x%x 64:%llu)\n",
							pInstance->mListSize,find_index,expected_offset_diff, find_ptn->offset,find_ptn->pts,find_ptn->pts_64);
				} else {
					if (!pInstance->mPtsCheckoutStarted && !list_empty(&pInstance->pts_list)) {
						find_ptn = list_first_entry(&pInstance->pts_list, struct ptsnode, node);
						mCheckoutPtsOffset->pts = find_ptn->pts;
						mCheckoutPtsOffset->pts_64 = find_ptn->pts_64;
						cur_offset = find_ptn->offset;
						if (ptsserver_debuglevel >= 1)
							pts_pr_info(index,"set first pts in list as the selected node pts(32:0x%x 64:%llu)\n",
							mCheckoutPtsOffset->pts, mCheckoutPtsOffset->pts_64);
					} else {
						cur_offset = pInstance->mLastCheckoutCurOffset;
						if (ptsserver_debuglevel >= 1)
							pts_pr_info(index,"set lastCheck pts as the selected node pts(32:0x%x 64:%llu)\n",
							mCheckoutPtsOffset->pts, mCheckoutPtsOffset->pts_64);
					}
				}
				find = 1;
			}
		}

		pInstance->mLastCheckoutIndex = find_index;

		// Deal with all the key ptn
		if (find && find_ptn) {
			if (!invalid_mode) {
				mCheckoutPtsOffset->pts = find_ptn->pts;
				mCheckoutPtsOffset->pts_64 = find_ptn->pts_64;
				pInstance->mLastCheckoutDurationCount = find_ptn->duration_count;
				if (ptsserver_debuglevel >= 1 ||
					!pInstance->mPtsCheckoutStarted) {
					pts_pr_info(index,
						"Checkout ok ListCount:%d find:%d offset(diff:%d L:0x%x C:%x) pts(32:0x%x 64:%lld) l_Checkoutpts:%lld, dur_count:%lld, diff:%lld us shot_bound:%d\n",
									pInstance->mListSize,find_index,abs(cur_offset - find_ptn->offset),
									find_ptn->offset,cur_offset,find_ptn->pts,find_ptn->pts_64, pInstance->mLastCheckoutPts64,
									pInstance->mLastCheckoutDurationCount,mCheckoutPtsOffset->pts_64 - lastCheckoutPtsU64,
									shot_bound);
				}
			}

			list_del(&find_ptn->node);
			list_add_tail(&find_ptn->node, &pInstance ->pts_free_list);
			pInstance->mListSize--;
		}
	}

	// Unnormal Case: Calculate pts with duration.
	// 1.cant find node in ptslist within threshold
	// 2.one package with multi-frame,this case has no offset
	// 3.pts == -1 case, pts has error value
	if (!find || invalid_mode) {
		lastCheckoutPts = pInstance->mLastCheckoutPts;
		pInstance->mPtsCheckoutFailCount++;

		// If cur_offset smaller than last offset means sticky package from codec
		if (cur_offset < pInstance->mLastCheckoutCurOffset) {
			pInstance->mStickyWrapFlag = false;
			if (ptsserver_debuglevel > 1) {
				pts_pr_info(index, "Clean Sticky Flag in PtsCheckoutFail\n");
			}
		} else {
			if (ptsserver_debuglevel > 1) {
				pts_pr_info(index, "Checkout Might occure discontinue case in PtsCheckoutFail, need reset by offset\n");
			}
		}

		shot_bound = 3;
		if (invalid_mode && find_ptn) {
			pInstance->mLastCheckoutDurationCount = find_ptn->duration_count;
		}
		if (ptsserver_debuglevel >= 1 || pInstance->mPtsCheckoutFailCount % 30 == 0) {
			pts_pr_info(index,"Checkout fail mPtsCheckoutFailCount:%d level:%d \n",
				pInstance->mPtsCheckoutFailCount,pInstance->mLastCheckinOffset - cur_offset);
		}
		if (pInstance->mDecoderDuration != 0) {
			pInstance->mLastCheckoutPts = pInstance->mLastCheckoutPts + pInstance->mDecoderDuration * 90 / 96;
			pInstance->mLastCheckoutPts64 = pInstance->mLastCheckoutPts64 + pInstance->mDecoderDuration * 1000 / 96;
			if (ptsserver_debuglevel >= 1) {
				pts_pr_info(index,"Checkout fail Calculate by mDecoderDuration:%d pts(32:%x 64:%lld) lastCheckoutpts:%x, shot_bound:%d  diff:%lld us\n",
									pInstance->mDecoderDuration,
									pInstance->mLastCheckoutPts,
									pInstance->mLastCheckoutPts64,
									lastCheckoutPts,
									shot_bound,
									pInstance->mLastCheckoutPts64 - lastCheckoutPtsU64);
			}
		} else {
			if (pInstance->mFrameDuration != 0) {
				pInstance->mLastCheckoutPts = pInstance->mLastCheckoutPts + pInstance->mFrameDuration;
			}
			if (pInstance->mFrameDuration64 != 0) {
				pInstance->mLastCheckoutPts64 = pInstance->mLastCheckoutPts64 + pInstance->mFrameDuration64;
			}
			if (pInstance->mPtsCheckoutStarted == 0) {

				pts_pr_info(index,"first Checkout fail cur_offset:0x%x expected_offset_diff:0x%x\n",cur_offset,expected_offset_diff);
				pInstance->mLastCheckoutPts = -1;
				pInstance->mLastCheckoutPts64 = -1;

			}
			if (ptsserver_debuglevel >= 1) {
				pts_pr_info(index,"Checkout fail Calculate by FrameDuration(32:%d 64:%lld) pts(32:%x 64:%lld) lastCheckoutpts:%x, shot_bound:%d diff:%lld us\n",
									pInstance->mFrameDuration,
									pInstance->mFrameDuration64,
									pInstance->mLastCheckoutPts,
									pInstance->mLastCheckoutPts64,
									lastCheckoutPts,
									shot_bound,
									pInstance->mLastCheckoutPts64 - lastCheckoutPtsU64);
			}
		}
		mCheckoutPtsOffset->pts = pInstance->mLastCheckoutPts;
		mCheckoutPtsOffset->pts_64 = pInstance->mLastCheckoutPts64;
		pInstance->mLastCheckoutCurOffset = cur_offset;
		mutex_unlock(&vPtsServerIns->mListLock);
		return 0;
	}

	// Calculate duration if checkout first time
	if (pInstance->mPtsCheckoutStarted) {
		if (pInstance->mFrameDuration == 0 && pInstance->mFrameDuration64 == 0) {

			pInstance->mFrameDuration = div_u64(mCheckoutPtsOffset->pts - pInstance->mLastCheckoutPts,
													pInstance->mPtsCheckoutFailCount + 1);

			pInstance->mFrameDuration64 = div_u64(mCheckoutPtsOffset->pts_64 - pInstance->mLastCheckoutPts64,
													pInstance->mPtsCheckoutFailCount + 1);

			if (pInstance->mFrameDuration < 0 ||
				pInstance->mFrameDuration > 9000) {
				pInstance->mFrameDuration = 0;
			}
			if (pInstance->mFrameDuration64 < 0 ||
				pInstance->mFrameDuration64 > 100000) {
				pInstance->mFrameDuration64 = 0;
			}
		} else {

			FrameDur = div_u64(mCheckoutPtsOffset->pts - pInstance->mLastDoubleCheckoutPts,
								pInstance->mPtsCheckoutFailCount + 1);

			FrameDur64 = div_u64(mCheckoutPtsOffset->pts_64 - pInstance->mLastDoubleCheckoutPts64,
									pInstance->mPtsCheckoutFailCount + 1);

			if (ptsserver_debuglevel > 1) {
				pts_pr_info(index,"checkout FrameDur(32:%d 64:%lld) DoubleCheckFrameDuration(32:%x 64:%llu)\n",
									FrameDur,
									FrameDur64,
									pInstance->mDoubleCheckFrameDuration,
									pInstance->mDoubleCheckFrameDuration64);
				pts_pr_info(index,"checkout LastDoubleCheckoutPts(32:%d 64:%lld) pts(32:%x 64:%lld) PtsCheckoutFailCount:%d diff:%lld us\n",
									pInstance->mLastDoubleCheckoutPts,
									pInstance->mLastDoubleCheckoutPts64,
									mCheckoutPtsOffset->pts,
									mCheckoutPtsOffset->pts_64,
									pInstance->mPtsCheckoutFailCount,
									mCheckoutPtsOffset->pts_64 - lastCheckoutPtsU64);
			}


			if ((FrameDur == pInstance->mDoubleCheckFrameDuration) ||
				(FrameDur64 == pInstance->mDoubleCheckFrameDuration64)) {
				pInstance->mDoubleCheckFrameDurationCount ++;
			} else {
				pInstance->mDoubleCheckFrameDuration = FrameDur;
				pInstance->mDoubleCheckFrameDuration64 = FrameDur64;
				pInstance->mDoubleCheckFrameDurationCount = 0;
			}
			if (pInstance->mDoubleCheckFrameDurationCount > pInstance->kDoubleCheckThreshold) {
				if (ptsserver_debuglevel > 1) {
					pts_pr_info(index,"checkout DoubleCheckFrameDurationCount(%d) DoubleCheckFrameDuration(32:%x 64:%llu)\n",
										pInstance->mDoubleCheckFrameDurationCount,
										pInstance->mDoubleCheckFrameDuration,
										pInstance->mDoubleCheckFrameDuration64);
				}
				pInstance->mFrameDuration = pInstance->mDoubleCheckFrameDuration;
				pInstance->mFrameDuration64 = pInstance->mDoubleCheckFrameDuration64;
				pInstance->mDoubleCheckFrameDurationCount = 0;
			}
		}
	} else {
		pInstance->mPtsCheckoutStarted = 1;
	}

	pInstance->mPtsCheckoutFailCount = 0;
	pInstance->mLastCheckoutOffset = mCheckoutPtsOffset->offset;
	pInstance->mLastCheckoutPts = mCheckoutPtsOffset->pts;
	pInstance->mLastCheckoutPts64 = mCheckoutPtsOffset->pts_64;
	pInstance->mLastPeekPts = mCheckoutPtsOffset->pts;
	pInstance->mLastPeekPts64 = mCheckoutPtsOffset->pts_64;
	pInstance->mLastDoubleCheckoutPts = mCheckoutPtsOffset->pts;
	pInstance->mLastDoubleCheckoutPts64 = mCheckoutPtsOffset->pts_64;
	pInstance->mDecoderDuration = cur_duration;
	pInstance->mLastCheckoutCurOffset = cur_offset;
	pInstance->mLastShotBound = shot_bound;
	mutex_unlock(&vPtsServerIns->mListLock);
	return 0;
}
EXPORT_SYMBOL(ptsserver_checkout_pts_offset);

long ptsserver_peek_pts_offset(s32 pServerInsId,checkout_pts_offset* mCheckoutPtsOffset) {
	PtsServerManage* vPtsServerIns = NULL;
	ptsserver_ins* pInstance = NULL;
	s32 index = pServerInsId;
	pts_node* ptn = NULL;
	pts_node* find_ptn = NULL;
	pts_node* fit_offset_ptn = NULL;
	pts_node* ptn_tmp = NULL;

	u32 cur_offset = 0xFFFFFFFF & mCheckoutPtsOffset->offset;

	u32 expected_offset_diff = 2500;
	u64 expected_pts = 0;
	s32 find_frame_num = 0;
	s32 find = 0;
	u32 offsetAbs = 0;
	s32 fit_offset_number = -1;
	s32 must_have_fit = 0;
	s32 retryCount = MAX_CHECKOUT_RETRY_COUNT;

	s32 find_index = -1;
	s32 invalid_mode = 0;
	s32 i = 0;
	u32 mCalculateLastCheckoutPts = 0;
	u64 mCalculateLastCheckoutPts64 = 0;
	s32 shot_bound = MAX_CHECKOUT_BOUND_VALUE;// show the credibility of the current pts
	if (index < 0 || index >= MAX_INSTANCE_NUM)
		return -1;

	vPtsServerIns = &(vPtsServerInsList[index]);
	mutex_lock(&vPtsServerIns->mListLock);
	pInstance = vPtsServerIns->pInstance;
	if (pInstance == NULL) {
		pts_pr_info(pServerInsId,"ptsserver_peek_pts_offset pInstance == NULL ok \n");
		mutex_unlock(&vPtsServerIns->mListLock);
		return -1;
	}

	expected_offset_diff = pInstance->mLookupThreshold;
	expected_pts = pInstance->mLastPeekPts64;

	// Normal Case: Pull pts from list if decoder send offset with value.
	// 1.stream mode with normal offset
	// 2.v4l2 pileline es mode, pts must checkout from checkin node
	if (cur_offset != 0xFFFFFFFF &&
		!list_empty(&pInstance->pts_list)) {
		find_frame_num = 0;
		find = 0;
		list_for_each_entry_safe(ptn,ptn_tmp,&pInstance->pts_list, node) {

			if (ptn != NULL) {
				offsetAbs = abs(cur_offset - ptn->offset);

				// Print all ptn base info
				if (ptsserver_debuglevel > 3) {
					pts_pr_info(index,"Peek i:%d offset(diff:%d L:0x%x C:0x%x pts:0x%x pts_64:%llu expired_count:%d dur_count:%lld) expected_pts:0x%llu\n",
									i, offsetAbs, ptn->offset, cur_offset, ptn->pts, ptn->pts_64, ptn->expired_count, ptn->duration_count, expected_pts);
				}

				// If shot the threshold, there must be one ptn node be selected.
				// So trust mLookupThreshold,if not shot in mLookupThreshold, ignore them all.
				// Otherwise mLookupThreshold is not right,need adjust it.
				if (offsetAbs <=  pInstance->mLookupThreshold) {

					// Record the minimum offset as anchors, if any case need to reset pts, use this node fit_offset_ptn.
					// If in mLookupThreshold,any ptn fit in offset, we mark must_have_fit,
					// which means we can pull one ptn in list for sure, no matter where it is.
					if ((offsetAbs <= expected_offset_diff && cur_offset > ptn->offset)) {
						fit_offset_ptn = ptn;
						fit_offset_number = i;
						expected_offset_diff = offsetAbs;
						// cause we consider fit pts as find one, so it is we can find key one even in mLookupThreshold,
						// But if in mLookupThreshold,we can pull one for sure so we mark must_have_fit to find ptn not in mLookupThreshold.
						must_have_fit = 1;
						if (ptsserver_debuglevel >= 3) {
							pts_pr_info(index, "Record fit in minimum offset case. find_index:%d\n", fit_offset_number);
						}
						// Deal with invalid pts == -1 case,
						// If first node is invalid case, set it as found node
						// If there is other node found, we can only compare offset, and set invalid flag
						if (ptn->pts == -1) {
							pInstance->mOffsetMode = 1;
							find = 1;// Here to set find to avoid retryCount count wrong
							if (ptsserver_debuglevel >= 3) {
								// pts_pr_info(index, "Found invalid pts. in invalid mode\n");
							}
						}
					}
				}

				if (find) {
					// Deal with the case pts rewind and insert the first node in pts_list,
					// and offset is also within correct threshhold.But the key on is at the end of pts_list,
					// and we can`t find within just in MAX_CHECKOUT_RETRY_COUNT,we need retry all list
					if (find_index == 0 && pInstance->mLastCheckoutIndex > 0) {
						retryCount = pInstance->mListSize;
					}
					find_frame_num++;
				}

				if (find_frame_num >= retryCount) {
					break;
				}
			}
			i++;
		}

		if (pInstance->mOffsetMode) {
			if (fit_offset_ptn) {
				find = 1;
				find_ptn = fit_offset_ptn;
				find_index = fit_offset_number;
				shot_bound --;
				if (find_ptn->pts == -1) {
					invalid_mode = 1;
					if (ptsserver_debuglevel >= 3) {
						pts_pr_info(index, "Peek invalid pts case find_index:%d\n", find_index);
					}
				}
			}
		} else {
			// to see if the key one within offset the same as the minimum pts diff one,
			// if not fit, we choose to trust pts fit.
			if (must_have_fit) {
				find = 1;
				if ((fit_offset_ptn && !find_ptn) || !pInstance->mPtsCheckoutStarted) {
					find_ptn = fit_offset_ptn;
					find_index = fit_offset_number;
					shot_bound --;
					if (ptsserver_debuglevel >= 3) {
						pts_pr_info(index, "Peek Might occure discontinue case number:%d, need reset by offset\n", find_index);
					}
				}
			}
		}

		// Deal with all the key ptn
		if (find && find_ptn && !invalid_mode) {
			mCheckoutPtsOffset->pts = find_ptn->pts;
			mCheckoutPtsOffset->pts_64 = find_ptn->pts_64;
			if (ptsserver_debuglevel >= 3) {
				pts_pr_info(index,"Peek ok ListCount:%d find:%d offset(diff:%d L:0x%x C:%x) pts(32:0x%x 64:%lld) l_Checkoutpts:%x, dur_count:%lld, shot_bound:%d\n",
									pInstance->mListSize,find_index,abs(cur_offset - find_ptn->offset),
									find_ptn->offset,cur_offset,find_ptn->pts,find_ptn->pts_64, pInstance->mLastPeekPts, pInstance->mLastCheckoutDurationCount, shot_bound);
			}
		}
	}

	if (!find || invalid_mode) {
		if (pInstance->mDecoderDuration != 0) {
			mCalculateLastCheckoutPts = pInstance->mLastPeekPts + pInstance->mDecoderDuration;
			mCalculateLastCheckoutPts64 = pInstance->mLastPeekPts64 + div_u64(pInstance->mDecoderDuration * 1000,96);
		} else {
			if (pInstance->mFrameDuration != 0) {
				mCalculateLastCheckoutPts = pInstance->mLastPeekPts + pInstance->mFrameDuration;
			}
			if (pInstance->mFrameDuration64 != 0) {
				mCalculateLastCheckoutPts64 = pInstance->mLastPeekPts64 + pInstance->mFrameDuration64;
			}
		}

		mCheckoutPtsOffset->pts = mCalculateLastCheckoutPts;
		mCheckoutPtsOffset->pts_64 = mCalculateLastCheckoutPts64;
	}

	pInstance->mLastPeekPts = mCheckoutPtsOffset->pts;
	pInstance->mLastPeekPts64 = mCheckoutPtsOffset->pts_64;

	mutex_unlock(&vPtsServerIns->mListLock);

	return 0;
}
EXPORT_SYMBOL(ptsserver_peek_pts_offset);

long ptsserver_get_last_checkin_pts(s32 pServerInsId,last_checkin_pts* mLastCheckinPts) {
	PtsServerManage* vPtsServerIns = NULL;
	ptsserver_ins* pInstance = NULL;
	s32 index = pServerInsId;
	if (index < 0 || index >= MAX_INSTANCE_NUM)
		return -1;

	vPtsServerIns = &(vPtsServerInsList[index]);
	mutex_lock(&vPtsServerIns->mListLock);
	pInstance = vPtsServerIns->pInstance;
	if (pInstance == NULL) {
		mutex_unlock(&vPtsServerIns->mListLock);
		return -1;
	}
	mLastCheckinPts->mLastCheckinPts = pInstance->mLastCheckinPts;
	mLastCheckinPts->mLastCheckinPts64 = pInstance->mLastCheckinPts64;
	mutex_unlock(&vPtsServerIns->mListLock);

	return 0;
}

long ptsserver_get_last_checkout_pts(s32 pServerInsId,last_checkout_pts* mLastCheckOutPts) {

	PtsServerManage* vPtsServerIns = NULL;
	ptsserver_ins* pInstance = NULL;
	s32 index = pServerInsId;
	if (index < 0 || index >= MAX_INSTANCE_NUM)
		return -1;

	vPtsServerIns = &(vPtsServerInsList[index]);
	mutex_lock(&vPtsServerIns->mListLock);
	pInstance = vPtsServerIns->pInstance;
	if (pInstance == NULL) {
		mutex_unlock(&vPtsServerIns->mListLock);
		return -1;
	}
	mLastCheckOutPts->mLastCheckoutPts = pInstance->mLastCheckoutPts;
	mLastCheckOutPts->mLastCheckoutPts64 = pInstance->mLastCheckoutPts64;
	mutex_unlock(&vPtsServerIns->mListLock);
	return 0;
}

long ptsserver_ins_release(s32 pServerInsId) {
	PtsServerManage* vPtsServerIns = NULL;
	ptsserver_ins* pInstance = NULL;
	s32 index = get_index_from_ptsserver_id(pServerInsId);
	pts_node *ptn = NULL;
	if (index < 0 || index >= MAX_INSTANCE_NUM)
		return -1;

	pts_pr_info(pServerInsId,"ptsserver_ins_release in \n");
	vPtsServerIns = &(vPtsServerInsList[index]);
	mutex_lock(&vPtsServerIns->mListLock);
	pInstance = vPtsServerIns->pInstance;
	if (pInstance == NULL) {
		mutex_unlock(&vPtsServerIns->mListLock);
		return -1;
	}

	pInstance->mRef--;
	pts_pr_info(index,"mRef:%d",pInstance->mRef);
	if (pInstance->mRef > 0) {
		mutex_unlock(&vPtsServerIns->mListLock);
		ptsserver_ins_reset(pServerInsId);
		return 0;
	}

	pts_pr_info(index,"ptsserver_ins_release ListSize:%d\n",pInstance->mListSize);
	while (!list_empty(&pInstance->pts_free_list)) {
		ptn = list_entry(pInstance->pts_free_list.next,
						struct ptsnode, node);
		if (ptn != NULL) {
			list_del(&ptn->node);
			ptn = NULL;
		}
	}
	while (!list_empty(&pInstance->pts_list)) {
		ptn = list_entry(pInstance->pts_list.next,
						struct ptsnode, node);
		if (ptn != NULL) {
			list_del(&ptn->node);
			ptn = NULL;
		}
	}
	if (pInstance->all_free_ptn) {
		vfree(pInstance->all_free_ptn);
		pInstance->all_free_ptn = NULL;
	}
	memset(pInstance, 0, sizeof(ptsserver_ins));
	kfree(pInstance);
	pInstance = NULL;
	vPtsServerInsList[index].pInstance = NULL;
	mutex_unlock(&vPtsServerIns->mListLock);
	pts_pr_info(pServerInsId,"ptsserver_ins_release ok \n");
	return 0;
}
EXPORT_SYMBOL(ptsserver_ins_release);

long ptsserver_set_trick_mode(s32 pServerInsId, s32 mode) {
	PtsServerManage* vPtsServerIns = NULL;
	ptsserver_ins* pInstance = NULL;
	s32 index = pServerInsId;
	if (index < 0 || index >= MAX_INSTANCE_NUM)
		return -1;
	pts_pr_info(pServerInsId,"ptsserver_set_trick_mode:%d \n", mode);
	vPtsServerIns = &(vPtsServerInsList[index]);
	mutex_lock(&vPtsServerIns->mListLock);
	pInstance = vPtsServerIns->pInstance;
	if (pInstance == NULL) {
		mutex_unlock(&vPtsServerIns->mListLock);
		return -1;
	}
	pInstance->mTrickMode = mode;
	// If set trickmode we set mOffsetMode default
	pInstance->mOffsetMode = mode;
	mutex_unlock(&vPtsServerIns->mListLock);
	return 0;
}

long ptsserver_checkin_apts_size(s32 pServerInsId,checkin_apts_size* mCheckinPtsSize) {
	PtsServerManage* vPtsServerIns = NULL;
	ptsserver_ins* pInstance = NULL;
	pts_node* ptn = NULL;
	pts_node* del_ptn = NULL;
	s32 index = get_index_from_ptsserver_id(pServerInsId);
	s32 level = 0;
	ulong flags = 0;
	if (index < 0 || index >= MAX_INSTANCE_NUM)
		return -1;

	vPtsServerIns = &(vPtsServerInsList[index]);
	// mutex_lock(&vPtsServerIns->mListLock);
	spin_lock_irqsave(&vPtsServerIns->mListSlock, flags);
	pInstance = vPtsServerIns->pInstance;
	if (pInstance == NULL) {
		// mutex_unlock(&vPtsServerIns->mListLock);
		spin_unlock_irqrestore(&vPtsServerIns->mListSlock, flags);
		return -1;
	}

	if (pInstance->mListSize >= pInstance->mMaxCount) {
		del_ptn = list_first_entry(&pInstance->pts_list, pts_node, node);
		list_del(&del_ptn->node);
		list_add_tail(&del_ptn->node, &pInstance->pts_free_list);	//queue empty buffer
		if (ptsserver_debuglevel >= 1) {
		pts_pr_info(index,"Checkin delete node size:%d pts:0x%llx pts_64:%lld\n",
							del_ptn->offset,
							del_ptn->pts_90k,
							del_ptn->pts_64);
		}
		pInstance->mListSize--;
	}

	if (!list_empty(&pInstance->pts_free_list)) {	//dequeue empty buffer
		ptn = list_first_entry(&pInstance->pts_free_list, struct ptsnode, node);
		list_del(&ptn->node);
		if (ptn != NULL) {
			ptn->offset = mCheckinPtsSize->offset;
			ptn->pts_90k = mCheckinPtsSize->pts_90k;
			ptn->pts_64 = mCheckinPtsSize->pts_64;
			if (ptsserver_debuglevel >= 1) {
				pts_pr_info(index,"-->dequeue empty ptn:%px offset:0x%x pts(32:0x%llx 64:%lld)\n",
									ptn,ptn->offset,ptn->pts_90k,
									ptn->pts_64);
			}
		}
	}

	if (ptn == NULL) {
		if (ptsserver_debuglevel >= 1) {
			pts_pr_info(index,"ptn is null return \n");
		}
		// mutex_unlock(&vPtsServerIns->mListLock);
		spin_unlock_irqrestore(&vPtsServerIns->mListSlock, flags);
		return -1;
	}

	list_add_tail(&ptn->node, &pInstance->pts_list);

	pInstance->mListSize++;
	if (!pInstance->mPtsCheckinStarted) {
		pts_pr_info(index,"-->Checkin(First) ListSize:%d CheckinPtsSize(%d)-> offset:0x%x pts(32:0x%llx 64:%lld)\n",
							pInstance->mListSize,
							mCheckinPtsSize->size,
							ptn->offset,ptn->pts_90k,
							ptn->pts_64);
		pInstance->mFirstCheckinOffset = mCheckinPtsSize->offset;
		pInstance->mFirstCheckinSize = mCheckinPtsSize->size;
		pInstance->mFirstCheckinPts90k = mCheckinPtsSize->pts_90k;
		pInstance->mFirstCheckinPts64 = mCheckinPtsSize->pts_64;
		pInstance->mPtsCheckinStarted = 1;
	}

	if (ptsserver_debuglevel >= 1) {
		level = ptn->offset - pInstance->mLastCheckoutCurOffset;
		pts_pr_info(index,"Checkin ListCount:%d LastCheckinOffset:0x%x LastCheckinSize:0x%x\n",
								pInstance->mListSize,
								pInstance->mLastCheckinOffset,
								pInstance->mLastCheckinSize);
		pts_pr_info(index,"Checkin Size(%d) %s:0x%x (%s:%d) pts(32:0x%llx 64:%lld)\n",
							mCheckinPtsSize->size,
							ptn->offset < pInstance->mLastCheckinOffset?"rest offset":"offset",
							ptn->offset,
							level >= 5242880 ? ">5M level": "level",
							level,
							ptn->pts_90k,
							ptn->pts_64);
	}

	if (pInstance->mAlignmentOffset != 0) {
		pInstance->mLastCheckinOffset = pInstance->mAlignmentOffset;
		pInstance->mAlignmentOffset = 0;
	} else {
		pInstance->mLastCheckinOffset = ptn->offset;
	}

	pInstance->mLastCheckinSize = mCheckinPtsSize->size;
	pInstance->mLastCheckinPts90k = mCheckinPtsSize->pts_90k;
	pInstance->mLastCheckinPts64 = mCheckinPtsSize->pts_64;

	// mutex_unlock(&vPtsServerIns->mListLock);
	spin_unlock_irqrestore(&vPtsServerIns->mListSlock, flags);

	return 0;
}
EXPORT_SYMBOL(ptsserver_checkin_apts_size);

long ptsserver_checkout_apts_offset(s32 pServerInsId,checkout_apts_offset* mCheckoutPtsOffset) {
	PtsServerManage* vPtsServerIns = NULL;
	ptsserver_ins* pInstance = NULL;
	s32 index = get_index_from_ptsserver_id(pServerInsId);
	pts_node* ptn = NULL;
	pts_node* find_ptn = NULL;
	pts_node* ptn_tmp = NULL;

	u32 cur_offset = 0xFFFFFFFF & mCheckoutPtsOffset->offset;

	u32 offsetDiff = 1024;
	s32 find_frame_num = 0;
	s32 find = 0;
	s32 offsetAbs = 0;
	u32 FrameDur = 0;
	u64 FrameDur64 = 0;
	s32 number = -1;
	s32 i = 0;
	ulong flags = 0;

	if (index < 0 || index >= MAX_INSTANCE_NUM) {
		return -1;
	}
	vPtsServerIns = &(vPtsServerInsList[index]);
	// mutex_lock(&vPtsServerIns->mListLock);
	spin_lock_irqsave(&vPtsServerIns->mListSlock, flags);
	pInstance = vPtsServerIns->pInstance;

	if (pInstance == NULL) {
		// mutex_unlock(&vPtsServerIns->mListLock);
		spin_unlock_irqrestore(&vPtsServerIns->mListSlock, flags);
		return -1;
	}

	offsetDiff = pInstance->mLookupThreshold;

	if (!pInstance->mPtsCheckoutStarted) {
		pts_pr_info(index,"Checkout(First) ListSize:%d offset:0x%x\n",
							pInstance->mListSize,
							cur_offset);
	}

	if (ptsserver_debuglevel >= 1) {
		pts_pr_info(index,"checkout ListCount:%d offset:%llx cur_offset:0x%x\n",
							pInstance->mListSize,
							mCheckoutPtsOffset->offset,
							cur_offset);
	}

	if (cur_offset != 0xFFFFFFFF &&
		!list_empty(&pInstance->pts_list)) {
		find_frame_num = 0;
		find = 0;
		list_for_each_entry_safe(ptn,ptn_tmp,&pInstance->pts_list, node) {
			if (ptn != NULL) {
				offsetAbs = abs(cur_offset - ptn->offset);
				if (ptsserver_debuglevel > 1) {
					pts_pr_info(index,"Checkout i:%d offset(diff:%d L:0x%x C:0x%x)\n",i,offsetAbs,ptn->offset,cur_offset);
				}
				if (offsetAbs <=  pInstance->mLookupThreshold) {
					if (offsetAbs <= offsetDiff && cur_offset >= ptn->offset) {
						offsetDiff = offsetAbs;
						find = 1;
						number = i;
						find_ptn = ptn;
					}
				} else if (offsetAbs > 251658240) {
					// > 240M (15M*16)
					if (ptsserver_debuglevel >= 1) {
						pts_pr_info(index,"Checkout delete(%d) diff:%d offset:0x%x pts:0x%llx pts_64:%lld\n",
											i,
											offsetAbs,
											ptn->offset,
											ptn->pts_90k,
											ptn->pts_64);
					}
					list_del(&ptn->node);
					list_add_tail(&ptn->node, &pInstance->pts_free_list);
					pInstance->mListSize--;

				} else if (find_frame_num > 5) {
					break;
				}
				if (find) {
					find_frame_num++;
				}
			}
			i++;
		}

		if (find) {
			mCheckoutPtsOffset->pts_90k = find_ptn->pts_90k;
			mCheckoutPtsOffset->pts_64 = find_ptn->pts_64;
			if (ptsserver_debuglevel >= 1 ||
				!pInstance->mPtsCheckoutStarted) {
				pts_pr_info(index,"Checkout ok ListCount:%d find:%d offset(diff:%d L:0x%x) pts(32:0x%llx 64:%lld)\n",
									pInstance->mListSize,number,offsetDiff,
									find_ptn->offset,find_ptn->pts_90k,find_ptn->pts_64);
			}
			list_del(&find_ptn->node);
			list_add_tail(&find_ptn->node, &pInstance->pts_free_list);
			pInstance->mListSize--;
		}
	}
	if (find == 1 && (s64)mCheckoutPtsOffset->pts_64 == -1) {
		find = 0; //invalid pts
	}
	if (!find) {
		pInstance->mPtsCheckoutFailCount++;
		if (ptsserver_debuglevel >= 1 || pInstance->mPtsCheckoutFailCount % 30 == 0) {
			pts_pr_info(index,"Checkout fail mPtsCheckoutFailCount:%d level:%d \n",
				pInstance->mPtsCheckoutFailCount,pInstance->mLastCheckinOffset - cur_offset);
		}

		if (pInstance->mFrameDuration != 0) {
			pInstance->mLastCheckoutPts90k = pInstance->mLastCheckoutPts90k + pInstance->mFrameDuration;
		}
		if (pInstance->mFrameDuration64 != 0) {
			pInstance->mLastCheckoutPts64 = pInstance->mLastCheckoutPts64 + pInstance->mFrameDuration64;
		}
		if (pInstance->mPtsCheckoutStarted == 0) {
			pts_pr_info(index,"first Checkout fail cur_offset:0x%x firstCheckinPts90k:%llx \n",cur_offset, pInstance->mFirstCheckinPts90k);
			if (!cur_offset && pInstance->mFirstCheckinPts90k != 0) {
				pInstance->mLastCheckoutPts64 = pInstance->mFirstCheckinPts64;
				pInstance->mLastCheckoutPts90k = pInstance->mFirstCheckinPts90k;
				pInstance->mPtsCheckoutStarted = 1;
			} else {
				pInstance->mLastCheckoutPts64 = -1;
				pInstance->mLastCheckoutPts90k = -1;
			}
		}
		if (ptsserver_debuglevel >= 1) {
			pts_pr_info(index,"Checkout fail Calculate by FrameDuration(32:%d 64:%lld) pts(32:%llx 64:%lld)\n",
								pInstance->mFrameDuration,
								pInstance->mFrameDuration64,
								pInstance->mLastCheckoutPts90k,
								pInstance->mLastCheckoutPts64);
		}

		mCheckoutPtsOffset->pts_90k = pInstance->mLastCheckoutPts90k;
		mCheckoutPtsOffset->pts_64 = pInstance->mLastCheckoutPts64;
		pInstance->mLastCheckoutCurOffset = cur_offset;
		// mutex_unlock(&vPtsServerIns->mListLock);
		spin_unlock_irqrestore(&vPtsServerIns->mListSlock, flags);
		return 0;
	}

	if (pInstance->mPtsCheckoutStarted) {
		if (pInstance->mFrameDuration == 0 && pInstance->mFrameDuration64 == 0) {

			pInstance->mFrameDuration = div_u64(mCheckoutPtsOffset->pts_90k - pInstance->mLastCheckoutPts90k,
													pInstance->mPtsCheckoutFailCount + 1);

			pInstance->mFrameDuration64 = div_u64(mCheckoutPtsOffset->pts_64 - pInstance->mLastCheckoutPts64,
													pInstance->mPtsCheckoutFailCount + 1);

			if (pInstance->mFrameDuration < 0 ||
				pInstance->mFrameDuration > 9000) {
				pInstance->mFrameDuration = 0;
			}
			if (pInstance->mFrameDuration64 < 0 ||
				pInstance->mFrameDuration64 > 100000) {
				pInstance->mFrameDuration64 = 0;
			}
		} else {

			FrameDur = div_u64(mCheckoutPtsOffset->pts_90k - pInstance->mLastCheckoutPts90k,
								pInstance->mPtsCheckoutFailCount + 1);

			FrameDur64 = div_u64(mCheckoutPtsOffset->pts_64 - pInstance->mLastDoubleCheckoutPts64,
									pInstance->mPtsCheckoutFailCount + 1);

			if (ptsserver_debuglevel > 1) {
				pts_pr_info(index,"checkout FrameDur(32:%d 64:%lld) DoubleCheckFrameDuration(32:%d 64:%lld)\n",
									FrameDur,
									FrameDur64,
									pInstance->mDoubleCheckFrameDuration,
									pInstance->mDoubleCheckFrameDuration64);
				pts_pr_info(index,"checkout LastDoubleCheckoutPts(64:%lld) pts(32:%llx 64:%lld) PtsCheckoutFailCount:%d\n",
									pInstance->mLastDoubleCheckoutPts64,
									mCheckoutPtsOffset->pts_90k,
									mCheckoutPtsOffset->pts_64,
									pInstance->mPtsCheckoutFailCount);
			}

			if ((FrameDur == pInstance->mDoubleCheckFrameDuration) ||
				(FrameDur64 == pInstance->mDoubleCheckFrameDuration64)) {
				pInstance->mDoubleCheckFrameDurationCount ++;
			} else {
				pInstance->mDoubleCheckFrameDuration = FrameDur;
				pInstance->mDoubleCheckFrameDuration64 = FrameDur64;
				pInstance->mDoubleCheckFrameDurationCount = 0;
			}
			if (pInstance->mDoubleCheckFrameDurationCount > pInstance->kDoubleCheckThreshold) {
				if (ptsserver_debuglevel > 1) {
					pts_pr_info(index,"checkout DoubleCheckFrameDurationCount(%d) DoubleCheckFrameDuration(32:%d 64:%lld)\n",
										pInstance->mDoubleCheckFrameDurationCount,
										pInstance->mDoubleCheckFrameDuration,
										pInstance->mDoubleCheckFrameDuration64);
				}
				pInstance->mFrameDuration = pInstance->mDoubleCheckFrameDuration;
				pInstance->mFrameDuration64 = pInstance->mDoubleCheckFrameDuration64;
				pInstance->mDoubleCheckFrameDurationCount = 0;
			}
		}
	} else {
		pInstance->mPtsCheckoutStarted = 1;
	}

	pInstance->mPtsCheckoutFailCount = 0;
	pInstance->mLastCheckoutOffset = mCheckoutPtsOffset->offset;
	pInstance->mLastCheckoutPts64 = mCheckoutPtsOffset->pts_64;
	pInstance->mLastDoubleCheckoutPts64 = mCheckoutPtsOffset->pts_64;
	pInstance->mLastCheckoutPts90k = mCheckoutPtsOffset->pts_90k;
	pInstance->mLastCheckoutCurOffset = cur_offset;
	// mutex_unlock(&vPtsServerIns->mListLock);
	spin_unlock_irqrestore(&vPtsServerIns->mListSlock, flags);
	return 0;
}
EXPORT_SYMBOL(ptsserver_checkout_apts_offset);


long ptsserver_static_ins_binder(s32 pServerInsId, ptsserver_ins** pIns, ptsserver_alloc_para allocParm) {
	ptsserver_ins* pInstance = NULL;
	s32 index = -1;
	s32 temp_ins;
	index = get_index_from_ptsserver_id(pServerInsId);

	if (index >= 0 && index < MAX_INSTANCE_NUM) {
		mutex_lock(&(vPtsServerInsList[index].mListLock));
		pInstance = vPtsServerInsList[index].pInstance;
		if (pInstance == NULL) {
			mutex_unlock(&(vPtsServerInsList[index].mListLock));
			return -1;
		}
		pInstance->mRef++;
		*pIns = pInstance;
		pts_pr_info(index,"ptsserver_static_instance has exist! mRef:%d\n", pInstance->mRef);
		mutex_unlock(&(vPtsServerInsList[index].mListLock));
	} else if (index < 0) {
		pInstance = kzalloc(sizeof(ptsserver_ins), GFP_KERNEL);
		if (pInstance == NULL) {
			pr_info("[%s]%d pInstance == NULL\n", __func__, __LINE__);
			return -1;
		}

		for (temp_ins = MAX_DYNAMIC_INSTANCE_NUM; temp_ins < MAX_INSTANCE_NUM; temp_ins++) {
			mutex_lock(&(vPtsServerInsList[temp_ins].mListLock));
			if (vPtsServerInsList[temp_ins].pInstance == NULL) {
				vPtsServerInsList[temp_ins].pInstance = pInstance;
				pInstance->mPtsServerInsId = pServerInsId;
				// *pServerInsId = temp_ins;
				pInstance->mRef++;
				ptsserver_ins_init_syncinfo(pInstance, &allocParm);
				pr_info("ptsserver_static_ins_binder --> pServerInsId:%d index:%d mRef:%d\n", pServerInsId, temp_ins, pInstance->mRef);
				mutex_unlock(&(vPtsServerInsList[temp_ins].mListLock));
				break;
			}
			mutex_unlock(&(vPtsServerInsList[temp_ins].mListLock));
		}
		*pIns = pInstance;
	}

	return 0;
}
EXPORT_SYMBOL(ptsserver_static_ins_binder);

long ptsserver_get_list_size(s32 pServerInsId, u32* ListSize) {
	PtsServerManage* vPtsServerIns = NULL;
	ptsserver_ins* pInstance = NULL;
	s32 index = get_index_from_ptsserver_id(pServerInsId);

	if (index < 0 || index >= MAX_INSTANCE_NUM) {
		return -1;
	}
	vPtsServerIns = &(vPtsServerInsList[index]);
	mutex_lock(&vPtsServerIns->mListLock);
	pInstance = vPtsServerIns->pInstance;

	if (pInstance == NULL) {
		mutex_unlock(&vPtsServerIns->mListLock);
		return -1;
	}

	*ListSize = pInstance->mListSize;
	mutex_unlock(&vPtsServerIns->mListLock);

	return 0;
}

long ptsserver_ins_reset(s32 pServerInsId) {
	PtsServerManage* vPtsServerIns = NULL;
	ptsserver_ins* pInstance = NULL;
	s32 index = get_index_from_ptsserver_id(pServerInsId);
	pts_node *ptn = NULL;
	ulong flags = 0;
	if (index < 0 || index >= MAX_INSTANCE_NUM)
		return -1;

	vPtsServerIns = &(vPtsServerInsList[index]);
	spin_lock_irqsave(&vPtsServerIns->mListSlock, flags);
	pInstance = vPtsServerIns->pInstance;
	if (pInstance == NULL) {
		spin_unlock_irqrestore(&vPtsServerIns->mListSlock, flags);
		return -1;
	}

	pts_pr_info(index,"ptsserver_ins_reset ListSize:%d\n",pInstance->mListSize);

	while (!list_empty(&pInstance->pts_list)) {
		ptn = list_entry(pInstance->pts_list.next,
						struct ptsnode, node);
		list_del(&ptn->node);
		list_add_tail(&ptn->node, &pInstance->pts_free_list);//queue empty buffer
		pInstance->mListSize--;
	}

	pInstance->mPtsCheckinStarted = 0;
	pInstance->mPtsCheckoutStarted = 0;
	pInstance->mPtsCheckoutFailCount = 0;
	pInstance->mFrameDuration = 0;
	pInstance->mFrameDuration64 = 0;
	pInstance->mFirstCheckinPts = 0;
	pInstance->mFirstCheckinOffset = 0;
	pInstance->mFirstCheckinSize = 0;
	pInstance->mLastCheckinPts = 0;
	pInstance->mLastCheckinOffset = 0;
	pInstance->mLastCheckinSize = 0;
	pInstance->mAlignmentOffset = 0;
	pInstance->mLastCheckoutPts = 0;
	pInstance->mLastCheckoutOffset = 0;
	pInstance->mFirstCheckinPts64 = 0;
	pInstance->mLastCheckinPts64 = 0;
	pInstance->mLastCheckoutPts64 = 0;
	pInstance->mDoubleCheckFrameDuration = 0;
	pInstance->mDoubleCheckFrameDuration64 = 0;
	pInstance->mDoubleCheckFrameDurationCount = 0;
	pInstance->mLastDoubleCheckoutPts = 0;
	pInstance->mLastDoubleCheckoutPts64 = 0;
	pInstance->mDecoderDuration = 0;
	pInstance->mListSize = 0;
	pInstance->mLastCheckoutCurOffset = 0;
	pInstance->mLastCheckinPiecePts = 0;
	pInstance->mLastCheckinPiecePts64 = 0;
	pInstance->mLastCheckinPieceOffset = 0;
	pInstance->mLastCheckinPieceSize = 0;
	pInstance->mLastCheckoutIndex = 0;
	pInstance->mLastCheckinDurationCount = 0;
	pInstance->mLastCheckoutDurationCount = 0;
	pInstance->mLastShotBound = 0;
	pInstance->mTrickMode = 0;
	pInstance->setC2Mode = false;
	pInstance->mOffsetMode = 0;
	pInstance->mStickyWrapFlag = false;
	pInstance->mLastCheckoutPts90k = 0;
	pInstance->mFirstCheckinPts90k = 0;
	pInstance->mLastCheckinPts90k = 0;

	INIT_LIST_HEAD(&pInstance->pts_list);
	INIT_LIST_HEAD(&pInstance->pts_free_list);

	spin_unlock_irqrestore(&vPtsServerIns->mListSlock, flags);
	pts_pr_info(pServerInsId,"ptsserver_ins_reset ok \n");
	return 0;
}
EXPORT_SYMBOL(ptsserver_ins_reset);

module_param(ptsserver_debuglevel, uint, 0664);
MODULE_PARM_DESC(ptsserver_debuglevel, "\n pts server debug level\n");

