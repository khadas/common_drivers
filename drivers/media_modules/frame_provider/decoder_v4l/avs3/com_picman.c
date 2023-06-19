/* ====================================================================================================================

	The copyright in this software is being made available under the License included below.
	This software may be subject to other third party and contributor rights, including patent rights, and no such
	rights are granted under this license.

	Copyright (c) 2018, HUAWEI TECHNOLOGIES CO., LTD. All rights reserved.
	Copyright (c) 2018, SAMSUNG ELECTRONICS CO., LTD. All rights reserved.
	Copyright (c) 2018, PEKING UNIVERSITY SHENZHEN GRADUATE SCHOOL. All rights reserved.
	Copyright (c) 2018, PENGCHENG LABORATORY. All rights reserved.

	Redistribution and use in source and binary forms, with or without modification, are permitted only for
	the purpose of developing standards within Audio and Video Coding Standard Workgroup of China (AVS) and for testing and
	promoting such standards. The following conditions are required to be met:

	* Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.
	* The name of HUAWEI TECHNOLOGIES CO., LTD. or SAMSUNG ELECTRONICS CO., LTD. may not be used to endorse or promote products derived from
	this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

* ====================================================================================================================
*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/kfifo.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/amlogic/media/vfm/vframe.h>
#include <linux/amlogic/media/utils/amstream.h>
#include <linux/amlogic/media/utils/vformat.h>
#include <linux/amlogic/media/frame_sync/ptsserv.h>
#include <linux/amlogic/media/frame_sync/tsync.h>
#include <linux/amlogic/media/canvas/canvas.h>
#include <linux/amlogic/media/vfm/vframe_provider.h>
#include <linux/amlogic/media/vfm/vframe_receiver.h>
#include <linux/dma-mapping.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
#include <linux/dma-map-ops.h>
#else
#include <linux/dma-contiguous.h>
#endif
#include <linux/slab.h>

//#include "com_def.h"
#include "avs3_global.h"

#ifdef SIMULATION
#ifdef SYSTEM_TEST
#define printf st_printf
#define printk st_printf
#else
#define printf io_printf
#define printk io_printf
#endif
#endif

/* macros for reference picture flag */
#define IS_REF(pic)          (((pic)->is_ref) != 0)
#define SET_REF_UNMARK(pic)  (((pic)->is_ref) = 0)
#define SET_REF_MARK(pic)    (((pic)->is_ref) = 1)

#define PRINT_DPB(pm)\
	printf("%s: current num_ref = %d, dpb_size = %d\n", __FUNCTION__, \
	pm->cur_num_ref_pics, picman_get_num_allocated_pics(pm));

static unsigned long com_picman_lock(COM_PM * pm)
{
	unsigned long flags;
	spin_lock_irqsave(&pm->pm_lock, flags);

	return flags;
}

void com_picman_unlock(COM_PM * pm, unsigned long flags)
{
	spin_unlock_irqrestore(&pm->pm_lock, flags);
}

#if 0 //for v4l
static int picman_get_num_allocated_pics(COM_PM * pm)
{
	int i, cnt = 0;
	for (i = 0; i < pm->max_pb_size; i++) /* this is coding order */
	{
		if (pm->pic[i])
			cnt++;
	}
	return cnt;
}
#endif

static int picman_move_pic(COM_PM *pm, int from, int to)
{
	int i;
	COM_PIC * pic;
	int temp_cur_num_ref_pics = pm->cur_num_ref_pics;
	BOOL found_empty_pos = 0;

	pic = pm->pic[from];

	for (i = from; i < temp_cur_num_ref_pics - 1; i++) {
		pm->pic[i] = pm->pic[i + 1];// move the remaining ref pic to the front
	}
	pm->pic[temp_cur_num_ref_pics - 1] = NULL; // the new end fill with NULL.
	temp_cur_num_ref_pics--;// update, since the ref-pic number decrease
	for (i = to; i > temp_cur_num_ref_pics; i--) {
		if (pm->pic[i] == NULL) {
			pm->pic[i] = pic;// find the first NULL pos from end to front, and fill with the un-ref pic
			found_empty_pos = 1;
			break;
		}
	}

	if (found_empty_pos != 1) {
		pr_info("%s pic %p will be discarded\n", __func__, pic);
	}
	assert(found_empty_pos == 1);
	return 0;
}

static void picman_update_pic_ref(COM_PM * pm)
{
	COM_PIC ** pic;
	COM_PIC ** pic_ref;
	COM_PIC  * pic_t;
	int i, j, cnt;
	ulong flags;

	flags = com_picman_lock(pm);

	pic = pm->pic;
	pic_ref = pm->pic_ref;
	for (i = 0, j = 0; i < pm->max_pb_size; i++) {
		if (pic[i] && IS_REF(pic[i])) {
#ifdef BUFMGR_SANITY_CHECK
			if (j < MAX_NUM_REF_PICS)
				pic_ref[j++] = pic[i];
			else {
				if (avs3_get_debug_flag())
					pr_info("%s ref pic num(%d) is more than the MAX_NUM_REF_PICS\n",
						__func__, j);
			}
#else
			pic_ref[j++] = pic[i];
#endif
		}
	}
	cnt = j;
	while (j < MAX_NUM_REF_PICS)
		pic_ref[j++] = NULL;
	/* descending order sort based on PTR */
	for (i = 0; i < cnt - 1; i++) {
		for (j = i + 1; j < cnt; j++) {
#ifdef BUFMGR_SANITY_CHECK
			if (i >= MAX_NUM_REF_PICS || j >= MAX_NUM_REF_PICS) {
				if (avs3_get_debug_flag())
					pr_info("%s pm pic num(%d,%d) is more than the MAX_NUM_REF_PICS\n",
						__func__, i, j);
				return;
			}
#endif
			if (pic_ref[i]->ptr < pic_ref[j]->ptr) {
				pic_t = pic_ref[i];
				pic_ref[i] = pic_ref[j];
				pic_ref[j] = pic_t;
			}
		}
	}
	com_picman_unlock(pm, flags);
}

#if 0 //for v4l
static COM_PIC * picman_remove_pic_from_pb(COM_PM * pm, int pos)
{
	int i;
	COM_PIC * pic_rem;
	ulong flags;

	flags = com_picman_lock(pm);

	pic_rem = pm->pic[pos];
	printf("%s: pic %p pos %d\n", __func__, pic_rem, pos);
	pm->pic[pos] = NULL;
	/* fill empty pic buffer */
	for (i = pos; i < pm->max_pb_size - 1; i++) {
		pm->pic[i] = pm->pic[i + 1];
	}
	pm->pic[pm->max_pb_size - 1] = NULL;
	pm->cur_pb_size--;
	com_picman_unlock(pm, flags);
	return pic_rem;
}
#endif

static void picman_set_pic_to_pb(COM_PM * pm, COM_PIC * pic,
	COM_REFP(*refp)[REFP_NUM], int pos)
{
	int i;
	printf("%s: pic %p pos %d\n", __func__, pic, pos);
	for (i = 0; i < pm->num_refp[REFP_0]; i++) {
#if ETMVP || SUB_TMVP || AWP
		pic->list_ptr[REFP_0][i] = refp[i][REFP_0].ptr;
#else
		pic->list_ptr[i] = refp[i][REFP_0].ptr;
#endif
	}
#if ETMVP || SUB_TMVP || AWP
	for (i = 0; i < pm->num_refp[REFP_1]; i++) {
		pic->list_ptr[REFP_1][i] = refp[i][REFP_1].ptr;
	}
#endif
	if (pos >= 0) {
		com_assert(pm->pic[pos] == NULL);
		if (pm->pic[pos] != NULL) {
			COM_PIC * pos_pic;
			pos_pic = pm->pic[pos];

			/* search empty pic buffer position */
			for (i = (pm->max_pb_size - 1); i >= 0; i--)
			{
				if (pm->pic[i] == NULL)
				{
					pm->pic[i] = pos_pic;
					break;
				}
			}

			if (i < 0)
			{
				printf("pos pic will be discarded\n");
			}
		}
		pm->pic[pos] = pic;
	} else /* pos < 0 */
	{
		/* search empty pic buffer position */
		for (i = (pm->max_pb_size - 1); i >= 0; i--) {
			if (pm->pic[i] == NULL) {
				pm->pic[i] = pic;
				break;
			}
		}
		if (i < 0) {
			printf("i=%d\n", i);
			com_assert(i >= 0);
		}
	}
	pm->cur_pb_size++;
}

#if 0 // for v4l
static int picman_get_empty_pic_from_list(COM_PM * pm)
{
	//COM_IMGB * imgb;
	COM_PIC  * pic;
	int i;
	for (i = 0; i < pm->max_pb_size; i++) {
		pic = pm->pic[i];
		if (pic != NULL && !IS_REF(pic) && pic->need_for_out == 0
#ifdef AML
		&& pic->buf_cfg.vf_ref == 0
		&& pic->buf_cfg.backend_ref == 0
		&& pic->buf_cfg.in_dpb == 0
#endif
		) {
#ifdef ORI_CODE
			imgb = pic->imgb;
			com_assert(imgb != NULL);
			/* check reference count */
			if (1 == imgb->getref(imgb)) {
				return i; /* this is empty buffer */
			}
#else
			printf("%s: pm index %d pic index %d\n",
				__func__, i, pic->buf_cfg.index);
			return i;
#endif
		}
	}
	return -1;
}
#endif

void set_refp(COM_REFP * refp, COM_PIC  * pic_ref)
{
	refp->pic      = pic_ref;
	refp->ptr      = pic_ref->ptr;
	refp->map_mv   = pic_ref->map_mv;
	refp->map_refi = pic_ref->map_refi;
	refp->list_ptr = pic_ref->list_ptr;
#if LIBVC_ON
	refp->is_library_picture = 0;
#endif
}

void copy_refp(COM_REFP * refp_dst, COM_REFP * refp_src)
{
	refp_dst->pic      = refp_src->pic;
	refp_dst->ptr      = refp_src->ptr;
	refp_dst->map_mv   = refp_src->map_mv;
	refp_dst->map_refi = refp_src->map_refi;
	refp_dst->list_ptr = refp_src->list_ptr;
#if LIBVC_ON
	refp_dst->is_library_picture = refp_src->is_library_picture;
#endif
}

int check_copy_refp(COM_REFP(*refp)[REFP_NUM], int cnt, int lidx, COM_REFP  * refp_src)
{
	int i;
	for (i = 0; i < cnt; i++) {
#if LIBVC_ON
		if (refp[i][lidx].ptr == refp_src->ptr && refp[i][lidx].is_library_picture == refp_src->is_library_picture)
#else
		if (refp[i][lidx].ptr == refp_src->ptr)
#endif
		{
			return -1;
		}
	}
	copy_refp(&refp[cnt][lidx], refp_src);
	return COM_OK;
}

#if HLS_RPL   //This is implementation of reference picture list construction based on RPL. This is meant to replace function int com_picman_refp_init(COM_PM *pm, int num_ref_pics_act, int slice_type, u32 ptr, u8 layer_id, int last_intra, COM_REFP(*refp)[REFP_NUM])
int com_picman_refp_rpl_based_init(COM_PM *pm, COM_PIC_HEADER *pic_header, COM_REFP(*refp)[REFP_NUM])
{
	//MX: IDR?
	int i;
	if ((pic_header->slice_type == SLICE_I) && (pic_header->poc == 0))
	{
		pm->num_refp[REFP_0] = pm->num_refp[REFP_1] = 0;
		return COM_OK;
	}

	picman_update_pic_ref(pm);
#if LIBVC_ON
	if (!pm->libvc_data->library_picture_enable_flag && pic_header->slice_type != SLICE_I)
#endif
	{
		com_assert_rv(pm->cur_num_ref_pics > 0, COM_ERR_UNEXPECTED);
	}

	for (i = 0; i < MAX_NUM_REF_PICS; i++) {
		refp[i][REFP_0].pic = refp[i][REFP_1].pic = NULL;
	}

	pm->num_refp[REFP_0] = pm->num_refp[REFP_1] = 0;

	//Do the L0 first
	for (i = 0; i < pic_header->rpl_l0.ref_pic_active_num; i++) {
#if LIBVC_ON
		if (pm->libvc_data->library_picture_enable_flag && pic_header->rpl_l0.library_index_flag[i]) {
			int ref_lib_index = pic_header->rpl_l0.ref_pics[i];
			com_assert_rv(ref_lib_index == pm->pb_libpic_library_index, COM_ERR_UNEXPECTED);

			set_refp(&refp[i][REFP_0], pm->pb_libpic);
			refp[i][REFP_0].ptr = pic_header->poc - 1;
			refp[i][REFP_0].is_library_picture = 1;
			pm->num_refp[REFP_0] = pm->num_refp[REFP_0] + 1;
		}
		else
#endif
		{
			int refPicPoc = pic_header->poc - pic_header->rpl_l0.ref_pics[i];
			//Find the ref pic in the DPB
			int j = 0;
			int diff;
			while (j < pm->cur_num_ref_pics && pm->pic_ref[j]->ptr != refPicPoc)
				j++;

			//If the ref pic is found, set it to RPL0
			if (j < pm->cur_num_ref_pics && pm->pic_ref[j]->ptr == refPicPoc) {

				set_refp(&refp[i][REFP_0], pm->pic_ref[j]);
				pm->num_refp[REFP_0] = pm->num_refp[REFP_0] + 1;
				//make sure delta doi of RPL0 correct,in case of last incomplete GOP
				diff = (pic_header->decode_order_index%DOI_CYCLE_LENGTH - pm->pic_ref[j]->dtr);//MX: doi is not increase mono, but in the range of [0,255]; the diff may be minus value.
				if (diff != pic_header->rpl_l0.ref_pics_ddoi[i]) {
					pic_header->ref_pic_list_sps_flag[0] = 0;
					pic_header->rpl_l0.ref_pics_ddoi[i] = diff;
				}
			} else {
				printf("%s: The L0 Reference Picture(%d) is not find in dpb",
					__func__, refPicPoc);
				return COM_ERR;   //The refence picture must be available in the DPB, if not found then there is problem
			}
		}
	}

	//update inactive ref ddoi in L0,in case of last incomplete GOP
	for (i = 0; i < MAX_NUM_REF_PICS; i++) {
#if LIBVC_ON
		if (!pic_header->rpl_l0.library_index_flag[i] && pic_header->rpl_l0.ref_pics[i] > 0)
#else
		if (pic_header->rpl_l0.ref_pics[i] > 0)
#endif
		{
			int refPicPoc = pic_header->poc - pic_header->rpl_l0.ref_pics[i];
			//Find the ref pic in the DPB
			int j = 0;
			while (j < pm->cur_num_ref_pics && pm->pic_ref[j]->ptr != refPicPoc)
				j++;

			//If the ref pic is found, set it to RPL0
			if (j < pm->cur_num_ref_pics && pm->pic_ref[j]->ptr == refPicPoc) {
				int diff = (pic_header->decode_order_index % DOI_CYCLE_LENGTH - pm->pic_ref[j]->dtr);//MX: doi is not increase mono, but in the range of [0,255]; the diff may be minus value.
				if (diff != pic_header->rpl_l0.ref_pics_ddoi[i]) {
					pic_header->ref_pic_list_sps_flag[0] = 0;
					pic_header->rpl_l0.ref_pics_ddoi[i] = diff;
				}
			}
		}
	}
	if (pic_header->slice_type == SLICE_P) return COM_OK;

	//Do the L1 first
	for (i = 0; i < pic_header->rpl_l1.ref_pic_active_num; i++) {
#if LIBVC_ON
		if (pm->libvc_data->library_picture_enable_flag && pic_header->rpl_l1.library_index_flag[i]) {
			int ref_lib_index = pic_header->rpl_l1.ref_pics[i];
			com_assert_rv(ref_lib_index == pm->pb_libpic_library_index, COM_ERR_UNEXPECTED);

			set_refp(&refp[i][REFP_1], pm->pb_libpic);
			refp[i][REFP_1].ptr = pic_header->poc - 1;
			refp[i][REFP_1].is_library_picture = 1;
			pm->num_refp[REFP_1] = pm->num_refp[REFP_1] + 1;
		} else
#endif
		{
			int refPicPoc = pic_header->poc - pic_header->rpl_l1.ref_pics[i];
			//Find the ref pic in the DPB
			int j = 0;
			int diff;
			while (j < pm->cur_num_ref_pics && pm->pic_ref[j]->ptr != refPicPoc)
				j++;

			//If the ref pic is found, set it to RPL1
			if (j < pm->cur_num_ref_pics && pm->pic_ref[j]->ptr == refPicPoc) {
				set_refp(&refp[i][REFP_1], pm->pic_ref[j]);
				pm->num_refp[REFP_1] = pm->num_refp[REFP_1] + 1;
				//make sure delta doi of RPL0 correct
				diff = (pic_header->decode_order_index%DOI_CYCLE_LENGTH - pm->pic_ref[j]->dtr);//MX: doi is not increase mono, but in the range of [0,255]; the diff may be minus value.
				if (diff != pic_header->rpl_l1.ref_pics_ddoi[i]) {
					pic_header->ref_pic_list_sps_flag[1] = 0;
					pic_header->rpl_l1.ref_pics_ddoi[i] = diff;
				}
			} else {
				printf("%s: The L1 Reference Picture(%d) is not find in dpb",
					__func__, refPicPoc);
				return COM_ERR;   //The refence picture must be available in the DPB, if not found then there is problem
			}
		}
	}
	//update inactive ref ddoi in L1
	for (i = 0; i < MAX_NUM_REF_PICS; i++) {
#if LIBVC_ON
		if (!pic_header->rpl_l1.library_index_flag[i] && pic_header->rpl_l1.ref_pics[i] > 0)
#else
		if (pic_header->rpl_l1.ref_pics[i] > 0)
#endif
		{
			int refPicPoc = pic_header->poc - pic_header->rpl_l1.ref_pics[i];
			//Find the ref pic in the DPB
			int j = 0;
			while (j < pm->cur_num_ref_pics && pm->pic_ref[j]->ptr != refPicPoc)
				j++;

			//If the ref pic is found, set it to RPL0
			if (j < pm->cur_num_ref_pics && pm->pic_ref[j]->ptr == refPicPoc) {
				int diff = (pic_header->decode_order_index % DOI_CYCLE_LENGTH - pm->pic_ref[j]->dtr);//MX: doi is not increase mono, but in the range of [0,255]; the diff may be minus value.
				if (diff != pic_header->rpl_l1.ref_pics_ddoi[i]) {
					pic_header->ref_pic_list_sps_flag[1] = 0;
					pic_header->rpl_l1.ref_pics_ddoi[i] = diff;
				}
			}
		}
	}
	return COM_OK;  //RPL construction completed
}

int com_cleanup_useless_pic_buffer_in_pm(COM_PM *pm)
{
	//remove the pic if it is a unref pic, and has been output.
	int i;
	printf("%s\n", __func__);
	for (i = 0; i < pm->max_pb_size; i++) {
		if (pm->pic[i] != NULL)
			printf("pm pic %p index %d\n", pm->pic[i], i);
		if ((pm->pic[i] != NULL) && (pm->pic[i]->need_for_out == 0)
		&& (pm->pic[i]->is_ref == 0)
#ifdef AML
		&& (pm->pic[i]->buf_cfg.backend_ref == 0)
		&& (pm->pic[i]->buf_cfg.vf_ref == 0)
#endif
		) {
			com_pic_free(pm->hw, &pm->pa, pm->pic[i]);
			pm->cur_pb_size--;
			pm->pic[i] = NULL;
		}
	}
	return COM_OK;
}

int com_picman_dpbpic_doi_minus_cycle_length(COM_PM *pm)
{
	COM_PIC * pic;
	int i;
	for (i = 0; i < pm->max_pb_size; i++) {
		pic = pm->pic[i];
		if (pic != NULL)//MX: no matter whether is ref or unref(for output), need to minus 256.
		{
			pic->dtr = pic->dtr - DOI_CYCLE_LENGTH;
			assert(pic->dtr >= (-256));//MX:minus once at most.
		}
	}
	return COM_OK;
}

//This is implementation of reference picture list construction based on RPL for decoder
//in decoder, use DOI as pic reference instead of POC
int com_picman_refp_rpl_based_init_decoder(COM_PM *pm, COM_PIC_HEADER *pic_header, COM_REFP(*refp)[REFP_NUM])
{
	int i;
	picman_update_pic_ref(pm);
#ifdef BUFMGR_SANITY_CHECK
	for (i = 0; i < pm->cur_num_ref_pics; i++) {
		if (pm->pic_ref[i] == NULL) {
			if (avs3_get_debug_flag())
				pr_info("%s error, pm->cur_num_ref_pics = %d,  pm->pic_ref[%d] is NULL\n",
					__func__, pm->cur_num_ref_pics, i);
			return COM_ERR;
		}
	}
#endif
#if LIBVC_ON
	if (!pm->libvc_data->library_picture_enable_flag && pic_header->slice_type != SLICE_I)
#endif
	{
		com_assert_rv(pm->cur_num_ref_pics > 0, COM_ERR_UNEXPECTED);
	}

	for (i = 0; i < MAX_NUM_REF_PICS; i++)
		refp[i][REFP_0].pic = refp[i][REFP_1].pic = NULL;
	pm->num_refp[REFP_0] = pm->num_refp[REFP_1] = 0;

	//Do the L0 first
	for (i = 0; i < pic_header->rpl_l0.ref_pic_active_num; i++) {
#if LIBVC_ON
		if (pm->libvc_data->library_picture_enable_flag && pic_header->rpl_l0.library_index_flag[i]) {
			int ref_lib_index;
			ref_lib_index = pic_header->rpl_l0.ref_pics_ddoi[i];
			com_assert_rv(ref_lib_index == pm->pb_libpic_library_index, COM_ERR_UNEXPECTED);
#ifdef BUFMGR_SANITY_CHECK
			if (pm->pb_libpic == NULL) {
				if (avs3_get_debug_flag())
					pr_info("%s error, pm->pb_libpic is NULL\n", __func__);
				return COM_ERR;
			}
#endif
			set_refp(&refp[i][REFP_0], pm->pb_libpic);
			refp[i][REFP_0].ptr = pic_header->poc - 1;
			refp[i][REFP_0].is_library_picture = 1;
			pm->num_refp[REFP_0] = pm->num_refp[REFP_0] + 1;
		} else
#endif
		{
			int refPicDoi = pic_header->rpl_l0.ref_pics_doi[i];//MX: no need to fix the value in the range of 0~255. because DOI in the DPB can be a minus value, after minus 256.
			//Find the ref pic in the DPB
			int j = 0;
			ulong flags;

			flags = com_picman_lock(pm);
			while (j < pm->cur_num_ref_pics && pm->pic_ref[j] && pm->pic_ref[j]->dtr != refPicDoi)
				j++;

			//If the ref pic is found, set it to RPL0
			if (j < pm->cur_num_ref_pics && pm->pic_ref[j] && pm->pic_ref[j]->dtr == refPicDoi) {
				set_refp(&refp[i][REFP_0], pm->pic_ref[j]);
				pm->num_refp[REFP_0] = pm->num_refp[REFP_0] + 1;
			} else {
				com_picman_unlock(pm, flags);
				printf("%s: The L0 Reference Picture(%d) is not find in dpb",
					__func__, refPicDoi);
				return COM_ERR;   //The refence picture must be available in the DPB, if not found then there is problem
			}

			com_picman_unlock(pm, flags);
		}
	}

	if (pic_header->slice_type == SLICE_P)
		return COM_OK;

	//Do the L1 first
	for (i = 0; i < pic_header->rpl_l1.ref_pic_active_num; i++) {
#if LIBVC_ON
		if (pm->libvc_data->library_picture_enable_flag && pic_header->rpl_l1.library_index_flag[i]) {
			int ref_lib_index = pic_header->rpl_l1.ref_pics_ddoi[i];
			com_assert_rv(ref_lib_index == pm->pb_libpic_library_index, COM_ERR_UNEXPECTED);
#ifdef BUFMGR_SANITY_CHECK
			if (pm->pb_libpic == NULL) {
				if (avs3_get_debug_flag())
					pr_info("%s error, pm->pb_libpic is NULL\n", __func__);
				return COM_ERR;
			}
#endif
			set_refp(&refp[i][REFP_1], pm->pb_libpic);
			refp[i][REFP_1].ptr = pic_header->poc - 1;
			refp[i][REFP_1].is_library_picture = 1;
			pm->num_refp[REFP_1] = pm->num_refp[REFP_1] + 1;
		} else
#endif
		{
			int refPicDoi = pic_header->rpl_l1.ref_pics_doi[i];//MX: no need to fix the value in the range of 0~255. because DOI in the DPB can be a minus value, after minus 256.
			//Find the ref pic in the DPB
			int j = 0;
#ifdef BUFMGR_SANITY_CHECK
			while (j < pm->cur_num_ref_pics && pm->pic_ref[j] && pm->pic_ref[j]->dtr != refPicDoi)
#else
			while (j < pm->cur_num_ref_pics && pm->pic_ref[j]->dtr != refPicDoi)
#endif
			{
				j++;
			}

			//If the ref pic is found, set it to RPL1
#ifdef BUFMGR_SANITY_CHECK
			if (j < pm->cur_num_ref_pics && pm->pic_ref[j] && pm->pic_ref[j]->dtr == refPicDoi)
#else
			if (j < pm->cur_num_ref_pics && pm->pic_ref[j]->dtr == refPicDoi)
#endif
			{
				set_refp(&refp[i][REFP_1], pm->pic_ref[j]);
				pm->num_refp[REFP_1] = pm->num_refp[REFP_1] + 1;
			} else {
				printf("%s: The L1 Reference Picture(%d) is not find in dpb",
					__func__, refPicDoi);
				return COM_ERR;   //The refence picture must be available in the DPB, if not found then there is problem
			}
		}
	}

	return COM_OK;  //RPL construction completed
}
#endif

COM_PIC * com_picman_get_empty_pic(COM_PM * pm, int * err)
{
	int ret;
	COM_PIC * pic = NULL;
#if 0 //for v4l
#if LIBVC_ON
	if (pm->libvc_data->is_libpic_processing) {
		/* no need to find empty picture buffer in list */
		ret = -1;
	} else
#endif
	{
		/* try to find empty picture buffer in list */
		ret = picman_get_empty_pic_from_list(pm);
	}
	if (ret >= 0) {
		pic = picman_remove_pic_from_pb(pm, ret);
		goto END;
	}
	/* else if available, allocate picture buffer */
	pm->cur_pb_size = picman_get_num_allocated_pics(pm);
	printf("cur_pb_size %d, cur_libpb_size %d, max_pb_size %d\n",
	pm->cur_pb_size, pm->cur_libpb_size, pm->max_pb_size);
#if LIBVC_ON
	if (pm->cur_pb_size + pm->cur_libpb_size < pm->max_pb_size)
#else
	if (pm->cur_pb_size < pm->max_pb_size)
#endif
	{
		/* create picture buffer */
		pic = com_pic_alloc(pm->hw, &pm->pa, &ret);
		com_assert_gv(pic != NULL, ret, COM_ERR_OUT_OF_MEMORY, ERR);
		goto END;
	}
	com_assert_gv(0, ret, COM_ERR_UNKNOWN, ERR);
END:
	pm->pic_lease = pic;
	if (err) *err = COM_OK;
	return pic;
#else
	pic = com_pic_alloc(pm->hw, &pm->pa, &ret);
	com_assert_gv(pic != NULL, ret, COM_ERR_OUT_OF_MEMORY, ERR);
	pm->pic_lease = pic;
	if (err) *err = COM_OK;
	return pic;
#endif
ERR:
	if (err) *err = ret;
	if (pic) com_pic_free(pm->hw, &pm->pa, pic);
	return NULL;
}
#if HLS_RPL
/*This is the implementation of reference picture marking based on RPL*/
int com_picman_refpic_marking(COM_PM *pm, COM_PIC_HEADER *pic_header)
{
	int i;
	COM_PIC * pic;
	int count_library_picture = 0;
	int referenced_library_picture_index = -1;
	int numberOfPicsToCheck;

	picman_update_pic_ref(pm);
#if LIBVC_ON
	if (!pic_header->rpl_l0.reference_to_library_enable_flag && !pic_header->rpl_l1.reference_to_library_enable_flag && pic_header->slice_type != SLICE_I && pic_header->poc != 0)
#else
	if (pic_header->slice_type != SLICE_I && pic_header->poc != 0)
#endif
		com_assert_rv(pm->cur_num_ref_pics > 0, COM_ERR_UNEXPECTED);

	numberOfPicsToCheck = pm->cur_num_ref_pics;
	for (i = 0; i < numberOfPicsToCheck; i++) {
		pic = pm->pic[i];
		if (pm->pic[i] && IS_REF(pm->pic[i])) {
			//If the pic in the DPB is a reference picture, check if this pic is included in RPL0
			int isIncludedInRPL = 0;
			int j = 0;
			while (!isIncludedInRPL && j < pic_header->rpl_l0.ref_pic_num) {
#if LIBVC_ON
				if ((pic->ptr == (pic_header->poc - pic_header->rpl_l0.ref_pics[j])) && !pic_header->rpl_l0.library_index_flag[j])  //NOTE: we need to put POC also in COM_PIC
#else
				if (pic->ptr == (pic_header->poc - pic_header->rpl_l0.ref_pics[j]))  //NOTE: we need to put POC also in COM_PIC
#endif
				{
					isIncludedInRPL = 1;
				}
				j++;
			}
			//Check if the pic is included in RPL1. This while loop will be executed only if the ref pic is not included in RPL0
			j = 0;
			while (!isIncludedInRPL && j < pic_header->rpl_l1.ref_pic_num) {
#if LIBVC_ON
				if ((pic->ptr == (pic_header->poc - pic_header->rpl_l1.ref_pics[j])) && !pic_header->rpl_l1.library_index_flag[j])  //NOTE: we need to put POC also in COM_PIC
#else
				if (pic->ptr == (pic_header->poc - pic_header->rpl_l1.ref_pics[j]))
#endif
				{
					isIncludedInRPL = 1;
				}
				j++;
			}
			//If the ref pic is not included in either RPL0 nor RPL1, then mark it as not used for reference. move it to the end of DPB.
			if (!isIncludedInRPL) {
				SET_REF_UNMARK(pic);
				picman_move_pic(pm, i, pm->max_pb_size - 1);
				pm->cur_num_ref_pics--;
				i--;                                           //We need to decrement i here because it will be increment by i++ at for loop. We want to keep the same i here because after the move, the current ref pic at i position is the i+1 position which we still need to check.
				numberOfPicsToCheck--;                         //We also need to decrement this variable to avoid checking the moved ref picture twice.
			}
		}
	}
#if LIBVC_ON
	// count the libpic in rpl
	for (i = 0; i < pic_header->rpl_l0.ref_pic_num; i++) {
		if (pic_header->rpl_l0.library_index_flag[i]) {
			if (count_library_picture == 0) {
				referenced_library_picture_index = pic_header->rpl_l0.ref_pics[i];
				count_library_picture++;
			}

			com_assert_rv(referenced_library_picture_index == pic_header->rpl_l0.ref_pics[i], COM_ERR_UNEXPECTED);
		}
	}
	for (i = 0; i < pic_header->rpl_l1.ref_pic_num; i++) {
		if (pic_header->rpl_l1.library_index_flag[i]) {
			if (count_library_picture == 0) {
				referenced_library_picture_index = pic_header->rpl_l1.ref_pics[i];
				count_library_picture++;
			}

			com_assert_rv(referenced_library_picture_index == pic_header->rpl_l1.ref_pics[i], COM_ERR_UNEXPECTED);
		}
	}

	if (count_library_picture > 0) {
		// move out lib pic
		if (!pm->is_library_buffer_empty && (referenced_library_picture_index != pm->pb_libpic_library_index)) {
			//com_picbuf_free(pm->libvc_data->pb_libpic);
			pm->pb_libpic = NULL;
			pm->cur_libpb_size--;
			pm->pb_libpic_library_index = -1;
			pm->is_library_buffer_empty = 1;
		}
		// move in lib pic from the buffer outside the decoder
		if (pm->is_library_buffer_empty) {
			//send out referenced_library_picture_index
			int  libpic_idx = -1;
			int ii;
			for (ii = 0; ii < pm->libvc_data->num_libpic_outside; ii++) {
				if (referenced_library_picture_index == pm->libvc_data->list_library_index_outside[ii]) {
					libpic_idx = ii;
					break;
				}
			}
			com_assert_rv(libpic_idx >= 0, COM_ERR_UNEXPECTED);

			// move in lib pic from the buffer outside the decoder
			pm->pb_libpic = pm->libvc_data->list_libpic_outside[libpic_idx];
			pm->pb_libpic_library_index = referenced_library_picture_index;
			pm->cur_libpb_size++;
			pm->is_library_buffer_empty = 0;
			}
		}
#endif
	return COM_OK;
}

int com_construct_ref_list_doi(COM_PIC_HEADER *pic_header)
{
	int i;

	if (pic_header == NULL)
		return COM_ERR;

	if ((pic_header->rpl_l0.ref_pic_num > MAX_NUM_REF_PICS) ||
		(pic_header->rpl_l1.ref_pic_num > MAX_NUM_REF_PICS))
		return COM_ERR;

	for (i = 0; i < pic_header->rpl_l0.ref_pic_num; i++) {
		pic_header->rpl_l0.ref_pics_doi[i] = pic_header->decode_order_index % DOI_CYCLE_LENGTH - pic_header->rpl_l0.ref_pics_ddoi[i];
		if (is_avs3_print_bufmgr_detail()) {
		printf("rpl_l0.ref_pics_doi[%d]=%d, pic_header->rpl_l0.ref_pics_ddoi[%d]=%d\n",
			i, pic_header->rpl_l0.ref_pics_doi[i], i, pic_header->rpl_l0.ref_pics_ddoi[i]);
		}
	}
	for (i = 0; i < pic_header->rpl_l1.ref_pic_num; i++)
	{
		pic_header->rpl_l1.ref_pics_doi[i] = pic_header->decode_order_index%DOI_CYCLE_LENGTH - pic_header->rpl_l1.ref_pics_ddoi[i];
		if (is_avs3_print_bufmgr_detail()) {
			printf("rpl_l1.ref_pics_doi[%d]=%d, pic_header->rpl_l1.ref_pics_ddoi[%d]=%d\n",
				i, pic_header->rpl_l1.ref_pics_doi[i], i, pic_header->rpl_l1.ref_pics_ddoi[i]);
		}
	}
	return COM_OK;
}

/*This is the implementation of reference picture marking based on RPL for decoder */
/*In decoder side, use DOI as pic reference instead of POI */
int com_picman_refpic_marking_decoder(COM_PM *pm, COM_PIC_HEADER *pic_header)
{
	int i;
	COM_PIC * pic;
	int numberOfPicsToCheck;
	int count_library_picture = 0;
	int referenced_library_picture_index = -1;

	picman_update_pic_ref(pm);
#if LIBVC_ON
	if (!pic_header->rpl_l0.reference_to_library_enable_flag && !pic_header->rpl_l1.reference_to_library_enable_flag && pic_header->slice_type != SLICE_I && pic_header->poc != 0)
#else
	if (pic_header->slice_type != SLICE_I && pic_header->poc != 0)
#endif
		com_assert_rv(pm->cur_num_ref_pics > 0, COM_ERR_UNEXPECTED);

	numberOfPicsToCheck = pm->cur_num_ref_pics;
	for (i = 0; i < numberOfPicsToCheck; i++) {
		pic = pm->pic[i];
		if (pm->pic[i] && IS_REF(pm->pic[i])) {
			//If the pic in the DPB is a reference picture, check if this pic is included in RPL0
			int isIncludedInRPL = 0;
			int j = 0;
			while (!isIncludedInRPL && j < pic_header->rpl_l0.ref_pic_num) {
#if LIBVC_ON
				if (!pic_header->rpl_l0.library_index_flag[j] && pic->dtr == pic_header->rpl_l0.ref_pics_doi[j])
#else
				if (pic->dtr == ((pic_header->decode_order_index - pic_header->rpl_l0.ref_pics_ddoi[j] + DOI_CYCLE_LENGTH) % DOI_CYCLE_LENGTH))  //NOTE: we need to put POC also in COM_PIC
#endif
				{
					isIncludedInRPL = 1;
				}
				j++;
			}
			//Check if the pic is included in RPL1. This while loop will be executed only if the ref pic is not included in RPL0
			j = 0;
			while (!isIncludedInRPL && j < pic_header->rpl_l1.ref_pic_num) {
#if LIBVC_ON
				if (!pic_header->rpl_l1.library_index_flag[j] && pic->dtr == pic_header->rpl_l1.ref_pics_doi[j])
#else
				if (pic->dtr == ((pic_header->decode_order_index - pic_header->rpl_l1.ref_pics_ddoi[j] + DOI_CYCLE_LENGTH) % DOI_CYCLE_LENGTH))
#endif
				{
					isIncludedInRPL = 1;
				}
				j++;
			}
			//If the ref pic is not included in either RPL0 nor RPL1, then mark it as not used for reference. move it to the end of DPB.
			if (!isIncludedInRPL) {
				SET_REF_UNMARK(pic);
				picman_move_pic(pm, i, pm->max_pb_size - 1);
				pm->cur_num_ref_pics--;
				i--;                                           //We need to decrement i here because it will be increment by i++ at for loop. We want to keep the same i here because after the move, the current ref pic at i position is the i+1 position which we still need to check.
				numberOfPicsToCheck--;                         //We also need to decrement this variable to avoid checking the moved ref picture twice.
			}
		}
	}
#if LIBVC_ON
	// count the libpic in rpl
	for (i = 0; i < pic_header->rpl_l0.ref_pic_num; i++) {
		if (pic_header->rpl_l0.library_index_flag[i]) {
			if (count_library_picture == 0) {
				referenced_library_picture_index = pic_header->rpl_l0.ref_pics_ddoi[i];
				count_library_picture++;
			}
			com_assert_rv(referenced_library_picture_index == pic_header->rpl_l0.ref_pics_ddoi[i], COM_ERR_UNEXPECTED);
		}
	}
	for (i = 0; i < pic_header->rpl_l1.ref_pic_num; i++) {
		if (pic_header->rpl_l1.library_index_flag[i]) {
			if (count_library_picture == 0) {
				referenced_library_picture_index = pic_header->rpl_l1.ref_pics_ddoi[i];
				count_library_picture++;
			}
			com_assert_rv(referenced_library_picture_index == pic_header->rpl_l1.ref_pics_ddoi[i], COM_ERR_UNEXPECTED);
		}
	}

	if (count_library_picture > 0) {
		// move out lib pic
		if (!pm->is_library_buffer_empty && (referenced_library_picture_index != pm->pb_libpic_library_index)) {
			//com_picbuf_free(pm->libvc_data->pb_libpic);
			pm->pb_libpic = NULL;
			pm->cur_libpb_size--;
			pm->pb_libpic_library_index = -1;
			pm->is_library_buffer_empty = 1;
		}
		// move in lib pic from the buffer outside the decoder
		if (pm->is_library_buffer_empty) {
			//send out referenced_library_picture_index
			int  libpic_idx = -1;
			int ii;
			for (ii = 0; ii < pm->libvc_data->num_libpic_outside; ii++) {
				if (referenced_library_picture_index == pm->libvc_data->list_library_index_outside[ii])
				{
					libpic_idx = ii;
					break;
				}
			}
			com_assert_rv(libpic_idx >= 0, COM_ERR_UNEXPECTED);

			//move in the corresponding referenced library pic
			pm->pb_libpic = pm->libvc_data->list_libpic_outside[libpic_idx];
			pm->pb_libpic_library_index = referenced_library_picture_index;
			pm->cur_libpb_size++;
			pm->is_library_buffer_empty = 0;
		}
	}
#endif
	return COM_OK;
}
#endif

#if LIBVC_ON
int com_picman_put_libpic(COM_PM * pm, COM_PIC * pic, int slice_type, u32 ptr, u32 dtr, u8 temporal_id, int need_for_output, COM_REFP(*refp)[REFP_NUM], COM_PIC_HEADER * pic_header)
{
	int i;
	SET_REF_MARK(pic);
	pic->temporal_id = temporal_id;
	pic->ptr = ptr;
	pic->dtr = dtr;
	pic->need_for_out = (u8)need_for_output;

	for (i = 0; i < pm->num_refp[REFP_0]; i++) {
#if ETMVP || SUB_TMVP || AWP
		pic->list_ptr[REFP_0][i] = refp[i][REFP_0].ptr;
#else
		pic->list_ptr[i] = refp[i][REFP_0].ptr;
#endif
	}
#if ETMVP || SUB_TMVP || AWP
	for (i = 0; i < pm->num_refp[REFP_1]; i++) {
		pic->list_ptr[REFP_1][i] = refp[i][REFP_1].ptr;
	}
#endif

	// move out
	if (!pm->is_library_buffer_empty) {
		//com_picbuf_free(pm->pb_libpic);
		pm->pb_libpic = NULL;
		pm->cur_libpb_size--;
		pm->pb_libpic_library_index = -1;
		pm->is_library_buffer_empty = 1;
	}

	// move in
	pm->pb_libpic = pic;
	pm->pb_libpic_library_index = pic_header->library_picture_index;
	pm->cur_libpb_size++;
	pm->is_library_buffer_empty = 0;

	if (pm->pic_lease == pic) {
		pm->pic_lease = NULL;
	}

	return COM_OK;
}
#endif

int com_picman_put_pic(COM_PM * pm, COM_PIC * pic, int slice_type, u32 ptr, u32 dtr,
	u32 picture_output_delay, u8 temporal_id, int need_for_output, COM_REFP(*refp)[REFP_NUM])
{
	/* manage RPB */
	SET_REF_MARK(pic);
	pic->temporal_id = temporal_id;
	pic->ptr = ptr;
	pic->dtr = dtr%DOI_CYCLE_LENGTH;//MX: range of 0~255
	pic->picture_output_delay = picture_output_delay;
	pic->need_for_out = (u8)need_for_output;
	/* put picture into listed RPB */
	if (IS_REF(pic)) {
		picman_set_pic_to_pb(pm, pic, refp, pm->cur_num_ref_pics);
		pm->cur_num_ref_pics++;
#ifdef BUFMGR_SANITY_CHECK
		if (pm->cur_num_ref_pics > MAX_NUM_REF_PICS) {
			if (avs3_get_debug_flag())
			pr_info("pm->cur_num_ref_pics (%d) is beyond limit, force it to %d\n",
			pm->cur_num_ref_pics, MAX_NUM_REF_PICS);
			pm->cur_num_ref_pics = MAX_NUM_REF_PICS;
		}
#endif
	} else {
		picman_set_pic_to_pb(pm, pic, refp, -1);
	}
	if (pm->pic_lease == pic) {
		pm->pic_lease = NULL;
	}
	/*PRINT_DPB(pm);*/
	return COM_OK;
}
#if LIBVC_ON
int com_picman_out_libpic(COM_PIC * pic, int library_picture_index, COM_PM * pm)
{
	if (pic != NULL && pic->need_for_out)
	{
		//output to the buffer outside the decoder
		int num_libpic_outside = pm->libvc_data->num_libpic_outside;
		pm->libvc_data->list_libpic_outside[num_libpic_outside] = pic;
		pm->libvc_data->list_library_index_outside[num_libpic_outside] = library_picture_index;
		pm->libvc_data->num_libpic_outside++;
		pic->need_for_out = 0;

		return COM_OK;
	} else {
		return COM_ERR_UNEXPECTED;
	}
}
#endif

COM_PIC * com_picman_out_pic(COM_PM * pm, int * err, int cur_pic_doi, int state)
{
	COM_PIC ** ps;
	int i, ret, any_need_for_out = 0;
	BOOL exist_pic = 0;
	int temp_smallest_poc = MAX_INT;
	int temp_idx_for_smallest_poc = 0;
	ps = pm->pic;

	if (state != 1) {
		for (i = 0; i < pm->max_pb_size; i++) {
			if (ps[i] != NULL && ps[i]->need_for_out) {
				any_need_for_out = 1;
				if ((ps[i]->dtr + ps[i]->picture_output_delay <= cur_pic_doi)) {
					exist_pic = 1;
					if (temp_smallest_poc >= ps[i]->ptr)
					{
						temp_smallest_poc = ps[i]->ptr;
						temp_idx_for_smallest_poc = i;
					}
				}
			}
		}
		if (exist_pic) {
			ps[temp_idx_for_smallest_poc]->need_for_out = 0;
			if (err) *err = COM_OK;
			return ps[temp_idx_for_smallest_poc];
		}
	} else {
		//bumping state, bumping the pic in the DPB according to the POC number, from small to larger.
		for (i = 0; i < pm->max_pb_size; i++) {
			if (ps[i] != NULL && ps[i]->need_for_out) {
				any_need_for_out = 1;
				//Need to output the smallest poc
				if ((ps[i]->ptr <= temp_smallest_poc)) {
					exist_pic = 1;
					temp_smallest_poc = ps[i]->ptr;
					temp_idx_for_smallest_poc = i;
				}
			}
		}
		if (exist_pic) {
			ps[temp_idx_for_smallest_poc]->need_for_out = 0;
			if (err) *err = COM_OK;
			return ps[temp_idx_for_smallest_poc];
		}
	}

	if (any_need_for_out == 0) {
		ret = COM_ERR_UNEXPECTED;
	} else {
		ret = COM_OK_FRM_DELAYED;
	}
	if (err) *err = ret;
	return NULL;
}

int com_picman_deinit(COM_PM * pm)
{
	int i;
	/* remove allocated picture and picture store buffer */
	for (i = 0; i < pm->max_pb_size; i++) {
		if (pm->pic[i]) {
			com_pic_free(pm->hw, &pm->pa, pm->pic[i]);
			pm->pic[i] = NULL;
		}
	}
	if (pm->pic_lease) {
		com_pic_free(pm->hw, &pm->pa, pm->pic_lease);
		pm->pic_lease = NULL;
	}

	pm->cur_num_ref_pics = 0;

#if LIBVC_ON
	if (pm->pb_libpic) {
		pm->pb_libpic = NULL;
	}
	pm->cur_libpb_size = 0;
	pm->pb_libpic_library_index = -1;
	pm->is_library_buffer_empty = 1;
#endif
	return COM_OK;
}

int com_picman_init(COM_PM * pm, int max_pb_size, int max_num_ref_pics, PICBUF_ALLOCATOR * pa)
{
	if (max_num_ref_pics > MAX_NUM_REF_PICS || max_pb_size > MAX_PB_SIZE) {
		return COM_ERR_UNSUPPORTED;
	}
	spin_lock_init(&pm->pm_lock);
	pm->max_num_ref_pics = max_num_ref_pics;
	pm->max_pb_size = max_pb_size;
	pm->ptr_increase = 1;
	pm->pic_lease = NULL;
	pm->cur_num_ref_pics = 0;
	com_mcpy(&pm->pa, pa, sizeof(PICBUF_ALLOCATOR));
#if LIBVC_ON
	pm->pb_libpic = NULL;
	pm->cur_libpb_size = 0;
	pm->pb_libpic_library_index = -1;
	pm->is_library_buffer_empty = 1;
#endif
	return COM_OK;
}

int com_picman_check_repeat_doi(COM_PM * pm, COM_PIC_HEADER * pic_header)
{
	COM_PIC * pic;
	int i;
	for (i = 0; i < pm->max_pb_size; i++) {
		pic = pm->pic[i];
		if (pic != NULL) {
			assert(pic->dtr != pic_header->decode_order_index);//the DOI of current frame cannot be the same as the DOI of pic in DPB.
		}
	}
	return COM_OK;
}

void com_picman_print_state(COM_PM * pm)
{
	COM_PIC * pic;
	char tmpbuf[128];
	int i, ii, j;
	printf("pm->pic:\n");
	for (i = 0; i < pm->max_pb_size; i++) {
		pic = pm->pic[i];
		if (pic != NULL) {
			int pos = 0;
			for (ii = 0; ii < 2; ii++) {
				pos += sprintf(&tmpbuf[pos], "(");
				for (j = 0; j < MAX_NUM_REF_PICS; j++)
				pos += sprintf(&tmpbuf[pos], "%d ", pic->list_ptr[ii][j]);
				pos += sprintf(&tmpbuf[pos], ")");
			}

		printf("%d: pic %p index %d dtr %d ptr %d is_ref %d need_for_out %d, backend_ref %d, vf_ref %d, output_delay %d, w/h(%d,%d) id %d, list_ptr %s\n",
			i, pic, pic->buf_cfg.index, pic->dtr, pic->ptr, pic->is_ref,
				pic->need_for_out, pic->buf_cfg.backend_ref, pic->buf_cfg.vf_ref,
				pic->picture_output_delay,
				pic->width_luma, pic->height_luma, pic->temporal_id,
				tmpbuf);
		}
	}
}
