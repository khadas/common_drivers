// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/bitops.h>
#include <linux/mailbox_client.h>
#include <linux/amlogic/meson_mhu_common.h>

static LIST_HEAD(mhu_cons);
static DEFINE_MUTEX(mhu_con_mutex);

struct mhu_chan *mhu_get_channel(struct mbox_client *cl, int index)
{
	struct device *dev = cl->dev;
	struct of_phandle_args spec;
	struct mhu_ctlr *mhu_ctlr;
	struct mhu_chan *mhu_chan;
	struct list_head *list;

	if (!dev || !dev->of_node) {
		pr_debug("%s: No owner device node\n", __func__);
		return ERR_PTR(-ENODEV);
	}

	if (of_parse_phandle_with_args(dev->of_node, "mboxes",
				       "#mbox-cells", index, &spec)) {
		dev_dbg(dev, "%s: can't parse \"mboxes\" property\n", __func__);
		return ERR_PTR(-ENODEV);
	}
	mhu_chan = ERR_PTR(-EPROBE_DEFER);
	list_for_each(list, &mhu_cons) {
		mhu_ctlr = list_entry(list, struct mhu_ctlr, list);
		if (mhu_ctlr->dev->of_node == spec.np) {
			mhu_chan = &mhu_ctlr->channels[spec.args[0]];
			break;
		}
	}

	of_node_put(spec.np);

	return mhu_chan;
}
EXPORT_SYMBOL_GPL(mhu_get_channel);

/**
 * mhu_controller_register - Register the mhu mailbox controller
 * @mhu_ctlr:	Pointer to the mhu controller.
 *
 * The controller driver registers its communication channels
 */
int mhu_controller_register(struct mhu_ctlr *mhu_ctlr)
{
	/* Sanity check */
	if (!mhu_ctlr || !mhu_ctlr->dev)
		return -EINVAL;

	mutex_lock(&mhu_con_mutex);
	list_add_tail(&mhu_ctlr->list, &mhu_cons);
	mutex_unlock(&mhu_con_mutex);

	return 0;
}
EXPORT_SYMBOL_GPL(mhu_controller_register);

/**
 * mhu_controller_unregister - Unregister the mhu controller
 * @mhu_ctlr:	Pointer to the mhu controller.
 */
void mhu_controller_unregister(struct mhu_ctlr *mhu_ctlr)
{
	if (!mhu_ctlr)
		return;

	mutex_lock(&mhu_con_mutex);
	list_del(&mhu_ctlr->list);
	mutex_unlock(&mhu_con_mutex);
}
EXPORT_SYMBOL_GPL(mhu_controller_unregister);
