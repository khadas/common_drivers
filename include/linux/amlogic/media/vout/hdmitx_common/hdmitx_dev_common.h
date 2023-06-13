/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_DEV_COMMON_H
#define __HDMITX_DEV_COMMON_H

#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_common.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_hw_common.h>

/************************************************************************************
 * hdmitx_dev_common only define api which will use hdmitx_common&hdmitx_hw_common.
 * no data struct of hdmitx_dev_common, all struct should move to hdmitx_common or
 * hdmitx_hw_common.
 ***********************************************************************************/

int hdmitx_dev_setup_vsif_packet(struct hdmitx_common *tx_comm,
	struct hdmitx_hw_common *tx_hw, enum vsif_type type, int on, void *param);

#endif
