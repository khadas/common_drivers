// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/mailbox_controller.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/mailbox_client.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/of_device.h>
#include <linux/amlogic/aml_mbox.h>
#include <dt-bindings/mailbox/t5w-mbox.h>
#include <dt-bindings/mailbox/txhd2-mbox.h>
#include "meson_mbox_pl.h"
#include "meson_mbox_comm.h"

#define DRIVER_NAME     "meson_mbox_pl_v2"

#define MBOX_HEAD_SIZE       0x1C
#define MBOX_PAYLOAD_SIZE    0x80
#define ASYNC	1
#define SYNC	2

struct mbox_header {
	u32 status;
	u64 task;
	u64 complete;
	u64 ullclt;
} __packed;

struct mbox_data {
	struct mbox_header mbox_header;
	char data[MBOX_DATA_SIZE];
} __packed;

struct aml_chan_priv {
	struct aml_mbox_chan *aml_chan;
	u32 mbox_nums;
};

struct mbox_domain {
	u32 drvid;
	u32 mboxid;
	u32 flags;
};

struct mbox_domain_data {
	struct mbox_domain *mbox_domains;
	u32 domain_counts;
};

#define MBOX_DOMAIN(drv_id, mbox_id, flag)		\
{					\
		.flags = flag,					\
		.drvid = drv_id,				\
		.mboxid = mbox_id,				\
}

static void mbox_fifo_write(void __iomem *to, void *from, long count)
{
	int i = 0;
	long len = count / 4;
	long rlen = count % 4;
	u32 rdata = 0;
	u32 *p = (u32 *)from;

	while (len > 0) {
		writel(*(const u32 *)p, (to + 4 * i));
		len--;
		i++;
		p++;
	}

	if (rlen != 0) {
		memcpy(&rdata, p, rlen);
		writel(rdata, (to + 4 * i));
	}
}

static void mbox_fifo_read(void *to, void __iomem *from, long count)
{
	int i = 0;
	long len = count / 4;
	long rlen = count % 4;
	u32 rdata = 0;
	u32 *p = to;

	while (len > 0) {
		*p = readl(from + (4 * i));
		len--;
		i++;
		p++;
	}

	if (rlen != 0) {
		rdata = readl(from + (4 * i));
		memcpy(p, &rdata, rlen);
	}
}

static int mbox_transfer_data(struct mbox_chan *mbox_chan, void *data)
{
	struct aml_mbox_chan *aml_chan = mbox_chan->con_priv;
	struct aml_mbox_data *aml_data = data;
	union mbox_stat mbox_stat;
	struct mbox_cmd_t *mbox_cmd = &mbox_stat.mbox_cmd_t;
	struct mbox_header mbox_header;

	mbox_cmd->cmd = aml_data->cmd;

	if (!aml_chan->tx_chan) {
		dev_err(mbox_chan->cl->dev, "%s: mbox channel is not tx channel\n",
			__func__);
		return -EINVAL;
	}
	if (aml_data->txsize > MBOX_DATA_SIZE) {
		dev_err(mbox_chan->cl->dev, "%s: size %x > %x\n",
			__func__, aml_data->txsize, MBOX_DATA_SIZE);
		return -EINVAL;
	}

	memset(&mbox_header, 0, sizeof(mbox_header));
	if (aml_data->sync == MBOX_SYNC) {
		mbox_cmd->sync = SYNC;
	} else {
		mbox_cmd->sync = ASYNC;
		if (aml_data->sync == MBOX_TSYNC)
			mbox_header.complete = (unsigned long)(&aml_data->complete);
		else
			mbox_header.complete = MBOX_NO_FEEDBACK;
		mbox_fifo_write(aml_chan->mbox_wr_addr, &mbox_header, sizeof(mbox_header));
	}

	mbox_cmd->size = aml_data->txsize + sizeof(mbox_header);
	aml_chan->tx_complete = &mbox_chan->tx_complete;
	if (aml_data->txbuf) {
		mbox_fifo_write(aml_chan->mbox_wr_addr + MBOX_HEAD_SIZE,
				aml_data->txbuf, aml_data->txsize);
	}
	writel(mbox_stat.set_cmd, aml_chan->mbox_fset_addr);
	return 0;
}

static int mbox_startup(struct mbox_chan *mbox_chan)
{
	return 0;
}

static void mbox_shutdown(struct mbox_chan *mbox_chan)
{
}

static bool mbox_last_tx_done(struct mbox_chan *mbox_chan)
{
	struct aml_mbox_chan *aml_chan = mbox_chan->con_priv;
	void __iomem *mbox_fsts_addr = aml_chan->mbox_fsts_addr;

	return !readl(mbox_fsts_addr);
}

static struct mbox_chan_ops mbox_pl_ops = {
	.send_data = mbox_transfer_data,
	.startup = mbox_startup,
	.shutdown = mbox_shutdown,
	.last_tx_done = mbox_last_tx_done,
};

static void mbox_wakeup_wait_task(void *mssg)
{
	struct aml_mbox_data *aml_rdata = mssg;
	struct completion *p_comp = (struct completion *)(unsigned long)aml_rdata->rev_complete;
	struct aml_mbox_data *aml_sdata = container_of(p_comp, struct aml_mbox_data, complete);

	if (IS_ERR_OR_NULL(p_comp))
		return;

	if (aml_rdata->rxsize && aml_rdata->rxbuf) {
		aml_sdata->rxsize = aml_rdata->rxsize;
		memcpy(aml_sdata->rxbuf, aml_rdata->rxbuf, aml_rdata->rxsize);
	}
	complete(p_comp);
}

static irqreturn_t mbox_irq_handler(int irq, void *p)
{
	struct aml_mbox_chan *aml_chan = p;
	struct mbox_chan *mbox_chan;
	struct mbox_controller *mbox_contr = aml_chan->mbox;
	struct mbox_data mbox_data;
	struct mbox_header *mbox_header;
	struct aml_mbox_data aml_data;
	union mbox_stat mbox_stat;
	struct mbox_cmd_t *mbox_cmd = &mbox_stat.mbox_cmd_t;

	mbox_stat.set_cmd = readl(aml_chan->mbox_fsts_addr);
	if (mbox_stat.set_cmd) {
		aml_data.cmd = mbox_cmd->cmd;
		if (mbox_cmd->size >= MBOX_HEAD_SIZE) {
			aml_data.cmd = mbox_cmd->cmd;
			if (mbox_cmd->size > (MBOX_DATA_SIZE + MBOX_HEAD_SIZE))
				mbox_cmd->size = MBOX_DATA_SIZE + MBOX_HEAD_SIZE;
			mbox_fifo_read(&mbox_data, aml_chan->mbox_rd_addr, mbox_cmd->size);
			mbox_header = &mbox_data.mbox_header;
			if (mbox_header->complete == MBOX_NO_FEEDBACK)
				goto mbox_irq_handler_done;
			aml_data.rxsize = mbox_cmd->size - MBOX_HEAD_SIZE;
			aml_data.rxbuf = mbox_data.data;
			if (mbox_header->complete != 0) {
				aml_data.rev_complete = mbox_header->complete;
				mbox_wakeup_wait_task(&aml_data);
				goto mbox_irq_handler_done;
			}

			if (mbox_header->status < mbox_contr->num_chans) {
				mbox_chan = &mbox_contr->chans[mbox_header->status];
				mbox_chan_received_data(mbox_chan, &aml_data);
			}
		}
	}
mbox_irq_handler_done:
	writel(~0, aml_chan->mbox_fclr_addr);
	return IRQ_HANDLED;
}

static irqreturn_t mbox_ack_irq_handler(int irq, void *p)
{
	struct aml_mbox_chan *aml_chan = p;
	struct mbox_chan *mbox_chan = container_of(aml_chan->tx_complete,
						   struct mbox_chan, tx_complete);
	struct aml_mbox_data *aml_data = mbox_chan->active_req;

	if (aml_data && aml_data->sync == MBOX_SYNC) {
		if (aml_data->rxbuf && aml_data->rxsize) {
			if (aml_data->rxsize > MBOX_DATA_SIZE) {
				dev_err(mbox_chan->cl->dev, "%s: rxsize %x > %x\n",
					__func__, aml_data->txsize, MBOX_DATA_SIZE);
				aml_data->rxsize = MBOX_DATA_SIZE;
			}
			mbox_fifo_read(aml_data->rxbuf,
				       aml_chan->mbox_rd_addr + MBOX_HEAD_SIZE,
				       aml_data->rxsize);
		}
	}
	mbox_chan_txdone(mbox_chan, 0);
	return IRQ_HANDLED;
}

static int mbox_pl_parse_dt(struct platform_device *pdev, struct aml_chan_priv *priv)
{
	struct aml_mbox_chan *aml_chan;
	struct device *dev = &pdev->dev;
	const char *dir;
	u32 mbox_nums = 0;
	int idx = 0;
	int err = 0;

	err = of_property_read_u32(dev->of_node,
				   "mbox-nums", &mbox_nums);
	if (err) {
		dev_err(dev, "failed to get mbox num %d\n", err);
		return -ENXIO;
	}
	priv->mbox_nums = mbox_nums;
	aml_chan = devm_kzalloc(dev, sizeof(*aml_chan) * mbox_nums, GFP_KERNEL);
	if (IS_ERR(aml_chan))
		return -ENOMEM;
	for (idx = 0; idx < mbox_nums; idx++) {
		err = of_property_read_string_index(dev->of_node, "mbox-tx", idx, &dir);
		if (!err && !strncmp(dir, "tx", 2))
			aml_chan[idx].tx_chan = true;
		else
			aml_chan[idx].tx_chan = false;
		err = of_property_read_u32_index(dev->of_node, "mboxids",
						 idx, &aml_chan[idx].mboxid);
		if (err) {
			dev_err(dev, "mboxids define error\n");
			return err;
		}
		aml_chan[idx].mbox_irq = platform_get_irq(pdev, idx);
		aml_chan[idx].mbox_wr_addr = devm_platform_ioremap_resource(pdev, idx * 4 + 0);
		if (IS_ERR_OR_NULL(aml_chan[idx].mbox_wr_addr))
			return PTR_ERR(aml_chan[idx].mbox_wr_addr);
		aml_chan[idx].mbox_rd_addr = aml_chan[idx].mbox_wr_addr;

		aml_chan[idx].mbox_fset_addr = devm_platform_ioremap_resource(pdev, idx * 4 + 1);
		if (IS_ERR(aml_chan[idx].mbox_fset_addr))
			return PTR_ERR(aml_chan[idx].mbox_fset_addr);

		aml_chan[idx].mbox_fclr_addr = devm_platform_ioremap_resource(pdev, idx * 4 + 2);
		if (IS_ERR(aml_chan[idx].mbox_fclr_addr))
			return PTR_ERR(aml_chan[idx].mbox_fclr_addr);

		aml_chan[idx].mbox_fsts_addr = devm_platform_ioremap_resource(pdev, idx * 4 + 3);
		if (IS_ERR(aml_chan[idx].mbox_fsts_addr))
			return PTR_ERR(aml_chan[idx].mbox_fsts_addr);

		mutex_init(&aml_chan[idx].mutex);
		aml_chan[idx].tx_complete = NULL;
	}
	priv->aml_chan = aml_chan;
	return 0;
}

static int mbox_pl_probe(struct platform_device *pdev)
{
	struct mbox_chan *mbox_chans;
	struct device *dev = &pdev->dev;
	struct aml_mbox_chan *aml_chan;
	struct mbox_controller *mbox_cons;
	const struct mbox_domain_data *match;
	struct mbox_domain *mbox_domains;
	struct aml_chan_priv aml_priv;
	u32 mboxid = 0;
	u32 mbox_nums;
	int err = 0;
	int idx0 = 0;
	int idx1 = 0;

	dev_dbg(dev, "mbox pl probe\n");

	err = mbox_pl_parse_dt(pdev, &aml_priv);
	if (err) {
		dev_err(dev, "mbox parse dt fail\n");
		return err;
	}
	aml_chan = aml_priv.aml_chan;
	mbox_nums = aml_priv.mbox_nums;

	match = of_device_get_match_data(&pdev->dev);
	if (!match) {
		dev_err(&pdev->dev, "failed to get match data\n");
		return -ENODEV;
	}
	mbox_domains = match->mbox_domains;
	mbox_chans = devm_kzalloc(dev, sizeof(*mbox_chans) * match->domain_counts, GFP_KERNEL);
	if (IS_ERR(mbox_chans))
		return PTR_ERR(mbox_chans);

	for (idx0 = 0; idx0 < match->domain_counts; idx0++) {
		mboxid = mbox_domains[idx0].mboxid;
		for (idx1 = 0; idx1 < mbox_nums; idx1++) {
			if (mboxid == aml_chan[idx1].mboxid) {
				mbox_chans[idx0].con_priv = &aml_chan[idx1];
				if (!aml_chan[idx1].tx_chan)
					aml_chan[idx1].tx_complete = &mbox_chans[idx0].tx_complete;
				break;
			}
		}
	}
	mbox_cons = devm_kzalloc(dev, sizeof(*mbox_cons), GFP_KERNEL);
	if (IS_ERR(mbox_cons))
		return PTR_ERR(mbox_cons);

	mbox_cons->chans = mbox_chans;
	mbox_cons->num_chans = match->domain_counts;
	mbox_cons->txdone_irq = true;
	mbox_cons->ops = &mbox_pl_ops;
	mbox_cons->dev = dev;
	platform_set_drvdata(pdev, mbox_cons);
	if (mbox_controller_register(mbox_cons)) {
		dev_err(dev, "failed to register mailbox controller\n");
		return -ENOMEM;
	}

	for (idx0 = 0; idx0 < mbox_nums; idx0++) {
		aml_chan[idx0].mbox = mbox_cons;
		if (aml_chan[idx0].tx_chan) {
			err = request_threaded_irq(aml_chan[idx0].mbox_irq,
						   mbox_ack_irq_handler,
						   NULL, IRQF_ONESHOT | IRQF_NO_SUSPEND,
						   DRIVER_NAME, &aml_chan[idx0]);
			if (err) {
				dev_err(dev, "request irq error %x\n",
					aml_chan[idx0].mbox_irq);
				return err;
			}
		} else {
			err = request_threaded_irq(aml_chan[idx0].mbox_irq, mbox_irq_handler,
						   NULL, IRQF_ONESHOT | IRQF_NO_SUSPEND,
						   DRIVER_NAME, &aml_chan[idx0]);
			if (err) {
				dev_err(dev, "request irq error %x\n",
					aml_chan[idx0].mbox_irq);
				return err;
			}
		}
	}
	dev_dbg(dev, "pl mbox init done\n");
	return 0;
}

static int mbox_pl_remove(struct platform_device *pdev)
{
	platform_set_drvdata(pdev, NULL);
	return 0;
}

struct mbox_domain t5w_mbox_domains[] = {
	[T5W_AO2REE]    = MBOX_DOMAIN(T5W_AO2REE, T5W_MBOX_AO2REE, 0),
	[T5W_REE2AO0]   = MBOX_DOMAIN(T5W_REE2AO0, T5W_MBOX_REE2AO, 0),
	[T5W_REE2AO1]   = MBOX_DOMAIN(T5W_REE2AO1, T5W_MBOX_REE2AO, 0),
	[T5W_REE2AO2]   = MBOX_DOMAIN(T5W_REE2AO2, T5W_MBOX_REE2AO, 0),
	[T5W_REE2AO3]   = MBOX_DOMAIN(T5W_REE2AO3, T5W_MBOX_REE2AO, 0),
	[T5W_REE2AO4]   = MBOX_DOMAIN(T5W_REE2AO4, T5W_MBOX_REE2AO, 0),
	[T5W_REE2AO5]   = MBOX_DOMAIN(T5W_REE2AO5, T5W_MBOX_REE2AO, 0),
};

static struct mbox_domain_data t5w_mbox_domains_data __initdata = {
	.mbox_domains = t5w_mbox_domains,
	.domain_counts = ARRAY_SIZE(t5w_mbox_domains),
};

struct mbox_domain txhd2_mbox_domains[] = {
	[TXHD2_AO2REE]    = MBOX_DOMAIN(TXHD2_AO2REE, TXHD2_MBOX_AO2REE, 0),
	[TXHD2_REE2AO0]   = MBOX_DOMAIN(TXHD2_REE2AO0, TXHD2_MBOX_REE2AO, 0),
	[TXHD2_REE2AO1]   = MBOX_DOMAIN(TXHD2_REE2AO1, TXHD2_MBOX_REE2AO, 0),
	[TXHD2_REE2AO2]   = MBOX_DOMAIN(TXHD2_REE2AO2, TXHD2_MBOX_REE2AO, 0),
	[TXHD2_REE2AO3]   = MBOX_DOMAIN(TXHD2_REE2AO3, TXHD2_MBOX_REE2AO, 0),
	[TXHD2_REE2AO4]   = MBOX_DOMAIN(TXHD2_REE2AO4, TXHD2_MBOX_REE2AO, 0),
	[TXHD2_REE2AO5]   = MBOX_DOMAIN(TXHD2_REE2AO5, TXHD2_MBOX_REE2AO, 0),
};

static struct mbox_domain_data txhd2_mbox_domains_data __initdata = {
	.mbox_domains = txhd2_mbox_domains,
	.domain_counts = ARRAY_SIZE(txhd2_mbox_domains),
};

static const struct of_device_id mbox_of_match[] = {
	{
		.compatible = "amlogic, t5w-mbox-pl",
		.data = &t5w_mbox_domains_data,
	},
	{
		.compatible = "amlogic, txhd2-mbox-pl",
		.data = &txhd2_mbox_domains_data,
	},
	{}
};

static struct platform_driver mbox_pl_driver = {
	.probe  = mbox_pl_probe,
	.remove = mbox_pl_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name  = DRIVER_NAME,
		.of_match_table = mbox_of_match,
	},
};

int __init mbox_pl_v2_init(void)
{
	return platform_driver_register(&mbox_pl_driver);
}

void __exit mbox_pl_v2_exit(void)
{
	platform_driver_unregister(&mbox_pl_driver);
}
