// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * drivers/amlogic/media/vin/tvin/tvin_frontend.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

/* Standard Linux headers */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/device.h>
/* Amlogic headers */
#include <linux/amlogic/media/vfm/vframe.h>

/* Local headers */
#include "tvin_frontend.h"

#define CLASS_NAME      "tvin"

static struct class *tvcom_clsp;
static struct list_head head = LIST_HEAD_INIT(head);
static DEFINE_SPINLOCK(list_lock);

#if IS_ENABLED(CONFIG_AMLOGIC_TVIN_USE_DEBUG_FILE)
/*
 * dump file with debug file API
 */
int tvin_df_write(struct debug_file *df, void *buf,
	unsigned int want_size)
{
	int try_size, ret = 0;
	unsigned int retry = 0, time_out;
	void *buf_tmp = NULL;

	/* write times in theory */
	time_out    = (want_size + DEBUG_FILE_FIFO_SIZE / 2)
					/ DEBUG_FILE_FIFO_SIZE;
	time_out   *= 2;/* wait twice per call */
	if (time_out < DEBUG_FILE_TIMEOUT)
		time_out = DEBUG_FILE_TIMEOUT;
	buf_tmp  = buf;
	try_size = want_size;
	while (try_size > 0 && retry++ < time_out) {
		ret = debug_file_write(df, buf_tmp, try_size);
		if (ret < 0) {
			pr_err("debug_file_write failed with:%d\n", ret);
			break;
		} else if (ret == try_size) {
			break;
		}
		pr_err("retry = %d try_size:%d, ret:%d, total:%d\n",
			retry, try_size, ret, want_size);
		try_size = try_size - ret;
		buf_tmp  = (unsigned char *)buf_tmp + ret;
	}
	if (ret < 0) {
		pr_err("retry = %d/%d try_size:%d, ret:%d, total:%d\n",
			retry, time_out, try_size, ret, want_size);
		return DF_WRITE_RET_FAILED;
	} else if (retry >= time_out) {
		pr_err("retry = %d/%d try_size:%d, ret:%d, total:%d\n",
			retry, time_out, try_size, ret, want_size);
		return DF_WRITE_RET_TIMEOUT;
	}
	return DF_WRITE_RET_OK;
}
EXPORT_SYMBOL(tvin_df_write);
#endif

int tvin_frontend_init(struct tvin_frontend_s *fe,
		       struct tvin_decoder_ops_s *dec_ops,
		       struct tvin_state_machine_ops_s *sm_ops,
		       int index)
{
	if (!fe)
		return -1;
	fe->dec_ops = dec_ops;
	fe->sm_ops  = sm_ops;
	fe->index   = index;

	INIT_LIST_HEAD(&fe->list);
	return 0;
}
EXPORT_SYMBOL(tvin_frontend_init);

int tvin_reg_frontend(struct tvin_frontend_s *fe)
{
	ulong flags;
	struct tvin_frontend_s *f, *t;

	if (!strlen(fe->name) || !fe->dec_ops ||
	    !fe->dec_ops->support || !fe->sm_ops)
		return -1;

	/* check whether the frontend is registered already */
	list_for_each_entry_safe(f, t, &head, list) {
		if (!strcmp(f->name, fe->name))
			return -1;
	}
	spin_lock_irqsave(&list_lock, flags);
	list_add_tail(&fe->list, &head);
	spin_unlock_irqrestore(&list_lock, flags);
	return 0;
}
EXPORT_SYMBOL(tvin_reg_frontend);

/*
 * Notes: This unregister interface doesn't call kfree to free memory
 * for the object, because the register interface doesn't call kmalloc
 * to allocate memory for the object. You should call the kfree yourself
 * to free memory for the object you allocated.
 */
void tvin_unreg_frontend(struct tvin_frontend_s *fe)
{
	ulong flags;
	struct tvin_frontend_s *f, *t;

	list_for_each_entry_safe(f, t, &head, list) {
		if (!strcmp(f->name, fe->name)) {
			spin_lock_irqsave(&list_lock, flags);
			list_del(&f->list);
			spin_unlock_irqrestore(&list_lock, flags);
			return;
		}
	}
}
EXPORT_SYMBOL(tvin_unreg_frontend);

struct tvin_frontend_s *tvin_get_frontend(enum tvin_port_e port, int index)
{
	struct tvin_frontend_s *f = NULL;

	list_for_each_entry(f, &head, list) {
		if (f->dec_ops && f->dec_ops->support) {
			if ((!f->dec_ops->support(f, port)) &&
			    f->index == index)
				return f;
		}
	}
	return NULL;
}
EXPORT_SYMBOL(tvin_get_frontend);

struct tvin_decoder_ops_s *tvin_get_fe_ops(enum tvin_port_e port, int index)
{
	struct tvin_frontend_s *f = tvin_get_frontend(port, index);

	/* if ((f = tvin_get_frontend(port, index)) && (f->dec_ops)) { */
	if (f->dec_ops)
		return f->dec_ops;

	return NULL;
}
EXPORT_SYMBOL(tvin_get_fe_ops);

struct tvin_state_machine_ops_s *tvin_get_sm_ops(enum tvin_port_e port,
						 int index)
{
	struct tvin_frontend_s *f = tvin_get_frontend(port, index);

	/* if ((f = tvin_get_frontend(port, index)) && (f->sm_ops)) { */
	if (f->sm_ops)
		return f->sm_ops;

	return NULL;
}
EXPORT_SYMBOL(tvin_get_sm_ops);

static ssize_t frontend_names_show(struct class *cls,
				   struct class_attribute *attr, char *buf)
{
	size_t len = 0;
	struct tvin_frontend_s *f = NULL;

	list_for_each_entry(f, &head, list) {
		len += sprintf(buf + len, "%s\n", f->name);
	}
	return len;
}

static CLASS_ATTR_RO(frontend_names);

int __init tvin_common_init(void)
{
	int ret = 0;

	tvcom_clsp = class_create(THIS_MODULE, CLASS_NAME);
	if (!tvcom_clsp) {
		pr_err("[tvin_com..]%s: create tvin common class error.\n",
		       __func__);
		return -1;
	}
	ret = class_create_file(tvcom_clsp, &class_attr_frontend_names);
	return ret;
}

void __exit tvin_common_exit(void)
{
	class_remove_file(tvcom_clsp, &class_attr_frontend_names);
	class_destroy(tvcom_clsp);
}

//MODULE_LICENSE("GPL");

