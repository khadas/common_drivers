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

#include "avs3_global.h"
//#define param_proc(mark, index, val) val
int aml_print_header_info;

#define AML
#if HLS_RPL
#if LIBVC_ON
int dec_eco_rlp(union param_u *param, COM_RPL * rpl, COM_SQH * sqh)
#else
int dec_eco_rlp(union param_u *param, COM_RPL * rpl)
#endif
{
#ifndef AML
#if LIBVC_ON
	int ddoi_base = 0;
	int i;

	rpl->reference_to_library_enable_flag = 0;
	if (sqh->library_picture_enable_flag) {
		rpl->reference_to_library_enable_flag = param->p.rpl_reference_to_library_enable_flag;
		if (aml_print_header_info) printf(" * 18-bits reference_to_library_enable_flag : %d\n", rpl->reference_to_library_enable_flag);
	}
	else {
		if (aml_print_header_info) printf(" * set     reference_to_library_enable_flag : %d\n", rpl->reference_to_library_enable_flag);
	}
#endif

	rpl->ref_pic_num = (u32)param->p.rpl_ref_pic_num;
	if (aml_print_header_info) printf(" * UE      ref_pic_num : %d\n", rpl->ref_pic_num);
	//note, here we store DOI of each ref pic instead of delta DOI
	if (rpl->ref_pic_num > 0) {
#if LIBVC_ON
		rpl->library_index_flag[0] = 0;
		if (sqh->library_picture_enable_flag && rpl->reference_to_library_enable_flag) {
			rpl->library_index_flag[0] = param->p.rpl_library_index_flag[0];
			if (aml_print_header_info) printf(" * 1-bit   library_index_flag[0] : %d\n", rpl->library_index_flag[0]);
		} else {
			if (aml_print_header_info) printf(" * set     library_index_flag[0] : %d\n", rpl->library_index_flag[0]);
		}
		if (sqh->library_picture_enable_flag && rpl->library_index_flag[0]) {
			rpl->ref_pics_ddoi[0] = (u32)param->p.rpl_ref_pics_ddoi[0];
			if (aml_print_header_info) printf(" * UE      ref_pics_ddoi[0] : %d\n", rpl->ref_pics_ddoi[0]);
		} else
#endif
		{
#ifdef ORI_CODE
			rpl->ref_pics_ddoi[0] = (u32)com_bsr_read_ue(bs);
			if (aml_print_header_info) printf(" * UE      abs_doi[0] : %d\n", rpl->ref_pics_ddoi[0]);
			if (rpl->ref_pics_ddoi[0] != 0) rpl->ref_pics_ddoi[0] *= 1 - ((u32)com_bsr_read1(bs) << 1);
			if (aml_print_header_info) printf(" * 1-bit   sign_doi[0] : %d --> ref_pics_ddoi[0] : %d\n", (rpl->ref_pics_ddoi[0] < 0), rpl->ref_pics_ddoi[0]);
#if LIBVC_ON
			ddoi_base = rpl->ref_pics_ddoi[0];
#endif
#endif
			rpl->ref_pics_ddoi[0] = param->p.rpl_ref_pics_ddoi[0];
		}
	}
#ifdef BUFMGR_SANITY_CHECK
	for (i = 1; i < rpl->ref_pic_num && i < MAX_NUM_REF_PICS; ++i)
#else
	for (i = 1; i < rpl->ref_pic_num; ++i)
#endif
	{
#if LIBVC_ON
		rpl->library_index_flag[i] = 0;
		if (sqh->library_picture_enable_flag && rpl->reference_to_library_enable_flag) {
			rpl->library_index_flag[i] = param->p.rpl_library_index_flag[i];
			if (aml_print_header_info) printf(" * 1-bit   library_index_flag[%d] : %d\n", i, rpl->library_index_flag[i]);
		} else {
			if (aml_print_header_info) printf(" * set     library_index_flag[%d] : %d\n", i, rpl->library_index_flag[i]);
		}
#ifdef ORI_CODE
		if (sqh->library_picture_enable_flag && rpl->library_index_flag[i]) {
			rpl->ref_pics_ddoi[i] = (u32)com_bsr_read_ue(bs);
			if (aml_print_header_info) printf(" * UE      ref_pics_ddoi[%d] : %d\n", i, rpl->ref_pics_ddoi[i]);
		} else {
			int deltaRefPic = (u32)com_bsr_read_ue(bs);
			if (aml_print_header_info) printf(" * UE      abs_delta_doi[%d] : %d\n", i, deltaRefPic);
			if (deltaRefPic != 0) deltaRefPic *= 1 - ((u32)com_bsr_read1(bs) << 1);
			if (aml_print_header_info) printf(" * 1-bit   sign_delta_doi[%d] : %d --> deltaRefPic : %d\n", i, (deltaRefPic < 0), deltaRefPic);
#if LIBVC_ON
			rpl->ref_pics_ddoi[i] = ddoi_base + deltaRefPic;
			if (aml_print_header_info) printf(" - ref_pics_ddoi[%d] : %d\n", i, rpl->ref_pics_ddoi[i]);
			ddoi_base = rpl->ref_pics_ddoi[i];
#else
			rpl->ref_pics_ddoi[i] = rpl->ref_pics_ddoi[i - 1] + deltaRefPic;
#endif
		}
#endif
		/*ORI_CODE*/
#endif
		rpl->ref_pics_ddoi[i] = param->p.rpl_ref_pics_ddoi[i];
	}
	/*!AML*/
#endif
	return COM_OK;
}
#endif

int dec_eco_sqh(union param_u *param, COM_SQH * sqh)
{
	int i, j;
	int max_cuwh;
#ifdef ORI_CODE
	//video_sequence_start_code
	unsigned int ret = com_bsr_read(bs, 24);
	assert(ret == 1);
	unsigned int start_code;
	start_code = com_bsr_read(bs, 8);
	assert(start_code == 0xB0);
#endif
	sqh->profile_id = (u8)param->p.sqh_profile_id;
	if (aml_print_header_info) {
		printf(" * 8-bits  profile_id : 0x%02x ", sqh->profile_id);
		if (sqh->profile_id == 0x00) printf(" -- Forbidden Profile\n");
		else if (sqh->profile_id == 0x20) printf(" -- Main Profile\n");
		else if (sqh->profile_id == 0x22) printf(" -- Main-10bit Profile\n");
		else printf(" -- Reserved Profile\n");
	}
#if PHASE_2_PROFILE
	if (sqh->profile_id != 0x22 && sqh->profile_id != 0x20
		&& sqh->profile_id != 0x32 && sqh->profile_id != 0x30) {
		printf("unknown profile id: 0x%x\n", sqh->profile_id);
		assert(0);
	}
#endif
	sqh->level_id = (u8)param->p.sqh_level_id;
	if (aml_print_header_info) {
		printf(" * 8-bits  level_id : 0x%02x ", sqh->level_id);
		if (sqh->level_id == 0x10) printf(" -- Level 2.0.15\n");
		else if (sqh->level_id == 0x12) printf(" -- Level 2.0.30\n");
		else if (sqh->level_id == 0x14) printf(" -- Level 2.0.60\n");
		else if (sqh->level_id == 0x20) printf(" -- Level 4.0.30\n");
		else if (sqh->level_id == 0x22) printf(" -- Level 4.0.60\n");
		else if (sqh->level_id == 0x40) printf(" -- Level 6.0.30\n");
		else if (sqh->level_id == 0x42) printf(" -- Level 6.2.30\n");
		else if (sqh->level_id == 0x44) printf(" -- Level 6.0.60\n");
		else if (sqh->level_id == 0x46) printf(" -- Level 6.2.60\n");
		else if (sqh->level_id == 0x48) printf(" -- Level 6.0.120\n");
		else if (sqh->level_id == 0x4a) printf(" -- Level 6.2.120\n");
		else if (sqh->level_id == 0x50) printf(" -- Level 8.0.30\n");
		else if (sqh->level_id == 0x52) printf(" -- Level 8.2.30\n");
		else if (sqh->level_id == 0x54) printf(" -- Level 8.0.60\n");
		else if (sqh->level_id == 0x56) printf(" -- Level 8.2.60\n");
		else if (sqh->level_id == 0x58) printf(" -- Level 8.0.120\n");
		else if (sqh->level_id == 0x5a) printf(" -- Level 8.2.120\n");
		else if (sqh->level_id == 0x60) printf(" -- Level 10.0.30\n");
		else if (sqh->level_id == 0x62) printf(" -- Level 10.2.30\n");
		else if (sqh->level_id == 0x64) printf(" -- Level 10.0.60\n");
		else if (sqh->level_id == 0x66) printf(" -- Level 10.2.60\n");
		else if (sqh->level_id == 0x68) printf(" -- Level 10.0.120\n");
		else if (sqh->level_id == 0x6a) printf(" -- Level 10.2.120\n");
		else printf(" -- Reserved Level\n");
	}
	sqh->progressive_sequence = (u8)param->p.sqh_progressive_sequence;
	if (aml_print_header_info) printf(" * 1-bit   progressive_sequence : %d\n", sqh->progressive_sequence);
	assert(sqh->progressive_sequence == 1);
	sqh->field_coded_sequence = (u8)param->p.sqh_field_coded_sequence;
	if (aml_print_header_info) printf(" * 1-bit   field_coded_sequence : %d\n", sqh->field_coded_sequence);
#if LIBVC_ON
	sqh->library_stream_flag = (u8)param->p.sqh_library_stream_flag;
	if (aml_print_header_info) {
		printf(" * 1-bit   library_stream_flag : %d\n", sqh->library_stream_flag);
	}
	sqh->library_picture_enable_flag = 0;
	//sqh->duplicate_sequence_header_flag = 0;
	if (!sqh->library_stream_flag) {
		sqh->library_picture_enable_flag = (u8)param->p.sqh_library_picture_enable_flag;
		if (aml_print_header_info) printf(" * 1-bit   library_picture_enable_flag : %d\n", sqh->library_picture_enable_flag);
		if (sqh->library_picture_enable_flag) {
			//sqh->duplicate_sequence_header_flag = (u8)param->p.sqh_duplicate_sequence_header_flag;
			if (aml_print_header_info) printf(" * 1-bit   duplicate_sequence_header_flag : %d\n", sqh->duplicate_sequence_header_flag);
		} else {
			if (aml_print_header_info) printf(" - set     duplicate_sequence_header_flag : %d\n", sqh->duplicate_sequence_header_flag);
		}
	}
	else {
		if (aml_print_header_info) printf(" - set     library_picture_enable_flag : %d\n", sqh->library_picture_enable_flag);
		if (aml_print_header_info) printf(" - set     duplicate_sequence_header_flag : %d\n", sqh->duplicate_sequence_header_flag);
	}
#endif
	//assert(com_bsr_read1(bs) == 1);                                //marker_bit
	if (aml_print_header_info) printf(" * 1-bit   marker_bit : %d\n", 1);
	sqh->horizontal_size = param->p.sqh_horizontal_size;
	if (aml_print_header_info) printf(" * 14-bits horizontal_size : %d\n", sqh->horizontal_size);
	//assert(com_bsr_read1(bs) == 1);                                //marker_bit
	if (aml_print_header_info) printf(" * 1-bit   marker_bit : %d\n", 1);
	sqh->vertical_size = param->p.sqh_vertical_size;
	if (aml_print_header_info) printf(" * 14-bits vertical_size : %d\n", sqh->vertical_size);
	if (aml_print_header_info) printf("   - pic_frame_size : %d x %d\n", sqh->horizontal_size, sqh->vertical_size);
	//sqh->chroma_format = (u8)param->p.sqh_chroma_format;
	if (aml_print_header_info) {
		printf(" * 2-bits  chroma_format : %d ", sqh->chroma_format);
		if (sqh->chroma_format == 1) printf(" -- 4:2:0\n");
		// else if (sqh->chroma_format == 2) printf(" -- 4:2:2\n");
		else printf(" -- Reserved chroma_format\n");
	}
	sqh->sample_precision = (u8)param->p.sqh_sample_precision;
	if (aml_print_header_info) printf(" * 3-bits  sample_precision : %d -- %d bits\n", sqh->sample_precision, sqh->sample_precision*2+6);

#if PHASE_2_PROFILE
	if (sqh->profile_id == 0x22 || sqh->profile_id == 0x32)
#else
	if (sqh->profile_id == 0x22)
#endif
	{
		sqh->encoding_precision = (u8)param->p.sqh_encoding_precision;
		if (aml_print_header_info) printf(" * 3-bits  encoding_precision : %d -- %d-bits\n", sqh->encoding_precision, sqh->encoding_precision*2+6);
		//assert(sqh->encoding_precision == 2);
	}
	//assert(com_bsr_read1(bs) == 1);                                //marker_bit
	if (aml_print_header_info) printf(" * 1-bit   marker_bit : %d\n", 1);
	sqh->aspect_ratio = (u8)param->p.sqh_aspect_ratio;
	if (aml_print_header_info) {
		printf(" * 4-bits  aspect_ratio : %d ", sqh->aspect_ratio);
		if (sqh->aspect_ratio == 1) printf(" -- SAR = 1.0\n");
		else if (sqh->aspect_ratio == 2) printf(" -- DAR = 4:3\n");
		else if (sqh->aspect_ratio == 3) printf(" -- DAR = 16:9\n");
		else if (sqh->aspect_ratio == 4) printf(" -- DAR = 2.21:1\n");
		else printf(" -- Reserved aspect_ratio_information\n");
	}

	sqh->frame_rate_code = (u8)param->p.sqh_frame_rate_code;
	if (aml_print_header_info) {
		printf(" * 4-bits  frame_rate_code : %d ", sqh->frame_rate_code);
		if (sqh->frame_rate_code == 1) printf(" -- 23.976 fps\n");
		else if (sqh->frame_rate_code == 2) printf(" -- 24 fps\n");
		else if (sqh->frame_rate_code == 3) printf(" -- 25 fps\n");
		else if (sqh->frame_rate_code == 4) printf(" -- 29.97 fps\n");
		else if (sqh->frame_rate_code == 5) printf(" -- 30 fps\n");
		else if (sqh->frame_rate_code == 6) printf(" -- 50 fps\n");
		else if (sqh->frame_rate_code == 7) printf(" -- 59.94 fps\n");
		else if (sqh->frame_rate_code == 8) printf(" -- 60 fps\n");
		else if (sqh->frame_rate_code == 9) printf(" -- 100 fps\n");
		else if (sqh->frame_rate_code == 10) printf(" -- 120 fps\n");
		else if (sqh->frame_rate_code == 11) printf(" -- 200 fps\n");
		else if (sqh->frame_rate_code == 12) printf(" -- 240 fps\n");
		else if (sqh->frame_rate_code == 13) printf(" -- 300 fps\n");
		else printf(" -- Reserved frame_rate_code\n");
	}
	//assert(com_bsr_read1(bs) == 1);                                //marker_bit
	if (aml_print_header_info) printf(" * 1-bit   marker_bit : %d\n", 1);
	//sqh->bit_rate_lower = param->p.sqh_bit_rate_lower;
	if (aml_print_header_info) printf(" * 18-bits bit_rate_lower : %d\n", sqh->bit_rate_lower);
	//assert(com_bsr_read1(bs) == 1);                                //marker_bit
	if (aml_print_header_info) printf(" * 1-bit   marker_bit : %d\n", 1);
	//sqh->bit_rate_upper = param->p.sqh_bit_rate_upper;
	if (aml_print_header_info) printf(" * 12-bits bit_rate_upper : %d\n", sqh->bit_rate_upper);
	printf(" -  BitRate = %d kbit/s \n", ((sqh->bit_rate_upper << 18) + sqh->bit_rate_lower) * 400 / 1024);
	sqh->low_delay = (u8)param->p.sqh_low_delay;

	sqh->temporal_id_enable_flag = (u8)param->p.sqh_temporal_id_enable_flag;
	if (aml_print_header_info) printf(" * 1-bit   temporal_id_enable_flag : %d\n", sqh->temporal_id_enable_flag);
	//assert(com_bsr_read1(bs) == 1);                                //marker_bit
	if (aml_print_header_info) printf(" * 1-bit   marker_bit : %d\n", 1);
	//sqh->bbv_buffer_size = param->p.sqh_bbv_buffer_size;
	if (aml_print_header_info) printf(" * 18-bits bbv_buffer_size : %d\n", sqh->bbv_buffer_size);
	//assert(com_bsr_read1(bs) == 1);                                //marker_bit
	if (aml_print_header_info) printf(" * 1-bit   marker_bit : %d\n", 1);
	sqh->max_dpb_size = param->p.sqh_max_dpb_size;
	if (aml_print_header_info) printf(" * 4-bits  max_dpb_size_minus1 : %d --> max_dpb_size : %d\n", sqh->max_dpb_size-1, sqh->max_dpb_size);

#if HLS_RPL
	if (aml_print_header_info) printf("--BEGIN HLS_RPL--------------------------------------------\n");
	//sqh->rpl1_index_exist_flag = (u32)param->p.sqh_rpl1_index_exist_flag;
	if (aml_print_header_info) printf(" * 1-bit   rpl1_index_exist_flag : %d\n", sqh->rpl1_index_exist_flag);
	//sqh->rpl1_same_as_rpl0_flag = (u32)param->p.sqh_rpl1_same_as_rpl0_flag;
	if (aml_print_header_info) printf(" * 1-bit   rpl1_same_as_rpl0_flag : %d\n", sqh->rpl1_same_as_rpl0_flag);
	//assert(com_bsr_read1(bs) == 1);                                //marker_bit
	if (aml_print_header_info) printf(" * 1-bit   marker_bit : %d\n", 1);
	//sqh->rpls_l0_num = (u32)param->p.sqh_rpls_l0_num;
	if (aml_print_header_info) printf(" * UE      rpls_l0_num : %d\n", sqh->rpls_l0_num);
	for (i = 0; i < sqh->rpls_l0_num; ++i) {
#if LIBVC_ON
		if (aml_print_header_info) printf(" ## dec_eco_rlp_l0[%d] ##\n", i);
		dec_eco_rlp(param, &sqh->rpls_l0[i], sqh);
#else
		dec_eco_rlp(param, &sqh->rpls_l0[i]);
#endif
	}

	if (!sqh->rpl1_same_as_rpl0_flag) {
		//sqh->rpls_l1_num = (u32)param->p.sqh_rpls_l1_num;
		if (aml_print_header_info) printf(" * UE      rpls_l1_num : %d\n", sqh->rpls_l1_num);
		for (i = 0; i < sqh->rpls_l1_num; ++i)
		{
#if LIBVC_ON
		if (aml_print_header_info) printf(" ## dec_eco_rlp_l1[%d] ##\n", i);
		dec_eco_rlp(param, &sqh->rpls_l1[i], sqh);
#else
		dec_eco_rlp(param, &sqh->rpls_l1[i]);
#endif
		}
	} else {
		//Basically copy everything from sqh->rpls_l0 to sqh->rpls_l1
		//MX: LIBVC is not harmonization yet.
		//sqh->rpls_l1_num = sqh->rpls_l0_num;
		for (i = 0; i < sqh->rpls_l1_num; i++) {
#if LIBVC_ON
			sqh->rpls_l1[i].reference_to_library_enable_flag = sqh->rpls_l0[i].reference_to_library_enable_flag;
#endif
			sqh->rpls_l1[i].ref_pic_num = sqh->rpls_l0[i].ref_pic_num;
			for (j = 0; j < sqh->rpls_l1[i].ref_pic_num; j++) {
#if LIBVC_ON
				sqh->rpls_l1[i].library_index_flag[j] = sqh->rpls_l0[i].library_index_flag[j];
#endif
				sqh->rpls_l1[i].ref_pics_ddoi[j] = sqh->rpls_l0[i].ref_pics_ddoi[j];
			}
		}
	}
	sqh->num_ref_default_active_minus1[0] = (u32)param->p.sqh_num_ref_default_active_minus1[0];
	if (aml_print_header_info) printf("-----------------------------------------------------------\n");
	if (aml_print_header_info) printf(" * UE      num_ref_default_active_minus1[0] : %d\n", sqh->num_ref_default_active_minus1[0]);
	sqh->num_ref_default_active_minus1[1] = (u32)param->p.sqh_num_ref_default_active_minus1[1];
	if (aml_print_header_info) printf(" * UE      num_ref_default_active_minus1[1] : %d\n", sqh->num_ref_default_active_minus1[1]);
	if (aml_print_header_info) printf("--END HLS_RPL----------------------------------------------\n");
#endif

#ifdef ORI_CODE
	if (aml_print_header_info) {
		int temp_size_read;
		temp_size_read = com_bsr_read(bs, 3); // log2_lcu_size_minus2
		sqh->log2_max_cu_width_height = param->p.sqh_log2_max_cu_width_height;
		printf(" * 3-bits  log2_lcu_size_minus2 : %d       --> lcu_size : %d x %d\n", temp_size_read, (1<< sqh->log2_max_cu_width_height), (1<< sqh->log2_max_cu_width_height));
		temp_size_read = com_bsr_read(bs, 2);  // log2_min_cu_size_minus2
		//sqh->min_cu_size = param->p.sqh_min_cu_size;
		printf(" * 2-bits  log2_min_cu_size_minus2 : %d    --> min_cu_size : %d\n", temp_size_read, sqh->min_cu_size);
		temp_size_read = com_bsr_read(bs, 2);  // log2_max_part_ratio_minus2
		//sqh->max_part_ratio = param->p.sqh_max_part_ratio;
		printf(" * 2-bits  log2_max_part_ratio_minus2 : %d --> max_part_ratio : %d\n", temp_size_read, sqh->max_part_ratio);
		temp_size_read = com_bsr_read(bs, 3);  // max_split_times_minus6
		//sqh->max_split_times = param->p.sqh_max_split_times;
		printf(" * 3-bits  max_split_times_minus6 : %d     --> max_split_times : %d\n", temp_size_read, sqh->max_split_times);
		temp_size_read = com_bsr_read(bs, 3);  // log2_min_qt_size_minus2
		//sqh->min_qt_size = param->p.sqh_min_qt_size;
		printf(" * 3-bits  log2_min_qt_size_minus2 : %d    --> min_qt_size : %d\n", temp_size_read, sqh->min_qt_size);
		temp_size_read = com_bsr_read(bs, 3);  // log2_max_bt_size_minus2
		//sqh->max_bt_size = param->p.sqh_max_bt_size;
		printf(" * 3-bits  log2_max_bt_size_minus2 : %d    --> max_bt_size : %d\n", temp_size_read, sqh->max_bt_size);
		temp_size_read = com_bsr_read(bs, 2);  // log2_max_eqt_size_minus3
		//sqh->max_eqt_size = param->p.sqh_max_eqt_size;
		printf(" * 2-bits  log2_max_eqt_size_minus3 : %d   --> max_eqt_size : %d\n", temp_size_read, sqh->max_eqt_size);
	} else
#endif
	{
		sqh->log2_max_cu_width_height = param->p.sqh_log2_max_cu_width_height;
		//sqh->min_cu_size = param->p.sqh_min_cu_size;
		//sqh->max_part_ratio = param->p.sqh_max_part_ratio;
		//sqh->max_split_times = param->p.sqh_max_split_times;
		//sqh->min_qt_size = param->p.sqh_min_qt_size;
		//sqh->max_bt_size = param->p.sqh_max_bt_size;
		//sqh->max_eqt_size = param->p.sqh_max_eqt_size;
	}

#ifndef ORI_CODE
	sqh->adaptive_leveling_filter_enable_flag = (u8)param->p.sqh_adaptive_leveling_filter_enable_flag;
	sqh->num_of_hmvp_cand = (u8)param->p.sqh_num_of_hmvp_cand;
	printf("sqh->num_of_hmvp_cand=%d\n", sqh->num_of_hmvp_cand);
#if ALF_SHAPE || ALF_IMP
	if (sqh->profile_id == 0x30 || sqh->profile_id == 0x32) {
		if (sqh->adaptive_leveling_filter_enable_flag) {
			sqh->adaptive_filter_shape_enable_flag = (u8)param->p.sqh_adaptive_filter_shape_enable_flag;
			if (aml_print_header_info) printf(" * 1-bit   adaptive_filter_shape_enable_flag : %d\n", sqh->adaptive_filter_shape_enable_flag);
		}
		else {
			sqh->adaptive_filter_shape_enable_flag = 0;
			if (aml_print_header_info) printf(" - set     adaptive_filter_shape_enable_flag : %d\n", sqh->adaptive_filter_shape_enable_flag);
		}
	} else {
		sqh->adaptive_filter_shape_enable_flag = 0;
	}
#endif
#else
	assert(com_bsr_read1(bs) == 1);                                //marker_bit
	if (aml_print_header_info) printf(" * 1-bit   marker_bit : %d\n", 1);
	//verify parameters allowed in Profile
	assert(sqh->log2_max_cu_width_height >= 5 && sqh->log2_max_cu_width_height <= 7);
	assert(sqh->min_cu_size == 4);
	assert(sqh->max_part_ratio == 8);
	assert(sqh->max_split_times == 6);
	assert(sqh->min_qt_size == 4 || sqh->min_qt_size == 8 || sqh->min_qt_size == 16 || sqh->min_qt_size == 32 || sqh->min_qt_size == 64 || sqh->min_qt_size == 128);
	assert(sqh->max_bt_size == 64 || sqh->max_bt_size == 128);
	assert(sqh->max_eqt_size == 8 || sqh->max_eqt_size == 16 || sqh->max_eqt_size == 32 || sqh->max_eqt_size == 64);

	//sqh->wq_enable = (u8)param->p.sqh_wq_enable;
	if (aml_print_header_info) printf(" * 1-bit   wq_enable : %d\n", sqh->wq_enable);
	if (sqh->wq_enable) {
		sqh->seq_wq_mode = param->p.sqh_seq_wq_mode;
		if (aml_print_header_info) printf(" * 1-bit   seq_wq_mode : %d\n", sqh->seq_wq_mode);
		if (sqh->seq_wq_mode) {
			read_wq_matrix(bs, sqh->wq_4x4_matrix, sqh->wq_8x8_matrix);  //weight_quant_matrix( )
		}
		else {
			memcpy(sqh->wq_4x4_matrix, tab_WqMDefault4x4, sizeof(tab_WqMDefault4x4));
			memcpy(sqh->wq_8x8_matrix, tab_WqMDefault8x8, sizeof(tab_WqMDefault8x8));
		}
	}
	//sqh->secondary_transform_enable_flag = (u8)param->p.sqh_secondary_transform_enable_flag;
	if (aml_print_header_info) printf(" * 1-bit   secondary_transform_enable_flag : %d\n", sqh->secondary_transform_enable_flag);
	//sqh->sample_adaptive_offset_enable_flag = (u8)param->p.sqh_sample_adaptive_offset_enable_flag;
	if (aml_print_header_info) printf(" * 1-bit   sample_adaptive_offset_enable_flag : %d\n", sqh->sample_adaptive_offset_enable_flag);
	sqh->adaptive_leveling_filter_enable_flag = (u8)param->p.sqh_adaptive_leveling_filter_enable_flag;
	if (aml_print_header_info) printf(" * 1-bit   adaptive_leveling_filter_enable_flag : %d\n", sqh->adaptive_leveling_filter_enable_flag);
	//sqh->affine_enable_flag = (u8)param->p.sqh_affine_enable_flag;
	if (aml_print_header_info) printf(" * 1-bit   affine_enable_flag : %d\n", sqh->affine_enable_flag);
#if SMVD
	//sqh->smvd_enable_flag = (u8)param->p.sqh_smvd_enable_flag;
	if (aml_print_header_info) printf(" * 1-bit   smvd_enable_flag : %d\n", sqh->smvd_enable_flag);
#endif
#if IPCM
	//sqh->ipcm_enable_flag = param->p.sqh_ipcm_enable_flag;
	if (aml_print_header_info) printf(" * 1-bit   ipcm_enable_flag : %d\n", sqh->ipcm_enable_flag);
	assert(sqh->sample_precision == 1 || sqh->sample_precision == 2);
#endif
	//sqh->amvr_enable_flag = (u8)param->p.sqh_amvr_enable_flag;
	if (aml_print_header_info) printf(" * 1-bit   amvr_enable_flag : %d\n", sqh->amvr_enable_flag);
	sqh->num_of_hmvp_cand = (u8)param->p.sqh_num_of_hmvp_cand;
	if (aml_print_header_info) printf(" * 4-bits  num_of_hmvp_cand : %d\n", sqh->num_of_hmvp_cand);
	//sqh->umve_enable_flag = (u8)param->p.sqh_umve_enable_flag;
	if (aml_print_header_info) printf(" * 1-bit   umve_enable_flag : %d\n", sqh->umve_enable_flag);
#if EXT_AMVR_HMVP
	if (sqh->amvr_enable_flag && sqh->num_of_hmvp_cand) {
		//sqh->emvr_enable_flag = (u8)param->p.sqh_emvr_enable_flag;
		if (aml_print_header_info) printf(" * 1-bit   emvr_enable_flag : %d\n", sqh->emvr_enable_flag);
	} else {
		//sqh->emvr_enable_flag = 0;
		if (aml_print_header_info) printf(" - set     emvr_enable_flag : %d\n", sqh->emvr_enable_flag);
	}
#endif
	//sqh->ipf_enable_flag = (u8)param->p.sqh_ipf_enable_flag;
	if (aml_print_header_info) printf(" * 1-bit   ipf_enable_flag : %d\n", sqh->ipf_enable_flag);
#if TSCPM
	//sqh->tscpm_enable_flag = (u8)param->p.sqh_tscpm_enable_flag;
	if (aml_print_header_info) printf(" * 1-bit   tscpm_enable_flag : %d\n", sqh->tscpm_enable_flag);
#endif
	//assert(com_bsr_read1(bs) == 1);              //marker_bit
	if (aml_print_header_info) printf(" * 1-bit   marker_bit : %d\n", 1);
#if DT_PARTITION
	//sqh->dt_intra_enable_flag = (u8)param->p.sqh_dt_intra_enable_flag;
	if (aml_print_header_info) printf(" * 1-bit   dt_intra_enable_flag : %d\n", sqh->dt_intra_enable_flag);
	if (sqh->dt_intra_enable_flag) {
		//u32 log2_max_dt_size_minus4 = com_bsr_read(bs, 2);
		//assert(log2_max_dt_size_minus4 <= 2);
		//sqh->max_dt_size = param->p.sqh_max_dt_size;
		if (aml_print_header_info) printf(" * 2-bits  log2_max_dt_size_minus4 : %d --> max_dt_size : %d\n", log2_max_dt_size_minus4, sqh->max_dt_size);
	}
#endif
	//sqh->position_based_transform_enable_flag = (u8)param->p.sqh_position_based_transform_enable_flag;
	if (aml_print_header_info) printf(" * 1-bit   position_based_transform_enable_flag : %d\n", sqh->position_based_transform_enable_flag);

#if PHASE_2_PROFILE
	if (sqh->profile_id == 0x30 || sqh->profile_id == 0x32) {
#endif
		// begin of phase-2 sqh
#if ESAO
		sqh->esao_enable_flag = (u8)param->p.sqh_esao_enable_flag;
		if (aml_print_header_info) printf(" * 1-bit   esao_enable_flag : %d\n", sqh->esao_enable_flag);
#endif
#if CCSAO
		sqh->ccsao_enable_flag = (u8)param->p.sqh_ccsao_enable_flag;
		if (aml_print_header_info) printf(" * 1-bit   ccsao_enable_flag : %d\n", sqh->ccsao_enable_flag);
#endif
#if ALF_SHAPE || ALF_IMP
		if (sqh->adaptive_leveling_filter_enable_flag) {
			sqh->adaptive_filter_shape_enable_flag = (u8)param->p.sqh_adaptive_filter_shape_enable_flag;
			if (aml_print_header_info) printf(" * 1-bit   adaptive_filter_shape_enable_flag : %d\n", sqh->adaptive_filter_shape_enable_flag);
		}
		else {
			sqh->adaptive_filter_shape_enable_flag = 0;
			if (aml_print_header_info) printf(" - set     adaptive_filter_shape_enable_flag : %d\n", sqh->adaptive_filter_shape_enable_flag);
		}
#endif
#if UMVE_ENH
		if (sqh->umve_enable_flag) {
			sqh->umve_enh_enable_flag = (u8)param->p.sqh_umve_enh_enable_flag;
			if (aml_print_header_info) printf(" * 1-bit   umve_enh_enable_flag : %d\n", sqh->umve_enh_enable_flag);
		}
#endif
#if AWP
		sqh->awp_enable_flag = (u8)param->p.sqh_awp_enable_flag;
		if (aml_print_header_info) printf(" * 1-bit   awp_enable_flag : %d\n", sqh->awp_enable_flag);
#endif
#if AWP_MVR
		if (sqh->awp_enable_flag) {
			sqh->awp_mvr_enable_flag = (u8)param->p.sqh_awp_mvr_enable_flag;
			if (aml_print_header_info) printf(" * 1-bit   awp_mvr_enable_flag : %d\n", sqh->awp_mvr_enable_flag);
		} else {
			sqh->awp_mvr_enable_flag = 0;
			if (aml_print_header_info) printf(" - set     awp_mvr_enable_flag : %d\n", sqh->awp_mvr_enable_flag);
		}
#endif
#if BIO
		sqh->bio_enable_flag = (u8)param->p.sqh_bio_enable_flag;
		if (aml_print_header_info) printf(" * 1-bit   bio_enable_flag : %d\n", sqh->bio_enable_flag);
#endif
#if BGC
		sqh->bgc_enable_flag = (u8)param->p.sqh_bgc_enable_flag;
		if (aml_print_header_info) printf(" * 1-bit   bgc_enable_flag : %d\n", sqh->bgc_enable_flag);
#endif
#if DMVR
		sqh->dmvr_enable_flag = (u8)param->p.sqh_dmvr_enable_flag;
		if (aml_print_header_info) printf(" * 1-bit   dmvr_enable_flag : %d\n", sqh->dmvr_enable_flag);
#endif
#if INTERPF
		sqh->interpf_enable_flag = (u8)param->p.sqh_interpf_enable_flag;
		if (aml_print_header_info) printf(" * 1-bit   interpf_enable_flag : %d\n", sqh->interpf_enable_flag);
#endif
#if IBC_ABVR
		sqh->abvr_enable_flag = (u8)param->p.sqh_abvr_enable_flag;
		if (aml_print_header_info) printf(" * 1-bit   abvr_enable_flag : %d\n", sqh->abvr_enable_flag);
#endif
#if USE_SP
		sqh->sp_enable_flag = (u8)param->p.sqh_sp_enable_flag;
		if (aml_print_header_info) printf(" * 1-bit   sp_enable_flag : %d\n", sqh->sp_enable_flag);
#if EVS_UBVS_MODE
		if (sqh->sp_enable_flag) {
			sqh->evs_ubvs_enable_flag = (u8)param->p.sqh_evs_ubvs_enable_flag;
			if (aml_print_header_info) printf(" * 1-bit   evs_ubvs_enable_flag : %d\n", sqh->evs_ubvs_enable_flag);
		} else {
			sqh->evs_ubvs_enable_flag = 0;
			if (aml_print_header_info) printf(" - set     evs_ubvs_enable_flag : %d\n", sqh->evs_ubvs_enable_flag);
		}
#endif
#endif
#if IBC_BVP
		sqh->num_of_hbvp_cand = (u8)param->p.sqh_num_of_hbvp_cand;
		if (aml_print_header_info) printf(" * 4-bits  num_of_hbvp_cand : %d\n", sqh->num_of_hbvp_cand);
#endif
#if ASP
		if (sqh->affine_enable_flag) {
			sqh->asp_enable_flag = (u8)param->p.sqh_asp_enable_flag;
			if (aml_print_header_info) printf(" * 1-bit   asp_enable_flag : %d\n", sqh->asp_enable_flag);
		} else {
			sqh->asp_enable_flag = 0;
			if (aml_print_header_info) printf(" - set     asp_enable_flag : %d\n", sqh->asp_enable_flag);
		}
#endif
#if AFFINE_UMVE
		if (sqh->affine_enable_flag && sqh->umve_enable_flag) {
			sqh->affine_umve_enable_flag = (u8)param->p.sqh_affine_umve_enable_flag;
		} else {
			sqh->affine_umve_enable_flag = 0;
		}
#endif
#if MIPF
		sqh->mipf_enable_flag = (u8)param->p.sqh_mipf_enable_flag;
#endif
#if TSCPM && ENHANCE_TSPCM
		if (sqh->tscpm_enable_flag) {
			sqh->enhance_tscpm_enable_flag = (u8)param->p.sqh_enhance_tscpm_enable_flag;
		} else {
			sqh->enhance_tscpm_enable_flag = 0;
		}
#endif
#if IPF_CHROMA
		if (sqh->ipf_enable_flag) {
			sqh->chroma_ipf_enable_flag = (u8)param->p.sqh_chroma_ipf_enable_flag;
		} else {
			sqh->chroma_ipf_enable_flag = 0;
		}
#endif
#if IIP
		sqh->iip_enable_flag = (u8)param->p.sqh_iip_enable_flag;
#endif
#if PMC
		sqh->pmc_enable_flag = (u8)param->p.sqh_pmc_enable_flag;
#endif
#if FIMC
		sqh->fimc_enable_flag = (u8)param->p.sqh_fimc_enable_flag;
#endif
#if USE_IBC
		sqh->ibc_flag = (u8)param->p.sqh_ibc_flag;
#endif
#if SIBC
		if (sqh->ibc_flag) {
			sqh->sibc_enable_flag = (u8)param->p.sqh_sibc_enable_flag;
		} else {
			sqh->sibc_enable_flag = 0;
		}
#endif
#if SBT
		sqh->sbt_enable_flag = (u8)param->p.sqh_sbt_enable_flag;
#endif
#if IST
		sqh->ist_enable_flag = (u8)param->p.sqh_ist_enable_flag;
#endif
#if ISTS
		sqh->ists_enable_flag = (u8)param->p.sqh_ists_enable_flag;
#endif
#if TS_INTER
		sqh->ts_inter_enable_flag = (u8)param->p.sqh_ts_inter_enable_flag;
#endif
#if EST
		if (sqh->secondary_transform_enable_flag) {
			sqh->est_enable_flag = (u8)param->p.sqh_est_enable_flag;
		}
#endif
#if ST_CHROMA
		if (sqh->secondary_transform_enable_flag) {
			sqh->st_chroma_enable_flag = (u8)param->p.sqh_st_chroma_enable_flag;
		} else {
			sqh->st_chroma_enable_flag = 0;
		}
#endif
#if SRCC
		sqh->srcc_enable_flag = (u8)param->p.sqh_srcc_enable_flag;
#endif
#if CABAC_MULTI_PROB
		sqh->mcabac_enable_flag = (u8)param->p.sqh_mcabac_enable_flag;
#endif
#if EIPM
		sqh->eipm_enable_flag = (u8)param->p.sqh_eipm_enable_flag;
#endif
#if ETMVP
		sqh->etmvp_enable_flag = (u8)param->p.sqh_etmvp_enable_flag;
#endif
#if SUB_TMVP
		sqh->sbtmvp_enable_flag = (u8)param->p.sqh_sbtmvp_enable_flag;
#endif
#if MVAP
		sqh->mvap_enable_flag = (u8)param->p.sqh_mvap_enable_flag;
		sqh->num_of_mvap_cand = sqh->mvap_enable_flag ? ALLOWED_MVAP_NUM : 0;
#endif
#if DBK_SCC
		sqh->loop_filter_type_enable_flag = (u8)param->p.sqh_loop_filter_type_enable_flag;
#endif
#if DBR
		sqh->dbr_enable_flag = (u8)param->p.sqh_dbr_enable_flag;
#endif
		// end of phase-2 sqh
#if PHASE_2_PROFILE
	} else {
#if ESAO
		sqh->esao_enable_flag = 0;
#endif
#if CCSAO
		sqh->ccsao_enable_flag = 0;
#endif
#if ALF_SHAPE || ALF_IMP
		sqh->adaptive_filter_shape_enable_flag = 0;
#endif
#if UMVE_ENH
		sqh->umve_enh_enable_flag = 0;
#endif
#if AWP
		sqh->awp_enable_flag = 0;
#endif
#if AWP_MVR
		sqh->awp_mvr_enable_flag = 0;
#endif
#if BIO
		sqh->bio_enable_flag = 0;
#endif
#if BGC
		sqh->bgc_enable_flag = 0;
#endif
#if DMVR
		sqh->dmvr_enable_flag = 0;
#endif
#if INTERPF
		sqh->interpf_enable_flag = 0;
#endif
#if IBC_ABVR
		sqh->abvr_enable_flag = 0;
#endif
#if USE_SP
		sqh->sp_enable_flag = 0;
#if EVS_UBVS_MODE
		sqh->evs_ubvs_enable_flag = 0;
#endif
#endif
#if IBC_BVP
		sqh->num_of_hbvp_cand = 0;
#endif
#if ASP
		sqh->asp_enable_flag = 0;
#endif
#if AFFINE_UMVE
		sqh->affine_umve_enable_flag = 0;
#endif
#if MIPF
		sqh->mipf_enable_flag = 0;
#endif
#if TSCPM && ENHANCE_TSPCM
		sqh->enhance_tscpm_enable_flag = 0;
#endif
#if IPF_CHROMA
		sqh->chroma_ipf_enable_flag = 0;
#endif
#if IIP
		sqh->iip_enable_flag = 0;
#endif
#if PMC
		sqh->pmc_enable_flag = 0;
#endif
#if FIMC
		sqh->fimc_enable_flag = 0;
#endif
#if USE_IBC
		sqh->ibc_flag = 0;
#endif
#if SIBC
		sqh->sibc_enable_flag = 0;
#endif
#if SBT
		sqh->sbt_enable_flag = 0;
#endif
#if IST
		sqh->ist_enable_flag = 0;
#endif
#if ISTS
		sqh->ists_enable_flag = 0;
#endif
#if TS_INTER
		sqh->ts_inter_enable_flag = 0;
#endif
#if EST
		sqh->est_enable_flag = 0;
#endif
#if ST_CHROMA
		sqh->st_chroma_enable_flag = 0;
#endif
#if SRCC
		sqh->srcc_enable_flag = 0;
#endif
#if CABAC_MULTI_PROB
		sqh->mcabac_enable_flag = 0;
#endif
#if EIPM
		sqh->eipm_enable_flag = 0;
#endif
#if ETMVP
		sqh->etmvp_enable_flag = 0;
#endif
#if SUB_TMVP
		sqh->sbtmvp_enable_flag = 0;
#endif
#if MVAP
		sqh->mvap_enable_flag = 0;
		sqh->num_of_mvap_cand = 0;
#endif
#if DBK_SCC
		sqh->loop_filter_type_enable_flag = 0;
#endif
#if DBR
		sqh->dbr_enable_flag = 0;
#endif
	}
#endif // end of PHASE_2_PROFILE
	/*ORI_CODE*/
#endif
	if (sqh->low_delay == 0) {
		sqh->output_reorder_delay = (u8)param->p.sqh_output_reorder_delay;
		if (aml_print_header_info) printf(" * 5-bits  output_reorder_delay : %d\n", sqh->output_reorder_delay);
	} else {
		sqh->output_reorder_delay = 0;
		if (aml_print_header_info) printf(" - set     output_reorder_delay : %d\n", sqh->output_reorder_delay);
	}
#ifdef ORI_CODE
#if PATCH
	sqh->cross_patch_loop_filter = (u8)param->p.sqh_cross_patch_loop_filter;
	if (aml_print_header_info) printf(" * 1-bit   cross_patch_loop_filter : %d\n", sqh->cross_patch_loop_filter);
	//sqh->patch_ref_colocated = (u8)param->p.sqh_patch_ref_colocated;
	if (aml_print_header_info) printf(" * 1-bit   patch_ref_colocated : %d\n", sqh->patch_ref_colocated);
	//sqh->patch_stable = (u8)param->p.sqh_patch_stable;
	if (aml_print_header_info) printf(" * 1-bit   patch_stable : %d\n", sqh->patch_stable);
	if (sqh->patch_stable) {
		//sqh->patch_uniform = (u8)param->p.sqh_patch_uniform;
		if (aml_print_header_info) printf(" * 1-bit   patch_uniform : %d\n", sqh->patch_uniform);
		if (sqh->patch_uniform) {
			assert(com_bsr_read1(bs) == 1);                      //marker_bit
			//sqh->patch_width_minus1 = (u8)param->p.sqh_patch_width_minus1;
			if (aml_print_header_info) printf(" * UE      patch_width_minus1 : %d\n", sqh->patch_width_minus1);
			//sqh->patch_height_minus1 = (u8)param->p.sqh_patch_height_minus1;
			if (aml_print_header_info) printf(" * UE      patch_height_minus1 : %d\n", sqh->patch_height_minus1);
		}
	}
#endif
	com_bsr_read(bs, 2); //reserved_bits r(2)
	if (aml_print_header_info) printf(" * 2-bits  reserved_bits dropped\n");

	//next_start_code()
	assert(com_bsr_read1(bs)==1); // stuffing_bit '1'
	while (!COM_BSR_IS_BYTE_ALIGN(bs)) {
		assert(com_bsr_read1(bs) == 0); // stuffing_bit '0'
	}
	while (com_bsr_next(bs, 24) != 0x1) {
		assert(com_bsr_read(bs, 8) == 0); // stuffing_byte '00000000'
	};
#endif
	/* check values */
	max_cuwh = 1 << sqh->log2_max_cu_width_height;
	if (max_cuwh < MIN_CU_SIZE || max_cuwh > MAX_CU_SIZE) {
		printf("max_cuwh %d, MIN_CU_SIZE %d, MAX_CU_SIZE %d\n",
		max_cuwh, MIN_CU_SIZE, MAX_CU_SIZE);
		return COM_ERR_MALFORMED_BITSTREAM;
	}
	printf("%s return\n", __func__);
	return COM_OK;
}

int dec_eco_pic_header(union param_u *param, COM_PIC_HEADER * pic_header, COM_SQH * sqh, int* need_minus_256, unsigned int start_code)
{
	int i, ii;
#ifdef ORI_CODE
	unsigned int ret = com_bsr_read(bs, 24);
	assert(ret == 1);
	unsigned int start_code = com_bsr_read(bs, 8);
#endif
	if (start_code == 0xB3) {
		pic_header->slice_type = SLICE_I;
	}
	assert(start_code == 0xB6 || start_code == 0xB3);
	if (start_code != 0xB3)//MX: not SLICE_I
	{
		pic_header->random_access_decodable_flag = param->p.pic_header_random_access_decodable_flag;
	}

	if (start_code == 0xB6) {
		pic_header->slice_type = (u8)param->p.pic_header_slice_type;
	}
	if (pic_header->slice_type == SLICE_I) {
		//pic_header->time_code_flag = param->p.pic_header_time_code_flag;
		if (pic_header->time_code_flag == 1) {
		//pic_header->time_code = param->p.pic_header_time_code;
		}
	}

	pic_header->decode_order_index = param->p.pic_header_decode_order_index;

#if LIBVC_ON
	pic_header->library_picture_index = -1;
	if (pic_header->slice_type == SLICE_I) {
		if (sqh->library_stream_flag) {
			pic_header->library_picture_index = param->p.pic_header_library_picture_index;
		}
	}
#endif

	if (sqh->temporal_id_enable_flag == 1) {
		//pic_header->temporal_id = (u8)param->p.pic_header_temporal_id;
	}
	if (sqh->low_delay == 0) {
		pic_header->picture_output_delay = param->p.pic_header_picture_output_delay;
		pic_header->bbv_check_times = 0;
	} else {
		pic_header->picture_output_delay = 0;
		//pic_header->bbv_check_times = param->p.pic_header_bbv_check_times;
	}

	//the field information below is not used by decoder -- start
	pic_header->progressive_frame = param->p.pic_header_progressive_frame;
	assert(pic_header->progressive_frame == 1);
	if (pic_header->progressive_frame == 0) {
		//pic_header->picture_structure = param->p.pic_header_picture_structure;
	} else {
		//pic_header->picture_structure = 1;
	}
	pic_header->top_field_first = param->p.pic_header_top_field_first;
	pic_header->repeat_first_field = param->p.pic_header_repeat_first_field;
	if (sqh->field_coded_sequence == 1) {
		pic_header->top_field_picture_flag = param->p.pic_header_top_field_picture_flag;
		//com_bsr_read1(bs); // reserved_bits r(1)
	}
	// -- end

#if LIBVC_ON
	if (!sqh->library_stream_flag) {
#endif
		if (pic_header->decode_order_index < pic_header->g_DOIPrev) {
			*need_minus_256 = 1;
			pic_header->g_CountDOICyCleTime++;                    // initialized the number .
		}
		pic_header->g_DOIPrev = pic_header->decode_order_index;
		pic_header->dtr = pic_header->decode_order_index + (DOI_CYCLE_LENGTH * pic_header->g_CountDOICyCleTime) + pic_header->picture_output_delay - sqh->output_reorder_delay;//MX: in the decoder, the only usage of g_CountDOICyCleTime is to derive the POC/POC. here is the dtr
#if LIBVC_ON
	} else {
		pic_header->dtr = pic_header->decode_order_index + (DOI_CYCLE_LENGTH * pic_header->g_CountDOICyCleTime) + pic_header->picture_output_delay - sqh->output_reorder_delay;
	}
#endif
	//initialization
	pic_header->rpl_l0_idx = pic_header->rpl_l1_idx = -1;
	pic_header->ref_pic_list_sps_flag[0] = pic_header->ref_pic_list_sps_flag[1] = 0;
	pic_header->rpl_l0.ref_pic_num = 0;
	pic_header->rpl_l1.ref_pic_num = 0;
	pic_header->rpl_l0.ref_pic_active_num = 0;
	pic_header->rpl_l1.ref_pic_active_num = 0;
	for (i = 0; i < MAX_NUM_REF_PICS; i++) {
		pic_header->rpl_l0.ref_pics[i] = 0;
		pic_header->rpl_l1.ref_pics[i] = 0;
		pic_header->rpl_l0.ref_pics_ddoi[i] = 0;
		pic_header->rpl_l1.ref_pics_ddoi[i] = 0;
		pic_header->rpl_l0.ref_pics_doi[i] = 0;
		pic_header->rpl_l1.ref_pics_doi[i] = 0;
#if LIBVC_ON
		pic_header->rpl_l0.library_index_flag[i] = 0;
		pic_header->rpl_l1.library_index_flag[i] = 0;
#endif
	}

#if HLS_RPL
	//TBD(@Chernyak) if (!IDR) condition to be added here
	pic_header->poc = pic_header->dtr;
	//pic_header->poc = com_bsr_read_ue(bs);  = param_proc???("", -1, );
	// L0 candidates signaling
	pic_header->ref_pic_list_sps_flag[0] = param->p.pic_header_ref_pic_list_sps_flag[0];
	if (pic_header->ref_pic_list_sps_flag[0]) {
		if (sqh->rpls_l0_num) {
			if (sqh->rpls_l0_num > 1) {
				pic_header->rpl_l0_idx = param->p.pic_header_rpl_l0_idx;
			} else//if sps only have 1 RPL, no need to signal the idx
			{
				pic_header->rpl_l0_idx = 0;
			}
#ifdef BUFMGR_SANITY_CHECK
			if ((pic_header->rpl_l1_idx >= 0) &&
				(pic_header->rpl_l1_idx < MAX_NUM_RPLS))
#endif
				memcpy(&pic_header->rpl_l0, &sqh->rpls_l0[pic_header->rpl_l0_idx], sizeof(pic_header->rpl_l0));
			pic_header->rpl_l0.poc = pic_header->poc;
		}
	} else {
#if LIBVC_ON
		dec_eco_rlp(param, &pic_header->rpl_l0, sqh);
#else
		dec_eco_rlp(param, &pic_header->rpl_l0);
#endif
		pic_header->rpl_l0.poc = pic_header->poc;
	}

	//L1 candidates signaling
	pic_header->ref_pic_list_sps_flag[1] = param->p.pic_header_ref_pic_list_sps_flag[1];
	if (pic_header->ref_pic_list_sps_flag[1]) {
		if (sqh->rpls_l1_num > 1 && sqh->rpl1_index_exist_flag) {
			pic_header->rpl_l1_idx = param->p.pic_header_rpl_l1_idx;
		} else if (!sqh->rpl1_index_exist_flag) {
			pic_header->rpl_l1_idx = pic_header->rpl_l0_idx;
		} else//if sps only have 1 RPL, no need to signal the idx
		{
			assert(sqh->rpls_l1_num == 1);
			pic_header->rpl_l1_idx = 0;
		}
#ifdef BUFMGR_SANITY_CHECK
		if ((pic_header->rpl_l1_idx >= 0) &&
			(pic_header->rpl_l1_idx < MAX_NUM_RPLS))
#endif
			memcpy(&pic_header->rpl_l1, &sqh->rpls_l1[pic_header->rpl_l1_idx], sizeof(pic_header->rpl_l1));
		pic_header->rpl_l1.poc = pic_header->poc;
	} else {
#if LIBVC_ON
		dec_eco_rlp(param, &pic_header->rpl_l1, sqh);
#else
		dec_eco_rlp(param, &pic_header->rpl_l1);
#endif
		pic_header->rpl_l1.poc = pic_header->poc;
	}
#ifdef AML
	pic_header->rpl_l0.ref_pic_num = param->p.pic_header_rpl_l0_ref_pic_num;
	for (ii=0; ii<MAX_NUM_REF_PICS; ii++) {
		pic_header->rpl_l0.ref_pics_ddoi[ii] = param->p.pic_header_rpl_l0_ref_pics_ddoi[ii];
	}
	pic_header->rpl_l1.ref_pic_num = param->p.pic_header_rpl_l1_ref_pic_num;
	for (ii=0; ii<MAX_NUM_REF_PICS; ii++) {
		pic_header->rpl_l1.ref_pics_ddoi[ii] = param->p.pic_header_rpl_l1_ref_pics_ddoi[ii];
	}

	pic_header->rpl_l0.reference_to_library_enable_flag = param->p.pic_header_rpl_l0_reference_to_library_enable_flag;
	for (ii=0; ii<MAX_NUM_REF_PICS; ii++) {
		pic_header->rpl_l0.library_index_flag[ii] = param->p.pic_header_rpl_l0_library_index_flag[ii];
	}
	pic_header->rpl_l1.reference_to_library_enable_flag = param->p.pic_header_rpl_l1_reference_to_library_enable_flag;
	for (ii=0; ii<MAX_NUM_REF_PICS; ii++) {
		pic_header->rpl_l1.library_index_flag[ii] = param->p.pic_header_rpl_l1_library_index_flag[ii];
	}

#endif

	if (pic_header->slice_type != SLICE_I) {
		pic_header->num_ref_idx_active_override_flag = param->p.pic_header_num_ref_idx_active_override_flag;
		if (pic_header->num_ref_idx_active_override_flag) {
			pic_header->rpl_l0.ref_pic_active_num = (u32)param->p.pic_header_rpl_l0_ref_pic_active_num;
#ifdef BUFMGR_SANITY_CHECK
			if (pic_header->rpl_l0.ref_pic_active_num > MAX_NUM_REF_PICS) {
				if (avs3_get_debug_flag())
					pr_info("<1>pic_header->rpl_l0.ref_pic_active_num (%d) is beyond limit, force it to %d\n",
				pic_header->rpl_l0.ref_pic_active_num, MAX_NUM_REF_PICS);
				pic_header->rpl_l0.ref_pic_active_num = MAX_NUM_REF_PICS;
			}
#endif
			if (pic_header->slice_type == SLICE_P) {
				pic_header->rpl_l1.ref_pic_active_num = 0;
			} else if (pic_header->slice_type == SLICE_B) {
				pic_header->rpl_l1.ref_pic_active_num = (u32)param->p.pic_header_rpl_l1_ref_pic_active_num;
#ifdef BUFMGR_SANITY_CHECK
				if (pic_header->rpl_l1.ref_pic_active_num > MAX_NUM_REF_PICS) {
					if (avs3_get_debug_flag())
						pr_info("<2>pic_header->rpl_l1.ref_pic_active_num (%d) is beyond limit, force it to %d\n",
					pic_header->rpl_l1.ref_pic_active_num, MAX_NUM_REF_PICS);
					pic_header->rpl_l1.ref_pic_active_num = MAX_NUM_REF_PICS;
				}
#endif
			}
		} else {
			//Hendry -- @Roman: we need to signal the num_ref_idx_default_active_minus1[ i ]. This syntax element is in the PPS in the spec document
			pic_header->rpl_l0.ref_pic_active_num = sqh->num_ref_default_active_minus1[0]+1;
#ifdef BUFMGR_SANITY_CHECK
			if (pic_header->rpl_l0.ref_pic_active_num > MAX_NUM_REF_PICS) {
				if (avs3_get_debug_flag())
					pr_info("<3>pic_header->rpl_l0.ref_pic_active_num (%d) is beyond limit, force it to %d\n",
				pic_header->rpl_l0.ref_pic_active_num, MAX_NUM_REF_PICS);
				pic_header->rpl_l0.ref_pic_active_num = MAX_NUM_REF_PICS;
			}
#endif
			if (pic_header->slice_type == SLICE_P) {
				pic_header->rpl_l1.ref_pic_active_num = 0;
			} else {
				pic_header->rpl_l1.ref_pic_active_num = sqh->num_ref_default_active_minus1[1] + 1;
#ifdef BUFMGR_SANITY_CHECK
				if (pic_header->rpl_l1.ref_pic_active_num > MAX_NUM_REF_PICS) {
					if (avs3_get_debug_flag())
						pr_info("<4>pic_header->rpl_l1.ref_pic_active_num (%d) is beyond limit, force it to %d\n",
					pic_header->rpl_l1.ref_pic_active_num, MAX_NUM_REF_PICS);
					pic_header->rpl_l1.ref_pic_active_num = MAX_NUM_REF_PICS;
				}
#endif
			}
		}
	}
	if (pic_header->slice_type == SLICE_I) {
		pic_header->rpl_l0.ref_pic_active_num = 0;
		pic_header->rpl_l1.ref_pic_active_num = 0;
	}
#if LIBVC_ON
	pic_header->is_RLpic_flag = 0;
	if (pic_header->slice_type != SLICE_I) {
		int only_ref_libpic_flag = 1;
#ifdef BUFMGR_SANITY_CHECK
		for (i = 0; i < pic_header->rpl_l0.ref_pic_active_num && i < MAX_NUM_REF_PICS; i++)
#else
		for (i = 0; i < pic_header->rpl_l0.ref_pic_active_num; i++)
#endif
		{
			if (!pic_header->rpl_l0.library_index_flag[i]) {
				only_ref_libpic_flag = 0;
				break;
			}
		}
		if (only_ref_libpic_flag) {
#ifdef BUFMGR_SANITY_CHECK
			for (i = 0; i < pic_header->rpl_l1.ref_pic_active_num && i < MAX_NUM_REF_PICS; i++)
#else
			for (i = 0; i < pic_header->rpl_l1.ref_pic_active_num; i++)
#endif
			{
				if (!pic_header->rpl_l1.library_index_flag[i]) {
					only_ref_libpic_flag = 0;
					break;
				}
			}
		}
		pic_header->is_RLpic_flag = only_ref_libpic_flag;
	}
#endif
#endif

#ifdef ORI_CODE

	//pic_header->fixed_picture_qp_flag = param->p.pic_header_fixed_picture_qp_flag;
	//pic_header->picture_qp = param->p.pic_header_picture_qp;
#if CUDQP && PHASE_2_PROFILE
	pic_header->cu_delta_qp_flag = pic_header->cu_qp_group_size = pic_header->cu_qp_group_area_size = 0;
	if (!pic_header->fixed_picture_qp_flag && (sqh->profile_id == 0x32 || sqh->profile_id == 0x30)) {
		pic_header->cu_delta_qp_flag = param->p.pic_header_cu_delta_qp_flag;
		if (pic_header->cu_delta_qp_flag) {
			pic_header->cu_qp_group_size = param->p.pic_header_cu_qp_group_size;
			pic_header->cu_qp_group_area_size = pic_header->cu_qp_group_size * pic_header->cu_qp_group_size;
		}
	}
#endif
#if CABAC_MULTI_PROB
	if (pic_header->slice_type == SLICE_I) {
		mCabac_ws = MCABAC_SHIFT_I;
		mCabac_offset = (1 << (mCabac_ws - 1));
		counter_thr1 = 0;
		counter_thr2 = COUNTER_THR_I;
	} else if (pic_header->slice_type == SLICE_B) {
		mCabac_ws = MCABAC_SHIFT_B;
		mCabac_offset = (1 << (mCabac_ws - 1));
		counter_thr1 = 3;
		counter_thr2 = COUNTER_THR_B;
	} else {
		mCabac_ws = MCABAC_SHIFT_P;
		mCabac_offset = (1 << (mCabac_ws - 1));
		counter_thr1 = 3;
		counter_thr2 = COUNTER_THR_P;
	}
	if (sqh->mcabac_enable_flag) {
		g_compatible_back = 0;
	} else {
		g_compatible_back = 1;
	}
#endif
	//the reserved_bits only appears in inter_picture_header, so add the non-I-slice check
	if ( pic_header->slice_type != SLICE_I && !(pic_header->slice_type == SLICE_B && pic_header->picture_structure == 1)) {
		com_bsr_read1( bs ); // reserved_bits r(1)
	}
	dec_eco_DB_param(bs, pic_header
#if DBK_SCC || DBR
		, sqh
#endif
		);

	//pic_header->chroma_quant_param_disable_flag = (u8)param->p.pic_header_chroma_quant_param_disable_flag;
	if (pic_header->chroma_quant_param_disable_flag == 0) {
		pic_header->chroma_quant_param_delta_cb = (u8)com_bsr_read_se(bs);
		pic_header->chroma_quant_param_delta_cr = (u8)com_bsr_read_se(bs);
	} else {
		pic_header->chroma_quant_param_delta_cb = pic_header->chroma_quant_param_delta_cr = 0;
	}

	if (sqh->wq_enable) {
		pic_header->pic_wq_enable = param->p.pic_header_pic_wq_enable;
		if (pic_header->pic_wq_enable) {
			pic_header->pic_wq_data_idx = param->p.pic_header_pic_wq_data_idx;
			if (pic_header->pic_wq_data_idx == 0) {
				memcpy(pic_header->wq_4x4_matrix, sqh->wq_4x4_matrix, sizeof(sqh->wq_4x4_matrix));
				memcpy(pic_header->wq_8x8_matrix, sqh->wq_8x8_matrix, sizeof(sqh->wq_8x8_matrix));
			} else if (pic_header->pic_wq_data_idx == 1) {
				int delta, i;
				com_bsr_read1( bs ); //reserved_bits r(1)
				pic_header->wq_param = param->p.pic_header_wq_param;
				pic_header->wq_model = param->p.pic_header_wq_model;
				if (pic_header->wq_param == 0) {
					memcpy(pic_header->wq_param_vector, tab_wq_param_default[1], sizeof(pic_header->wq_param_vector));
				} else if (pic_header->wq_param == 1) {
					for (i = 0; i < 6; i++) {
						delta = com_bsr_read_se(bs);
						pic_header->wq_param_vector[i] = delta + tab_wq_param_default[0][i];
					}
				} else {
					for (i = 0; i < 6; i++) {
						delta = com_bsr_read_se(bs);
						pic_header->wq_param_vector[i] = delta + tab_wq_param_default[1][i];
					}
				}
				set_pic_wq_matrix_by_param(pic_header->wq_param_vector, pic_header->wq_model, pic_header->wq_4x4_matrix, pic_header->wq_8x8_matrix);
			} else {
				read_wq_matrix(bs, pic_header->wq_4x4_matrix, pic_header->wq_8x8_matrix);
			}
		} else {
			init_pic_wq_matrix(pic_header->wq_4x4_matrix, pic_header->wq_8x8_matrix);
		}
	} else {
		pic_header->pic_wq_enable = 0;
		init_pic_wq_matrix(pic_header->wq_4x4_matrix, pic_header->wq_8x8_matrix);
	}
#if ESAO
	if (sqh->esao_enable_flag) {
#if ESAO_PH_SYNTAX
		dec_eco_esao_param(bs, pic_header);
#else
		dec_eco_esao_pic_header(bs, pic_header);
#endif
	} else {
		memset(pic_header->pic_esao_on, 0, N_C * sizeof(int));
	}
#endif
#if CCSAO
	if (sqh->ccsao_enable_flag) {
		dec_eco_ccsao_pic_header(bs, pic_header);
#if CCSAO_PH_SYNTAX
		dec_eco_ccsao_param(pic_header, bs);
#endif
	} else {
		memset(pic_header->pic_ccsao_on, 0, (N_C-1) * sizeof(int));
	}
#endif
	/*ORI_CODE*/
#endif
	if (pic_header->tool_alf_on) {
		/* decode ALF flag and ALF coeff */
#if ALF_SHAPE
		int num_coef = (sqh->adaptive_filter_shape_enable_flag) ? ALF_MAX_NUM_COEF_SHAPE2 : ALF_MAX_NUM_COEF;
#endif

#if ALF_IMP
		int max_filter_num = (sqh->adaptive_filter_shape_enable_flag) ? NO_VAR_BINS:NO_VAR_BINS_16;
#endif

		dec_eco_alf_param(param, pic_header
#if ALF_SHAPE
			, num_coef
#endif
#if ALF_IMP
		,max_filter_num
#endif
		);
	} else {
		memset( pic_header->pic_alf_on, 0, N_C * sizeof( int ) );
	}
#ifdef ORI_CODE
	if (pic_header->slice_type != SLICE_I && sqh->affine_enable_flag) {
		//pic_header->affine_subblock_size_idx = param->p.pic_header_affine_subblock_size_idx;
	}
#if USE_IBC
#if PHASE_2_PROFILE
	if (sqh->ibc_flag) {
#endif
		pic_header->ibc_flag = (u8)param->p.pic_header_ibc_flag;
#if PHASE_2_PROFILE
	} else {
		pic_header->ibc_flag = 0;
	}
#endif
#endif
#if USE_SP
#if PHASE_2_PROFILE
	if (sqh->sp_enable_flag) {
#endif
		pic_header->sp_pic_flag = (u8)param->p.pic_header_sp_pic_flag;
#if PHASE_2_PROFILE
	} else {
		pic_header->sp_pic_flag = 0;
	}
#endif
#if EVS_UBVS_MODE
	pic_header->evs_ubvs_pic_flag = sqh->evs_ubvs_enable_flag;
#endif
#endif
#if FIMC
#if PHASE_2_PROFILE
	if (sqh->fimc_enable_flag) {
#endif
		pic_header->fimc_pic_flag = (u8)param->p.pic_header_fimc_pic_flag;
#if PHASE_2_PROFILE
	} else {
		pic_header->fimc_pic_flag = 0;
	}
#endif
#endif
#if ISTS
	if (sqh->ists_enable_flag) {
		pic_header->ph_ists_enable_flag = (u8)param->p.pic_header_ph_ists_enable_flag;
	}
#if PHASE_2_PROFILE
	else {
		pic_header->ph_ists_enable_flag = 0;
	}
#endif
#endif
#if TS_INTER
	if (sqh->ts_inter_enable_flag && pic_header->slice_type != SLICE_I) {
		pic_header->ph_ts_inter_enable_flag = (u8)param->p.pic_header_ph_ts_inter_enable_flag;
	}
#if PHASE_2_PROFILE
	else {
		pic_header->ph_ts_inter_enable_flag = 0;
	}
#endif
#endif
#if AWP
	if (sqh->awp_enable_flag) {
		pic_header->ph_awp_refine_flag = (u8)param->p.pic_header_ph_awp_refine_flag;
	}
#if PHASE_2_PROFILE
	else {
		pic_header->ph_awp_refine_flag = 0;
	}
#endif
#endif
#if UMVE_ENH
	if (sqh->umve_enh_enable_flag && pic_header->slice_type != SLICE_I) {
		pic_header->umve_set_flag = (u8)param->p.pic_header_umve_set_flag;
	} else {
		pic_header->umve_set_flag = 0;
	}
#endif
#if EPMC
	if (sqh->pmc_enable_flag) {
		pic_header->ph_epmc_model_flag = (u8)param->p.pic_header_ph_epmc_model_flag;
	} else {
		pic_header->ph_epmc_model_flag = 0;
	}
#endif
	/* byte align */
	ret = com_bsr_read1(bs);
	assert(ret == 1);
	while (!COM_BSR_IS_BYTE_ALIGN(bs)) {
		assert(com_bsr_read1(bs) == 0);
	}
	while (com_bsr_next(bs, 24) != 0x1) {
		assert(com_bsr_read(bs, 8) == 0);
	}
	/*ORI_CODE*/
#endif
	return COM_OK;
}

int dec_eco_patch_header(union param_u *param, COM_SQH *sqh, COM_PIC_HEADER * ph, COM_SH_EXT * sh,PATCH_INFO *patch)
{
	return COM_OK;
}

int dec_eco_alf_param(union param_u *rpm_param, COM_PIC_HEADER *sh
#if ALF_SHAPE
	, int num_coef
#endif
#if ALF_IMP
	, int max_filter_num
#endif
)
{
	ALF_PARAM **alf_picture_param = sh->alf_picture_param;
	int *pic_alf_on = sh->pic_alf_on;
	int compIdx;
	if (alf_picture_param == NULL) {
		pr_err("%s, alf_picture_param null\n", __func__);
		return COM_ERR;
	}

#if ALF_SHAPE
	alf_picture_param[Y_C]->num_coeff = num_coef;
	alf_picture_param[U_C]->num_coeff = num_coef;
	alf_picture_param[V_C]->num_coeff = num_coef;
#endif
#if ALF_IMP
	alf_picture_param[Y_C]->max_filter_num = max_filter_num;
	alf_picture_param[U_C]->max_filter_num = 1;
	alf_picture_param[V_C]->max_filter_num = 1;
#endif
	pic_alf_on[Y_C] = rpm_param->alf.picture_alf_enable_Y;
	pic_alf_on[U_C] = rpm_param->alf.picture_alf_enable_Cb;
	pic_alf_on[V_C] = rpm_param->alf.picture_alf_enable_Cr;

	alf_picture_param[Y_C]->alf_flag = pic_alf_on[Y_C];
	alf_picture_param[U_C]->alf_flag = pic_alf_on[U_C];
	alf_picture_param[V_C]->alf_flag = pic_alf_on[V_C];
	if (pic_alf_on[Y_C] || pic_alf_on[U_C] || pic_alf_on[V_C]) {
		for (compIdx = 0; compIdx < N_C; compIdx++) {
			if (pic_alf_on[compIdx]) {
				dec_eco_alf_coeff(rpm_param, alf_picture_param[compIdx]);
			}
		}
	}
	return COM_OK;
}

int dec_eco_alf_coeff(union param_u *rpm_param, ALF_PARAM *alf_param)
{
	int pos;
	int f = 0, symbol, pre_symbole;
	int32_t region_distance_idx = 0;
#if ALF_SHAPE
	const int num_coeff = alf_param->num_coeff;
#else
	const int num_coeff = (int)ALF_MAX_NUM_COEF;
#endif

	if (num_coeff > ALF_MAX_NUM_COEF)
	return COM_ERR;

	switch (alf_param->component_id) {
		case U_C:
		case V_C:
			for (pos = 0; pos < num_coeff; pos++) {
				if (alf_param->component_id == U_C)
					alf_param->coeff_multi[0][pos] = (int16_t)rpm_param->alf.alf_cb_coeffmulti[pos];
				else
					alf_param->coeff_multi[0][pos] = (int16_t)rpm_param->alf.alf_cr_coeffmulti[pos];
			}
			break;
		case Y_C:
			alf_param->filters_per_group = rpm_param->alf.alf_filters_num_m_1;
			alf_param->filters_per_group = alf_param->filters_per_group + 1;
#if ALF_IMP
			if (alf_param->max_filter_num == NO_VAR_BINS) {
				alf_param->dir_index = rpm_param->alf.dir_index;
			} else {
				assert(alf_param->max_filter_num == NO_VAR_BINS_16);
			}
#endif
			memset(alf_param->filter_pattern, 0, NO_VAR_BINS * sizeof(int));
			pre_symbole = 0;
			symbol = 0;
			for (f = 0; f < alf_param->filters_per_group; f++) {
				if (f > 0) {
#if ALF_IMP
					if (alf_param->filters_per_group != alf_param->max_filter_num)
#else
					if (alf_param->filters_per_group != NO_VAR_BINS)
#endif
					{
						symbol = rpm_param->alf.region_distance[region_distance_idx++];
					} else {
						symbol = 1;
					}
					alf_param->filter_pattern[symbol + pre_symbole] = 1;
					pre_symbole = symbol + pre_symbole;
				}
				for (pos = 0; pos < num_coeff; pos++) {
					alf_param->coeff_multi[f][pos] = (int16_t)rpm_param->alf.alf_y_coeffmulti[f][pos]; //com_bsr_read_se(bs);
				}
			}
			break;
		default:
			printf("Not a legal component ID\n");
			assert(0);
			//exit(-1);
			return -1;
	}
	return COM_OK;
}

