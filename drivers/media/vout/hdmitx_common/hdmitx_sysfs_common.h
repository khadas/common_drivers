/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_SYSFS_COMMON_H
#define __HDMITX_SYSFS_COMMON_H

#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_common.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_hw_common.h>

int hdmitx_sysfs_common_create(struct device *dev,
		struct hdmitx_common *tx_comm,
		struct hdmitx_hw_common *tx_hw);

int hdmitx_sysfs_common_destroy(struct device *dev);

#endif
