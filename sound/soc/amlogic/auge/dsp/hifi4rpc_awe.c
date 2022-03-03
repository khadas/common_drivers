// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/**
 * amlogic wake up engine wrapper at kernel side
 *
 * Author: Yang Liu <Yang.Liu@amlogic.com>
 * Version:
 * - 0.1        init
 */
#include <linux/printk.h> /* pr_info */
#include <linux/string.h> /* memcpy */
#include <linux/slab.h> /* kmalloc */
#include <linux/uaccess.h> /* copy_to_user */

#include "aml_wakeup_api.h"

/** temp solution, only support one instance */
static struct AWE *g_awe;
static char *g_buf;
static size_t g_period_size, g_periods;
static int (*g_notify_cb)(void *);
static void *g_user;

static size_t g_hwptr;
static size_t g_swptr;
static size_t g_times;

static void awe_asr_fn(struct AWE *p, const enum AWE_DATA_TYPE type,
		       char *out, size_t size, void *user)
{
	size_t buf_size = g_period_size * g_periods;
	size_t i;
	s16 *b = (s16 *)out, sum = 0;

	if (type != AWE_DATA_TYPE_ASR || size == 0)
		return;
	pr_debug("awe: p=%lx type=%d out=%lx size=%zu user=%lx\n",
		 (unsigned long)p, type, (unsigned long)out, size, (unsigned long)user);

	if (size > g_period_size * g_periods) {
		pr_err("awe: FATAL error: cannot fill size=%zu data to buffer=%zu*%zu\n",
		       size, g_period_size, g_periods);
		return;
	}
	for (i = 0; i != size / sizeof(s16); i++)
		sum += b[i];
	pr_debug("awe ASR: %lx %04x %04x %04x %04x sum=%04x times=%zu acc=%zu+%zu\n",
		 (unsigned long)b, b[0], b[1], b[2], b[3], sum, g_times, g_hwptr, size);

	if ((((g_hwptr + size) / buf_size) > (g_hwptr / buf_size)) &&
	    ((g_hwptr + size) % buf_size != 0)) {
		size_t head, tail;

		tail = (g_hwptr + size) % buf_size;
		head = size - tail;
		pr_debug("hwptr=%zu size=%zu buffer=%zu*%zu head=%zu tail=%zu\n",
			 g_hwptr, size, g_period_size, g_periods, head, tail);
		memcpy(g_buf + (g_hwptr % buf_size), out, head);
		memcpy(g_buf, out + head, tail);
	} else {
		pr_debug("hwptr=%zu size=%zu buffer=%zu*%zu\n",
			 g_hwptr, size, g_period_size, g_periods);
		memcpy(g_buf + (g_hwptr % buf_size), out, size);
	}

	if (((g_hwptr + size) / g_period_size) > (g_hwptr / g_period_size)) {
		/** update hwptr, then notify upper layer */
		pr_debug("awe: acc=%zu+%zu notify\n", g_hwptr, size);
		g_hwptr += size;
		g_notify_cb(g_user);
	} else {
		g_hwptr += size;
	}
	g_times++;
}

static void awe_voip_fn(struct AWE *p, const enum AWE_DATA_TYPE type,
			char *out, size_t size, void *user)
{
	if (type != AWE_DATA_TYPE_VOIP || size == 0)
		return;
	/** skip handle, due to voip data is all zero */
	pr_debug("awe: p=%lx type=%d out=%lx size=%zu user=%lx\n",
		 (unsigned long)p, type, (unsigned long)out, size, (unsigned long)user);
	pr_debug("awe: VOIP: data size=%zu %x %x %x %x\n",
		 size, out[0], out[1], out[2], out[3]);
}

static void awe_ev_fn(struct AWE *p, const enum AWE_EVENT_TYPE type, s32 code,
		      const void *payload, void *user)
{
	if (type != AWE_EVENT_TYPE_WAKE)
		return;
	/** skip handle, ALSA framework cannot pass this event to user */
	pr_info("awe: p=%lx type=%d code=%d payload=%lx user=%lx\n",
		(unsigned long)p, type, code, (unsigned long)payload, (unsigned long)user);
	pr_info("wake word detected !!!\n");
}

int awe_close(void)
{
	if (!g_awe) {
		pr_err("awe: fatal: try to close before open\n");
		return 0;
	}
	aml_awe_close(g_awe);
	aml_awe_destroy(g_awe);
	g_awe = NULL;

	g_period_size = 0;
	g_periods = 0;
	kfree(g_buf);
	g_buf = NULL;
	g_hwptr = 0;
	g_swptr = 0;
	g_notify_cb = NULL;
	g_user = NULL;

	return 0;
}

int awe_pointer(void)
{
	return g_hwptr % (g_period_size * g_periods);
}

int awe_copy_user(void __user *buf, unsigned long bytes)
{
	size_t size = bytes;
	size_t buf_size = g_period_size * g_periods;
	size_t r = 0;
	/* s16 b0, b1, b2, b3, *b = buf; */

	pr_debug("awe: copy_user: buf=%lx size=%zu g_buf=%lx buffer=%zu*%zu\n",
		 (unsigned long)buf, size, (unsigned long)g_buf, g_period_size, g_periods);
	if (g_swptr + size > g_hwptr) {
		pr_err("awe: fatal error: swptr=%zu bytes=%zu hwptr=%zu\n",
		       g_swptr, size, g_hwptr);
		return r;
	}
	if (size > g_period_size * g_periods) {
		pr_err("awe: fatal error: cannot fill user buffer\n");
		return r;
	}

	if ((((g_swptr + size) / buf_size) > (g_swptr / buf_size)) &&
	    ((g_swptr + size) % buf_size != 0)) {
		size_t head, tail;

		tail = (g_swptr + size) % buf_size;
		head = size - tail;
		pr_info("swptr=%zu size=%zu buffer=%zu*%zu head=%zu tail=%zu\n",
			g_swptr, size, g_period_size, g_periods,
			head, tail);
		r = copy_to_user(buf, g_buf + g_swptr % buf_size, head);
		pr_info("awe: copy_to_user %zu %zu head\n", head, r);
		r = copy_to_user(buf + head, g_buf, tail);
		pr_info("awe: copy_to_user %zu %zu tail\n", size, r);
	} else {
		pr_debug("swptr=%zu size=%zu buffer=%zu*%zu\n",
			 g_swptr, size, g_period_size, g_periods);
		r = copy_to_user(buf, g_buf + (g_swptr % buf_size), size);
		pr_debug("awe: copy_to_user %zu %zu\n", size, r);
	}
	/* get_user(b0, b); */
	/* get_user(b1, b + 1); */
	/* get_user(b2, b + 2); */
	/* get_user(b3, b + 3); */
	/* pr_info("awe: copy_user: %zu+%zu r=%zu data=%04x %04x %04x %04x\n", */
	/* g_swptr, size, r, b0, b1, b2, b3); */
	g_swptr += size;
	return 0;
}

int awe_open(size_t period_size, size_t periods,
	     int (*notify_cb)(void *), void *user)
{
	enum AWE_RET awe_ret;
	union AWE_PARA awe_para;

	g_period_size = period_size;
	g_periods = periods;
	g_buf = kzalloc(g_period_size * g_periods, GFP_KERNEL);
	g_hwptr = 0;
	g_swptr = 0;
	g_notify_cb = notify_cb;
	g_user = user;
	pr_debug("awe: open: g_buf=%lx\n", (unsigned long)g_buf);

	awe_ret = aml_awe_create(&g_awe);
	if (awe_ret != AWE_RET_OK) {
		pr_err("cannot create awe service\n");
		return awe_ret;
	}
	/* register handler fn */
	aml_awe_adddatahandler(g_awe, AWE_DATA_TYPE_ASR, awe_asr_fn, NULL);
	aml_awe_adddatahandler(g_awe, AWE_DATA_TYPE_VOIP, awe_voip_fn, NULL);
	aml_awe_addeventhandler(g_awe, AWE_EVENT_TYPE_WAKE, awe_ev_fn, NULL);
	/* set input mode as dsp input */
	awe_para.input_mode = AWE_DSP_INPUT_MODE;
	awe_ret = aml_awe_setparam(g_awe, AWE_PARA_INPUT_MODE, &awe_para);
	if (awe_ret != AWE_RET_OK) {
		pr_err("awe: fail to set input mode\n");
		return awe_ret;
	}
	/* set sample rate */
	awe_ret = aml_awe_getparam(g_awe, AWE_PARA_SUPPORT_SAMPLE_RATES,
				   &awe_para);
	if (awe_ret != AWE_RET_OK) {
		pr_err("awe: fail to get sample rate\n");
		return awe_ret;
	}
	awe_para.in_samp_rate = awe_para.support_samp_rates[0];
	awe_ret = aml_awe_setparam(g_awe, AWE_PARA_IN_SAMPLE_RATE, &awe_para);
	if (awe_ret != AWE_RET_OK) {
		pr_err("awe: fail to set sample rate\n");
		return awe_ret;
	}
	/* set sample bits */
	awe_ret = aml_awe_getparam(g_awe, AWE_PARA_SUPPORT_SAMPLE_BITS,
				   &awe_para);
	if (awe_ret != AWE_RET_OK) {
		pr_err("awe: fail to get sample bits\n");
		return awe_ret;
	}
	awe_para.in_samp_bits = awe_para.support_samp_bits[0];
	awe_ret = aml_awe_setparam(g_awe, AWE_PARA_IN_SAMPLE_BITS, &awe_para);
	if (awe_ret != AWE_RET_OK) {
		pr_err("awe: fail to set sample bits\n");
		return awe_ret;
	}
	awe_para.is_bypass_input = false;
	awe_ret = aml_awe_setparam(g_awe, AWE_PARA_BYPASS_INPUT, &awe_para);
	if (awe_ret != AWE_RET_OK) {
		pr_err("awe: fail to disable pass input mode\n");
		return awe_ret;
	}

	awe_ret = aml_awe_open(g_awe);
	if (awe_ret != AWE_RET_OK) {
		pr_err("awe: fail to open\n");
		return awe_ret;
	}
	return awe_ret;
}
