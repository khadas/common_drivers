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
#include <linux/module.h>

#include "aml_vcodec_drv.h"
#include "aml_vcodec_util.h"

void aml_vcodec_set_curr_ctx(struct aml_vcodec_dev *dev,
	struct aml_vcodec_ctx *ctx)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->irqlock, flags);
	dev->curr_ctx = ctx;
	spin_unlock_irqrestore(&dev->irqlock, flags);
}
EXPORT_SYMBOL(aml_vcodec_set_curr_ctx);

struct aml_vcodec_ctx *aml_vcodec_get_curr_ctx(struct aml_vcodec_dev *dev)
{
	unsigned long flags;
	struct aml_vcodec_ctx *ctx;

	spin_lock_irqsave(&dev->irqlock, flags);
	ctx = dev->curr_ctx;
	spin_unlock_irqrestore(&dev->irqlock, flags);
	return ctx;
}
EXPORT_SYMBOL(aml_vcodec_get_curr_ctx);

int user_to_task(enum buf_core_user user)
{
	enum task_type_e t;

	switch (user) {
	case BUF_USER_DEC:
		t = TASK_TYPE_DEC;
		break;
	case BUF_USER_VPP:
		t = TASK_TYPE_VPP;
		break;
	case BUF_USER_GE2D:
		t = TASK_TYPE_GE2D;
		break;
	case BUF_USER_VSINK:
		t = TASK_TYPE_V4L_SINK;
		break;
	default:
		t = TASK_TYPE_MAX;
	}

	return t;
}
EXPORT_SYMBOL(user_to_task);

int task_to_user(enum task_type_e task)
{
	enum buf_core_user t;

	switch (task) {
	case TASK_TYPE_DEC:
		t = BUF_USER_DEC;
		break;
	case TASK_TYPE_VPP:
		t = BUF_USER_VPP;
		break;
	case TASK_TYPE_GE2D:
		t = BUF_USER_GE2D;
		break;
	case TASK_TYPE_V4L_SINK:
		t = BUF_USER_VSINK;
		break;
	default:
		t = BUF_USER_MAX;
	}

	return t;
}
EXPORT_SYMBOL(task_to_user);

