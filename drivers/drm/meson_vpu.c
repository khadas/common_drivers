// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <drm/drmP.h>
#include <drm/drm_plane.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_gem_cma_helper.h>

#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/component.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-map-ops.h>
#include <linux/clk.h>
#include <linux/cma.h>
#include <linux/sync_file.h>

/* Amlogic Headers */
#include <linux/amlogic/media/vout/vout_notify.h>
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT
#include <linux/amlogic/media/amvecm/amvecm.h>
#endif
#include <drm/amlogic/meson_drm_bind.h>
#include <vout/vout_serve/vout_func.h>
#ifdef CONFIG_AMLOGIC_DRM_USE_ION
#include "meson_fb.h"
#endif
#include "meson_vpu.h"
#include "meson_plane.h"
#include "meson_crtc.h"
#include "meson_vpu_pipeline.h"
#include "vpu-hw/meson_vpu_postblend.h"

#define AM_VOUT_NULL_MODE "null"

void meson_vout_notify_mode_change(int idx,
		enum vmode_e mode, enum meson_vout_event event)
{
	/*pre_process event*/
	if (event == EVENT_MODE_SET_START) {
		switch (idx) {
		case 1:
			vout_set_uevent(VOUT_EVENT_MODE_CHANGE, 1);
			vout_notifier_call_chain(VOUT_EVENT_MODE_CHANGE_PRE, &mode);
			break;
		case 2:
#ifdef CONFIG_AMLOGIC_VOUT2_SERVE
			vout2_set_uevent(VOUT_EVENT_MODE_CHANGE, 1);
			vout2_notifier_call_chain(VOUT_EVENT_MODE_CHANGE_PRE, &mode);
#endif
			break;
		case 3:
#ifdef CONFIG_AMLOGIC_VOUT3_SERVE
			vout3_set_uevent(VOUT_EVENT_MODE_CHANGE, 1);
			vout3_notifier_call_chain(VOUT_EVENT_MODE_CHANGE_PRE, &mode);
#endif
			break;
		default:
			DRM_ERROR("%s:unknown vout %d\n", __func__, idx);
			break;
		};
	} else if (event == EVENT_MODE_SET_FINISH) {
		switch (idx) {
		case 1:
			vout_notifier_call_chain(VOUT_EVENT_MODE_CHANGE, &mode);
			vout_set_uevent(VOUT_EVENT_MODE_CHANGE, 0);
			break;
		case 2:
#ifdef CONFIG_AMLOGIC_VOUT2_SERVE
			vout2_notifier_call_chain(VOUT_EVENT_MODE_CHANGE, &mode);
			vout2_set_uevent(VOUT_EVENT_MODE_CHANGE, 0);
#endif
			break;
		case 3:
#ifdef CONFIG_AMLOGIC_VOUT3_SERVE
			vout3_notifier_call_chain(VOUT_EVENT_MODE_CHANGE, &mode);
			vout3_set_uevent(VOUT_EVENT_MODE_CHANGE, 0);
#endif
			break;
		default:
			DRM_ERROR("%s: unknown vout %d\n", __func__, idx);
			break;
		};
	}
}

/*debug only, set mode to vout_server, so display/mode can be still used.*/
void meson_vout_update_mode_name(int idx, char *modename, char *ctx)
{
	DRM_INFO("%s: %s update vout %d name %s.\n",
		__func__, ctx, idx, modename);
	if (idx == 1)
		set_vout_mode_name(modename);
#ifdef CONFIG_AMLOGIC_VOUT2_SERVE
	else if (idx == 2)
		set_vout2_mode_name(modename);
#endif
#ifdef CONFIG_AMLOGIC_VOUT3_SERVE
	else if (idx == 3)
		set_vout3_mode_name(modename);
#endif
	else
		DRM_ERROR("%s:unsupported vout idx %d.\n", __func__, idx);
}

static void meson_drm_handle_vpp_crc(struct am_meson_crtc *amcrtc)
{
	u32 crc;
	struct drm_crtc *crtc = &amcrtc->base;

	if (amcrtc->vpp_crc_enable && cpu_after_eq(MESON_CPU_MAJOR_ID_SM1)) {
		crc = meson_vpu_read_reg(VPP_RO_CRCSUM);
		drm_crtc_add_crc_entry(crtc, true,
				       drm_crtc_accurate_vblank_count(crtc),
				       &crc);
	}
}

static void meson_drm_signal_present_fence(struct am_meson_crtc *amcrtc)
{
	struct am_meson_crtc_present_fence *pre_fence = &amcrtc->present_fence;

	if (pre_fence->fence) {
		DRM_DEBUG("%s fd=%d, fence=%px\n", __func__,
			pre_fence->fd, pre_fence->fence);
		dma_fence_signal(pre_fence->fence);
		dma_fence_put(pre_fence->fence);
		pre_fence->fence = NULL;
	}
}

void am_meson_crtc_handle_vsync(struct am_meson_crtc *amcrtc)
{
	unsigned long flags;
	struct drm_crtc *crtc;

	crtc = &amcrtc->base;
	drm_crtc_handle_vblank(crtc);

	spin_lock_irqsave(&crtc->dev->event_lock, flags);
	if (amcrtc->event) {
		drm_crtc_send_vblank_event(crtc, amcrtc->event);
		amcrtc->event = NULL;
	}
	spin_unlock_irqrestore(&crtc->dev->event_lock, flags);

	meson_drm_signal_present_fence(amcrtc);
	meson_drm_handle_vpp_crc(amcrtc);
}

static irqreturn_t am_meson_vpu_irq(int irq, void *arg)
{
	struct am_meson_crtc *amcrtc = arg;
	struct meson_drm *priv = amcrtc->priv;

	if (!priv->irq_enabled)
		return IRQ_NONE;

	am_meson_crtc_handle_vsync(amcrtc);
	amcrtc->priv->pan_async_commit_ran = false;

	return IRQ_HANDLED;
}

static void am_meson_vpu_power_config(bool en)
{
	meson_vpu_power_config(VPU_MAIL_AFBCD, en);
	meson_vpu_power_config(VPU_VIU_OSD2, en);
	meson_vpu_power_config(VPU_VIU_OSD_SCALE, en);
	meson_vpu_power_config(VPU_VD2_OSD2_SCALE, en);
	meson_vpu_power_config(VPU_VIU_OSD3, en);
	meson_vpu_power_config(VPU_OSD_BLD34, en);
	meson_vpu_power_config(VPU_VIU_OSD2, en);

	meson_vpu_power_config(VPU_VIU2_OSD1, en);
	meson_vpu_power_config(VPU_VIU2_OSD1, en);
	meson_vpu_power_config(VPU_VIU2_OSD_ROT, en);
}

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
static void g12b_pipeline_pre_init(struct meson_vpu_pipeline *pipeline, struct device *dev)
{
	struct clk *vpu_clkc;

	vpu_clkc = devm_clk_get(dev, "vpu_clkc");
	if (IS_ERR_OR_NULL(vpu_clkc)) {
		DRM_ERROR("cannot get vpu_clkc\n");
		vpu_clkc = NULL;
	} else {
		int vpu_clkc_rate;

		/* use default clock rate in dts */
		clk_prepare_enable(vpu_clkc);
		vpu_clkc_rate = clk_get_rate(vpu_clkc);
		DRM_INFO("vpu clkc clock is %d MHZ\n",
				vpu_clkc_rate / 1000000);
	}
}
#endif

static void vpu_pipeline_pre_init(struct meson_vpu_pipeline *pipeline, struct device *dev)
{
	struct meson_drm *private = pipeline->priv;
	struct meson_vpu_data *vpu_data = private->vpu_data;
	int i;

	for (i = 0; i < pipeline->num_postblend; i++)
		pipeline->subs[i].reg_ops = &vpu_data->crtc_func.reg_ops[i];

	if (vpu_data->crtc_func.pre_init)
		vpu_data->crtc_func.pre_init(pipeline, dev);
}

static int am_meson_vpu_bind(struct device *dev,
			     struct device *master, void *data)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct meson_drm_bound_data *bound_data = data;
	struct drm_device *drm_dev = bound_data->drm;
	struct meson_drm *private = drm_dev->dev_private;
	struct meson_vpu_pipeline *pipeline = private->pipeline;
	struct am_meson_crtc *amcrtc;
	struct meson_vpu_data *vpu_data;
	int i, ret, irq;

	DRM_DEBUG("%s in[%d]\n", __func__, __LINE__);

	vpu_data = (struct meson_vpu_data *)of_device_get_match_data(dev);
	if (!vpu_data)
		return -EFAULT;
	private->vpu_data = vpu_data;
	pipeline->ops = private->vpu_data->pipe_ops;

	vpu_topology_populate(pipeline);
	meson_vpu_block_state_init(private, private->pipeline);

	meson_of_init(dev, drm_dev, private);

	ret = am_meson_plane_create(private);
	if (ret) {
		dev_err(dev, "am_meson_plane_create FAILED [%d]\n", ret);
		return ret;
	}

	/*subpipeline/postblend/crtc have same index.*/
	for (i = 0; i < pipeline->num_postblend; i++) {
		if (pipeline->subs[i].index == -1)
			break;

		amcrtc = meson_crtc_bind(private, pipeline->subs[i].index);
		if (!amcrtc) {
			dev_err(dev, "create crtc %d failed\n", i);
			break;
		}

		irq = platform_get_irq(pdev, i);
		if (irq < 0) {
			dev_err(dev, "cannot find irq for crtc %d\n", i);
			return irq;
		}
		amcrtc->irq = (unsigned int)irq;
		ret = devm_request_irq(dev, amcrtc->irq, am_meson_vpu_irq,
					IRQF_SHARED, dev_name(dev), amcrtc);
		if (ret)
			return ret;
	}

	vpu_pipeline_pre_init(pipeline, dev);
	vpu_pipeline_init(pipeline);

	/* HW config for different VPUs */
	if (vpu_data && vpu_data->crtc_func.init_default_reg)
		vpu_data->crtc_func.init_default_reg();

	if (0)
		am_meson_vpu_power_config(1);
	else
		osd_vpu_power_on();

	DRM_DEBUG("%s out[%d]\n", __func__, __LINE__);
	return 0;
}

static void am_meson_vpu_unbind(struct device *dev,
				struct device *master, void *data)
{
	struct meson_drm_bound_data *bound_data = data;
	struct drm_device *drm_dev = bound_data->drm;
	struct meson_drm *private = drm_dev->dev_private;

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT
	amvecm_drm_gamma_disable(0);
	am_meson_ctm_disable();
#endif
	am_meson_vpu_power_config(0);
	vpu_pipeline_fini(private->pipeline);
}

static const struct component_ops am_meson_vpu_component_ops = {
	.bind = am_meson_vpu_bind,
	.unbind = am_meson_vpu_unbind,
};

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT_C1A
static const struct meson_vpu_data vpu_g12a_data = {
	.crtc_func = {
		.reg_ops = common_reg_ops,
	},
	.pipe_ops = &g12a_vpu_pipeline_ops,
	.osd_ops = &osd_ops,
	.afbc_ops = &afbc_ops,
	.scaler_ops = &scaler_ops,
	.osdblend_ops = &osdblend_ops,
	.hdr_ops = &hdr_ops,
	.dv_ops = &db_ops,
	.postblend_ops = &postblend_ops,
	.video_ops = &video_ops,
	.osd_formats = &osd_formats,
	.video_formats = &video_formats,
};
#endif

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
static const struct meson_vpu_data vpu_g12b_data = {
	.crtc_func = {
		.reg_ops = g12b_reg_ops,
		.pre_init = g12b_pipeline_pre_init,
	},
	.pipe_ops = &g12a_vpu_pipeline_ops,
	.osd_ops = &g12b_osd_ops,
	.afbc_ops = &afbc_ops,
	.scaler_ops = &scaler_ops,
	.osdblend_ops = &osdblend_ops,
	.hdr_ops = &hdr_ops,
	.dv_ops = &db_ops,
	.postblend_ops = &g12b_postblend_ops,
	.video_ops = &video_ops,
};

static const struct meson_vpu_data vpu_s7_data = {
	.crtc_func = {
		.reg_ops = common_reg_ops,
	},
	.pipe_ops = &g12a_vpu_pipeline_ops,
	.osd_ops = &t7_osd_ops,
	.afbc_ops = &s7_afbc_ops,
	.scaler_ops = &scaler_ops,
	.osdblend_ops = &osdblend_ops,
	.hdr_ops = &hdr_ops,
	.dv_ops = &db_ops,
	.postblend_ops = &s7_postblend_ops,
	.video_ops = &video_ops,
	.osd_formats = &osd_formats_s1a,
	.video_formats = &video_formats,
};

static const struct meson_vpu_data vpu_t7_data = {
	.crtc_func = {
		.reg_ops = common_reg_ops,
	},
	.pipe_ops = &t7_vpu_pipeline_ops,
	.osd_ops = &t7_osd_ops,
	.afbc_ops = &t7_afbc_ops,
	.scaler_ops = &scaler_ops,
	.osdblend_ops = &osdblend_ops,
	.hdr_ops = &hdr_ops,
	.dv_ops = &db_ops,
	.postblend_ops = &t7_postblend_ops,
	.video_ops = &video_ops,
	.osd_formats = &osd_formats,
	.video_formats = &video_formats,
	.enc_method = 1,
	.max_osdblend_width = 3840,
	.max_osdblend_height = 2160,
};

static const struct meson_vpu_data vpu_t3_data = {
	.crtc_func = {
		.reg_ops = common_reg_ops,
	},
	.pipe_ops = &t7_vpu_pipeline_ops,
	.osd_ops = &t7_osd_ops,
	.afbc_ops = &t3_afbc_ops,
	.scaler_ops = &scaler_ops,
	.osdblend_ops = &osdblend_ops,
	.hdr_ops = &hdr_ops,
	.dv_ops = &db_ops,
	.postblend_ops = &t3_postblend_ops,
	.video_ops = &video_ops,
	.osd_formats = &osd_formats,
	.video_formats = &video_formats,
	.enc_method = 1,
};

static const struct meson_vpu_data vpu_t5w_data = {
	.crtc_func = {
		.reg_ops = common_reg_ops,
	},
	.pipe_ops = &t7_vpu_pipeline_ops,
	.osd_ops = &t7_osd_ops,
	.afbc_ops = &t3_afbc_ops,
	.scaler_ops = &scaler_ops,
	.osdblend_ops = &osdblend_ops,
	.hdr_ops = &hdr_ops,
	.dv_ops = &db_ops,
	.postblend_ops = &t3_postblend_ops,
	.video_ops = &video_ops,
	.osd_formats = &osd_formats,
	.video_formats = &video_formats,
	.enc_method = 1,
};

static const struct meson_vpu_data vpu_t5m_data = {
	.crtc_func = {
		.reg_ops = common_reg_ops,
	},
	.pipe_ops = &t7_vpu_pipeline_ops,
	.osd_ops = &t7_osd_ops,
	.afbc_ops = &t3_afbc_ops,
	.scaler_ops = &scaler_ops,
	.osdblend_ops = &osdblend_ops,
	.hdr_ops = &hdr_ops,
	.dv_ops = &db_ops,
	.postblend_ops = &t3_postblend_ops,
	.video_ops = &video_ops,
	.osd_formats = &osd_formats_t5m,
	.video_formats = &video_formats,
	.enc_method = 1,
};

static const struct meson_vpu_data vpu_s5_data = {
	.crtc_func = {
		.reg_ops = common_reg_ops,
	},
	.pipe_ops = &s5_vpu_pipeline_ops,
	.osd_ops = &s5_osd_ops,
	.afbc_ops = &s5_afbc_ops,
	.scaler_ops = &s5_scaler_ops,
	.osdblend_ops = &s5_osdblend_ops,
	.hdr_ops = &hdr_ops,
	.dv_ops = &db_ops,
	.postblend_ops = &s5_postblend_ops,
	.video_ops = &video_ops,
	.slice2ppc_ops = &slice2ppc_ops,
	.osd_formats = &osd_formats,
	.video_formats = &video_formats,
	.enc_method = 1,
	.slice_mode = 1,
	.max_osdblend_width = 3840,
	.max_osdblend_height = 2160,
};

static const struct meson_vpu_data vpu_t3x_data = {
	.crtc_func = {
		.reg_ops = common_reg_ops,
	},
	.pipe_ops = &s5_vpu_pipeline_ops,
	.osd_ops = &s5_osd_ops,
	.afbc_ops = &t3x_afbc_ops,
	.scaler_ops = &s5_scaler_ops,
	.osdblend_ops = &t3x_osdblend_ops,
	.hdr_ops = &hdr_ops,
	.dv_ops = &db_ops,
	.postblend_ops = &t3x_postblend_ops,
	.video_ops = &video_ops,
	.slice2ppc_ops = &slice2ppc_ops,
	.osd_formats = &osd_formats_t3x,
	.video_formats = &video_formats,
	.enc_method = 1,
	.slice_mode = 1,
	.max_osdblend_width = 3840,
	.max_osdblend_height = 2160,
};

static const struct meson_vpu_data vpu_txhd2_data = {
		.crtc_func = {
		.reg_ops = common_reg_ops,
	},
	.pipe_ops = &g12a_vpu_pipeline_ops,
	.osd_ops = &osd_ops,
	.afbc_ops = &afbc_ops,
	.scaler_ops = &scaler_ops,
	.osdblend_ops = &txhd2_osdblend_ops,
	.hdr_ops = &hdr_ops,
	.dv_ops = &db_ops,
	.postblend_ops = &txhd2_postblend_ops,
	.video_ops = &video_ops,
	.osd_formats = &osd_formats,
	.video_formats = &video_formats,
};
#endif

static const struct meson_vpu_data vpu_s1a_data = {
	.crtc_func = {
		.reg_ops = common_reg_ops,
	},
	.pipe_ops = &g12a_vpu_pipeline_ops,
	.osd_ops = &osd_ops,
	.afbc_ops = &afbc_ops,
	.scaler_ops = &scaler_ops,
	.osdblend_ops = &osdblend_ops,
	.hdr_ops = &hdr_ops,
	.dv_ops = &db_ops,
	.postblend_ops = &postblend_ops,
	.video_ops = &video_ops,
	.osd_formats = &osd_formats_s1a,
	.video_formats = &video_formats,
};

static const struct of_device_id am_meson_vpu_driver_dt_match[] = {
#ifndef CONFIG_AMLOGIC_REMOVE_OLD
	{ .compatible = "amlogic, meson-gxbb-vpu",},
	{ .compatible = "amlogic, meson-gxl-vpu",},
	{ .compatible = "amlogic, meson-gxm-vpu",},
	{ .compatible = "amlogic, meson-txl-vpu",},
	{ .compatible = "amlogic, meson-txlx-vpu",},
	{ .compatible = "amlogic, meson-axg-vpu",},
	{.compatible = "amlogic, meson-tl1-vpu",},
#endif
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	{ .compatible = "amlogic, meson-g12a-vpu",
	  .data = &vpu_g12a_data,},
	{ .compatible = "amlogic, meson-g12b-vpu",
	  .data = &vpu_g12b_data,},
	{.compatible = "amlogic, meson-sm1-vpu",
	  .data = &vpu_g12b_data,},
	{.compatible = "amlogic, meson-tm2-vpu",
	  .data = &vpu_g12a_data,},
	{.compatible = "amlogic, meson-t5-vpu",
	  .data = &vpu_g12a_data,},
	{.compatible = "amlogic, meson-sc2-vpu",
	  .data = &vpu_g12a_data,},
#endif
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT_C1A
	{.compatible = "amlogic, meson-s4-vpu",
	  .data = &vpu_g12a_data,},
#endif
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	{.compatible = "amlogic, meson-t7-vpu",
	 .data = &vpu_t7_data,},
	{.compatible = "amlogic, meson-t5w-vpu",
	 .data = &vpu_t5w_data,},
	{.compatible = "amlogic, meson-t3-vpu",
	 .data = &vpu_t3_data,},
	{.compatible = "amlogic, meson-s5-vpu",
	 .data = &vpu_s5_data,},
	{.compatible = "amlogic, meson-t3x-vpu",
	 .data = &vpu_t3x_data,},
	{.compatible = "amlogic, meson-txhd2-vpu",
	  .data = &vpu_txhd2_data,},
	{.compatible = "amlogic, meson-t5m-vpu",
	 .data = &vpu_t5m_data,},
	{.compatible = "amlogic, meson-s7-vpu",
	  .data = &vpu_s7_data,},
#endif
	{.compatible = "amlogic, meson-s1a-vpu",
	  .data = &vpu_s1a_data,},
	{}
};

MODULE_DEVICE_TABLE(of, am_meson_vpu_driver_dt_match);

static int am_meson_vpu_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	DRM_DEBUG("%s in[%d]\n", __func__, __LINE__);
	if (!dev->of_node) {
		dev_err(dev, "can't find vpu devices\n");
		return -ENODEV;
	}
	DRM_DEBUG("%s out[%d]\n", __func__, __LINE__);
	return component_add(dev, &am_meson_vpu_component_ops);
}

static int am_meson_vpu_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &am_meson_vpu_component_ops);

	return 0;
}

static struct platform_driver am_meson_vpu_platform_driver = {
	.probe = am_meson_vpu_probe,
	.remove = am_meson_vpu_remove,
	.driver = {
		.name = "meson-vpu",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(am_meson_vpu_driver_dt_match),
	},
};

int __init am_meson_vpu_init(void)
{
	return platform_driver_register(&am_meson_vpu_platform_driver);
}

void __exit am_meson_vpu_exit(void)
{
	platform_driver_unregister(&am_meson_vpu_platform_driver);
}

#ifndef MODULE
module_init(am_meson_vpu_init);
module_exit(am_meson_vpu_exit);
#endif

MODULE_AUTHOR("MultiMedia Amlogic <multimedia-sh@amlogic.com>");
MODULE_DESCRIPTION("Amlogic Meson Drm VPU driver");
MODULE_LICENSE("GPL");
