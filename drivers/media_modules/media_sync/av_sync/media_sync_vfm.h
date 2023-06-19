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
#ifndef MEDIA_SYNC_VFM_HEAD_HH
#define MEDIA_SYNC_VFM_HEAD_HH

#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/amlogic/media/vfm/vframe.h>
#include <linux/amlogic/media/vfm/vframe_provider.h>
#include <linux/amlogic/media/vfm/vframe_receiver.h>
#include <linux/kfifo.h>
#include <linux/semaphore.h>
//#include "aml_vcodec_vfq.h"



#define RECEIVER_NAME "mediasync"
#define PROVIDER_NAME "mediasync"

#define MEDIASYNC_POOL_SIZE 16
#define MEDIASYNC_VF_NAME_SIZE 32

static LIST_HEAD(mediasync_vf_devlist);

typedef struct mediasync_video_frame {
	struct list_head mediasync_vf_devlist;
	s32 sSyncInsId;
	ulong sync_policy_instance;
	int dev_id;
	bool running;
	int frameStatus;
	u32 outCount;
	s64 getCountSysTimeUs;
	s64 lastVpts;
	s64 getSysTimeUs;
	wait_queue_head_t wq;
	struct semaphore sem;
	struct task_struct *thread;
	struct vframe_s *vf;
	char vf_receiver_name[MEDIASYNC_VF_NAME_SIZE];
	char vf_provider_name[MEDIASYNC_VF_NAME_SIZE];
	struct vframe_provider_s mediasync_vf_prov;
	struct vframe_receiver_s mediasync_vf_recv;
}mediasync_vf_dev;


int mediasync_vf_set_mediasync_id(int dev_id,s32 SyncInsId);
int mediasync_vf_release(void);
int mediasync_vf_init(void);


#endif

