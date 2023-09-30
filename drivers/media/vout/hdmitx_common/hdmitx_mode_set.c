// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/printk.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_common.h>

static int hdmitx_common_pre_enable_mode(struct hdmitx_common *tx_comm,
					 struct hdmi_format_para *para)
{
	struct vinfo_s *vinfo = &tx_comm->hdmitx_vinfo;
	unsigned int width, height;

	if (tx_comm->hpd_state == 0) {
		pr_info("current hpd_state0, exit %s\n", __func__);
		return -1;
	}
	if (tx_comm->suspend_flag) {
		pr_info("currently under suspend, exit %s\n", __func__);
		return -1;
	}

	memcpy(vinfo->hdmichecksum, tx_comm->rxcap.hdmichecksum, 10);
	if (vinfo->mode == VMODE_HDMI) {
		width = tx_comm->rxcap.physical_width;
		height = tx_comm->rxcap.physical_height;
		if (width == 0 || height == 0) {
			vinfo->screen_real_width = vinfo->aspect_ratio_num;
			vinfo->screen_real_height = vinfo->aspect_ratio_den;
		} else {
			vinfo->screen_real_width = width;
			vinfo->screen_real_height = height;
		}
	}

	tx_comm->ctrl_ops->pre_enable_mode(tx_comm, para);

	return 0;
}

static int hdmitx_common_enable_mode(struct hdmitx_common *tx_comm, struct hdmi_format_para *para)
{
	tx_comm->ctrl_ops->enable_mode(tx_comm, para);
	return 0;
}

static int hdmitx_common_post_enable_mode(struct hdmitx_common *tx_comm,
					  struct hdmi_format_para *para)
{
	tx_comm->ctrl_ops->post_enable_mode(tx_comm, para);
	return 0;
}

int hdmitx_common_do_mode_setting(struct hdmitx_common *tx_comm,
			struct hdmitx_binding_state *new_state)
{
	int ret;
	struct hdmi_format_para *new_para;

	new_para = &new_state->para;

	mutex_lock(&tx_comm->hdmimode_mutex);
	ret = hdmitx_common_pre_enable_mode(tx_comm, new_para);
	if (ret < 0) {
		pr_err("pre mode enable fail\n");
		goto fail;
	}
	ret = hdmitx_common_enable_mode(tx_comm, new_para);
	if (ret < 0) {
		pr_err("mode enable fail\n");
		goto fail;
	}
	ret = hdmitx_common_post_enable_mode(tx_comm, new_para);
	if (ret < 0) {
		pr_err("post mode enable fail\n");
		goto fail;
	}

	mutex_unlock(&tx_comm->hdmimode_mutex);
	return 0;

fail:
	mutex_unlock(&tx_comm->hdmimode_mutex);
	return -1;
}
EXPORT_SYMBOL(hdmitx_common_do_mode_setting);
