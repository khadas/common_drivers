/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_UEVENT_MGR_H
#define __HDMITX_UEVENT_MGR_H

enum hdmitx_event {
	HDMITX_NONE_EVENT = 0,
	HDMITX_HPD_EVENT,
	HDMITX_HDCP_EVENT,
	HDMITX_CUR_ST_EVENT,
	HDMITX_AUDIO_EVENT,
	HDMITX_HDCPPWR_EVENT,
	HDMITX_HDR_EVENT,
	HDMITX_RXSENSE_EVENT,
	HDMITX_CEDST_EVENT,
};

/*HDMITX_HDCPPWR_EVENT event value*/
#define HDMI_SUSPEND    0
#define HDMI_WAKEUP     1

#define MAX_UEVENT_LEN 64

/* struct and create function depends on platform,
 * check in platform header: for example, hdmitx_platform_linux.h
 */
struct hdmitx_event_mgr;
struct hdmitx_notifier_client;

int hdmitx_event_mgr_send_uevent(struct hdmitx_event_mgr *event_mgr,
	enum hdmitx_event type, int val);
int hdmitx_event_mgr_set_uevent_state(struct hdmitx_event_mgr *event_mgr,
	enum hdmitx_event type, int state);

int hdmitx_event_mgr_notifier_register(struct hdmitx_event_mgr *event_mgr,
	struct hdmitx_notifier_client *nb);
int hdmitx_event_mgr_notifier_unregister(struct hdmitx_event_mgr *event_mgr,
	struct hdmitx_notifier_client *nb);
int hdmitx_event_mgr_notify(struct hdmitx_event_mgr *event_mgr,
	unsigned long state, void *arg);

int hdmitx_event_mgr_destroy(struct hdmitx_event_mgr *event_mgr);

#endif
