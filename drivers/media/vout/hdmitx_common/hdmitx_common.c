// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/mm.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_common.h>
#include <hdmitx_boot_parameters.h>

int hdmitx_common_init(struct hdmitx_common *tx_common)
{
	struct hdmitx_boot_param *boot_param = get_hdmitx_boot_params();

	/*load tx boot params*/
	tx_common->hdr_priority = boot_param->hdr_mask;
	memcpy(tx_common->hdmichecksum, boot_param->edid_chksum, sizeof(tx_common->hdmichecksum));

	memcpy(tx_common->fmt_attr, boot_param->color_attr, sizeof(tx_common->fmt_attr));
	memcpy(tx_common->backup_fmt_attr, boot_param->color_attr, sizeof(tx_common->fmt_attr));

	tx_common->frac_rate_policy = boot_param->fraction_refreshrate;
	tx_common->backup_frac_rate_policy = boot_param->fraction_refreshrate;

	/*mutex init*/
	mutex_init(&tx_common->setclk_mutex);
	return 0;
}

int hdmitx_common_destroy(struct hdmitx_common *tx_common)
{
	return 0;
}

int hdmitx_hpd_notify_unlocked(struct hdmitx_common *tx_comm)
{
	if (tx_comm->drm_hpd_cb.callback)
		tx_comm->drm_hpd_cb.callback(tx_comm->drm_hpd_cb.data);

	return 0;
}

int hdmitx_register_hpd_cb(struct hdmitx_common *tx_comm, struct connector_hpd_cb *hpd_cb)
{
	mutex_lock(&tx_comm->setclk_mutex);
	tx_comm->drm_hpd_cb.callback = hpd_cb->callback;
	tx_comm->drm_hpd_cb.data = hpd_cb->data;
	mutex_unlock(&tx_comm->setclk_mutex);
	return 0;
}

int hdmitx_setup_attr(struct hdmitx_common *tx_comm, const char *buf)
{
	char attr[16] = {0};

	memcpy(attr, buf, sizeof(attr));
	memcpy(tx_comm->fmt_attr, attr, sizeof(tx_comm->fmt_attr));
	return 0;
}

int hdmitx_get_attr(struct hdmitx_common *tx_comm, char attr[16])
{
	memcpy(attr, tx_comm->fmt_attr, sizeof(tx_comm->fmt_attr));
	return 0;
}
