/*
*
* SPDX-License-Identifier: GPL-2.0
*
* Copyright (C) 2020 Amlogic or its affiliates
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; version 2.
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/
#include <linux/version.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/pm_runtime.h>
#include <linux/pm_domain.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/timex.h>

#include "aml_isp.h"
#include "aml_cam.h"

#include "aml_adapter.h"

#define AML_ISP_NAME	"isp-core"

static struct isp_dev_t *g_isp_dev[4];

static const struct aml_format isp_subdev_formats[] = {
	{0, 0, 0, 0, MEDIA_BUS_FMT_SBGGR8_1X8, 0, 1, 8},
	{0, 0, 0, 0, MEDIA_BUS_FMT_SGBRG8_1X8, 0, 1, 8},
	{0, 0, 0, 0, MEDIA_BUS_FMT_SGRBG8_1X8, 0, 1, 8},
	{0, 0, 0, 0, MEDIA_BUS_FMT_SRGGB8_1X8, 0, 1, 8},
	{0, 0, 0, 0, MEDIA_BUS_FMT_SBGGR10_1X10, 0, 1, 10},
	{0, 0, 0, 0, MEDIA_BUS_FMT_SGBRG10_1X10, 0, 1, 10},
	{0, 0, 0, 0, MEDIA_BUS_FMT_SGRBG10_1X10, 0, 1, 10},
	{0, 0, 0, 0, MEDIA_BUS_FMT_SRGGB10_1X10, 0, 1, 10},
	{0, 0, 0, 0, MEDIA_BUS_FMT_SBGGR12_1X12, 0, 1, 12},
	{0, 0, 0, 0, MEDIA_BUS_FMT_SGBRG12_1X12, 0, 1, 12},
	{0, 0, 0, 0, MEDIA_BUS_FMT_SGRBG12_1X12, 0, 1, 12},
	{0, 0, 0, 0, MEDIA_BUS_FMT_SRGGB12_1X12, 0, 1, 12},
	{0, 0, 0, 0, MEDIA_BUS_FMT_SBGGR14_1X14, 0, 1, 14},
	{0, 0, 0, 0, MEDIA_BUS_FMT_SGBRG14_1X14, 0, 1, 14},
	{0, 0, 0, 0, MEDIA_BUS_FMT_SGRBG14_1X14, 0, 1, 14},
	{0, 0, 0, 0, MEDIA_BUS_FMT_SRGGB14_1X14, 0, 1, 14},
	{0, 0, 0, 0, MEDIA_BUS_FMT_YUYV8_2X8, 0, 1, 8},
	{0, 0, 0, 0, MEDIA_BUS_FMT_YVYU8_2X8, 0, 1, 8},
};

struct isp_dev_t *isp_subdrv_get_dev(int index)
{
	if (index >= 4) {
		pr_err("err isp index num\n");
	}

	return g_isp_dev[index];
}

static int isp_subdrv_reg_buf_alloc(struct isp_dev_t *isp_dev)
{
	u32 wsize, rsize;
	u32 bsize = 0;
	dma_addr_t paddr = 0x0000;
	void *virtaddr = NULL, *vmaddr = NULL;

	wsize = 256 * 1024;
	rsize = 128 * 1024;
	bsize = wsize + rsize;

	virtaddr = dma_alloc_coherent(isp_dev->dev, bsize, &paddr, GFP_KERNEL);
	vmaddr = vmalloc(bsize);

	isp_dev->wreg_buff.nplanes = 1;
	isp_dev->wreg_buff.bsize = wsize;
	isp_dev->wreg_buff.addr[AML_PLANE_A] = paddr;
	isp_dev->wreg_buff.vaddr[AML_PLANE_A] = virtaddr;
	isp_dev->wreg_buff.vmaddr[AML_PLANE_A] = vmaddr;

	isp_dev->rreg_buff.nplanes = 1;
	isp_dev->rreg_buff.addr[AML_PLANE_A] = paddr + wsize;
	isp_dev->rreg_buff.vaddr[AML_PLANE_A] = virtaddr + wsize;
	isp_dev->rreg_buff.vmaddr[AML_PLANE_A] = vmaddr + wsize;

	pr_debug("reg alloc\n");

	return 0;
}

static int isp_subdrv_reg_buf_free(struct isp_dev_t *isp_dev)
{
	u32 paddr = 0x0000;
	void *vaddr = NULL;
	u32 bsize = 0;

	paddr = isp_dev->wreg_buff.addr[AML_PLANE_A];
	vaddr = isp_dev->wreg_buff.vaddr[AML_PLANE_A];
	bsize = 256 * 1024 + 128 * 1024;

	if (vaddr)
		dma_free_coherent(isp_dev->dev, bsize, vaddr, (dma_addr_t)paddr);

	vaddr = isp_dev->wreg_buff.vmaddr[AML_PLANE_A];
	if (vaddr)
		vfree(vaddr);

	isp_dev->wreg_buff.addr[AML_PLANE_A] = 0x0000;
	isp_dev->wreg_buff.vaddr[AML_PLANE_A] = NULL;
	isp_dev->wreg_buff.vmaddr[AML_PLANE_A] = NULL;

	isp_dev->rreg_buff.addr[AML_PLANE_A] = 0x0000;
	isp_dev->rreg_buff.vaddr[AML_PLANE_A] = NULL;
	isp_dev->rreg_buff.vmaddr[AML_PLANE_A] = NULL;

	pr_debug("reg free\n");

	return 0;
}

static int isp_subdev_ptnr_buf_alloc(struct isp_dev_t *isp_dev, struct aml_format *fmt)
{
	int rtn = 0;
	u32 bsize = 0;
	u32 stride = 0;
	u32 width, height;
	u32 paddr = 0x0000;
	void *virtaddr = NULL;

	width = fmt->width;
	height = fmt->height;
	stride = (width * 10 + 127) >> 7;

	bsize = (128 >> 3) * stride * height;
	bsize = ISP_SIZE_ALIGN(bsize, 1 << 12);

	rtn = aml_subdev_cma_alloc(isp_dev->pdev, &paddr, virtaddr, bsize);
	if (rtn != 0) {
		pr_err("Failed to alloc ptnr buff\n");
		return -1;
	}

	isp_dev->ptnr_buff.nplanes = 1;
	isp_dev->ptnr_buff.bsize = bsize;
	isp_dev->ptnr_buff.addr[AML_PLANE_A] = paddr;
	isp_dev->ptnr_buff.vaddr[AML_PLANE_A] = aml_subdev_map_vaddr(paddr, bsize);

	isp_dev->ops->hw_cfg_ptnr_mif_buf(isp_dev, &isp_dev->ptnr_buff);

	pr_info("ptnr alloc\n");

	return 0;
}

static int isp_subdev_ptnr_buf_free(struct isp_dev_t *isp_dev)
{
	u32 paddr = 0x0000;
	void *page = NULL;

	paddr = isp_dev->ptnr_buff.addr[AML_PLANE_A];
	page = phys_to_page(paddr);

	if (paddr)
		aml_subdev_cma_free(isp_dev->pdev, page, isp_dev->ptnr_buff.bsize);

	aml_subdev_unmap_vaddr(isp_dev->ptnr_buff.vaddr[AML_PLANE_A]);

	isp_dev->ptnr_buff.addr[AML_PLANE_A] = 0x0000;
	isp_dev->ptnr_buff.vaddr[AML_PLANE_A] = NULL;

	pr_info("ptnr free\n");

	return 0;
}

static int isp_subdev_mcnr_buf_alloc(struct isp_dev_t *isp_dev, struct aml_format *fmt)
{
	int rtn = 0;
	u32 bsize = 0;
	u32 width, height;
	u32 paddr = 0x0000;
	void *virtaddr = NULL;
	u32 iir_body_page = 0;
	u32 mix_body_page = 0;
	u32 meta_size = 0;
	u32 mv_size = 0;
	u32 iir_slice_size = 0;
	u32 mix_slice_size = 0;
	u32 iir_link_size = 0;
	u32 mix_link_size = 0;
	u32 iir_body_size = 0;
	u32 mix_body_size = 0;

	width = fmt->width;
	height = fmt->height;

	iir_body_size = (width * isp_dev->tnr_bits + width / 10) * height / 8;
	iir_body_size = ISP_SIZE_ALIGN(iir_body_size, 1 << 12);
	iir_body_page = iir_body_size >> 12;

	mix_body_size = (width * isp_dev->tnr_bits * 12 / 16 + width / 10) * height / 4 / 8;
	mix_body_size = ISP_SIZE_ALIGN(mix_body_size, 1 << 12);
	mix_body_page = mix_body_size >> 12;

	meta_size = (((width / 2 * 4 + 127) / 128) * 16) * height / 2;
	meta_size = ISP_SIZE_ALIGN(meta_size, 1 << 12);

	mv_size = (((width  / 8 * 20 + 127) / 128) * 16) * height / 8;
	mv_size = ISP_SIZE_ALIGN(mv_size, 1 << 12);

	iir_slice_size = 16 * 1024;
	iir_slice_size = ISP_SIZE_ALIGN(iir_slice_size, 1 << 12);

	mix_slice_size = 16 * 1024;
	mix_slice_size = ISP_SIZE_ALIGN(mix_slice_size, 1 << 12);

	iir_link_size = iir_body_page * sizeof(u32);
	iir_link_size = ISP_SIZE_ALIGN(iir_link_size, 1 << 12);

	mix_link_size = mix_body_page * sizeof(u32);
	mix_link_size = ISP_SIZE_ALIGN(mix_link_size, 1 << 12);

	bsize = meta_size + mv_size + iir_slice_size * 2 + mix_slice_size * 2;
	bsize += iir_link_size * 2 + mix_link_size * 2 + iir_body_size * 2 + mix_body_size * 2;

	rtn = aml_subdev_cma_alloc(isp_dev->pdev, &paddr, virtaddr, bsize);
	if (rtn != 0) {
		pr_err("Failed to alloc ptnr buff\n");
		return -1;
	}

	isp_dev->mcnr_buff.nplanes = 2;
	isp_dev->mcnr_buff.bsize = bsize;
	isp_dev->mcnr_buff.addr[AML_PLANE_A] = paddr;
	isp_dev->mcnr_buff.vaddr[AML_PLANE_A] = aml_subdev_map_vaddr(paddr, bsize);
	isp_dev->mcnr_buff.addr[AML_PLANE_B] = paddr + meta_size + mv_size;
	isp_dev->mcnr_buff.vaddr[AML_PLANE_B] = isp_dev->mcnr_buff.vaddr[AML_PLANE_A] + meta_size + mv_size;

	isp_dev->ops->hw_cfg_mcnr_mif_buf(isp_dev, fmt, &isp_dev->mcnr_buff);

	pr_debug("mcnr alloc %x, %p, %x\n", paddr, isp_dev->mcnr_buff.vaddr[AML_PLANE_A], bsize);

	return 0;
}

static int isp_subdev_mcnr_buf_free(struct isp_dev_t *isp_dev)
{
	u32 paddr = 0x0000;
	void *page = NULL;

	paddr = isp_dev->mcnr_buff.addr[AML_PLANE_A];
	page = phys_to_page(paddr);

	if (paddr)
		aml_subdev_cma_free(isp_dev->pdev, page, isp_dev->mcnr_buff.bsize);

	aml_subdev_unmap_vaddr(isp_dev->mcnr_buff.vaddr[AML_PLANE_A]);

	isp_dev->mcnr_buff.addr[AML_PLANE_A] = 0x0000;
	isp_dev->mcnr_buff.vaddr[AML_PLANE_A] = NULL;

	pr_info("mcnr free\n");

	return 0;
}

int isp_subdev_start_manual_dma(struct isp_dev_t *isp_dev)
{
	struct isp_global_info *g_info = isp_global_get_info();

	if ((isp_dev->apb_dma == 0) ||
		(isp_dev->twreg_cnt == 0) ||
		(g_info->user > 1))
		return 0;

	memcpy(isp_dev->wreg_buff.vaddr[AML_PLANE_A], isp_dev->wreg_buff.vmaddr[AML_PLANE_A], isp_dev->wreg_buff.bsize);
	dma_sync_single_for_device(isp_dev->dev, isp_dev->wreg_buff.addr[AML_PLANE_A], isp_dev->wreg_buff.bsize, DMA_TO_DEVICE);

	isp_dev->ops->hw_start_apb_dma(isp_dev);
	isp_dev->ops->hw_manual_trigger_apb_dma(isp_dev);
	isp_dev->ops->hw_check_done_apb_dma(isp_dev);
	isp_dev->ops->hw_stop_apb_dma(isp_dev);

	pr_info("manual dma count:%d-%d\n", isp_dev->fwreg_cnt, isp_dev->twreg_cnt);

	return 0;
}

int isp_subdev_start_auto_dma(struct isp_dev_t *isp_dev)
{
	if (isp_dev->apb_dma == 0)
		return 0;

	memcpy(isp_dev->wreg_buff.vaddr[AML_PLANE_A], isp_dev->wreg_buff.vmaddr[AML_PLANE_A], isp_dev->wreg_buff.bsize);
	dma_sync_single_for_device(isp_dev->dev, isp_dev->wreg_buff.addr[AML_PLANE_A], isp_dev->wreg_buff.bsize, DMA_TO_DEVICE);

	isp_dev->ops->hw_start_apb_dma(isp_dev);
	isp_dev->ops->hw_auto_trigger_apb_dma(isp_dev);
	isp_dev->ops->hw_check_done_apb_dma(isp_dev);

	pr_info("auto dma count:%d-%d\n", isp_dev->wreg_cnt, isp_dev->twreg_cnt);

	isp_dev->wreg_cnt = 0;
	isp_dev->twreg_cnt = 0;

	return 0;
}

int isp_subdev_update_auto_dma(struct isp_dev_t *isp_dev)
{
	struct isp_global_info *g_info = isp_global_get_info();

	if (g_info->mode != AML_ISP_SCAM)
		return 0;

	if ((isp_dev->apb_dma == 0) || (isp_dev->twreg_cnt == 0))
		return 0;

	memcpy(isp_dev->wreg_buff.vaddr[AML_PLANE_A], isp_dev->wreg_buff.vmaddr[AML_PLANE_A], isp_dev->twreg_cnt * 8);
	//dma_sync_single_for_device(isp_dev->dev,isp_dev->wreg_buff.addr[AML_PLANE_A], isp_dev->twreg_cnt * 8, DMA_TO_DEVICE);

	isp_dev->ops->hw_start_apb_dma(isp_dev);

	pr_debug("dma count:%d-%d\n", isp_dev->wreg_cnt, isp_dev->twreg_cnt);

	isp_dev->wreg_cnt = 0;
	isp_dev->twreg_cnt = 0;

	return 0;
}

static int isp_subdev_convert_fmt(struct isp_dev_t *isp_dev,
			struct v4l2_mbus_framefmt *mfmt,
			struct aml_format *afmt)
{
	int rtn = -EINVAL;
	u32 i = 0;

	for (i = 0; i < isp_dev->fmt_cnt; i++) {
		if (mfmt->code == isp_dev->formats[i].code) {
			afmt->bpp = isp_dev->formats[i].bpp;
			afmt->width = mfmt->width;
			afmt->height = mfmt->height;
			afmt->code = mfmt->code;
			afmt->xstart = 0;
			afmt->ystart = 0;

			rtn = 0;
			break;
		}
	}

	return rtn;
}

static int isp_subdev_set_format(void *priv, void *s_fmt, void *m_fmt)
{
	int rtn = 0;
	struct aml_format p_fmt;
	struct isp_dev_t *isp_dev = priv;
	struct v4l2_subdev_format *fmt = s_fmt;
	struct v4l2_mbus_framefmt *format = m_fmt;

	rtn = isp_subdev_convert_fmt(isp_dev, format, &p_fmt);
	if (rtn) {
		pr_err("Error to convert format\n");
		return rtn;
	}

	switch (fmt->pad) {
	case AML_ISP_PAD_SINK_VIDEO:
		isp_global_reset(isp_dev);
		isp_dev->ops->hw_fill_rreg_buff(isp_dev);
		isp_dev->ops->hw_init(isp_dev);
		isp_dev->ops->hw_set_input_fmt(isp_dev, &p_fmt);
	break;
	case AML_ISP_PAD_SINK_PATTERN:
		isp_dev->ops->hw_cfg_pattern(isp_dev, &p_fmt);
	break;
	}

	isp_dev->fmt = p_fmt;

	if (isp_dev->enWDRMode == WDR_MODE_2To1_LINE ||
		isp_dev->enWDRMode == WDR_MODE_2To1_FRAME ||
		isp_dev->enWDRMode == ISP_WDR_DCAM_LMODE ||
		isp_dev->enWDRMode == ISP_WDR_DCAM_FMODE)
		isp_dev->ops->hw_set_wdr_mode(isp_dev, 1);
	else
		isp_dev->ops->hw_set_wdr_mode(isp_dev, 0);

	return rtn;
}

static int isp_subdev_stream_on(void *priv)
{
	struct isp_dev_t *isp_dev = priv;
	struct isp_global_info *g_info = isp_global_get_info();
	int rtn = 0;

	rtn = isp_subdev_mcnr_buf_alloc(isp_dev, &isp_dev->fmt);
	if (rtn)
		return rtn;

	rtn = isp_subdev_ptnr_buf_alloc(isp_dev, &isp_dev->fmt);
	if (rtn) {
		isp_subdev_mcnr_buf_free(isp_dev);
		return rtn;
	}

	if (isp_dev->ops->hw_start)
		isp_dev->ops->hw_start(isp_dev);

	isp_dev->isp_status = STATUS_START;

	isp_global_stream_on();

	if (g_info->mode == AML_ISP_SCAM)
		isp_subdev_start_auto_dma(isp_dev);
	else
		isp_subdev_start_manual_dma(isp_dev);

	return 0;
}

static void isp_subdev_stream_off(void *priv)
{
	struct isp_dev_t *isp_dev = priv;

	isp_dev->isp_status = STATUS_STOP;
	isp_dev->enWDRMode = WDR_MODE_NONE;

	isp_global_stream_off();

	if (isp_dev->ops->hw_stop)
		isp_dev->ops->hw_stop(isp_dev);

	isp_dev->ops->hw_enable_ptnr_mif(isp_dev, 0);
	isp_dev->ops->hw_enable_mcnr_mif(isp_dev, 0);

	isp_subdev_start_manual_dma(isp_dev);

	isp_subdev_ptnr_buf_free(isp_dev);
	isp_subdev_mcnr_buf_free(isp_dev);
}

static void isp_subdev_log_status(void *priv)
{
	struct csiphy_dev_t *csiphy_dev = priv;

	dev_info(csiphy_dev->dev, "Log status done\n");
}

static const struct aml_sub_ops isp_subdev_ops = {
	.set_format = isp_subdev_set_format,
	.stream_on = isp_subdev_stream_on,
	.stream_off = isp_subdev_stream_off,
	.log_status = isp_subdev_log_status,
};

static int isp_subdev_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct isp_dev_t *isp_dev = container_of(ctrl->handler,
					     struct isp_dev_t, ctrls);
	struct isp_global_info *g_info = isp_global_get_info();
	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_AML_MODE:
		pr_info("isp_subdev_set_ctrl:%d\n", ctrl->val);
		isp_dev->enWDRMode = ctrl->val;
		g_info->mode = AML_ISP_SCAM;
		if (isp_dev->enWDRMode == ISP_SDR_DCAM_MODE ||
			isp_dev->enWDRMode == ISP_WDR_DCAM_LMODE ||
			isp_dev->enWDRMode == ISP_WDR_DCAM_FMODE)
			g_info->mode = AML_ISP_MCAM;
		break;
	default:
		pr_err( "Error ctrl->id %u, flag 0x%lx\n", ctrl->id, ctrl->flags);
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops isp_ctrl_ops = {
	.s_ctrl = isp_subdev_set_ctrl,
};

static struct v4l2_ctrl_config mode_cfg = {
	.ops = &isp_ctrl_ops,
	.id = V4L2_CID_AML_MODE,
	.name = "isp mode",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.min = 0,
	.max = 6,
	.step = 1,
	.def = 0,
};

int isp_subdev_ctrls_init(struct isp_dev_t *isp_dev)
{
	int rtn = 0;

	v4l2_ctrl_handler_init(&isp_dev->ctrls, 1);

	isp_dev->wdr = v4l2_ctrl_new_custom(&isp_dev->ctrls, &mode_cfg, NULL);

	isp_dev->sd.ctrl_handler = &isp_dev->ctrls;

	if (isp_dev->ctrls.error) {
		rtn = isp_dev->ctrls.error;
	}

	return rtn;

}

static irqreturn_t isp_subdev_irq_handler(int irq, void *dev)
{
	int status = 0, id = 0;
	unsigned long flags;
	struct isp_dev_t *isp_dev = dev;
	struct aml_video *video;
#ifdef IRQ_TIME_DEBUG
	u64 start_time, end_time, diff;
	start_time = ktime_get_real_ns();
#endif
	if (isp_dev->isp_status == STATUS_STOP) {
		if (aml_adap_global_get_vdev() == isp_dev->index) {
			pr_err("ISP%d: Stoped and Irq ignore\n", isp_dev->index);
			aml_adap_global_done_completion();
		}
		return IRQ_HANDLED;
	}

	if (aml_adap_global_get_vdev() != isp_dev->index)
		return IRQ_HANDLED;

	spin_lock_irqsave(&isp_dev->irq_lock, flags);

	status = isp_dev->ops->hw_interrupt_status(isp_dev);
	if (status & 0x1) {
		isp_dev->frm_cnt ++;
		for (id = AML_ISP_STREAM_0; id < AML_ISP_STREAM_MAX; id++) {
			video = &isp_dev->video[id];
			if (video->ops->cap_irq_handler)
				video->ops->cap_irq_handler(video, status);
		}
		tasklet_schedule(&isp_dev->irq_tasklet);
	}

	spin_unlock_irqrestore(&isp_dev->irq_lock, flags);
#ifdef IRQ_TIME_DEBUG
	end_time = ktime_get_real_ns();
	diff = end_time - start_time;
	pr_err("time consumed = %lld ns\n", diff);
#endif
	return IRQ_HANDLED;
}

static void isp_subdev_irq_tasklet(unsigned long data)
{
	int id = 0;
	int status = 0xff001234;
	struct aml_video *video;
	struct isp_dev_t *isp_dev = (struct isp_dev_t *)data;

	for (id = 0; id < AML_ISP_STREAM_0; id++) {
		video = &isp_dev->video[id];
		if (video->ops->cap_irq_handler)
			video->ops->cap_irq_handler(video, status);
	}

	isp_subdev_update_auto_dma(isp_dev);

	aml_adap_global_done_completion();
}

static int isp_subdev_parse_dev(struct isp_dev_t *isp_dev)
{
	int rtn = -EINVAL;
	struct resource *res = NULL;

	of_reserved_mem_device_init(isp_dev->dev);

	isp_dev->irq = irq_of_parse_and_map(isp_dev->dev->of_node, 0);
	if (!isp_dev->irq) {
		dev_err(isp_dev->dev, "Error to parse irq\n");
		return rtn;
	}

	//isp_dev->isp_clk = devm_clk_get(isp_dev->dev, "cts_mipi_isp_clk");
	isp_dev->isp_clk = devm_clk_get(isp_dev->dev, "mipi_isp_clk");
	if (IS_ERR(isp_dev->isp_clk)) {
		dev_err(isp_dev->dev, "Error to get isp_clk\n");
		return PTR_ERR(isp_dev->isp_clk);
	}

	res = platform_get_resource(isp_dev->pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(isp_dev->dev, "Error to get mem\n");
		return rtn;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	isp_dev->base = devm_ioremap(isp_dev->dev, res->start, resource_size(res));
#else
	isp_dev->base = devm_ioremap_nocache(isp_dev->dev, res->start, resource_size(res));
#endif
	if (!isp_dev->base) {
		dev_err(isp_dev->dev, "Error to ioremap mem\n");
		return rtn;
	}

	return 0;
}

static int isp_subdev_power_on(struct isp_dev_t *isp_dev)
{
	int rtn = 0;

	dev_pm_domain_attach(isp_dev->dev, true);
	pm_runtime_enable(isp_dev->dev);
	pm_runtime_get_sync(isp_dev->dev);

	switch (isp_dev->index) {
	case 0:
	case 1:
	case 2:
	case 3:
		clk_set_rate(isp_dev->isp_clk, 666666666);
	break;
	default:
		dev_err(isp_dev->dev, "ISP%d: Error to set clk rate\n", isp_dev->index);
	break;
	}
	rtn = clk_prepare_enable(isp_dev->isp_clk);
	if (rtn)
		dev_err(isp_dev->dev, "Error to enable isp_clk\n");

	return rtn;
}

static void isp_subdev_power_off(struct isp_dev_t *isp_dev)
{
	clk_disable_unprepare(isp_dev->isp_clk);

	pm_runtime_put_sync(isp_dev->dev);
	pm_runtime_disable(isp_dev->dev);
	dev_pm_domain_detach(isp_dev->dev, true);
}

int isp_subdev_resume(struct isp_dev_t *isp_dev)
{
	int rtn = 0;

	pm_runtime_enable(isp_dev->dev);
	pm_runtime_get_sync(isp_dev->dev);

	rtn = clk_prepare_enable(isp_dev->isp_clk);
	if (rtn)
		dev_err(isp_dev->dev, "Error to enable isp_clk\n");

	return rtn;
}

void isp_subdev_suspend(struct isp_dev_t *isp_dev)
{
	clk_disable_unprepare(isp_dev->isp_clk);

	pm_runtime_put_sync(isp_dev->dev);
	pm_runtime_disable(isp_dev->dev);
}

int aml_isp_subdev_init(void *c_dev)
{
	int rtn = -1;
	struct platform_device *pdev;
	struct resource *res;
	struct isp_dev_t *isp_dev;
	struct device_node *node;
	struct device *dev;
	struct cam_device *cam_dev = c_dev;

	isp_dev = &cam_dev->isp_dev;

	node = of_parse_phandle(cam_dev->dev->of_node, "isp", 0);
	if (!node) {
		pr_err("Failed to parse isp handle\n");
		return rtn;
	}

	isp_dev->pdev = of_find_device_by_node(node);
	if (!isp_dev->pdev) {
		of_node_put(node);
		pr_err("Failed to find isp platform device\n");
		return rtn;
	}
	of_node_put(node);

	pdev = isp_dev->pdev;
	dev = &pdev->dev;
	isp_dev->dev = dev;
	isp_dev->phy_base = ISP_REG_BASE;
	isp_dev->index = cam_dev->index;
	isp_dev->v4l2_dev = &cam_dev->v4l2_dev;
	isp_dev->bus_info = cam_dev->bus_info;
	isp_dev->emb_dev = &cam_dev->adap_dev;
	isp_dev->ops = &isp_hw_ops;
	isp_dev->emb_ops = &emb_hw_ops;
	isp_dev->apb_dma = 1;
	isp_dev->slice = 0;
	isp_dev->enWDRMode = WDR_MODE_NONE;
	isp_dev->mcnr_en = 0;
	isp_dev->tnr_bits = 8;
	platform_set_drvdata(pdev, isp_dev);

	rtn = isp_subdev_parse_dev(isp_dev);
	if (rtn) {
		dev_err(isp_dev->dev, "Failed to parse dev\n");
		return rtn;
	}

	rtn = devm_request_irq(dev, isp_dev->irq,
			isp_subdev_irq_handler, IRQF_SHARED,
			dev_driver_string(dev), isp_dev);
	if (rtn) {
		dev_err(isp_dev->dev, "Error to request irq: rtn %d\n", rtn);
		devm_iounmap(dev, isp_dev->base);
		return rtn;
	}

	spin_lock_init(&isp_dev->irq_lock);
	tasklet_init(&isp_dev->irq_tasklet, isp_subdev_irq_tasklet, (unsigned long)isp_dev);

	isp_subdev_power_on(isp_dev);

	isp_subdrv_reg_buf_alloc(isp_dev);

	isp_global_init(isp_dev);

	dev_info(isp_dev->dev, "ISP%u: subdev init\n", isp_dev->index);

	g_isp_dev[cam_dev->index] = isp_dev;

	return rtn;
}

void aml_isp_subdev_deinit(void *c_dev)
{
	struct isp_dev_t *isp_dev;
	struct cam_device *cam_dev = c_dev;

	isp_dev = &cam_dev->isp_dev;

	tasklet_kill(&isp_dev->irq_tasklet);

	isp_subdev_power_off(isp_dev);

	devm_free_irq(isp_dev->dev, isp_dev->irq, isp_dev);

	devm_iounmap(isp_dev->dev, isp_dev->base);

	devm_clk_put(isp_dev->dev, isp_dev->isp_clk);

	isp_subdrv_reg_buf_free(isp_dev);

	isp_global_deinit(isp_dev);

	dev_info(isp_dev->dev, "ISP%u: subdev deinit\n", isp_dev->index);
}

static int isp_proc_show(struct seq_file *proc_entry, void *arg ) {

	struct isp_dev_t *isp_dev = proc_entry->private;
	u32 *isp_info = isp_dev->ops->hw_status(isp_dev);

	seq_printf(
		proc_entry, "\n****************** ISP MODULE PARAM *******************\n");

	seq_printf(proc_entry, " ------- Pubattr Info ------- \n");
	seq_printf(proc_entry, "%8s" "%8s" "%8s" "%8s" "%8s \n"
				, "WndX", "WndY", "snsW", "snsH", "bpp");
	seq_printf(proc_entry, "%8d" "%8d" "%8d" "%8d" "%8d \n\n",
					isp_dev->fmt.xstart,
					isp_dev->fmt.ystart,
					isp_dev->fmt.width,
					isp_dev->fmt.height,
					isp_dev->fmt.bpp);

	seq_printf(proc_entry, " ------- Isp Solution ------- \n");
	seq_printf(proc_entry, "%15s" "%15s \n", "input vsize", "input hsize");
	seq_printf(proc_entry, "%15d" "%15d \n\n", *isp_info, *(isp_info + 1) );

	seq_printf(proc_entry, " ------- wrmif solution ------- \n");
	seq_printf(proc_entry, "%15s" "%15s \n", "fmt hsize", "fmt vsize");
	seq_printf(proc_entry, "%4s" "%11d" "%15d \n", "chn0", *(isp_info + 2), *(isp_info + 3) );
	seq_printf(proc_entry, "%4s" "%11d" "%15d \n", "chn1", *(isp_info + 4), *(isp_info + 5) );
	seq_printf(proc_entry, "%4s" "%11d" "%15d \n", "chn2", *(isp_info + 6), *(isp_info + 7) );
	seq_printf(proc_entry, "%4s" "%11d" "%15d \n", "chn3", *(isp_info + 8), *(isp_info + 9) );
	seq_printf(proc_entry, "%s\n", "wrmif_en");
	seq_printf(proc_entry, "0x%02x\n", *(isp_info + 10));

	seq_printf(proc_entry, "%15s" "%15s \n", "crp2 vsize", "crp2 hsize");
	seq_printf(proc_entry, "%4s" "%11d" "%15d \n", "pps0", *(isp_info + 11), *(isp_info + 12) );
	seq_printf(proc_entry, "%4s" "%11d" "%15d \n", "pps1", *(isp_info + 13), *(isp_info + 14) );
	seq_printf(proc_entry, "%4s" "%11d" "%15d \n", "pps2", *(isp_info + 15), *(isp_info + 16) );
	seq_printf(proc_entry, "%4s" "%11d" "%15d \n\n", "pps3", *(isp_info + 17), *(isp_info + 18) );

	seq_printf(proc_entry, " ------- 3a solution ------- \n");
	seq_printf(proc_entry, "%18s" "%18s \n", "af stat vsize", "af stat hsize");
	seq_printf(proc_entry, "%18d" "%18d \n", *(isp_info + 19), *(isp_info + 20) );
	seq_printf(proc_entry, "%18s" "%18s \n", "ae stat vsize", "ae stat hsize");
	seq_printf(proc_entry, "%18d" "%18d \n", *(isp_info + 21), *(isp_info + 22) );
	seq_printf(proc_entry, "%18s" "%18s \n", "awb stat vsize", "awb stat hsize");
	seq_printf(proc_entry, "%18d" "%18d \n\n", *(isp_info + 23), *(isp_info + 24) );

	seq_printf(proc_entry, " ------- dma configuration ------- \n");
	seq_printf(proc_entry, "%10s" "%5s" "%15s" "%15s" "%15s" "%15s \n"
					, "chx_size0-4", "af", "awb", "ae", "flkr", "post");
	seq_printf(proc_entry, "%16d" "%15d" "%15d" "%15d" "%15d \n\n",
					*(isp_info + 25),
					*(isp_info + 26),
					*(isp_info + 27),
					*(isp_info + 28),
					*(isp_info + 29));
	seq_printf(proc_entry, "%15s \n", "wreg_cnt");
	seq_printf(proc_entry, "%15d\n\n", isp_dev->wreg_cnt);

	seq_printf(proc_entry, " ------- isp checksum ------- \n");
	seq_printf(proc_entry, " -- ro_checksum_dat_[0-9] -- \n");
	seq_printf(proc_entry, "%15s" "%15s" "%15s" "%15s" "%15s \n"
					, "ofe_din", "ofe_dout", "dfe_dout", "obe_dout", "dms_dout");
	seq_printf(proc_entry, "%15u" "%15u" "%15u" "%15u" "%15u \n\n",
					*(isp_info + 30),
					*(isp_info + 31),
					*(isp_info + 32),
					*(isp_info + 33),
					*(isp_info + 34));
	seq_printf(proc_entry, "%15s" "%15s" "%15s" "%15s" "%15s \n"
					, "post_dout", "post_ir", "disp_0", "disp_1", "disp_2");
	seq_printf(proc_entry, "%15u" "%15u" "%15u" "%15u" "%15u \n\n",
					*(isp_info + 35),
					*(isp_info + 36),
					*(isp_info + 37),
					*(isp_info + 38),
					*(isp_info + 39));

	seq_printf(proc_entry, "******************* PROC END *******************\n\n");


	return 0;
}

static int isp_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, isp_proc_show, PDE_DATA(inode));
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
static const struct proc_ops isp_proc_file_ops = {
	/// kernel 5.15
	.proc_open = isp_debug_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations isp_proc_file_ops = {
	.open = isp_debug_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static int isp_proc_init(struct isp_dev_t *isp_dev)
{
	int rtn = -1;
	char file_name[50] = {0};
	struct aml_subdev *subdev = &isp_dev->subdev;

	sprintf(file_name, "%s%d", "isp", isp_dev->index);
	subdev->proc_node_entry = proc_create_data(file_name, 0644, NULL, &isp_proc_file_ops, isp_dev);
	if (subdev->proc_node_entry == NULL) {
		dev_err(isp_dev->dev, "isp%u create proc failed!\n", isp_dev->index);
		return rtn;
	}

	return 0;
}

static void isp_proc_exit(struct isp_dev_t *isp_dev) {

	char file_name[50] = {0};
	sprintf(file_name, "%s%d", "isp", isp_dev->index);

	remove_proc_entry(file_name, NULL);
}

int aml_isp_subdev_register(struct isp_dev_t *isp_dev)
{
	int rtn = -1;
	struct media_pad *pads = isp_dev->pads;
	struct aml_subdev *subdev = &isp_dev->subdev;

	isp_dev->formats = isp_subdev_formats;
	isp_dev->fmt_cnt = ARRAY_SIZE(isp_subdev_formats);

	pads[AML_ISP_PAD_SINK_VIDEO].flags = MEDIA_PAD_FL_SINK;
	pads[AML_ISP_PAD_SINK_PATTERN].flags = MEDIA_PAD_FL_SINK;
	pads[AML_ISP_PAD_SINK_DDR].flags = MEDIA_PAD_FL_SINK;
	pads[AML_ISP_PAD_SINK_PARAM].flags = MEDIA_PAD_FL_SINK;

	pads[AML_ISP_PAD_SOURCE_STATS].flags = MEDIA_PAD_FL_SOURCE;
	pads[AML_ISP_PAD_SOURCE_STREAM_0].flags = MEDIA_PAD_FL_SOURCE;
	pads[AML_ISP_PAD_SOURCE_STREAM_1].flags = MEDIA_PAD_FL_SOURCE;
	pads[AML_ISP_PAD_SOURCE_STREAM_2].flags = MEDIA_PAD_FL_SOURCE;
	pads[AML_ISP_PAD_SOURCE_STREAM_3].flags = MEDIA_PAD_FL_SOURCE;
	pads[AML_ISP_PAD_SOURCE_STREAM_RAW].flags = MEDIA_PAD_FL_SOURCE;

	subdev->name = AML_ISP_NAME;
	subdev->dev = isp_dev->dev;
	subdev->sd = &isp_dev->sd;
	subdev->function = MEDIA_ENT_F_PROC_VIDEO_PIXEL_FORMATTER;
	subdev->v4l2_dev = isp_dev->v4l2_dev;
	subdev->pads = pads;
	subdev->pad_max = AML_ISP_PAD_MAX;
	subdev->formats = isp_dev->formats;
	subdev->fmt_cnt = isp_dev->fmt_cnt;
	subdev->pfmt = isp_dev->pfmt;
	subdev->ops = &isp_subdev_ops;
	subdev->priv = isp_dev;

	rtn = isp_proc_init(isp_dev);
	if (rtn)
		goto error_rtn;

	isp_subdev_ctrls_init(isp_dev);

	rtn = aml_subdev_register(subdev);
	if (rtn)
		goto error_rtn;

	dev_info(isp_dev->dev, "ISP%u: register subdev\n", isp_dev->index);

error_rtn:
	return rtn;
}

void aml_isp_subdev_unregister(struct isp_dev_t *isp_dev)
{
	isp_proc_exit(isp_dev);
	aml_subdev_unregister(&isp_dev->subdev);
}
