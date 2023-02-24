// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/amlogic/media/vout/hdmi_tx_ext.h>
#include "hdmi_tx_ext.h"

unsigned int hdmitx_drv_ver(void)
{
#if defined(CONFIG_AMLOGIC_HDMITX)
	if (get_hdmitx20_init() == 1)
		return 20;
#endif
#if defined(CONFIG_AMLOGIC_HDMITX21)
	if (get_hdmitx21_init() == 1)
		return 21;
#endif
	return 0;
}
/* for notify to cec */
int hdmitx_event_notifier_regist(struct notifier_block *nb)
{
#if defined(CONFIG_AMLOGIC_HDMITX)
	if (get_hdmitx20_init() == 1)
		return hdmitx20_event_notifier_regist(nb);
#endif
#if defined(CONFIG_AMLOGIC_HDMITX21)
	if (get_hdmitx21_init() == 1)
		return hdmitx21_event_notifier_regist(nb);
#endif
	return 1;
}
EXPORT_SYMBOL(hdmitx_event_notifier_regist);

int hdmitx_event_notifier_unregist(struct notifier_block *nb)
{
#if defined(CONFIG_AMLOGIC_HDMITX)
	if (get_hdmitx20_init() == 1)
		return hdmitx20_event_notifier_unregist(nb);
#endif
#if defined(CONFIG_AMLOGIC_HDMITX21)
	if (get_hdmitx21_init() == 1)
		return hdmitx21_event_notifier_unregist(nb);
#endif
	return 1;
}
EXPORT_SYMBOL(hdmitx_event_notifier_unregist);

int get_hpd_state(void)
{
	int ret = 0;

#if defined(CONFIG_AMLOGIC_HDMITX)
	if (get_hdmitx20_init() == 1)
		ret = get20_hpd_state();
#endif
#if defined(CONFIG_AMLOGIC_HDMITX21)
	if (get_hdmitx21_init() == 1)
		ret = get21_hpd_state();
#endif
	return ret;
}
EXPORT_SYMBOL(get_hpd_state);

struct vsdb_phyaddr *get_hdmitx_phy_addr(void)
{
#if defined(CONFIG_AMLOGIC_HDMITX)
	if (get_hdmitx20_init() == 1)
		return get_hdmitx20_phy_addr();
#endif
#if defined(CONFIG_AMLOGIC_HDMITX21)
	if (get_hdmitx21_init() == 1)
		return get_hdmitx21_phy_addr();
#endif
	return NULL;
}
EXPORT_SYMBOL(get_hdmitx_phy_addr);

void get_attr(char attr[16])
{
#if defined(CONFIG_AMLOGIC_HDMITX)
	if (get_hdmitx20_init() == 1)
		get20_attr(attr);
#endif
#if defined(CONFIG_AMLOGIC_HDMITX21)
	if (get_hdmitx21_init() == 1)
		get21_attr(attr);
#endif
}
EXPORT_SYMBOL(get_attr);

void setup_attr(const char *buf)
{
#if defined(CONFIG_AMLOGIC_HDMITX)
	if (get_hdmitx20_init() == 1)
		setup20_attr(buf);
#endif
#if defined(CONFIG_AMLOGIC_HDMITX21)
	if (get_hdmitx21_init() == 1)
		setup21_attr(buf);
#endif
}
EXPORT_SYMBOL(setup_attr);

/*
 * hdmitx_audio_mute_op() is used by external driver call
 * flag: 0: audio off   1: audio_on
 *       2: for EDID auto mode
 */
void hdmitx_audio_mute_op(unsigned int flag)
{
#if defined(CONFIG_AMLOGIC_HDMITX)
	if (get_hdmitx20_init() == 1)
		hdmitx20_audio_mute_op(flag);
#endif
#if defined(CONFIG_AMLOGIC_HDMITX21)
	if (get_hdmitx21_init() == 1)
		hdmitx21_audio_mute_op(flag);
#endif
}
EXPORT_SYMBOL(hdmitx_audio_mute_op);

void hdmitx_video_mute_op(u32 flag)
{
#if defined(CONFIG_AMLOGIC_HDMITX)
	if (get_hdmitx20_init() == 1)
		hdmitx20_video_mute_op(flag);
#endif
#if defined(CONFIG_AMLOGIC_HDMITX21)
	if (get_hdmitx21_init() == 1)
		hdmitx21_video_mute_op(flag);
#endif
}
EXPORT_SYMBOL(hdmitx_video_mute_op);

void hdmitx_ext_set_audio_output(int enable)
{
#if defined(CONFIG_AMLOGIC_HDMITX)
	if (get_hdmitx20_init() == 1)
		hdmitx20_ext_set_audio_output(enable);
#endif
#if defined(CONFIG_AMLOGIC_HDMITX21)
	if (get_hdmitx21_init() == 1)
		hdmitx21_ext_set_audio_output(enable);
#endif
}
EXPORT_SYMBOL(hdmitx_ext_set_audio_output);

int hdmitx_ext_get_audio_status(void)
{
#if defined(CONFIG_AMLOGIC_HDMITX)
	if (get_hdmitx20_init() == 1)
		return hdmitx20_ext_get_audio_status();
#endif
#if defined(CONFIG_AMLOGIC_HDMITX21)
	if (get_hdmitx21_init() == 1)
		return hdmitx21_ext_get_audio_status();
#endif
	return 0;
}
EXPORT_SYMBOL(hdmitx_ext_get_audio_status);

int register_earcrx_callback(pf_callback callback)
{
#if defined(CONFIG_AMLOGIC_HDMITX)
	if (get_hdmitx20_init() == 1)
		hdmitx_earc_hpdst(callback);
#endif
#if defined(CONFIG_AMLOGIC_HDMITX21)
	if (get_hdmitx21_init() == 1)
		hdmitx21_earc_hpdst(callback);
#endif
	return 0;
}
EXPORT_SYMBOL(register_earcrx_callback);

void unregister_earcrx_callback(void)
{
#if defined(CONFIG_AMLOGIC_HDMITX)
	if (get_hdmitx20_init() == 1)
		hdmitx_earc_hpdst(NULL);
#endif
#if defined(CONFIG_AMLOGIC_HDMITX21)
	if (get_hdmitx21_init() == 1)
		hdmitx21_earc_hpdst(NULL);
#endif
}
EXPORT_SYMBOL(unregister_earcrx_callback);

/* Nofity client */

#undef pr_fmt
#define pr_fmt(fmt) "snd_notify: " fmt

#include <linux/module.h>

static BLOCKING_NOTIFIER_HEAD(aout_notifier_list);
/*
 *	aout_register_client - register a client notifier
 *	@nb: notifier block to callback on events
 */
int aout_register_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&aout_notifier_list, nb);
}
EXPORT_SYMBOL(aout_register_client);

/*
 *	aout_unregister_client - unregister a client notifier
 *	@nb: notifier block to callback on events
 */
int aout_unregister_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&aout_notifier_list, nb);
}
EXPORT_SYMBOL(aout_unregister_client);

/*
 * aout_notifier_call_chain - notify clients of fb_events
 *
 */
int aout_notifier_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&aout_notifier_list, val, v);
}
EXPORT_SYMBOL_GPL(aout_notifier_call_chain);
