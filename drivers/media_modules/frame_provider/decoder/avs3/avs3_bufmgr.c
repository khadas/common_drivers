#ifndef CONFIG_AMLOGIC_MEDIA_MULTI_DEC
#define CONFIG_AMLOGIC_MEDIA_MULTI_DEC
#endif
#ifndef CONFIG_AMLOGIC_MEDIA_MULTI_DEC
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/sched/clock.h>
#include <linux/amlogic/media/canvas/canvas.h>
//#define printf printk
#endif
#include "avs3_global.h"
#define CONV_LOG2(v)                    (com_tbl_log2[v])

void set_livcdata_dec(DEC id_lib, LibVCData *libvc_data)
{
	DEC_CTX      * tmp_ctx = (DEC_CTX *)id_lib;
	tmp_ctx->dpm.libvc_data = libvc_data;
}

void init_libvcdata(LibVCData *libvc_data)
{
	int i, j;
	libvc_data->bits_dependencyFile = 0;
	libvc_data->bits_libpic = 0;

	libvc_data->library_picture_enable_flag = 0;
#if IPPPCRR
#if LIB_PIC_UPDATE
	libvc_data->lib_pic_update = 0;
	libvc_data->update = 0;
	libvc_data->countRL = 0;
	libvc_data->encode_skip = 0;
	libvc_data->end_of_intra_period = 0;
#else
	libvc_data->first_pic_as_libpic = 0;
#endif
#endif
#if CRR_ENC_OPT_CFG
	libvc_data->lib_in_l0 = 2;
	libvc_data->lib_in_l1 = 0;
	libvc_data->pb_ref_lib = 0;
	libvc_data->rl_ref_lib = 2;
	libvc_data->max_list_refnum = 2;
	libvc_data->libpic_idx = -1;
#endif
	libvc_data->is_libpic_processing = 0;
	libvc_data->is_libpic_prepared = 0;

	libvc_data->num_candidate_pic = 0;
	libvc_data->num_lib_pic = 0;
	libvc_data->num_RLpic = 0;

	libvc_data->num_libpic_outside = 0;

	for (i = 0; i < MAX_CANDIDATE_PIC; i++)
	{
		libvc_data->list_poc_of_candidate_pic[i] = -1;
		libvc_data->list_candidate_pic[i] = NULL;

		libvc_data->list_hist_feature_of_candidate_pic[i].num_component = 0;
		libvc_data->list_hist_feature_of_candidate_pic[i].num_of_hist_interval = 0;
		libvc_data->list_hist_feature_of_candidate_pic[i].length_of_interval = 0;
		for (j = 0; j < MAX_NUM_COMPONENT; j++)
		{
		libvc_data->list_hist_feature_of_candidate_pic[i].list_hist_feature[j] = NULL;
		}

		libvc_data->list_poc_of_RLpic[i] = -1;
		libvc_data->list_libidx_for_RLpic[i] = -1;

	}
	for (i = 0; i < MAX_NUM_LIBPIC; i++)
	{
		libvc_data->list_poc_of_libpic[i] = -1;
		libvc_data->list_libpic_outside[i] = NULL;
		libvc_data->list_library_index_outside[i] = -1;
	}
}

static const s8 com_tbl_log2[257] =
{
	/* 0, 1 */
	-1, -1,
		/* 2, 3 */
		1, -1,
		/* 4 ~ 7 */
		2, -1, -1, -1,
		/* 8 ~ 15 */
		3, -1, -1, -1, -1, -1, -1, -1,
		/* 16 ~ 31 */
		4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		/* 31 ~ 63 */
		5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		/* 64 ~ 127 */
		6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		/* 128 ~ 255 */
		7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		/* 256 */
		8
	};

COM_PIC *dec_pull_frm(DEC_CTX *ctx, int state)
{
	int ret;
	COM_PIC *pic = NULL;
	int library_picture_index;
	//*imgb = NULL;
#if LIBVC_ON
	// output lib pic and corresponding library_picture_index
	if (ctx->info.sqh.library_stream_flag)
	{
		pic = ctx->pic;
		library_picture_index = ctx->info.pic_header.library_picture_index;

		//output to the buffer outside the decoder
		ret = com_picman_out_libpic(pic, library_picture_index, &ctx->dpm);
		if (pic)
		{
		printf("%s output index %d\n", __func__, pic->buf_cfg.index);
		//com_assert_rv(pic->imgb != NULL, COM_ERR);
		//pic->imgb->addref(pic->imgb);
		//*imgb = pic->imgb;
		}
		//Don't output the reconstructed libpics to yuv. Because lib pic should not display.

		return pic;
	}
	else
#endif
	{
		pic = com_picman_out_pic( &ctx->dpm, &ret, ctx->info.pic_header.decode_order_index, state ); //MX: doi is not increase mono, but in the range of [0,255]
		if (pic)
		{
		printf("%s output index %d\n", __func__, pic->buf_cfg.index);
		//com_assert_rv(pic->imgb != NULL, COM_ERR);
		/* increase reference count */
		//pic->imgb->addref(pic->imgb);
		//*imgb = pic->imgb;
		}
		return pic;
	}
}

static void make_stat(DEC_CTX * ctx, int btype, DEC_STAT * stat)
{
	int i, j;
	stat->read = 0;
	stat->ctype = btype;
	stat->stype = 0;
	stat->fnum = -1;
#if LIBVC_ON
	stat->is_RLpic_flag = ctx->info.pic_header.is_RLpic_flag;
#endif
	if (ctx)
	{
		//stat->read = COM_BSR_GET_READ_BYTE(&ctx->bs);
		if (btype == COM_CT_SLICE)
		{
		stat->fnum = ctx->pic_cnt;
		stat->stype = ctx->info.pic_header.slice_type;
		/* increase decoded picture count */
		ctx->pic_cnt++;
		stat->poc = ctx->ptr;
		for (i = 0; i < 2; i++)
		{
			stat->refpic_num[i] = ctx->dpm.num_refp[i];
			for (j = 0; j < stat->refpic_num[i]; j++)
			{
#if LIBVC_ON
			stat->refpic[i][j] = ctx->refp[j][i].pic->ptr;
#else
			stat->refpic[i][j] = ctx->refp[j][i].ptr;
#endif
			}
		}
		}
		else if (btype == COM_CT_PICTURE)
		{
		stat->fnum = -1;
		stat->stype = ctx->info.pic_header.slice_type;
		stat->poc = ctx->info.pic_header.dtr;
		for (i = 0; i < 2; i++)
		{
			stat->refpic_num[i] = ctx->dpm.num_refp[i];
			for (j = 0; j < stat->refpic_num[i]; j++)
			{
#if LIBVC_ON
			stat->refpic[i][j] = ctx->refp[j][i].pic->ptr;
#else
			stat->refpic[i][j] = ctx->refp[j][i].ptr;
#endif
			}
		}
		}
	}

#if PRINT_SQH_PARAM_DEC
#if PHASE_2_PROFILE
	stat->profile_id = ctx->info.sqh.profile_id;
#endif
	stat->internal_bit_depth = ctx->info.sqh.encoding_precision == 2 ? 10 : 8;;
#if ENHANCE_TSPCM
	stat->intra_tools = (ctx->info.sqh.tscpm_enable_flag << 0) + (ctx->info.sqh.enhance_tscpm_enable_flag << 1) +
		(ctx->info.sqh.ipf_enable_flag << 2) + (ctx->info.sqh.dt_intra_enable_flag << 3) + (ctx->info.sqh.ipcm_enable_flag << 4);
#if MIPF
	stat->intra_tools += (ctx->info.sqh.mipf_enable_flag << 5);
#endif
#else
	stat->intra_tools = (ctx->info.sqh.tscpm_enable_flag << 0) + (ctx->info.sqh.ipf_enable_flag << 1) + (ctx->info.sqh.dt_intra_enable_flag << 2) + (ctx->info.sqh.ipcm_enable_flag << 3);
#if MIPF
	stat->intra_tools += (ctx->info.sqh.mipf_enable_flag << 4);
#endif
#endif

#if PMC
	stat->intra_tools += (ctx->info.sqh.pmc_enable_flag << 6);
#endif

#if IPF_CHROMA
	stat->intra_tools += (ctx->info.sqh.chroma_ipf_enable_flag << 7);
#endif
#if IIP
	stat->intra_tools += (ctx->info.sqh.iip_enable_flag << 8);
#endif
	stat->inter_tools = (ctx->info.sqh.affine_enable_flag << 0) + (ctx->info.sqh.amvr_enable_flag << 1) + (ctx->info.sqh.umve_enable_flag << 2) + (ctx->info.sqh.emvr_enable_flag << 3);
	stat->inter_tools+= (ctx->info.sqh.smvd_enable_flag << 4) + (ctx->info.sqh.num_of_hmvp_cand << 10);
#if AWP
	stat->inter_tools += ctx->info.sqh.awp_enable_flag << 5;
#endif
	stat->trans_tools = (ctx->info.sqh.secondary_transform_enable_flag << 0) + (ctx->info.sqh.position_based_transform_enable_flag << 1);

	stat->filte_tools = (ctx->info.sqh.sample_adaptive_offset_enable_flag << 0) + (ctx->info.sqh.adaptive_leveling_filter_enable_flag << 1);

	stat->scc_tools   = 0;
#if FIMC
	stat->scc_tools  += (ctx->info.sqh.fimc_enable_flag << 0);
#endif
#if IBC_BVP
	stat->scc_tools += (ctx->info.sqh.num_of_hbvp_cand << 1);
#endif
#endif
}

static void sequence_deinit(DEC_CTX * ctx)
{
	com_picman_deinit(&ctx->dpm);
}

static int sequence_init(DEC_CTX * ctx, COM_SQH * sqh, int max_pb_size)
{
	int size;
	int ret;

	ctx->info.bit_depth_internal = (sqh->encoding_precision == 2) ? 10 : 8;
	assert(sqh->sample_precision == 1 || sqh->sample_precision == 2);
	ctx->info.bit_depth_input = (sqh->sample_precision == 1) ? 8 : 10;
	ctx->info.qp_offset_bit_depth = (8 * (ctx->info.bit_depth_internal - 8));

	sequence_deinit(ctx);
	ctx->info.pic_width  = ((sqh->horizontal_size + MINI_SIZE - 1) / MINI_SIZE) * MINI_SIZE;
	ctx->info.pic_height = ((sqh->vertical_size   + MINI_SIZE - 1) / MINI_SIZE) * MINI_SIZE;
	ctx->info.max_cuwh = 1 << sqh->log2_max_cu_width_height;
	ctx->info.log2_max_cuwh = CONV_LOG2(ctx->info.max_cuwh);

	size = ctx->info.max_cuwh;
	ctx->info.pic_width_in_lcu = (ctx->info.pic_width + (size - 1)) / size;
	ctx->info.pic_height_in_lcu = (ctx->info.pic_height + (size - 1)) / size;
	ctx->info.f_lcu = ctx->info.pic_width_in_lcu * ctx->info.pic_height_in_lcu;
	ctx->info.pic_width_in_scu = (ctx->info.pic_width + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
	ctx->info.pic_height_in_scu = (ctx->info.pic_height + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
	ctx->info.f_scu = ctx->info.pic_width_in_scu * ctx->info.pic_height_in_scu;
#if ASP
	ctx->info.skip_me_asp = FALSE;
	ctx->info.skip_umve_asp = FALSE;
#endif

	ctx->pa.width = ctx->info.pic_width;
	ctx->pa.height = ctx->info.pic_height;
	ctx->pa.pad_l = PIC_PAD_SIZE_L;
	ctx->pa.pad_c = PIC_PAD_SIZE_C;
	ret = com_picman_init(&ctx->dpm, max_pb_size, MAX_NUM_REF_PICS, &ctx->pa);
	com_assert_g(COM_SUCCEEDED(ret), ERR);

	/*
	...
	*/

	ctx->info.pic_header.tool_alf_on = ctx->info.sqh.adaptive_leveling_filter_enable_flag;
#if ALF_SHAPE || ALF_IMP
	ctx->info.pic_header.tool_alf_shape_on = ctx->info.sqh.adaptive_filter_shape_enable_flag;
#endif
	create_alf_global_buffer(ctx);
	ctx->info.pic_header.pic_alf_on = ctx->pic_alf_on;

	return COM_OK;
ERR:
	sequence_deinit(ctx);
	ctx->init_flag = 0;
	return ret;
}

int dec_cnk(DEC_CTX * ctx, DEC_STAT * stat, unsigned char start_code,
	union param_u *param, int max_pb_size)
{
	COM_PIC_HEADER   *pic_header;
	COM_SQH * sqh;
	COM_SH_EXT *shext;
	COM_CNKH *cnkh;
	int        ret = COM_OK;
	int i;
	PRINT_LINE();
	if (stat)
	{
		com_mset(stat, 0, sizeof(DEC_STAT));
	}
	sqh = &ctx->info.sqh;
	pic_header = &ctx->info.pic_header;
	shext = &ctx->info.shext;
	cnkh = &ctx->info.cnkh;

	/* set error status */
#ifdef TRACE_RDO_EXCLUDE_I
#if TRACE_RDO_EXCLUDE_I
	if (pic_header->slice_type != SLICE_I)
	{
#endif
#endif
		COM_TRACE_SET(1);
#ifdef TRACE_RDO_EXCLUDE_I
#if TRACE_RDO_EXCLUDE_I
	}
	else
	{
		COM_TRACE_SET(0);
	}
#endif
#endif
	/* bitstream reader initialization */
	//com_bsr_init(bs, bitb->addr, bitb->ssize, NULL);
	//SET_SBAC_DEC(bs, &ctx->sbac_dec);

	//start_store_param("", bs->cur[3]);
	if (start_code == 0xB0)
	{
		cnkh->ctype = COM_CT_SQH;
		ret = dec_eco_sqh(param, sqh);
		PRINT_LINE();
		com_assert_rv(COM_SUCCEEDED(ret), ret);
		PRINT_LINE();
#if LIBVC_ON
		ctx->dpm.libvc_data->is_libpic_processing = sqh->library_stream_flag;
		ctx->dpm.libvc_data->library_picture_enable_flag = sqh->library_picture_enable_flag;
		/*
		if (ctx->dpm.libvc_data->library_picture_enable_flag && !ctx->dpm.libvc_data->is_libpic_prepared)
		{
		ret = COM_ERR_UNEXPECTED;
		printf("\nError: when decode seq.bin with library picture enable, you need to input libpic.bin at the same time by using param: --input_libpics.");
		com_assert_rv(ctx->dpm.libvc_data->library_picture_enable_flag == ctx->dpm.libvc_data->is_libpic_prepared, ret);
		}
	 */
#endif
		PRINT_LINE();

#if EXTENSION_USER_DATA
		//extension_and_user_data(ctx, param, 0, sqh, pic_header);
#endif
		if ( !ctx->init_flag )
		{
		PRINT_LINE();
		ret = sequence_init(ctx, sqh, max_pb_size);
		PRINT_LINE();
		com_assert_rv(COM_SUCCEEDED(ret), ret);
		//g_DOIPrev = g_CountDOICyCleTime = 0;
		PRINT_LINE();
		ctx->init_flag = 1;
		}
	}
	else if ( start_code == 0xB1 )
	{
		ctx->init_flag = 0;
		cnkh->ctype = COM_CT_SEQ_END;
	}
	else if (start_code == 0xB3 || start_code == 0xB6)
	{
		int need_minus_256 = 0;
		PRINT_LINE();
		cnkh->ctype = COM_CT_PICTURE;
		if (ctx->init_flag != 1)
			return COM_ERR_MALFORMED_BITSTREAM;
		/* decode slice header */
		pic_header->low_delay = sqh->low_delay;
		ret = dec_eco_pic_header(param, pic_header, sqh, &need_minus_256, start_code);
		if (need_minus_256)
		{
		com_picman_dpbpic_doi_minus_cycle_length( &ctx->dpm );
		}

		ctx->wq[0] = pic_header->wq_4x4_matrix;
		ctx->wq[1] = pic_header->wq_8x8_matrix;

		if (!sqh->library_stream_flag)
		{
		com_picman_check_repeat_doi(&ctx->dpm, pic_header);
		}

#if EXTENSION_USER_DATA && WRITE_MD5_IN_USER_DATA
		//extension_and_user_data(ctx, param, 1, sqh, pic_header);
#endif
		com_construct_ref_list_doi(pic_header);

		//add by Yuqun Fan, init rpl list at ph instead of sh
#if HLS_RPL
#if LIBVC_ON
		if (!sqh->library_stream_flag)
#endif
		{
		ret = com_picman_refpic_marking_decoder(&ctx->dpm, pic_header);
		if (avs3_get_error_policy() & 0x4)
			com_assert_rv(ret == COM_OK, ret);
		}
		com_cleanup_useless_pic_buffer_in_pm(&ctx->dpm);

		/* reference picture lists construction */
		ret = com_picman_refp_rpl_based_init_decoder(&ctx->dpm, pic_header, ctx->refp);

#if AWP
		if (ctx->info.pic_header.slice_type == SLICE_P || ctx->info.pic_header.slice_type == SLICE_B)
		{
		for (i = 0; i < ctx->dpm.num_refp[REFP_0]; i++)
		{
			ctx->info.pic_header.ph_poc[REFP_0][i] = ctx->refp[i][REFP_0].ptr;
		}
		}

		if (ctx->info.pic_header.slice_type == SLICE_B)
		{
		for (i = 0; i < ctx->dpm.num_refp[REFP_1]; i++)
		{
			ctx->info.pic_header.ph_poc[REFP_1][i] = ctx->refp[i][REFP_1].ptr;
		}
		}
#endif
#else
		/* initialize reference pictures */
		//ret = com_picman_refp_init(&ctx->dpm, ctx->info.sqh.num_ref_pics_act, sh->slice_type, ctx->ptr, ctx->info.sh.temporal_id, ctx->last_intra_ptr, ctx->refp);
#endif
		if (avs3_get_error_policy() & 0x4) {
			com_assert_rv(COM_SUCCEEDED(ret), ret);
		} else if (ret != 0) {
			ret = 0;
			goto NEW_PICTURE;
		}
#ifdef ORI_CODE
	}
	else if (start_code >= 0x00 && start_code <= 0x8E)
	{
#endif
		cnkh->ctype = COM_CT_SLICE;
		ret = dec_eco_patch_header(param, sqh, pic_header, shext, ctx->patch);
		/* initialize slice */
#ifdef ORI_CODE
		ret = slice_init(ctx, ctx->core, pic_header);
#else
		ctx->dtr_prev_low = pic_header->dtr;
		ctx->dtr = pic_header->dtr;
		ctx->ptr = pic_header->dtr; /* PTR */
#endif
		com_assert_rv(COM_SUCCEEDED(ret), ret);
#if 1
		if (is_avs3_print_bufmgr_detail())
			com_picman_print_state(&ctx->dpm);
		if (pic_header->rpl_l0.ref_pic_active_num > 0) {
		char tmpbuf[128];
		int pos = 0;
		for (i = 0; i < pic_header->rpl_l0.ref_pic_active_num; i++)
			pos += sprintf(&tmpbuf[pos], "%d ", ctx->refp[i][REFP_0].ptr);
		printf("rpl_l0 num %d: %s\n", pic_header->rpl_l0.ref_pic_active_num, tmpbuf);
		}
		if (pic_header->rpl_l1.ref_pic_active_num > 0) {
		char tmpbuf[128];
		int pos = 0;
		for (i = 0; i < pic_header->rpl_l1.ref_pic_active_num; i++)
			pos += sprintf(&tmpbuf[pos], "%d ", ctx->refp[i][REFP_1].ptr);
		printf("rpl_l1 num %d: %s\n", pic_header->rpl_l1.ref_pic_active_num, tmpbuf);
		}
#endif
NEW_PICTURE:
		/* get available frame buffer for decoded image */
		ctx->pic = com_picman_get_empty_pic(&ctx->dpm, &ret);
		com_assert_rv(ctx->pic, ret);
		/* get available frame buffer for decoded image */
		ctx->map.map_refi = ctx->pic->map_refi;
		ctx->map.map_mv = ctx->pic->map_mv;
		/* decode slice layer */

		//ret = dec_pic(ctx, ctx->core, sqh, pic_header, shext);

#if 0
/*AML*/
/*put to post_process*/
		/* put decoded picture to DPB */
#if LIBVC_ON
		if (sqh->library_stream_flag)
		{
		ret = com_picman_put_libpic(&ctx->dpm, ctx->pic, ctx->info.pic_header.slice_type, ctx->ptr, pic_header->decode_order_index, ctx->info.pic_header.temporal_id, 1, ctx->refp, pic_header);
		}
		else
#endif
		{
		ret = com_picman_put_pic(&ctx->dpm, ctx->pic, ctx->info.pic_header.slice_type, ctx->ptr, pic_header->decode_order_index,
				         pic_header->picture_output_delay, ctx->info.pic_header.temporal_id, 1, ctx->refp);
		assert((&ctx->dpm)->cur_pb_size <= sqh->max_dpb_size);
		}
#endif
	}
	else
	{
		//stop_store_param();
		return COM_ERR_MALFORMED_BITSTREAM;
	}
		PRINT_LINE();
	make_stat(ctx, cnkh->ctype, stat);
		PRINT_LINE();
	//stop_store_param();
	return ret;
}

void allocate_alf_param(ALF_PARAM **alf_param, int comp_idx
#if ALF_SHAPE
			, int num_coef
#endif
)
{
	//*alf_param = (ALF_PARAM *)malloc(sizeof(ALF_PARAM));
	(*alf_param)->alf_flag = 0;
#if ALF_SHAPE
	(*alf_param)->num_coeff = num_coef;
#else
	(*alf_param)->num_coeff = ALF_MAX_NUM_COEF;
#endif
	(*alf_param)->filters_per_group = 1;
#if ALF_IMP
	(*alf_param)->dir_index = 0;
	(*alf_param)->max_filter_num = (comp_idx == Y_C) ? NO_VAR_BINS : 1;
#endif
	(*alf_param)->component_id = comp_idx;

#ifdef ORI_CODE
	(*alf_param)->coeff_multi = NULL;
	(*alf_param)->filter_pattern = NULL;
	switch (comp_idx)
	{
	case Y_C:
#if ALF_SHAPE
		get_mem_2D_int(&((*alf_param)->coeff_multi), NO_VAR_BINS, num_coef);
#else
		get_mem_2D_int(&((*alf_param)->coeff_multi), NO_VAR_BINS, ALF_MAX_NUM_COEF);
#endif
		get_mem_1D_int(&((*alf_param)->filter_pattern), NO_VAR_BINS);
		break;
	case U_C:
	case V_C:
#if ALF_SHAPE
		get_mem_2D_int(&((*alf_param)->coeff_multi), 1, num_coef);
#else
		get_mem_2D_int(&((*alf_param)->coeff_multi), 1, ALF_MAX_NUM_COEF);
#endif
		break;
	default:
		printf("Not a legal component ID\n");
		assert(0);
		exit(-1);
	}
#endif
}

void create_alf_global_buffer(DEC_CTX *ctx)
{
	int i;
#if ALF_SHAPE
	int num_coef = (ctx->info.sqh.adaptive_filter_shape_enable_flag) ? ALF_MAX_NUM_COEF_SHAPE2 : ALF_MAX_NUM_COEF;
#endif
	for (i = 0; i < N_C; i++)
	{
		allocate_alf_param(&ctx->info.pic_header.alf_picture_param[i], i
#if ALF_SHAPE
				, num_coef
#endif
		);
	}
}

void avs3_bufmgr_init(struct avs3_decoder *hw)
{
	int i;
	//DEC_CTX *ctx = &hw->ctx;
	hw->ctx.dpm.hw = hw;
	for (i = 0; i < NUM_ALF_COMPONENT; i++)
		hw->p_alfPictureParam[i] = &hw->m_alfPictureParam[i];
	hw->ctx.info.pic_header.alf_picture_param = &hw->p_alfPictureParam[0];

	init_pic_pool(hw);

	init_libvcdata(&hw->libvc_data);
	set_livcdata_dec(&hw->ctx, &hw->libvc_data);
}

int avs3_param_error_check(union param_u *param)
{
	if (param->p.sqh_max_dpb_size > MAX_PB_SIZE) {
		pr_err("%s, param->p.sqh_max_dpb_size %d error\n", __func__, param->p.sqh_max_dpb_size);
		return COM_ERR;
	}

	if ((param->p.pic_header_rpl_l0_ref_pic_num > MAX_NUM_REF_PICS) ||
		(param->p.pic_header_rpl_l1_ref_pic_num > MAX_NUM_REF_PICS)) {
		pr_info("%s, rpl_ref_pic_num %d, %d error\n", __func__,
		param->p.pic_header_rpl_l0_ref_pic_num,
		param->p.pic_header_rpl_l1_ref_pic_num);
		return COM_ERR;
	}

	return COM_OK;
}

int avs3_bufmgr_process(struct avs3_decoder *hw, int start_code)
{
	int ret;
	COM_PIC *pic;
	DEC_CTX *ctx = &hw->ctx;
	int i;
	printf("%s start_code 0x%x\n", __func__, start_code);

	if (avs3_param_error_check(&hw->param))
		return COM_ERR;

	ret = dec_cnk(&hw->ctx, &hw->stat, start_code, &hw->param,
		hw->max_pb_size);
	if (start_code == SEQUENCE_HEADER_CODE) {
		hw->lcu_size = hw->ctx.info.max_cuwh;
		hw->lcu_size_log2 = hw->ctx.info.log2_max_cuwh;
		hw->lcu_x_num = hw->ctx.info.pic_width_in_lcu;
		hw->lcu_y_num = hw->ctx.info.pic_height_in_lcu;
		hw->lcu_total = hw->ctx.info.f_lcu;
		printf("lcu_size %d lcu_size_log2 %d lcu_total %d\n", hw->lcu_size, hw->lcu_size_log2, hw->lcu_total);
	} else if (start_code == I_PICTURE_START_CODE || start_code == PB_PICTURE_START_CODE) {
		//hw->input.sample_bit_depth = hw->ctx.info.bit_depth_input;
		hw->input.sample_bit_depth = hw->ctx.info.bit_depth_internal;
		hw->input.alf_enable = ctx->info.sqh.adaptive_leveling_filter_enable_flag;
		for (i = 0; i < NUM_ALF_COMPONENT; i++)
		hw->img.pic_alf_on[i] = hw->ctx.pic_alf_on[i];
		hw->img.width = hw->ctx.pa.width;
		hw->img.height = hw->ctx.pa.height;
		hw->slice_type = hw->ctx.info.pic_header.slice_type;
		pic = ctx->pic;
		if (pic && (!ret)) {
		hw->cur_pic = &pic->buf_cfg;
			printf("set refpic before cur_pic index %d, pic %p L0 num:%d, L1 num:%d\n",
				hw->cur_pic->index, hw->cur_pic, hw->cur_pic->list0_num_refp, hw->cur_pic->list1_num_refp);
		hw->cur_pic->list0_num_refp = hw->ctx.dpm.num_refp[REFP_0];
		for (i = 0; i < hw->ctx.dpm.num_refp[REFP_0]; i++)
			hw->cur_pic->list0_ptr[i] = hw->ctx.refp[i][REFP_0].ptr;
#ifdef NEW_FRONT_BACK_CODE
		for (i = 0; i < hw->cur_pic->list0_num_refp; i++)
			hw->cur_pic->list0_index[i] = hw->ctx.refp[i][REFP_0].pic->buf_cfg.index;

		hw->cur_pic->list1_num_refp = hw->ctx.dpm.num_refp[REFP_1];
		for (i = 0; i < hw->cur_pic->list1_num_refp; i++)
			hw->cur_pic->list1_index[i] = hw->ctx.refp[i][REFP_1].pic->buf_cfg.index;
#endif
			printf("set refpic after cur_pic index %d, pic %p L0 num:%d, L1 num:%d\n",
				hw->cur_pic->index, hw->cur_pic, hw->cur_pic->list0_num_refp, hw->cur_pic->list1_num_refp);
		}
		//Read_ALF_param(hw);
	}
	return ret;
}

int check_poc_in_dpb(struct avs3_decoder *hw, int poc)
{
	COM_PIC * pic;
	int i = 0;

	for (i = 0; i < hw->max_pb_size; i++) {
		pic = &hw->pic_pool[i];
		if ((pic != NULL) && (pic->buf_cfg.used) &&
			(poc == pic->ptr) && (!(avs3_get_error_policy() & 0x8))) {
			return true;
		}
	}

	return false;
}

int avs3_bufmgr_post_process(struct avs3_decoder *hw)
{
	DEC_CTX *ctx = &hw->ctx;
	DEC_STAT *stat = &hw->stat;
	COM_SQH *sqh =  &ctx->info.sqh;
	COM_PIC_HEADER   *pic_header = &ctx->info.pic_header;
	COM_CNKH *cnkh = &ctx->info.cnkh;
	//COM_IMGB *imgb;
	int        ret = COM_OK;

	if ((ctx->pic != NULL) && !((ctx->pic->buf_cfg.in_dpb == 0)
		&& (ctx->pic->buf_cfg.used == 1)))
		return ret;

	if (check_poc_in_dpb(hw, ctx->ptr)) {
		ctx->pic->buf_cfg.used = 0;
		return 2;
	}

	ctx->pic->buf_cfg.in_dpb = true;
	if (stat)
	{
		com_mset(stat, 0, sizeof(DEC_STAT));
	}

	/* put decoded picture to DPB */
#if LIBVC_ON
	if (sqh->library_stream_flag)
	{
		ret = com_picman_put_libpic(&ctx->dpm, ctx->pic, ctx->info.pic_header.slice_type, ctx->ptr, pic_header->decode_order_index, ctx->info.pic_header.temporal_id, 1, ctx->refp, pic_header);
	}
	else
#endif
	{
		ret = com_picman_put_pic(&ctx->dpm, ctx->pic, ctx->info.pic_header.slice_type, ctx->ptr, pic_header->decode_order_index,
				     pic_header->picture_output_delay, ctx->info.pic_header.temporal_id, 1, ctx->refp);
#ifdef NEW_FRONT_BACK_CODE
		if ((&ctx->dpm)->cur_pb_size > sqh->max_dpb_size)
		printf("!!! (&ctx->dpm)->cur_pb_size %d > sqh->max_dpb_size %d\n",
			(&ctx->dpm)->cur_pb_size, sqh->max_dpb_size);
#else
		assert((& ctx->dpm)->cur_pb_size <= sqh->max_dpb_size);
#endif
	}
	cnkh->ctype = COM_CT_SLICE;

	make_stat(ctx, cnkh->ctype, stat);
	printf("### pic_cnt %d cur_num_ref_pics %d\n", ctx->pic_cnt, ctx->dpm.cur_num_ref_pics);
#if 0
	if (hw->stat.fnum >= 0)
	{
		//if (aml_print_header_info) printf(" # fnum : %d state : %d\n", stat.fnum, state);
		COM_PIC *pic = dec_pull_frm(&hw->ctx, &imgb, 0);
		/*
		if (ret == COM_ERR_UNEXPECTED)
		{
		//v1print("bumping process completed\n");
		//printf("COM_ERR_UNEXPECTED\n");
		//ret = -1;
		}
		else if (COM_FAILED(ret))
		{
		//v0print("failed to pull the decoded image\n");
		printf("failed to pull the decoded image\n");
		ret = -1;
		}*/
	}
	else
	{
		imgb = NULL;
	}
#endif
#if 0
	if (imgb)
	{
		width = imgb->width[0];
		height = imgb->height[0];
		if (op_flag[OP_FLAG_FNAME_OUT])
		{
		write_dec_img(id, op_fname_out, imgb, ((DEC_CTX *)id)->info.bit_depth_internal);
		}
		imgb->release(imgb);
		pic_cnt++;
	}
#endif
	return ret;

}

void avs3_cleanup_useless_pic_buffer_in_pm(struct avs3_decoder *hw)
{
	com_cleanup_useless_pic_buffer_in_pm(&hw->ctx.dpm);
}

COM_PIC * com_pic_alloc(struct avs3_decoder *hw, PICBUF_ALLOCATOR * pa, int * ret)
{
	COM_PIC * pic = NULL;
	int i;
	for (i = 0; i < hw->max_pb_size; i++) {
		if (hw->pic_pool[i].buf_cfg.used == 0) {
		break;
		}
	}
	if (i < hw->max_pb_size) {
		avs3_frame_t pic_cfg;
		pic = &hw->pic_pool[i];
		memcpy(&pic_cfg, &pic->buf_cfg, sizeof(avs3_frame_t));
		memset(pic, 0, sizeof(COM_PIC));
		pic->width_luma = pa->width;
		pic->height_luma = pa->height;
		memcpy(&pic->buf_cfg, &pic_cfg, sizeof(avs3_frame_t));
		pic->buf_cfg.used = 1;
		pic->buf_cfg.slice_type = hw->ctx.info.pic_header.slice_type;
#ifdef AML
		pic->buf_cfg.error_mark = 0;
		pic->buf_cfg.vf_ref = 0;
		pic->buf_cfg.backend_ref = 0;
		pic->buf_cfg.in_dpb = false;
		pic->buf_cfg.time = div64_u64(local_clock(), 1000) - hw->start_time;
		pic->buf_cfg.decoded_lcu = 0;
#ifdef NEW_FB_CODE
		pic->buf_cfg.back_done_mark = 1;
#endif
#endif

	}
	if (pic)
		printf("%s: pic index %d\n", __func__, pic->buf_cfg.index);
	else
		printf("%s: ret NULL\n", __func__);
	return pic;
	//return com_picbuf_alloc(pa->width, pa->height, pa->pad_l, pa->pad_c, ret);
}

void com_pic_free(struct avs3_decoder *hw, PICBUF_ALLOCATOR *pa, COM_PIC *pic)
{
	pic->buf_cfg.used = 0;
	pic->buf_cfg.in_dpb = 0;
	printf("%s: pic index %d\n", __func__, pic->buf_cfg.index);
}

void init_pic_pool(struct avs3_decoder *hw)
{
	int i;
	COM_PIC * pic;
	for (i = 0; i < MAX_PB_SIZE; i++) {
		pic = &hw->pic_pool[i];
		memset(pic, 0, sizeof (COM_PIC));
		if (i < hw->max_pb_size) {
		pic->buf_cfg.used = 0;
		pic->buf_cfg.index = i;
		} else {
		pic->buf_cfg.used = -1;
		pic->buf_cfg.index = -1;
		}
	}

}

#if 0
void readAlfCoeff(struct avs3_decoder *avs3_dec, ALFParam *Alfp)
{
	int32_t pos;
	union param_u *rpm_param = &avs3_dec->param;

	int32_t f = 0, symbol, pre_symbole;
	const int32_t numCoeff = (int32_t)ALF_MAX_NUM_COEF;

	switch (Alfp->componentID) {
	case ALF_Cb:
	case ALF_Cr: {
		for (pos = 0; pos < numCoeff; pos++) {
		if (Alfp->componentID == ALF_Cb)
			Alfp->coeffmulti[0][pos] = rpm_param->alf.alf_cb_coeffmulti[pos];
		else
			Alfp->coeffmulti[0][pos] = rpm_param->alf.alf_cr_coeffmulti[pos];
#if Check_Bitstream
		if (pos <= 7)
			assert( Alfp->coeffmulti[0][pos]>=-64&& Alfp->coeffmulti[0][pos]<=63);
		if (pos == 8)
			assert( Alfp->coeffmulti[0][pos]>=-1088&& Alfp->coeffmulti[0][pos]<=1071);
#endif
		}
	}
	break;
	case ALF_Y: {
		int32_t region_distance_idx = 0;
		Alfp->filters_per_group = rpm_param->alf.alf_filters_num_m_1;
#if Check_Bitstream
		assert(Alfp->filters_per_group >= 0&&Alfp->filters_per_group <= 15);
#endif
		Alfp->filters_per_group = Alfp->filters_per_group + 1;

		memset(Alfp->filterPattern, 0, NO_VAR_BINS * sizeof(int32_t));
		pre_symbole = 0;
		symbol = 0;
		for (f = 0; f < Alfp->filters_per_group; f++) {
		if (f > 0) {
			if (Alfp->filters_per_group != 16) {
			symbol = rpm_param->alf.region_distance[region_distance_idx++];
			} else {
			symbol = 1;
			}
			Alfp->filterPattern[symbol + pre_symbole] = 1;
			pre_symbole = symbol + pre_symbole;
		}

		for (pos = 0; pos < numCoeff; pos++) {
			Alfp->coeffmulti[f][pos] = rpm_param->alf.alf_y_coeffmulti[f][pos];
#if Check_Bitstream
			if (pos <= 7)
			assert( Alfp->coeffmulti[f][pos]>=-64&& Alfp->coeffmulti[f][pos]<=63);
			if (pos == 8)
			assert( Alfp->coeffmulti[f][pos]>=-1088&& Alfp->coeffmulti[f][pos]<=1071);
#endif

		}
		}

#if Check_Bitstream
		assert(pre_symbole >= 0&&pre_symbole <= 15);

#endif
	}
	break;
	default: {
		printf("Not a legal component ID\n");
		assert(0);
		exit(-1);
	}
	}
}

void Read_ALF_param(struct avs3_decoder *avs3_dec)
{
	struct inp_par    *input = &avs3_dec->input;
	ImageParameters    *img = &avs3_dec->img;
	union param_u *rpm_param = &avs3_dec->param;
	int32_t compIdx;
	int32_t j,k;
	if (input->alf_enable) {
		img->pic_alf_on[0] = rpm_param->alf.picture_alf_enable_Y;
		img->pic_alf_on[1] = rpm_param->alf.picture_alf_enable_Cb;
		img->pic_alf_on[2] = rpm_param->alf.picture_alf_enable_Cr;

		avs3_dec->m_alfPictureParam[ALF_Y].alf_flag  = img->pic_alf_on[ALF_Y];
		avs3_dec->m_alfPictureParam[ALF_Cb].alf_flag = img->pic_alf_on[ALF_Cb];
		avs3_dec->m_alfPictureParam[ALF_Cr].alf_flag = img->pic_alf_on[ALF_Cr];
		if (img->pic_alf_on[0] || img->pic_alf_on[1] || img->pic_alf_on[2]) {
		for (compIdx = 0; compIdx < NUM_ALF_COMPONENT; compIdx++) {
			if (img->pic_alf_on[compIdx]) {
			readAlfCoeff(avs3_dec, &avs3_dec->m_alfPictureParam[compIdx]);
			}
		}
		}
	}

}
#endif

void print_param(union param_u * param)
{

	printk("sqh->profile_id = %d (0x%x)\n", param->p.sqh_profile_id, param->p.sqh_profile_id);
	printk("sqh->level_id = %d (0x%x)\n", param->p.sqh_level_id, param->p.sqh_level_id);
	printk("sqh->progressive_sequence = %d (0x%x)\n", param->p.sqh_progressive_sequence, param->p.sqh_progressive_sequence);
	printk("sqh->field_coded_sequence = %d (0x%x)\n", param->p.sqh_field_coded_sequence, param->p.sqh_field_coded_sequence);
	printk("sqh->library_stream_flag = %d (0x%x)\n", param->p.sqh_library_stream_flag, param->p.sqh_library_stream_flag);
	printk("sqh->library_picture_enable_flag = %d (0x%x)\n", param->p.sqh_library_picture_enable_flag, param->p.sqh_library_picture_enable_flag);
	printk("sqh->horizontal_size = %d (0x%x)\n", param->p.sqh_horizontal_size, param->p.sqh_horizontal_size);
	printk("sqh->vertical_size = %d (0x%x)\n", param->p.sqh_vertical_size, param->p.sqh_vertical_size);
	printk("sqh->sample_precision = %d (0x%x)\n", param->p.sqh_sample_precision, param->p.sqh_sample_precision);
	printk("sqh->encoding_precision = %d (0x%x)\n", param->p.sqh_encoding_precision, param->p.sqh_encoding_precision);
	printk("sqh->aspect_ratio = %d (0x%x)\n", param->p.sqh_aspect_ratio, param->p.sqh_aspect_ratio);
	printk("sqh->frame_rate_code = %d (0x%x)\n", param->p.sqh_frame_rate_code, param->p.sqh_frame_rate_code);
	printk("sqh->low_delay = %d (0x%x)\n", param->p.sqh_low_delay, param->p.sqh_low_delay);
	printk("sqh->temporal_id_enable_flag = %d (0x%x)\n", param->p.sqh_temporal_id_enable_flag, param->p.sqh_temporal_id_enable_flag);
	printk("sqh->max_dpb_size = %d (0x%x)\n", param->p.sqh_max_dpb_size, param->p.sqh_max_dpb_size);
	printk("sqh->log2_max_cu_width_height = %d (0x%x)\n", param->p.sqh_log2_max_cu_width_height, param->p.sqh_log2_max_cu_width_height);
	printk("sqh->adaptive_leveling_filter_enable_flag = %d (0x%x)\n", param->p.sqh_adaptive_leveling_filter_enable_flag, param->p.sqh_adaptive_leveling_filter_enable_flag);
	printk("sqh->num_of_hmvp_cand = %d (0x%x)\n", param->p.sqh_num_of_hmvp_cand, param->p.sqh_num_of_hmvp_cand);
	printk("sqh->output_reorder_delay = %d (0x%x)\n", param->p.sqh_output_reorder_delay, param->p.sqh_output_reorder_delay);
	printk("sqh->cross_patch_loop_filter = %d (0x%x)\n", param->p.sqh_cross_patch_loop_filter, param->p.sqh_cross_patch_loop_filter);
	printk("pic_header->decode_order_index = %d (0x%x)\n", param->p.pic_header_decode_order_index, param->p.pic_header_decode_order_index);
	printk("pic_header->picture_output_delay = %d (0x%x)\n", param->p.pic_header_picture_output_delay, param->p.pic_header_picture_output_delay);
	printk("pic_header->progressive_frame = %d (0x%x)\n", param->p.pic_header_progressive_frame, param->p.pic_header_progressive_frame);
	printk("pic_header->top_field_first = %d (0x%x)\n", param->p.pic_header_top_field_first, param->p.pic_header_top_field_first);
	printk("pic_header->repeat_first_field = %d (0x%x)\n", param->p.pic_header_repeat_first_field, param->p.pic_header_repeat_first_field);
	printk("pic_header->rpl_l0_idx = %d (0x%x)\n", param->p.pic_header_rpl_l0_idx, param->p.pic_header_rpl_l0_idx);
	printk("pic_header->rpl_l1_idx = %d (0x%x)\n", param->p.pic_header_rpl_l1_idx, param->p.pic_header_rpl_l1_idx);
	printk("pic_header->rpl_l0.ref_pic_num = %d (0x%x)\n", param->p.pic_header_rpl_l0_ref_pic_num, param->p.pic_header_rpl_l0_ref_pic_num);
	printk("pic_header->rpl_l1.ref_pic_num = %d (0x%x)\n", param->p.pic_header_rpl_l1_ref_pic_num, param->p.pic_header_rpl_l1_ref_pic_num);
	printk("pic_header->rpl_l0.reference_to_library_enable_flag = %d (0x%x)\n", param->p.pic_header_rpl_l0_reference_to_library_enable_flag, param->p.pic_header_rpl_l0_reference_to_library_enable_flag);
	printk("pic_header->rpl_l1.reference_to_library_enable_flag = %d (0x%x)\n", param->p.pic_header_rpl_l1_reference_to_library_enable_flag, param->p.pic_header_rpl_l1_reference_to_library_enable_flag);
	printk("pic_header->loop_filter_disable_flag = %d (0x%x)\n", param->p.pic_header_loop_filter_disable_flag, param->p.pic_header_loop_filter_disable_flag);
	printk("pic_header->random_access_decodable_flag = %d (0x%x)\n", param->p.pic_header_random_access_decodable_flag, param->p.pic_header_random_access_decodable_flag);
	printk("pic_header->slice_type = %d (0x%x)\n", param->p.pic_header_slice_type, param->p.pic_header_slice_type);
	printk("pic_header->num_ref_idx_active_override_flag = %d (0x%x)\n", param->p.pic_header_num_ref_idx_active_override_flag, param->p.pic_header_num_ref_idx_active_override_flag);
	printk("pic_header->rpl_l0.ref_pic_active_num = %d (0x%x)\n", param->p.pic_header_rpl_l0_ref_pic_active_num, param->p.pic_header_rpl_l0_ref_pic_active_num);
	printk("pic_header->rpl_l1.ref_pic_active_num = %d (0x%x)\n", param->p.pic_header_rpl_l1_ref_pic_active_num, param->p.pic_header_rpl_l1_ref_pic_active_num);
	printk("sqh->adaptive_filter_shape_enable_flag = %d (0x%x)\n", param->p.sqh_adaptive_filter_shape_enable_flag, param->p.sqh_adaptive_filter_shape_enable_flag);
	printk("pic->header_library_picture_index = %d (0x%x)\n", param->p.pic_header_library_picture_index, param->p.pic_header_library_picture_index);
	printk("pic->header_top_field_picture_flag = %d (0x%x)\n", param->p.pic_header_top_field_picture_flag, param->p.pic_header_top_field_picture_flag);
	printk("pic->header->alpha_c_offset = %d (0x%x)\n", param->p.pic_header_alpha_c_offset, param->p.pic_header_alpha_c_offset);
	printk("pic->header->beta_offset = %d (0x%x)\n", param->p.pic_header_beta_offset, param->p.pic_header_beta_offset);
	printk("pic->header->chroma_quant_param_delta_cb = %d (0x%x)\n", param->p.pic_header_chroma_quant_param_delta_cb, param->p.pic_header_chroma_quant_param_delta_cb);
	printk("pic->header->chroma_quant_param_delta_cr = %d (0x%x)\n", param->p.pic_header_chroma_quant_param_delta_cr, param->p.pic_header_chroma_quant_param_delta_cr);
};

void print_alf_param(union param_u * param)
{
	int i, ii;
	int pos = 0;
	char tmpbuf[128];
	printk("picture_alf_enable_Y %d picture_alf_enable_Cb %d picture_alf_enable_Cr %d\n", param->alf.picture_alf_enable_Y, param->alf.picture_alf_enable_Cb, param->alf.picture_alf_enable_Cr);
	printk("alf_filters_num_m_1 %d dir_index %d\n", param->alf.alf_filters_num_m_1, param->alf.dir_index);
	for (i = 0; i < 16; i++)
		pos += sprintf(&tmpbuf[pos], "%d ", param->alf.region_distance[i]);
	printk("region_distance: %s\n", tmpbuf);

	pos = 0;
	for (i = 0; i < 9; i++)
		pos += sprintf(&tmpbuf[pos], "%d ", (int16_t)param->alf.alf_cb_coeffmulti[i]);
	printk("alf_cb_coeffmulti: %s\n", tmpbuf);

	pos = 0;
	for (i = 0; i < 9; i++)
		pos += sprintf(&tmpbuf[pos], "%d ", (int16_t)param->alf.alf_cr_coeffmulti[i]);
	printk("alf_cr_coeffmulti: %s\n", tmpbuf);

	for (ii = 0; ii < 16; ii++) {
		pos = 0;
		for (i = 0; i < 9; i++)
		pos += sprintf(&tmpbuf[pos], "%d ", (int16_t)param->alf.alf_y_coeffmulti[ii][i]);
		printk("alf_y_coeffmulti[%d][]: %s\n", ii, tmpbuf);
	}
}

void print_pic_pool(struct avs3_decoder *hw, char *mark)
{
	COM_PIC * pic;
	int i;
	int used_count = 0;
	char tmpbuf[128];
	COM_PM *pm = &hw->ctx.dpm;
	int pm_count = 0, pm_ref_count = 0;
	for (i = 0; i < hw->max_pb_size; i++) {
		pic = &hw->pic_pool[i];
		if (pic->buf_cfg.used)
		used_count++;
	}

	for (i = 0; i < pm->max_pb_size; i++)
	{
		if (pm->pic[i] != NULL) {
			pm_ref_count++;
		} else {
			break;
		}
	}

	for (i = 0; i < pm->max_pb_size; i++)
	{
		if (pm->pic[i] != NULL) {
			pm_count++;
		}
	}

	printk("%s----pic_pool (used %d, total %d) cur_num_ref_pics %d pm count %d, pm_ref_count %d diff %d\n", mark, used_count, hw->max_pb_size,
		hw->ctx.dpm.cur_num_ref_pics, pm_count, pm_ref_count,  used_count - pm_count);

	for (i = 0; i < hw->max_pb_size; i++) {
		pic = &hw->pic_pool[i];
		if (pic->buf_cfg.used) {
#ifdef NEW_FRONT_BACK_CODE
		int pos = 0, j;
		pos += sprintf(&tmpbuf[pos], "(");
		for (j = 0; j < pic->buf_cfg.list0_num_refp; j++)
			pos += sprintf(&tmpbuf[pos], "%d ", pic->buf_cfg.list0_index[j]);
		pos += sprintf(&tmpbuf[pos], ")");
		pos += sprintf(&tmpbuf[pos], "(");
		for (j = 0; j < pic->buf_cfg.list1_num_refp; j++)
			pos += sprintf(&tmpbuf[pos], "%d ", pic->buf_cfg.list1_index[j]);
		pos += sprintf(&tmpbuf[pos], ")");
#else
		tmpbuf[0] = 0;
#endif
		printk("%d (%p): buf_cfg index %d depth %d dtr %d ptr %d is_ref %d need_for_out %d, backend_ref %d, vf_ref %d, output_delay %d, w/h(%d,%d) id %d slicetype %d error_mark %d ref index:%s in_dpb %d time %lld\n",
			i, pic, pic->buf_cfg.index, pic->buf_cfg.depth,
			pic->dtr, pic->ptr, pic->is_ref,
			pic->need_for_out,
			pic->buf_cfg.backend_ref, pic->buf_cfg.vf_ref,
			pic->picture_output_delay,
			pic->width_luma, pic->height_luma, pic->temporal_id,
			pic->buf_cfg.slice_type,
			pic->buf_cfg.error_mark,
			tmpbuf,
			pic->buf_cfg.in_dpb,
			pic->buf_cfg.time
			);
		}
	}

	for (i = 0; i < pm->max_pb_size; i++)
	{
		if (pm->pic[i] != NULL) {
			printk("pm pic %p index %d\n", pm->pic[i], i);
		}
	}
}