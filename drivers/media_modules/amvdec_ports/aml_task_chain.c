/*
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Description:
 */
#include <linux/list.h>
#include <linux/spinlock.h>

#include "aml_vcodec_drv.h"
#include "aml_task_chain.h"

#define KERNEL_ATRACE_TAG KERNEL_ATRACE_TAG_V4L2
#include <trace/events/meson_atrace.h>

struct task_item_name_s {
	enum task_type_e	type;
	const u8		*name;
};

static const struct task_item_name_s iname[] = {
	{TASK_TYPE_DEC,		"dec"},
	{TASK_TYPE_VPP,		"vpp"},
	{TASK_TYPE_V4L_SINK,	"v4l-sink"},
	{TASK_TYPE_GE2D,	"ge2d"},
	{TASK_TYPE_MAX,		"unknown"},
};

static const u8 *type_to_name(enum task_type_e type)
{
	const u8 *name = "unknown";
	int i, size = ARRAY_SIZE(iname);

	for (i = 0; i < size; i++) {
		if (type == iname[i].type)
			name = iname[i].name;
	}

	return name;
}

static enum task_type_e name_to_type(const u8 *name)
{
	enum task_type_e type;
	int i, size = ARRAY_SIZE(iname);

	for (i = 0; i < size; i++) {
		if (name == iname[i].name)
			type = iname[i].type;
	}

	return type;
}

static struct task_item_s *find_task_item(struct task_chain_s *task,
					  enum task_type_e type)
{
	struct task_item_s *item = NULL;
	ulong flags;

	spin_lock_irqsave(&task->slock, flags);

	if (!list_empty(&task->list_item)) {
		struct task_item_s *p;

		list_for_each_entry(p, &task->list_item, node) {
			if (p->ops->type == type) {
				item = p;
				break;
			}
		}
	}

	if (item)
		kref_get(&item->ref);

	spin_unlock_irqrestore(&task->slock, flags);

	return item;
}

static void task_item_release(struct kref *kref);

static void task_item_vframe_push(struct task_item_s *item, struct vframe_s *vframe)
{
	int i = 0;

	for (i = 0 ; i < 3; i++) {
		if (item->vframe[i].index == -1) {
			memcpy(&item->vframe[i], vframe, sizeof(struct vframe_s));
			break;
		}
	}
}

static struct vframe_s *task_item_vframe_pop(struct task_item_s *item)
{
	int i = 0;

	for (i = 0 ; i < 3; i++) {
		if (item->vframe[i].index != -1) {
			break;
		}
	}

	if (i >= 3) {
		pr_info("[ERR] vframe pop fail!\n");
	}

	return &item->vframe[i];
}

static void task_item_vframe_reset(struct task_item_s *item,
							struct vframe_s * vf)
{
	int i = 0;

	for (i = 0 ; i < 3; i++) {
		if (vf == &item->vframe[i]) {
			item->vframe[i].index = -1;
			break;
		}
	}
}

static struct task_item_s *task_item_get(struct task_chain_s *task,
				  enum task_type_e type)
{
	struct task_item_s *item = NULL;

	item = find_task_item(task, type);

	return item;
}

static int task_item_put(struct task_item_s *item)
{
	return kref_put(&item->ref, task_item_release);
}

static int task_buffer_submit(struct task_chain_s *task,
			       enum task_type_e type)
{
	struct aml_buf *aml_buf =
		(struct aml_buf *)task->obj;
	struct task_item_s *item = NULL;
	struct task_item_s *item2 = NULL;
	int ret = 0;

	memset(task->vf_tmp, 0, sizeof(struct vframe_s));

	item = task_item_get(task, type);
	if (item) {
		item->ops->get_vframe(item->caller, task->vf_tmp);
		memcpy(&aml_buf->vframe, task->vf_tmp, sizeof(struct vframe_s));
		task_item_vframe_push(item, &aml_buf->vframe);
		item->is_active = false;

		item2 = task_item_get(task, task->map[0][type]);
		if (item2) {
			item2->is_active = true;

			item2->ops->fill_buffer(task->ctx, aml_buf);

			v4l_dbg(task->ctx, V4L_DEBUG_TASK_CHAIN,
				"TSK(%px):%d, vf idx:0x%x, phy:%lx, submit %d => %d.\n",
				task, task->id, task->vf_tmp->index, aml_buf->planes[0].addr,
				type, task->map[0][type]);

			task->direction = TASK_DIR_SUBMIT;

			task_item_put(item2);

			ret = 0;
		} else {
			v4l_dbg(task->ctx, V4L_DEBUG_CODEC_ERROR,
				"TSK(%px):%d get item:%d fail.\n", task, task->id, type);
			ret = -1;
		}
		task_item_put(item);

	} else {
		v4l_dbg(task->ctx, V4L_DEBUG_CODEC_ERROR,
			"TSK(%px):%d get item:%d fail.\n", task, task->id, type);
		ret = -1;
	}

	return ret;
}

static void task_buffer_recycle(struct task_chain_s *task,
			       enum task_type_e type)
{
	struct aml_buf *aml_buf =
		(struct aml_buf *)task->obj;
	struct task_item_s *item = NULL;
	struct task_item_s *item2 = NULL;

	item = task_item_get(task, type);
	if (item) {
		item->is_active = false;

		item2 = task_item_get(task, task->map[1][type]);
		if (item2) {
			struct vframe_s *vf = NULL;

			item2->is_active = true;

			vf = task_item_vframe_pop(item2);
			memcpy(vf, &aml_buf->vframe, sizeof(struct vframe_s));
			item2->ops->put_vframe(item2->caller, vf);

			v4l_dbg(task->ctx, V4L_DEBUG_TASK_CHAIN,
				"TSK(%px):%d, vf idx:%d, phy:%lx, recycle %d => %d.\n",
				task, task->id, vf->index, aml_buf->planes[0].addr,
				type, task->map[1][type]);

			task_item_vframe_reset(item2, vf);
			task->direction = TASK_DIR_RECYCLE;
			task_item_put(item2);
		}
		task_item_put(item);
	}
}

static enum task_type_e task_buffer_get_pre_user(struct task_chain_s *task,
			       enum task_type_e type)
{
	struct task_item_s *item = NULL;
	struct task_item_s *item2 = NULL;
	enum task_type_e type2 = TASK_TYPE_MAX;

	item = task_item_get(task, type);
	if (item) {
		item2 = task_item_get(task, task->map[1][type]);

		if (item2) {
			type2 = name_to_type(item2->name);
			task_item_put(item2);
		}
		task_item_put(item);
	}

	return type2;
}

static enum task_type_e task_buffer_get_next_user(struct task_chain_s *task,
			       enum task_type_e type)
{
	struct task_item_s *item = NULL;
	struct task_item_s *item2 = NULL;
	enum task_type_e type2 = TASK_TYPE_MAX;

	item = task_item_get(task, type);
	if (item) {
		item2 = task_item_get(task, task->map[0][type]);

		if (item2) {
			type2 = name_to_type(item2->name);
			task_item_put(item2);
		}
		task_item_put(item);
	}

	return type2;
}

ssize_t task_chain_show(struct task_chain_s *task, char *buf)
{
	struct task_item_s *item = NULL;
	char tempbuf[128] = {0};
	char *ptbuf = tempbuf;
	char *pbuf = buf;
	ulong flags;

	if (!task || !task->ctx)
		return 0;

	spin_lock_irqsave(&task->slock, flags);

	if (!list_empty(&task->list_item)) {
		struct aml_buf *aml_buf =
			(struct aml_buf *)task->obj;

		list_for_each_entry(item, &task->list_item, node) {
			ptbuf += sprintf(ptbuf, "%s(%d)",
				item->name, item->is_active);
			if (item->node.next != &task->list_item) {
				if (task->direction == TASK_DIR_SUBMIT)
					ptbuf += sprintf(ptbuf, " ==> ");
				else
					ptbuf += sprintf(ptbuf, " <== ");
			}
		}
		pbuf += sprintf(pbuf, "vb:%2d, phy:%lx  %s\n",
			task->id, aml_buf->planes[0].addr, tempbuf);
	}

	spin_unlock_irqrestore(&task->slock, flags);

	return pbuf - buf;
}
EXPORT_SYMBOL(task_chain_show);

static void task_chain_destroy(struct kref *kref)
{
	struct task_chain_s *task;

	task = container_of(kref, struct task_chain_s, ref);

	task->cur_type = TASK_TYPE_MAX;
	memset(task->map, 0, sizeof(task->map));
	kfree(task->vf_tmp);

	v4l_dbg(task->ctx, V4L_DEBUG_TASK_CHAIN,
		"TSK(%px):%d task chain destroyed.\n", task, task->id);

	kfree(task);
}

static void task_item_release(struct kref *kref)
{
	struct task_item_s *item;

	item = container_of(kref, struct task_item_s, ref);
	list_del(&item->node);

	v4l_dbg(item->task->ctx, V4L_DEBUG_TASK_CHAIN,
		"TSK(%px):%d task item:(%px,%d) released.\n",
		item->task, item->task->id, item, item->ops->type);

	kref_put(&item->task->ref, task_chain_destroy);

	kfree(item);
}

bool task_chain_empty(struct task_chain_s *task)
{
	return task->cur_type == TASK_TYPE_MAX ? true : false;
}
EXPORT_SYMBOL(task_chain_empty);

void task_chain_clean(struct task_chain_s *task)
{
	struct task_item_s *item, *tmp;

	v4l_dbg(task->ctx, V4L_DEBUG_TASK_CHAIN,
		"TSK(%px):%d task chain clean.\n", task, task->id);

	if (!list_empty(&task->list_item)) {
		list_for_each_entry_safe(item, tmp, &task->list_item, node)
			kref_put(&item->ref, task_item_release);
	}
}
EXPORT_SYMBOL(task_chain_clean);

void task_chain_release(struct task_chain_s *task)
{
	v4l_dbg(task->ctx, V4L_DEBUG_TASK_CHAIN,
		"TSK(%px):%d task chain release.\n", task, task->id);

	kref_put(&task->ref, task_chain_destroy);
}
EXPORT_SYMBOL(task_chain_release);

void task_order_attach(struct task_chain_s *task,
			 struct task_ops_s *ops,
			 void *caller)
{
	struct task_item_s *item;
	int i;

	item = kzalloc(sizeof(struct task_item_s), GFP_KERNEL);
	if (!item) {
		v4l_dbg(task->ctx, V4L_DEBUG_CODEC_ERROR,
			"TSK(%px):%d alloc item fail.\n", task, task->id);
		return;
	}

	item->task	= task;
	item->ops	= ops;
	item->caller	= caller;
	item->name	= type_to_name(ops->type);
	kref_init(&item->ref);
	for (i = 0 ; i < 3; i++) {
		item->vframe[i].index = -1;
	}

	task->map[0][ops->type] = task->cur_type;
	task->map[1][task->cur_type] = ops->type;
	task->cur_type = ops->type;
	kref_get(&task->ref);

	list_add(&item->node, &task->list_item);

	v4l_dbg(task->ctx, V4L_DEBUG_TASK_CHAIN,
		"TSK(%px):%d attach item:(%px,%d).\n",
		task, task->id, item, ops->type);
}
EXPORT_SYMBOL(task_order_attach);

void task_chain_update_object(struct task_chain_s *task, void *obj)
{
	/*
	 * Note: have to invoke this funtion
	 * if the task object has been changed.
	 */
	task->obj = obj;

	v4l_dbg(task->ctx, V4L_DEBUG_TASK_CHAIN,
		"TSK(%px):%d update task obj:%px.\n",
		task, task->id, obj);
}
EXPORT_SYMBOL(task_chain_update_object);

int task_chain_init(struct task_chain_s **task_out,
			    void *v4l_ctx,
			    void *obj,
			    int vb_idx)
{
	struct task_chain_s *task;

	task = kzalloc(sizeof(struct task_chain_s), GFP_KERNEL);
	if (!task) {
		v4l_dbg(v4l_ctx, V4L_DEBUG_CODEC_ERROR,
			"%s alloc task %d fail.\n", __func__, vb_idx);
		return -ENOMEM;
	}

	task->id	= vb_idx;
	task->obj	= obj;
	task->ctx	= v4l_ctx;
	task->cur_type	= TASK_TYPE_MAX;

	task->vf_tmp = kzalloc(sizeof(struct vframe_s), GFP_KERNEL);
	if (!task->vf_tmp) {
		v4l_dbg(task->ctx, V4L_DEBUG_CODEC_ERROR,
			"%s alloc failed!\n", __func__);
		kfree(task);
		return -ENOMEM;
	}
	kref_init(&task->ref);
	spin_lock_init(&task->slock);
	INIT_LIST_HEAD(&task->list_item);

	task->attach	= task_order_attach;
	task->submit	= task_buffer_submit;
	task->recycle	= task_buffer_recycle;
	task->get_pre_user	= task_buffer_get_pre_user;
	task->get_next_user	= task_buffer_get_next_user;

	*task_out = task;

	v4l_dbg(task->ctx, V4L_DEBUG_TASK_CHAIN,
		"TSK(%px):%d task chain creat success.\n", task, task->id);
	return 0;
}
EXPORT_SYMBOL(task_chain_init);

