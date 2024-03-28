// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/amlogic/media/vout/hdmi_tx_ext.h>
#include "hdmi_tx_ext.h"
#include "hdmi_tx_module.h"

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
	HDMITX_ERROR("Not support tx20 %s anymore.\n", __func__);
}
EXPORT_SYMBOL(get_attr);

void setup_attr(const char *buf)
{
	HDMITX_ERROR("Not support tx20 %s anymore.\n", __func__);
}
EXPORT_SYMBOL(setup_attr);

/* ARC IN audio capture not working due to init
 * sequence issue of eARC driver and HDMI Tx driver.
 * when eARC driver try to register_earcrx_callback,
 * HDMI Tx driver probe/init is not finish, that lead
 * register_earcrx_callback fail and eARC driver
 * doesn't know if HDMI Tx cable plug in/out.
 * so don't check hdmitx init or not. TODO
 */
int register_earcrx_callback(pf_callback callback)
{
#if defined(CONFIG_AMLOGIC_HDMITX)
	if (get_hdmitx20_init() == 1)
		hdmitx_earc_hpdst(callback);
#endif
#if defined(CONFIG_AMLOGIC_HDMITX21)
	/* ARC IN audio capture not working due to init
	 * sequence issue of eARC driver and HDMI Tx driver.
	 * when eARC driver try to register_earcrx_callback,
	 * HDMI Tx driver probe/init is not finish, that lead
	 * register_earcrx_callback fail and eARC driver
	 * doesn't know if HDMI Tx cable plug in/out.
	 * so don't check hdmitx21 init or not
	 */
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
