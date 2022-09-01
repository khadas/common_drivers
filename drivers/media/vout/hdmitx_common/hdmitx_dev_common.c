// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/mm.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_dev_common.h>
#include <hdmitx_boot_parameters.h>

int hdmitx_common_init(struct hdmitx_dev_common *tx_common)
{
	struct hdmitx_boot_param *boot_param = get_hdmitx_boot_params();

	/*load tx boot params*/
	tx_common->hdr_priority = boot_param->hdr_mask;
	memcpy(tx_common->hdmichecksum, boot_param->edid_chksum, sizeof(tx_common->hdmichecksum));

	memcpy(tx_common->fmt_attr, boot_param->color_attr, sizeof(tx_common->fmt_attr));
	memcpy(tx_common->backup_fmt_attr, boot_param->color_attr, sizeof(tx_common->fmt_attr));

	tx_common->frac_rate_policy = boot_param->fraction_refreshrate;
	tx_common->backup_frac_rate_policy = boot_param->fraction_refreshrate;

	return 0;
}
