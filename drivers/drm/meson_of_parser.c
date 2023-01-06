// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>

#include "meson_drv.h"
#include "meson_vpu_pipeline.h"

static void meson_parse_crtc_masks(struct device_node *node, struct meson_of_conf *conf)
{
	int i, ret;
	u32 crtc_masks[ENCODER_MAX];

	ret = 0;
	/*initialize encoders crtc_masks, it will replaced by dts*/
	for (i = 0; i < ENCODER_MAX; i++)
		conf->crtc_masks[i] = 1;

	ret = of_property_read_u32_array(node, "crtc_masks",
		crtc_masks, ENCODER_MAX);
	if (ret) {
		DRM_DEBUG("crtc_masks get fail!\n");
	} else {
		for (i = 0; i < ENCODER_MAX; i++)
			conf->crtc_masks[i] = crtc_masks[i];
	}
}

static void meson_parse_dma_mask(struct device *dev)
{
	int ret, vpu_dma_mask;

	ret = 0;
	vpu_dma_mask = 0;

	ret = of_property_read_u32(dev->of_node, "vpu_dma_mask", &vpu_dma_mask);
	if (!ret && vpu_dma_mask == 1) {
		ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(64));
		if (ret)
			ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));

		if (ret)
			DRM_ERROR("drm set dma mask fail\n");
	}
}

static void meson_video_parse_config(struct drm_device *dev, struct meson_of_conf *conf)
{
	u32 mode_flag = 0;
	int ret;

	ret = of_property_read_u32(dev->dev->of_node,
				   "vfm_mode", &mode_flag);
	if (ret)
		DRM_DEBUG("%s parse vfm mode fail!\n", __func__);

	conf->vfm_mode = mode_flag;
}

static void meson_osd_parse_config(struct drm_device *dev, struct meson_of_conf *conf)
{
	u32 osd_afbc_mask = 0xff;
	u32 osd_formats_group = 0;
	int ret;

	ret = of_property_read_u32(dev->dev->of_node,
				   "osd_afbc_mask", &osd_afbc_mask);
	if (ret)
		DRM_DEBUG("%s parse osd afbc mask fail!\n", __func__);

	conf->osd_afbc_mask = osd_afbc_mask;

	ret = of_property_read_u32(dev->dev->of_node,
				"osd_formats_group", &osd_formats_group);
	if (ret)
		DRM_DEBUG("%s parse osd formats group fail!\n", __func__);

	conf->osd_formats_group = osd_formats_group;
}

static void am_meson_vpu_get_plane_crtc_mask(struct meson_drm *priv,
	char *name, u32 num, u32 *crtc_mask)
{
	struct device_node *np = priv->dev->of_node;
	int ret;

	ret = of_property_read_u32_array(np, name,
		crtc_mask, num);
	if (ret) {
		DRM_DEBUG("undefined %s!\n", name);
		return;
	}
}

void meson_of_init(struct drm_device *dev, struct meson_drm *priv)
{
	int ret;
	u32 osd_occupied_index;
	struct meson_of_conf *conf = &priv->of_conf;
	struct meson_vpu_pipeline *pipeline = priv->pipeline;

	meson_parse_crtc_masks(dev->dev->of_node, conf);

	meson_parse_dma_mask(dev->dev);

	ret = of_property_read_u8(dev->dev->of_node,
				  "osd_ver", &pipeline->osd_version);

	ret = of_property_read_u32(dev->dev->of_node,
				"osd_occupied_index", &osd_occupied_index);
	if (!ret)
		priv->osd_occupied_index = osd_occupied_index;

	am_meson_vpu_get_plane_crtc_mask(priv, "crtcmask_of_osd",
		pipeline->num_osds, conf->crtcmask_osd);
	am_meson_vpu_get_plane_crtc_mask(priv, "crtcmask_of_video",
		pipeline->num_video, conf->crtcmask_video);
	/* overwrite ctrc mask of video&osd, these should be defined in xxx.dts,
	 * it is mainly suitable to the board with different configurations for
	 * the same chip.
	 */
	am_meson_vpu_get_plane_crtc_mask(priv, "overwrite_crtcmask_of_osd",
		pipeline->num_osds, conf->crtcmask_osd);
	am_meson_vpu_get_plane_crtc_mask(priv, "overwrite_crtcmask_of_video",
		pipeline->num_video, conf->crtcmask_video);

	meson_osd_parse_config(dev, conf);
	meson_video_parse_config(dev, conf);
}

