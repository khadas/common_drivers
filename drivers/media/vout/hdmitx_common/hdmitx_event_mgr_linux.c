// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/cdev.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/vmalloc.h>
#include <linux/extcon.h>
#include <linux/notifier.h>
#include <linux/extcon-provider.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_event_mgr.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_platform_linux.h>
#include "hdmitx_log.h"

struct hdmitx_event_mgr {
	/*for uevent*/
	struct kobject *kobj;
	/* can't send uevent after enter suspend */
	bool deep_suspend_flag;
	/*for extcon event*/
	struct extcon_dev *hdmitx_extcon_hdmi;
	struct device *attached_extcon_dev;
	/*notifier for driver*/
	struct blocking_notifier_head hdmitx_event_notify_list;
};

static const unsigned int hdmi_extcon_cable[] = {
	EXTCON_DISP_HDMI,
	EXTCON_NONE,
};

struct hdmitx_uevent {
	const enum hdmitx_event type;
	const char *env;
	int state;
};

static struct hdmitx_uevent hdmi_events[] = {
	{
		.type = HDMITX_HPD_EVENT,
		.env = "hdmitx_hpd=",
	},
	{
		.type = HDMITX_HDCP_EVENT,
		.env = "hdmitx_hdcp=",
	},
	{
		.type = HDMITX_CUR_ST_EVENT,
		.env = "hdmitx_current_status=",
	},
	{
		.type = HDMITX_AUDIO_EVENT,
		.env = "hdmitx_audio=",
	},
	{
		.type = HDMITX_HDCPPWR_EVENT,
		.env = "hdmitx_hdcppwr=",
	},
	{
		.type = HDMITX_HDR_EVENT,
		.env = "hdmitx_hdr=",
	},
	{
		.type = HDMITX_RXSENSE_EVENT,
		.env = "hdmitx_rxsense=",
	},
	{
		.type = HDMITX_CEDST_EVENT,
		.env = "hdmitx_cedst=",
	},
	{ /* end of hdmi_events[] */
		.type = HDMITX_NONE_EVENT,
	},
};

/* for compliance with p/q/r/s/t, need both extcon and hdmi_event
 * there're mixed android/kernel combination, P/Q
 * only listen to extcon; while R/S (framework) only listen to uevent
 * t need listen to extcon(only hdmi hpd) and uevent
 */
struct hdmitx_event_mgr *hdmitx_event_mgr_create(struct platform_device *extcon_dev,
	struct device *uevent_dev)
{
	struct hdmitx_event_mgr *instance = vmalloc(sizeof(*instance));
	struct blocking_notifier_head *notify_head = &instance->hdmitx_event_notify_list;
	int ret = 0;

	/*uevent*/
	instance->kobj = &uevent_dev->kobj;

	/*extcon event*/
	instance->hdmitx_extcon_hdmi = devm_extcon_dev_allocate(&extcon_dev->dev,
										hdmi_extcon_cable);
	instance->attached_extcon_dev = &extcon_dev->dev;
	if (IS_ERR(instance->hdmitx_extcon_hdmi)) {
		HDMITX_DEBUG_EVENT("%s[%d] hdmitx_extcon_hdmi allocated failed\n",
			__func__, __LINE__);
		instance->hdmitx_extcon_hdmi = NULL;
	} else {
		ret = devm_extcon_dev_register(&extcon_dev->dev, instance->hdmitx_extcon_hdmi);
		if (ret < 0) {
			HDMITX_ERROR("%s[%d] hdmitx_extcon_hdmi register failed\n",
				__func__, __LINE__);
		//	devm_extcon_dev_free(instance->attached_extcon_dev,
		//		instance->hdmitx_extcon_hdmi);

			instance->hdmitx_extcon_hdmi = NULL;
		}
	}

	/*blocking notifier*/
	BLOCKING_INIT_NOTIFIER_HEAD(notify_head);

	return instance;
}

void hdmitx_event_mgr_suspend(struct hdmitx_event_mgr *event_mgr, bool suspend_flag)
{
	if (event_mgr)
		event_mgr->deep_suspend_flag = suspend_flag;
}

int hdmitx_event_mgr_destroy(struct hdmitx_event_mgr *event_mgr)
{
	if (!event_mgr)
		return 0;

	if (event_mgr->hdmitx_extcon_hdmi && event_mgr->attached_extcon_dev) {
	//	devm_extcon_dev_unregister(event_mgr->attached_extcon_dev,
	//		event_mgr->hdmitx_extcon_hdmi);
	//	devm_extcon_dev_free(event_mgr->attached_extcon_dev,
	//		event_mgr->hdmitx_extcon_hdmi);
		event_mgr->hdmitx_extcon_hdmi = 0;
	}

	vfree(event_mgr);

	return 0;
}

int hdmitx_event_mgr_set_uevent_state(struct hdmitx_event_mgr *event_mgr,
	enum hdmitx_event type, int state)
{
	struct hdmitx_uevent *event;
	bool extcon_event = false;

	for (event = hdmi_events; event->type != HDMITX_NONE_EVENT; event++) {
		if (type == event->type)
			break;
	}

	if (event->type == HDMITX_NONE_EVENT)
		return -EINVAL;

	event->state = state;

	if (type == HDMITX_HPD_EVENT && event_mgr->hdmitx_extcon_hdmi) {
		extcon_set_state(event_mgr->hdmitx_extcon_hdmi, EXTCON_DISP_HDMI, state);
		extcon_event = true;
	}

	HDMITX_DEBUG_EVENT("[%s] event_type: %s%d, %d\n",
		__func__, event->env, state, extcon_event);

	return 0;
}

int hdmitx_event_mgr_send_uevent(struct hdmitx_event_mgr *uevent_mgr,
	enum hdmitx_event type, int state, bool force)
{
	char env[MAX_UEVENT_LEN];
	struct hdmitx_uevent *event = hdmi_events;
	char *envp[2];
	int ret = 0;
	bool extcon_event = false;

	for (event = hdmi_events; event->type != HDMITX_NONE_EVENT; event++) {
		if (type == event->type)
			break;
	}

	if (event->type == HDMITX_NONE_EVENT)
		return -EINVAL;

	if (event->state == state && !force)
		return 0;

	event->state = state;
	memset(env, 0, sizeof(env));
	envp[0] = env;
	envp[1] = NULL;
	snprintf(env, MAX_UEVENT_LEN, "%s%d", event->env, state);

	if (uevent_mgr->deep_suspend_flag) {
		if (type == HDMITX_HPD_EVENT && uevent_mgr->hdmitx_extcon_hdmi) {
			extcon_set_state(uevent_mgr->hdmitx_extcon_hdmi, EXTCON_DISP_HDMI, state);
			extcon_event = true;
		}
	} else {
		ret = kobject_uevent_env(uevent_mgr->kobj, KOBJ_CHANGE, envp);

		if (type == HDMITX_HPD_EVENT && uevent_mgr->hdmitx_extcon_hdmi) {
			extcon_set_state_sync(uevent_mgr->hdmitx_extcon_hdmi,
				EXTCON_DISP_HDMI, state);
			extcon_event = true;
		}
	}

	HDMITX_DEBUG_EVENT("%s %s %d %d\n", __func__, env, ret, extcon_event);
	return ret;
}

/* below interfaces are for hpd/phy_addr/edid notify to cec/hdmirx */
int hdmitx_event_mgr_notifier_register(struct hdmitx_event_mgr *event_mgr,
	struct hdmitx_notifier_client *nb)
{
	return blocking_notifier_chain_register(&event_mgr->hdmitx_event_notify_list,
		(struct notifier_block *)nb);
}

int hdmitx_event_mgr_notifier_unregister(struct hdmitx_event_mgr *event_mgr,
	struct hdmitx_notifier_client *nb)
{
	return blocking_notifier_chain_unregister(&event_mgr->hdmitx_event_notify_list,
		(struct notifier_block *)nb);
}

int hdmitx_event_mgr_notify(struct hdmitx_event_mgr *event_mgr,
	unsigned long state, void *arg)
{
	return blocking_notifier_call_chain(&event_mgr->hdmitx_event_notify_list, state, arg);
}

