/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_PLATFORM_LINUX_H
#define __HDMITX_PLATFORM_LINUX_H

#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_tracer.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_event_mgr.h>

struct hdmitx_tracer *hdmitx_tracer_create(void);

struct hdmitx_notifier_client {
	struct notifier_block base;
};

struct hdmitx_event_mgr *hdmitx_event_mgr_create(struct platform_device *extcon_dev,
		struct device *uevent_dev);

#endif
