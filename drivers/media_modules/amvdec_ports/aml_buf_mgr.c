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
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Description:
*/

#include <linux/device.h>
#include <linux/amlogic/media/codec_mm/codec_mm.h>
#include <linux/amlogic/meson_uvm_core.h>

#include "../frame_provider/decoder/utils/decoder_bmmu_box.h"
#include "../frame_provider/decoder/utils/decoder_mmu_box.h"
#include "aml_vcodec_drv.h"
#include "aml_vcodec_dec.h"
#include "aml_task_chain.h"
#include "aml_buf_mgr.h"
#include "aml_vcodec_util.h"
#include "vdec_drv_if.h"
#include "utils/common.h"
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
#include <linux/amlogic/media/video_processor/di_proc_buf_mgr.h>
#endif

#define IS_VPP_POST(bm)	(bm->vpp_work_mode == VPP_WORK_MODE_DI_POST)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
static void aml_buf_vpp_callback(void *caller_data, struct file *file, int id)
{
	struct buf_core_mgr_s *bc = caller_data;
	struct aml_buf_mgr_s *bm = bc_to_bm(bc);
	struct buf_core_entry *entry = NULL;
	struct dma_buf *dbuf = file->private_data;
	ulong key = (ulong)dbuf;

	hash_for_each_possible(bc->buf_table, entry, h_node, key) {
		if (key == entry->key) {
			break;
		}
	}

	if (entry && bc->buf_ops.vpp_cb)
		bc->buf_ops.vpp_cb(bc, entry);
	else
		v4l_dbg(bm->priv, V4L_DEBUG_CODEC_ERROR, "entry is NULL\n");
}

static int aml_buf_vpp_que(struct buf_core_mgr_s *bc, struct buf_core_entry *entry)
{
	struct aml_buf_mgr_s *bm = bc_to_bm(bc);
	struct aml_buf *buf = entry_to_aml_buf(entry);
	int ret = -1;

	if (buf->queued_mask & (1 << BUF_SUB0))
		buf = entry_to_aml_buf(entry->sub_entry[0]);

	if (buf->queued_mask & (1 << BUF_SUB1))
		buf = entry_to_aml_buf(entry->sub_entry[1]);

	ret = buf_mgr_q_checkin(bm->vpp_handle, buf->planes[0].dbuf->file);

	v4l_dbg(bm->priv, V4L_DEBUG_CODEC_BUFMGR,
		"%s, idx: %d, ret:%d\n",
		__func__, buf->index, ret);

	return ret;
}

static int aml_buf_vpp_dque(struct buf_core_mgr_s *bc, struct buf_core_entry *entry)
{
	struct aml_buf_mgr_s *bm = bc_to_bm(bc);
	struct aml_buf *buf = entry_to_aml_buf(entry);
	struct vframe_s *vf = &buf->vframe;
	int ret = -1;

	vf->index_disp	= bm->frm_cnt;
	vf->omx_index	= bm->frm_cnt;

	if (!(vf->type & VIDTYPE_V4L_EOS))
		bm->frm_cnt++;

	dmabuf_set_vframe(buf->planes[0].dbuf, &buf->vframe, VF_SRC_DECODER);

	ret = buf_mgr_dq_checkin(bm->vpp_handle, buf->planes[0].dbuf->file);

	v4l_dbg(bm->priv, V4L_DEBUG_CODEC_BUFMGR,
		"%s, idx: %d, ret:%d\n",
		__func__, buf->index, ret);

	return ret;
}

static int aml_buf_vpp_reset(struct buf_core_mgr_s *bc)
{
	struct aml_buf_mgr_s *bm = bc_to_bm(bc);
	int ret = -1;

	ret = buf_mgr_reset(bm->vpp_handle);

	v4l_dbg(bm->priv, V4L_DEBUG_CODEC_BUFMGR,
		"%s, ret:%d\n",
		__func__, ret);

	return ret;
}

static int aml_buf_vpp_mgr_init(struct aml_buf_mgr_s *bm)
{
	if (!IS_VPP_POST(bm))
		return 0;

	if (!bm->vpp_handle) {
		bm->vpp_handle = buf_mgr_creat(DEC_TYPE_V4L_DEC,
					      bm->bc.id,
					      &bm->bc,
					      aml_buf_vpp_callback);
		if (!bm->vpp_handle) {
			v4l_dbg(bm->priv, V4L_DEBUG_CODEC_ERROR,
				"%s, The vpp buf mgr creat fail.\n",
				__func__);
			return -1;
		}

		bm->bc.vpp_que	= aml_buf_vpp_que;
		bm->bc.vpp_dque = aml_buf_vpp_dque;
		bm->bc.vpp_reset = aml_buf_vpp_reset;
	}

	return 0;
}

static void aml_buf_vpp_mgr_release(struct aml_buf_mgr_s *bm)
{
	if (bm->vpp_handle)
		buf_mgr_release(bm->vpp_handle);
}
#endif

static int aml_buf_box_alloc(struct aml_buf_mgr_s *bm, void **mmu, void **mmu_1, void **bmmu) {
	struct aml_buf_fbc_info fbc_info;
	int mmu_flag = bm->config.enable_secure ? CODEC_MM_FLAGS_TVP : 0;
	int bmmu_flag = mmu_flag;
	struct aml_vcodec_ctx *ctx = container_of(bm,
		struct aml_vcodec_ctx, bm);

	bm->get_fbc_info(bm, &fbc_info);

	/* init mmu box */
	*mmu = decoder_mmu_box_alloc_box(bm->bc.name,
		bm->bc.id,
		BUF_FBC_NUM_MAX,
		fbc_info.max_size * SZ_1M,
		mmu_flag);
	if (!(*mmu)) {
		v4l_dbg(bm->priv, V4L_DEBUG_CODEC_ERROR,
			"fail to create mmu box\n");
		return -EINVAL;
	}

#ifdef NEW_FB_CODE
	if (ctx->front_back_mode) {
		*mmu_1 = decoder_mmu_box_alloc_box(bm->bc.name,
			bm->bc.id,
			BUF_FBC_NUM_MAX,
			fbc_info.max_size * SZ_1M,
			mmu_flag);
		if (!(*mmu_1)) {
			v4l_dbg(bm->priv, V4L_DEBUG_CODEC_ERROR,
				"fail to create mmu_1 box\n");
			goto free_mmubox;
		}
	}
#endif

	/* init bmmu box */
	bmmu_flag |= (CODEC_MM_FLAGS_CMA_CLEAR | CODEC_MM_FLAGS_FOR_VDECODER);
	*bmmu = decoder_bmmu_box_alloc_box(bm->bc.name,
		bm->bc.id,
		BUF_FBC_NUM_MAX,
		4 + PAGE_SHIFT,
		bmmu_flag, BMMU_ALLOC_FLAGS_WAIT);
	if (!(*bmmu)) {
		v4l_dbg(bm->priv, V4L_DEBUG_CODEC_ERROR,
			"fail to create bmmu box\n");
		goto free_mmubox1;
	}

	return 0;

free_mmubox1:
	decoder_mmu_box_free(*mmu_1);
	*mmu_1 = NULL;

free_mmubox:
	decoder_mmu_box_free(*mmu);
	*mmu = NULL;

	return -1;
}

static int aml_buf_box_init(struct aml_buf_mgr_s *bm)
{
	u32 dw_mode = DM_YUV_ONLY;
	struct aml_vcodec_ctx *ctx = container_of(bm,
		struct aml_vcodec_ctx, bm);
	bool buff_alloc_done = false;

	if (!bm->mmu || !bm->bmmu) {
		bm->fbc_array = vzalloc(sizeof(*bm->fbc_array) * BUF_FBC_NUM_MAX);
		if (!bm->fbc_array)
			return -ENOMEM;
		if (aml_buf_box_alloc(bm, &bm->mmu, &bm->mmu_1, &bm->bmmu)) {
			vfree(bm->fbc_array);
			bm->fbc_array = NULL;
			return -EINVAL;
		}
		buff_alloc_done = true;
	}

	if (!bm->mmu_dw || !bm->bmmu_dw) {
		if (vdec_if_get_param(ctx, GET_PARAM_DW_MODE, &dw_mode)) {
			v4l_dbg(bm->priv, V4L_DEBUG_CODEC_ERROR, "invalid dw_mode\n");
			return -EINVAL;
		}
		if (dw_mode & VDEC_MODE_MMU_DW_MASK) {
			if (aml_buf_box_alloc(bm, &bm->mmu_dw, &bm->mmu_dw_1, &bm->bmmu_dw)) {
				return -EINVAL;
			}
			buff_alloc_done = true;
		}
	}

	if (buff_alloc_done)
		v4l_dbg(bm->priv, V4L_DEBUG_CODEC_BUFMGR,
			"box init, bmmu: %px, mmu: %px, bmmu_dw: %px mmu_dw: %px mmu_1: %px, mmu_dw_1: %px\n",
			bm->bmmu, bm->mmu, bm->bmmu_dw, bm->mmu_dw, bm->mmu_1, bm->mmu_dw_1);

	return 0;
}

static int aml_buf_fbc_init(struct aml_buf_mgr_s *bm, struct aml_buf *buf)
{
	struct aml_buf_fbc_info fbc_info;
	struct aml_buf_fbc *fbc;
	int ret, i;

	if (aml_buf_box_init(bm))
		return -EINVAL;


	for (i = 0; i < BUF_FBC_NUM_MAX; i++) {
		if (!bm->fbc_array[i].ref)
			break;
	}

	if (i == BUF_FBC_NUM_MAX) {
		v4l_dbg(bm->priv, V4L_DEBUG_CODEC_ERROR,
			"out of fbc buf\n");
		return -EINVAL;
	}

	bm->get_fbc_info(bm, &fbc_info);

	fbc		= &bm->fbc_array[i];
	fbc->index	= i;
	fbc->hsize	= fbc_info.header_size;
	fbc->hsize_dw   = fbc_info.header_size;
	fbc->frame_size	= fbc_info.frame_size;
	fbc->bmmu	= bm->bmmu;
	fbc->mmu	= bm->mmu;
	fbc->bmmu_dw	= bm->bmmu_dw;
	fbc->mmu_dw	= bm->mmu_dw;
#ifdef NEW_FB_CODE
	fbc->mmu_1	= bm->mmu_1;
	fbc->mmu_dw_1	= bm->mmu_dw_1;
#endif
	fbc->used[i]	= 0;
	fbc->ref	= 1;

	/* allocate header */
	ret = decoder_bmmu_box_alloc_buf_phy(bm->bmmu,
					    fbc->index,
					    fbc->hsize,
					    bm->bc.name,
					    &fbc->haddr);
	if (ret < 0) {
		v4l_dbg(bm->priv, V4L_DEBUG_CODEC_ERROR,
			"fail to alloc %dth bmmu\n", i);
		return -ENOMEM;
	}
	if (!bm->config.enable_secure) {
		codec_mm_memset(fbc->haddr, 0, fbc->hsize);
	}

	if (bm->bmmu_dw) {
		ret = decoder_bmmu_box_alloc_buf_phy(bm->bmmu_dw,
						    fbc->index,
						    fbc->hsize_dw,
						    "v4ldec-m2m-dw",
						    &fbc->haddr_dw);
		if (ret < 0) {
			decoder_bmmu_box_free_idx(bm->bmmu, fbc->index);
			v4l_dbg(bm->priv, V4L_DEBUG_CODEC_ERROR,
				"fail to alloc %dth bmmu dw\n", i);
			return -ENOMEM;
		}
		if (!bm->config.enable_secure) {
			codec_mm_memset(fbc->haddr_dw, 0, fbc->hsize_dw);
		}
	}

	buf->fbc = fbc;

	v4l_dbg(bm->priv, V4L_DEBUG_CODEC_BUFMGR,
		"%s, fbc:%d, haddr:%lx, hsize:%d, frm size:%d "
		"haddr_dw:%lx, hsize_dw:%d\n",
		__func__,
		fbc->index,
		fbc->haddr,
		fbc->hsize,
		fbc->frame_size,
		fbc->haddr_dw,
		fbc->hsize_dw);

	return 0;
}

static int aml_buf_fbc_release(struct aml_buf_mgr_s *bm, struct aml_buf *buf)
{
	struct aml_buf_fbc *fbc = buf->fbc;
	int ret;
	struct aml_vcodec_ctx *ctx = container_of(bm,
		struct aml_vcodec_ctx, bm);

	ret = decoder_bmmu_box_free_idx(bm->bmmu, fbc->index);
	if (ret < 0) {
		v4l_dbg(bm->priv, V4L_DEBUG_CODEC_ERROR,
			"fail to free %dth bmmu\n", fbc->index);
		return -ENOMEM;
	}

	ret = decoder_mmu_box_free_idx(bm->mmu, fbc->index);
	if (ret < 0) {
		v4l_dbg(bm->priv, V4L_DEBUG_CODEC_ERROR,
			"fail to free %dth mmu\n", fbc->index);
		return -ENOMEM;
	}

#ifdef NEW_FB_CODE
	if (ctx->front_back_mode) {
		ret = decoder_mmu_box_free_idx(bm->mmu_1, fbc->index);
		if (ret < 0) {
			v4l_dbg(bm->priv, V4L_DEBUG_CODEC_ERROR,
				"fail to free %dth mmu 1\n", fbc->index);
			return -ENOMEM;
		}
	}
#endif

	if (bm->mmu_dw) {
		ret = decoder_mmu_box_free_idx(bm->mmu_dw, fbc->index);
		if (ret < 0) {
			v4l_dbg(bm->priv, V4L_DEBUG_CODEC_ERROR,
				"fail to free %dth mmu dw\n", fbc->index);
			return -ENOMEM;
		}
	}

#ifdef NEW_FB_CODE
	if (ctx->front_back_mode && bm->mmu_dw_1) {
		ret = decoder_mmu_box_free_idx(bm->mmu_dw_1, fbc->index);
		if (ret < 0) {
			v4l_dbg(bm->priv, V4L_DEBUG_CODEC_ERROR,
				"fail to free %dth mmu dw 1\n", fbc->index);
			return -ENOMEM;
		}
	}
#endif

	if (bm->bmmu_dw) {
		decoder_bmmu_box_free_idx(bm->bmmu_dw, fbc->index);
		if (ret < 0) {
			v4l_dbg(bm->priv, V4L_DEBUG_CODEC_ERROR,
				"fail to free %dth bmmu dw\n", fbc->index);
			return -ENOMEM;
		}
	}

	fbc->used[fbc->index] = 0;
	fbc->ref = 0;
	buf->fbc = NULL;

	v4l_dbg(bm->priv, V4L_DEBUG_CODEC_BUFMGR,
		"%s, fbc:%d, haddr:%lx, hsize:%d, frm size:%d "
		"haddr_dw:%lx, hsize_dw:%d\n",
		__func__,
		fbc->index,
		fbc->haddr,
		fbc->hsize,
		fbc->frame_size,
		fbc->haddr_dw,
		fbc->hsize_dw);

	return 0;
}

static void aml_buf_get_fbc_info(struct aml_buf_mgr_s *bm,
				struct aml_buf_fbc_info *info)
{
	struct vdec_comp_buf_info comp_info;

	if (vdec_if_get_param(bm->priv, GET_PARAM_COMP_BUF_INFO, &comp_info)) {
		v4l_dbg(bm->priv, V4L_DEBUG_CODEC_ERROR,
			"fail to get comp info\n");
		return;
	}

	info->max_size		= comp_info.max_size;
	info->header_size	= comp_info.header_size;
	info->frame_size	= comp_info.frame_buffer_size;
}

static void aml_buf_fbc_destroy(struct aml_buf_mgr_s *bm)
{
	v4l_dbg(bm->priv, V4L_DEBUG_CODEC_BUFMGR, "%s %d\n", __func__, __LINE__);
	if (bm->bmmu)
		decoder_bmmu_box_free(bm->bmmu);
	if (bm->mmu)
		decoder_mmu_box_free(bm->mmu);

	if (bm->mmu_dw)
		decoder_mmu_box_free(bm->mmu_dw);
	if (bm->bmmu_dw)
		decoder_bmmu_box_free(bm->bmmu_dw);

#ifdef NEW_FB_CODE
	if (bm->mmu_1)
		decoder_mmu_box_free(bm->mmu_1);
	if (bm->mmu_dw_1)
		decoder_mmu_box_free(bm->mmu_dw_1);
#endif
	if (bm->fbc_array)
		vfree(bm->fbc_array);
	bm->fbc_array = NULL;
}

static void aml_buf_mgr_destroy(struct kref *kref)
{
	struct aml_buf_mgr_s *bm =
		container_of(kref, struct aml_buf_mgr_s, ref);

	if (bm->fbc_array) {
		aml_buf_fbc_destroy(bm);
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	aml_buf_vpp_mgr_release(bm);
#endif
}

static void aml_buf_flush(struct aml_buf_mgr_s *bm,
				struct aml_buf *aml_buf)
{
	struct aml_buf_config *cfg = &bm->config;
	void *buf = NULL;
	int i;

	if (aml_buf->flush_flag)
		return;

	if (cfg->enable_secure)
		return;

	for (i = 0 ; i < aml_buf->num_planes ; i++) {
		buf = codec_mm_phys_to_virt(aml_buf->planes_tw[i].addr);
		if (buf) {
			codec_mm_dma_flush(buf,
				aml_buf->planes_tw[i].length, DMA_FROM_DEVICE);
		} else {
			buf = codec_mm_vmap(aml_buf->planes_tw[i].addr,
				aml_buf->planes_tw[i].length);
			if (!buf) {
				v4l_dbg(bm->priv, V4L_DEBUG_CODEC_ERROR,
					"%s flush tw dma buffer fail!\n", __func__);
				return;
			}

			codec_mm_dma_flush(buf,
				aml_buf->planes_tw[i].length, DMA_FROM_DEVICE);
			codec_mm_unmap_phyaddr(buf);
		}
	}

	aml_buf->flush_flag = true;
}

void aml_buf_set_planes_v4l2(struct aml_buf_mgr_s *bm,
				   struct aml_buf *aml_buf,
				   void *priv)
{
	struct vb2_buffer *vb = (struct vb2_buffer *)priv;
	struct vb2_v4l2_buffer *vb2_v4l2 = to_vb2_v4l2_buffer(vb);
	struct aml_v4l2_buf *aml_vb = container_of(vb2_v4l2, struct aml_v4l2_buf, vb);
	struct aml_buf_config *cfg = &bm->config;
	struct aml_buf *master_buf;
	char plane_n[3] = {'Y','U','V'};
	int i;

	aml_buf->vb		= vb;
	aml_buf->num_planes	= vb->num_planes;
	aml_vb->aml_buf		= aml_buf;
	aml_buf->entry.index	= aml_buf->index;
	aml_buf->inited		= aml_buf->entry.inited;
	aml_buf->entry.pair_state	= aml_buf->pair_state;

	for (i = 0 ; i < vb->num_planes ; i++) {
		if (i == 0) {
			//Y
			if (vb->num_planes == 1) {
				aml_buf->planes[0].length	= cfg->luma_length + cfg->chroma_length;
				aml_buf->planes[0].offset	= cfg->luma_length;
			} else {
				aml_buf->planes[0].length	= cfg->luma_length;
				aml_buf->planes[0].offset	= 0;
			}
		} else {
			if (vb->num_planes == 2) {
				//UV
				aml_buf->planes[1].length	= cfg->chroma_length;
				aml_buf->planes[1].offset	= cfg->chroma_length >> 1;
			} else {
				aml_buf->planes[i].length	= cfg->chroma_length >> 1;
				aml_buf->planes[i].offset	= 0;
			}
		}

		aml_buf->planes[i].addr	= get_addr(vb, i);
		aml_buf->planes[i].dbuf	= vb->planes[i].dbuf;

		if (aml_buf->pair == BUF_SUB0) {
			master_buf = (struct aml_buf *)aml_buf->master_buf;
			master_buf->entry.master_ref = &master_buf->entry.ref;
			master_buf->entry.sub_entry[0] = (void *)&aml_buf->entry;

			aml_buf->entry.master_entry = (void *)&master_buf->entry;
			aml_buf->entry.master_ref = &master_buf->entry.ref;
		}

		if (aml_buf->pair == BUF_SUB1) {
			master_buf = (struct aml_buf *)aml_buf->master_buf;
			master_buf->entry.sub_entry[1] = (void *)&aml_buf->entry;

			aml_buf->entry.master_entry = (void *)&master_buf->entry;
			aml_buf->entry.master_ref = &master_buf->entry.ref;
		}

		/* Make a fake used size for DW/TW:(0, 0). */
		if (!cfg->dw_mode)
			aml_buf->planes[i].bytes_used = 1;

		v4l_dbg(bm->priv, V4L_DEBUG_CODEC_BUFMGR,
			"Buffer info, id:%x, %c:(0x%lx, %d), DW:%x\n",
			vb->index,
			plane_n[i],
			aml_buf->planes[i].addr,
			aml_buf->planes[i].length,
			cfg->dw_mode);
	}

	if (cfg->tw_mode) {
		for (i = 0 ; i < vb->num_planes ; i++) {
			if (i == 0) {
				//Y
				if (vb->num_planes == 1) {
					aml_buf->planes_tw[0].length	= cfg->luma_length_tw + cfg->chroma_length_tw;
					aml_buf->planes_tw[0].offset	= cfg->luma_length_tw;
				} else {
					aml_buf->planes_tw[0].length	= cfg->luma_length_tw;
					aml_buf->planes_tw[0].offset	= 0;
				}
			} else {
				if (vb->num_planes == 2) {
					//UV
					aml_buf->planes_tw[1].length	= cfg->chroma_length_tw;
					aml_buf->planes_tw[1].offset	= cfg->chroma_length_tw >> 1;
				} else {
					aml_buf->planes_tw[i].length	= cfg->chroma_length_tw >> 1;
					aml_buf->planes_tw[i].offset	= 0;
				}
			}

			aml_buf->planes_tw[i].addr = (cfg->dw_mode == DM_AVBC_ONLY) ?
				vb2_dma_contig_plane_dma_addr(vb, i):
				(aml_buf->planes_tw[i].addr ?
					aml_buf->planes_tw[i].addr:
					codec_mm_alloc_for_dma_ex("tw_buf",
						(cfg->luma_length_tw +
						cfg->chroma_length_tw) / PAGE_SIZE,
						4,
						CODEC_MM_FLAGS_FOR_VDECODER,
						bm->bc.id,
						i));
			aml_buf->planes_tw[i].dbuf = vb->planes[i].dbuf;
			aml_buf_flush(bm, aml_buf);

			v4l_dbg(bm->priv, V4L_DEBUG_CODEC_BUFMGR,
				"Buffer info, id:%d, %c:(0x%lx, %d), TW:%x\n",
				vb->index,
				plane_n[i],
				aml_buf->planes_tw[i].addr,
				aml_buf->planes_tw[i].length,
				cfg->tw_mode);
		}
	}
}

static void aml_buf_set_planes(struct aml_buf_mgr_s *bm,
			      struct aml_buf *buf)
{
	//todo
}

static int aml_buf_set_default_parms(struct aml_buf_mgr_s *bm,
				     struct aml_buf *buf,
				     void *priv)
{
	int ret;

	buf->index = bm->bc.buf_num;

	if (bm->config.enable_extbuf) {
		aml_buf_set_planes_v4l2(bm, buf, priv);

		ret = aml_uvm_buff_attach(buf->vb);
		if (ret) {
			v4l_dbg(bm->priv, V4L_DEBUG_CODEC_ERROR,
				"uvm buffer attach failed.\n");
			return ret;
		}
	} else {
		// alloc buffer
		aml_buf_set_planes(bm, buf);
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	ret = aml_buf_vpp_mgr_init(bm);
	if (ret) {
		v4l_dbg(bm->priv, V4L_DEBUG_CODEC_ERROR,
			"VPP buf mgr init failed.\n");
		return ret;
	}
#endif

	ret = task_chain_init(&buf->task, bm->priv, buf, buf->index);
	if (ret) {
		v4l_dbg(bm->priv, V4L_DEBUG_CODEC_ERROR,
			"task chain init failed.\n");
	}

	return ret;
}

static int aml_buf_alloc(struct buf_core_mgr_s *bc,
			struct buf_core_entry **entry,
			void *priv)
{
	int ret;
	struct aml_buf *buf;
	struct aml_buf_mgr_s *bm = bc_to_bm(bc);

	buf = vzalloc(sizeof(struct aml_buf));
	if (buf == NULL) {
		return -ENOMEM;
	}

	/* afbc init. */
	if (bm->config.enable_fbc) {
		ret = aml_buf_fbc_init(bm, buf);
		if (ret) {
			goto err1;
		}
	}

	ret = aml_buf_set_default_parms(bm, buf, priv);
	if (ret) {
		goto err2;
	}

	v4l_dbg(bm->priv, V4L_DEBUG_CODEC_BUFMGR,
		"%s, entry:%px, free:%d\n",
		__func__,
		&buf->entry,
		bc->free_num);

	kref_get(&bm->ref);

	*entry = &buf->entry;

	return 0;

err2:
	aml_buf_fbc_destroy(bm);
err1:
	vfree(buf);

	return ret;
}

static void aml_buf_free(struct buf_core_mgr_s *bc,
			struct buf_core_entry *entry)
{
	struct aml_buf_mgr_s *bm = bc_to_bm(bc);
	struct aml_buf *buf = entry_to_aml_buf(entry);
	int i;

	v4l_dbg(bm->priv, V4L_DEBUG_CODEC_BUFMGR,
		"%s, entry:%px, user:%d, key:%lx, idx:%d, st:(%d, %d), ref:(%d, %d)\n",
		__func__,
		entry,
		entry->user,
		entry->key,
		entry->index,
		entry->state,
		bc->state,
		atomic_read(&entry->ref),
		kref_read(&bc->core_ref));

	/* afbc free */
	if (buf->fbc) {
		aml_buf_fbc_release(bm, buf);
	}

	/* task chain clean */
	task_chain_clean(buf->task);
	/* task chain release */
	task_chain_release(buf->task);

	/* free triple write buffer. */
	if (bm->config.tw_mode &&
		(bm->config.dw_mode != DM_AVBC_ONLY)) {
		for (i = 0 ; i < buf->num_planes ; i++) {
			if (buf->planes_tw[i].addr)
				codec_mm_free_for_dma("tw_buf", buf->planes_tw[i].addr);
		}
	}

	kref_put(&bm->ref, aml_buf_mgr_destroy);

	if (buf->vpp_buf)
		vfree(buf->vpp_buf);

	if (buf->ge2d_buf)
		vfree(buf->ge2d_buf);

	vfree(buf);
}

static void aml_buf_configure(struct buf_core_mgr_s *bc, void *cfg)
{
	struct aml_buf_mgr_s *bm = bc_to_bm(bc);

	bm->config = *(struct aml_buf_config *)cfg;
}

static void aml_external_process(struct buf_core_mgr_s *bc,
				struct buf_core_entry *entry)
{
	struct aml_buf *aml_buf = entry_to_aml_buf(entry);
	struct aml_buf_mgr_s *bm =
			container_of(bc, struct aml_buf_mgr_s, bc);

	aml_vdec_recycle_dec_resource(bm->priv, aml_buf);
}

static void aml_buf_prepare(struct buf_core_mgr_s *bc,
				struct buf_core_entry *entry)
{
	struct aml_buf_mgr_s *bm = bc_to_bm(bc);
	struct aml_buf *buf = entry_to_aml_buf(entry);
	struct aml_buf_fbc_info fbc_info = { 0 };
	struct aml_buf *sub_buf[2];
	struct buf_core_entry *sub_entry;

	v4l_dbg(bm->priv, V4L_DEBUG_CODEC_BUFMGR,
		"%s, user:%d, key:%lx, idx:%d, st:(%d, %d), ref:(%d, %d), free:%d\n",
		__func__,
		entry->user,
		entry->key,
		entry->index,
		entry->state,
		bc->state,
		atomic_read(&entry->ref),
		kref_read(&bc->core_ref),
		bc->free_num);

	if (entry->sub_entry[0]) {
		sub_entry = (struct buf_core_entry *)entry->sub_entry[0];
		sub_buf[0] = (struct aml_buf *)buf->sub_buf[0];
	}

	if (entry->sub_entry[1]) {
		sub_entry = (struct buf_core_entry *)entry->sub_entry[1];
		sub_buf[1] = (struct aml_buf *)buf->sub_buf[1];
	}

	if (entry->user != BUF_USER_MAX && !task_chain_empty(buf->task)) {
		task_chain_clean(buf->task);
		if (entry->sub_entry[0])
			task_chain_clean(sub_buf[0]->task);
		if (entry->sub_entry[1])
			task_chain_clean(sub_buf[1]->task);
	}

	if (bm->config.enable_fbc)
		bm->get_fbc_info(bm, &fbc_info);

	if (buf->fbc &&
		((fbc_info.frame_size != buf->fbc->frame_size) ||
		(fbc_info.header_size != buf->fbc->hsize) ||
		!bm->config.enable_fbc)) {
		aml_buf_fbc_release(bm, buf);
	}

	if (bm->config.enable_fbc &&
		!buf->fbc) {
		aml_buf_fbc_init(bm, buf);
	}

	if (bm->config.enable_extbuf) {
		aml_buf_set_planes_v4l2(bm, buf, entry->vb2);
	} else {
		aml_buf_set_planes(bm, buf);
	}

	if (entry->user != BUF_USER_MAX) {
		aml_creat_pipeline(bm->priv, buf, entry->user);
		if (entry->sub_entry[0])
			aml_creat_pipeline(bm->priv, sub_buf[0], entry->user);
		if (entry->sub_entry[1])
			aml_creat_pipeline(bm->priv, sub_buf[1], entry->user);
	}
}

static int aml_buf_output(struct buf_core_mgr_s *bc,
			  struct buf_core_entry *entry,
			  enum buf_core_user user)
{
	struct aml_buf_mgr_s *bm = bc_to_bm(bc);
	struct aml_buf *buf = entry_to_aml_buf(entry);
	struct aml_buf *master_buf;
	struct buf_core_entry *master_entry;
	int i;

	if (task_chain_empty(buf->task))
		return -1;

	if (entry->pair != BUF_MASTER) {
		master_entry = (struct buf_core_entry *)entry->master_entry;
		master_buf = entry_to_aml_buf(master_entry);

		for (i = 0; i < buf->num_planes; i++) {
			buf->planes[i].bytes_used = master_buf->planes[i].bytes_used;
		}
	}

	v4l_dbg(bm->priv, V4L_DEBUG_CODEC_BUFMGR,
		"%s, user:%d, key:%lx, st:(%d, %d), ref:(%d, %d), free:%d\n",
		__func__,
		entry->user,
		entry->key,
		entry->state,
		bc->state,
		atomic_read(&entry->ref),
		kref_read(&bc->core_ref),
		bc->free_num);

	return buf->task->submit(buf->task, user_to_task(user));
}

static void aml_buf_input(struct buf_core_mgr_s *bc,
		   struct buf_core_entry *entry,
		   enum buf_core_user user)
{
	struct aml_buf_mgr_s *bm = bc_to_bm(bc);
	struct aml_buf *buf = entry_to_aml_buf(entry);

	if (task_chain_empty(buf->task))
		return;

	v4l_dbg(bm->priv, V4L_DEBUG_CODEC_BUFMGR,
		"%s, user:%d, key:%lx, st:(%d, %d), ref:(%d, %d), free:%d\n",
		__func__,
		entry->user,
		entry->key,
		entry->state,
		bc->state,
		atomic_read(&entry->ref),
		kref_read(&bc->core_ref),
		bc->free_num);

	buf->task->recycle(buf->task, user_to_task(user));
}

static int aml_buf_get_pre_user(struct buf_core_mgr_s *bc,
		   struct buf_core_entry *entry,
		   enum buf_core_user user)
{
	//struct aml_buf_mgr_s *bm = bc_to_bm(bc);
	struct aml_buf *buf = entry_to_aml_buf(entry);
	int type;

	if (task_chain_empty(buf->task))
		return BUF_USER_MAX;
#if 0
	v4l_dbg(bm->priv, V4L_DEBUG_CODEC_BUFMGR,
		"%s, user:%d, key:%lx, st:(%d, %d), ref:(%d, %d), free:%d\n",
		__func__,
		entry->user,
		entry->key,
		entry->state,
		bc->state,
		atomic_read(&entry->ref),
		kref_read(&bc->core_ref),
		bc->free_num);
#endif
	type = buf->task->get_pre_user(buf->task, user_to_task(user));

	return task_to_user(type);
}

static int aml_buf_get_next_user(struct buf_core_mgr_s *bc,
		   struct buf_core_entry *entry,
		   enum buf_core_user user)
{
	//struct aml_buf_mgr_s *bm = bc_to_bm(bc);
	struct aml_buf *buf = entry_to_aml_buf(entry);
	int type;

	if (task_chain_empty(buf->task))
		return BUF_USER_MAX;
#if 0
	v4l_dbg(bm->priv, V4L_DEBUG_CODEC_BUFMGR,
		"%s, user:%d, key:%lx, st:(%d, %d), ref:(%d, %d), free:%d\n",
		__func__,
		entry->user,
		entry->key,
		entry->state,
		bc->state,
		atomic_read(&entry->ref),
		kref_read(&bc->core_ref),
		bc->free_num);
#endif
	type = buf->task->get_next_user(buf->task, user_to_task(user));

	return task_to_user(type);
}

int aml_buf_mgr_init(struct aml_buf_mgr_s *bm, char *name, int id, void *priv)
{
	int ret = -1;

	bm->bc.id		= id;
	bm->bc.name		= name;
	bm->priv		= priv;
	bm->get_fbc_info	= aml_buf_get_fbc_info;

	bm->bc.config		= aml_buf_configure;
	bm->bc.prepare		= aml_buf_prepare;
	bm->bc.input		= aml_buf_input;
	bm->bc.output		= aml_buf_output;
	bm->bc.get_pre_user	= aml_buf_get_pre_user;
	bm->bc.get_next_user	= aml_buf_get_next_user;
	bm->bc.external_process	= aml_external_process;
	bm->bc.mem_ops.alloc	= aml_buf_alloc;
	bm->bc.mem_ops.free	= aml_buf_free;

	kref_init(&bm->ref);

	ret = buf_core_mgr_init(&bm->bc);
	if (ret) {
		v4l_dbg(priv, V4L_DEBUG_CODEC_ERROR,
			"%s, init fail.\n", __func__);
	} else {
		v4l_dbg(priv, V4L_DEBUG_CODEC_BUFMGR,
			"%s\n", __func__);
	}

	return ret;
}

void aml_buf_mgr_release(struct aml_buf_mgr_s *bm)
{
	v4l_dbg(bm->priv, V4L_DEBUG_CODEC_BUFMGR, "%s\n", __func__);

	kref_put(&bm->ref, aml_buf_mgr_destroy);
	buf_core_mgr_release(&bm->bc);
}

