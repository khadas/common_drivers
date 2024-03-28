/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _AML_MBOX_H_
#define _AML_MBOX_H_
#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/mailbox_client.h>
#include <linux/mailbox_controller.h>
#include "aml_mbox_cmd.h"

#define MBOX_DATA_SIZE          0x60
#define MBOX_NO_FEEDBACK	0xA5A5A5A5

enum aml_mbox_sync {
	MBOX_ASYNC = 0,    /* Only send data, not get data from remote */
	MBOX_SYNC  = 1,    /* Send data, also can get data from remote if needed */
	MBOX_TSYNC = 2,    /* Send data, wait for data feedback, the wake up wait task */
};

/**
 * struct aml_mbox_data - mbox physical channel info
 * @cmd:               Use to remote mailbox driver recognize callback function
 * @txbuf:             Mbox tansfer data buffer
 * @rxbuf:             Mbox receive data buffer
 * @txsize:            Mbox transfer data size
 * @rxsize:            Mbox receive data size
 * @sync:              Transfer data to sync or not
 * @rev_complete:      Feedback complete address
 * @complete:          Use to process block, until resource release
 */
struct aml_mbox_data {
	u32 cmd;
	void *txbuf;
	void *rxbuf;
	u32 txsize;
	u32 rxsize;
	u8 sync;
	u64 rev_complete;
	struct completion complete;
};

/**
 * struct aml_mbox_chan - mbox physical channel info
 * @mboxid:               Mbox physical channel id
 * @irq_nums:             The total mbox irq nums
 * @mbox_irqctlr_addr:    Mbox irq controller address
 * @mbox_irqclr_addr:     Mbox irq clear address
 * @mbox_irq:             Mbox channel irq number
 * @tx_chan:              If transfer channel
 * @data:                 For payload mbox driver oldest version
 * @mbox_wr_addr:         Mbox data write address
 * @mbox_rd_addr:         Mbox data read address
 * @mbox_fset_addr:       Mbox set address
 * @mbox_fclr_addr:       Mbox clear address
 * @mbox_fsts_addr:       Mbox status address
 * @mutex:                Mbox channel mutex lock
 * @mbox:                 Mbox controller
 * @tx_complete:          Use to recognize the which channel send data
 */
struct aml_mbox_chan {
	/* mbox fifo */
	u32 mboxid;
	u32 irq_nums;
	void __iomem *mbox_irqctlr_addr;
	void __iomem *mbox_irqclr_addr;
	void __iomem *mbox_irqsts_addr;

	/* mbox payload */
	u32 mbox_irq;
	bool tx_chan;
	void *data;

	/* mbox common struct */
	void __iomem *mbox_wr_addr;
	void __iomem *mbox_rd_addr;
	void __iomem *mbox_fset_addr;
	void __iomem *mbox_fclr_addr;
	void __iomem *mbox_fsts_addr;
	void __iomem *aocpu_tick_cnt_addr;
	struct mutex mutex; /* for aml mbox chan mutex */
	struct mbox_controller *mbox;
	void *tx_complete;
};

static inline void mbox_receive_callback(struct mbox_client *cl, void *mssg)
{
	dev_err(cl->dev, "Driver need add callback function\n");
}

/**
 * aml_mbox_request_channel_byidx - Request mbox channel by index
 * @dev: client device node
 * @name: mbox chan index
 * Return: The mbox channel if success
 */
static inline struct mbox_chan *aml_mbox_request_channel_byidx(struct device *dev, int idx)
{
	struct mbox_client *mbox_client;
	struct mbox_chan *mbox_chan;

	mbox_client = devm_kzalloc(dev, sizeof(*mbox_client), GFP_KERNEL);
	if (IS_ERR_OR_NULL(mbox_client))
		return ERR_PTR(-ENOMEM);
	mbox_client->dev = dev;
	mbox_client->tx_block = true;
	mbox_client->tx_tout = 10000;
	mbox_client->rx_callback = mbox_receive_callback;
	mbox_chan = mbox_request_channel(mbox_client, idx);
	if (IS_ERR_OR_NULL(mbox_chan)) {
		devm_kfree(dev, mbox_client);
		dev_err(dev, "Not find mbox channel byidx\n");
	}
	return mbox_chan;
}

/**
 * aml_mbox_request_channel_byname - Request mbox by name
 * @dev: client device node
 * @name: mbox chan name
 * Return: The mbox channel if success
 */
static inline struct mbox_chan *aml_mbox_request_channel_byname(struct device *dev, char *name)
{
	struct mbox_client *mbox_client;
	struct mbox_chan *mbox_chan;

	mbox_client = devm_kzalloc(dev, sizeof(*mbox_client), GFP_KERNEL);
	if (IS_ERR_OR_NULL(mbox_client))
		return ERR_PTR(-ENOMEM);
	mbox_client->dev = dev;
	mbox_client->tx_block = true;
	mbox_client->tx_tout = 10000;
	mbox_client->rx_callback = mbox_receive_callback;
	mbox_chan = mbox_request_channel_byname(mbox_client, name);
	if (IS_ERR_OR_NULL(mbox_chan)) {
		devm_kfree(dev, mbox_client);
		dev_err(dev, "Not find mbox channel byname\n");
	}
	return mbox_chan;
}

/*
 * Check if the aocpu is alive
 * @sts_addr: aocpu tick count read address
 * return - 0: aocpu is alive
 *        - 1: aocpu is not alive
 */
static inline int check_aocpu_status(void __iomem *sts_addr)
{
	u32 aocpu_tick_count_1;
	u32 aocpu_tick_count_2;
	int ret;

	aocpu_tick_count_1 = readl(sts_addr);
	pr_info("%s: aocpu_tick_count_1 = 0x%x\n", __func__,
			aocpu_tick_count_1);
	/* Sleep for 50 msec to let tick count update */
	msleep(50);
	aocpu_tick_count_2 = readl(sts_addr);
	pr_info("%s: aocpu_tick_count_2 = 0x%x\n", __func__,
			aocpu_tick_count_2);
	if (aocpu_tick_count_1 != aocpu_tick_count_2)
		ret = 0;
	else
		ret = 1;
	return ret;
}

/**
 * aml_mbox_transfer_data - A way for transfer mbox data
 * @mbox_chan:            Mbox channel, this requested by mbox consumer driver
 * @cmd:                  Mbox cmd, use remote driver recognize callback function
 * @txbuf:                Mbox transfer data buffer
 * @txsize:               Mbox transfer data size
 * @rxbuf:                Mbox receive data size
 * @rxsize:               Mbox receive data size
 * @sync                  Mbox Sync info
 */
static inline int aml_mbox_transfer_data(struct mbox_chan *mbox_chan, int cmd,
					 void *txbuf, u32 txsize, void *rxbuf, u32 rxsize,
					 enum aml_mbox_sync sync)
{
	struct aml_mbox_data aml_data;
	struct aml_mbox_chan *aml_chan;
	void __iomem *sts_addr;
	int ret;
	int chk_ret;

	if (IS_ERR_OR_NULL(mbox_chan)) {
		pr_err("mbox chan is NULL\n");
		return -EINVAL;
	}
	aml_chan = mbox_chan->con_priv;
	init_completion(&aml_data.complete);
	aml_data.sync = sync;
	aml_data.cmd = cmd;
	aml_data.txbuf = txbuf;
	aml_data.txsize = txsize;
	aml_data.rxbuf = rxbuf;
	aml_data.rxsize = rxsize;
	mutex_lock(&aml_chan->mutex);
	ret = mbox_send_message(mbox_chan, &aml_data);
	mutex_unlock(&aml_chan->mutex);
	if (ret < 0) {
		dev_err(mbox_chan->cl->dev, "Fail to send mbox data %d\n", ret);
		sts_addr = aml_chan->aocpu_tick_cnt_addr;
		if (sts_addr) {
			chk_ret = check_aocpu_status(sts_addr);
			if (!chk_ret)
				pr_info("Info: aocpu is alive\n");
			else
				pr_info("Info: aocpu is not alive\n");
			return ret;
		}
	}
	if (sync == MBOX_TSYNC)
		ret = wait_for_completion_killable(&aml_data.complete);
	return ret;
}

#endif
