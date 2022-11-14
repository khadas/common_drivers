// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

// #define DEBUG

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
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/amlogic/aml_mbox.h>
#include "meson_mbox_sec.h"

#define DRIVER_NAME		"secmbox"
struct sp_mb_csr {
	union {
		u32 d32;
		struct {
			/* words lengthm1   [7:0]   */
			unsigned lengthm1:8;
			/* Full             [8]     */
			unsigned full:1;
			/* Empty            [9]     */
			unsigned empty:1;
			/* Reserved         [15:10] */
			unsigned reserved_1:6;
			/* A2B disable      [16]    */
			unsigned a2b_dis:1;
			/* A2B state        [19:17] */
			unsigned a2b_st:3;
			/* A2B error        [20]    */
			unsigned a2b_err:1;
			/* Reserved         [31:21] */
			unsigned reserved_2:11;
		} b;
	} reg;
};

/*
 * This function writes size of data into buffer of mailbox
 * for sending.
 *
 * Note:
 * Due to hardware design, data size for mailbox send/recv
 * should be a **MULTIPLE** of 4 bytes.
 */
static void mb_write(void *to, void *from, u32 size)
{
	u32 *rd_ptr = (u32 *)from;
	u32 *wr_ptr = (u32 *)to;
	u32 i = 0;

	if (!rd_ptr || !wr_ptr) {
		pr_debug("Invalid inputs for mb_write_buf!\n");
		return;
	}

	for (i = 0; i < size; i += sizeof(u32))
		writel(*rd_ptr++, wr_ptr++);
}

/*
 * This function read size of data from buffer of mailbox
 * for receiving.
 *
 * Note:
 * Due to hardware design, data size for mailbox send/recv
 * should be a **MULTIPLE** of 4 bytes.
 */
static void mb_read(void *to, void *from, u32 size)
{
	u32 *rd_ptr = (u32 *)from;
	u32 *wr_ptr = (u32 *)to;
	u32 i = 0;

	if (!rd_ptr || !wr_ptr) {
		pr_debug("Invalid inputs for mb_read_buf!\n");
		return;
	}

	for (i = 0; i < size; i += sizeof(u32))
		*wr_ptr++ = readl(rd_ptr++);
}

static irqreturn_t mbox_irq_handler(int irq, void *p)
{
	struct mbox_chan *mbox_chan = (struct mbox_chan *)p;
	struct aml_mbox_chan *aml_chan = mbox_chan->con_priv;
	void __iomem *mbox_scpu2nee_csr = aml_chan->mbox_fsts_addr;
	void __iomem *mbox_scpu2nee_data_st = aml_chan->mbox_rd_addr;
	struct aml_mbox_data *aml_data = aml_chan->data;
	struct sp_mb_csr vcsr;
	u32 rx_size;

	vcsr.reg.d32 = readl(mbox_scpu2nee_csr);
	rx_size = (vcsr.reg.b.lengthm1 + 1) * sizeof(u32);
	if (vcsr.reg.b.full) {
		if (IS_ERR_OR_NULL(aml_data))
			return IRQ_NONE;

		if (aml_data->rxbuf) {
			mb_read(aml_data->rxbuf, mbox_scpu2nee_data_st, rx_size);
			aml_data->rxsize = rx_size;
			aml_data->rev_complete = (unsigned long)(&aml_data->complete);
		}
	}

	mbox_chan_txdone(mbox_chan, 0);
	return IRQ_HANDLED;
}

static int mbox_transfer_data(struct mbox_chan *mbox_chan, void *msg)
{
	struct aml_mbox_chan *aml_chan = mbox_chan->con_priv;
	void __iomem *mbox_nee2scpu_csr = aml_chan->mbox_fset_addr;
	void __iomem *mbox_nee2scpu_data_st = aml_chan->mbox_wr_addr;
	struct aml_mbox_data *aml_data = (struct aml_mbox_data *)msg;
	u32 tx_size = 0;
	struct sp_mb_csr vcsr;

	if (IS_ERR_OR_NULL(aml_data))
		return -EINVAL;

	tx_size = aml_data->txsize & 0xff;

	vcsr.reg.d32 = readl(mbox_nee2scpu_csr);
	vcsr.reg.b.lengthm1 = (tx_size / sizeof(u32)) - 1;
	writel(vcsr.reg.d32, mbox_nee2scpu_csr);

	if (aml_data->txbuf) {
		mb_write(mbox_nee2scpu_data_st,
			 aml_data->txbuf, tx_size);
	}
	aml_chan->data = aml_data;
	return 0;
}

static int mbox_startup(struct mbox_chan *mbox_chan)
{
	struct aml_mbox_chan *aml_chan = mbox_chan->con_priv;
	struct mbox_client *cl = mbox_chan->cl;
	int err = 0;

	err = request_threaded_irq(aml_chan->mbox_irq, mbox_irq_handler,
			NULL, IRQF_ONESHOT | IRQF_NO_SUSPEND,
			DRIVER_NAME, mbox_chan);
	if (err)
		dev_err(cl->dev, "request irq error\n");
	return err;
}

static void mbox_shutdown(struct mbox_chan *mbox_chan)
{
	struct aml_mbox_chan *aml_chan = mbox_chan->con_priv;

	aml_chan->data = NULL;
	free_irq(aml_chan->mbox_irq, mbox_chan);
}

static bool mbox_last_tx_done(struct mbox_chan *mbox_chan)
{
	struct aml_mbox_chan *aml_chan = mbox_chan->con_priv;
	void __iomem *mbox_nee2scpu_csr = aml_chan->mbox_fset_addr;
	struct sp_mb_csr vcsr;

	vcsr.reg.d32 = readl(mbox_nee2scpu_csr);
	return !(vcsr.reg.b.empty);
}

static struct mbox_chan_ops mbox_sec_ops = {
	.send_data = mbox_transfer_data,
	.startup = mbox_startup,
	.shutdown = mbox_shutdown,
	.last_tx_done = mbox_last_tx_done,
};

static int mbox_sec_probe(struct platform_device *pdev)
{
	struct mbox_controller *mbox_con;
	struct mbox_chan *mbox_chans;
	struct device *dev = &pdev->dev;
	struct aml_mbox_chan *aml_chan;

	aml_chan = devm_kzalloc(dev, sizeof(*aml_chan), GFP_KERNEL);

	aml_chan->mbox_fset_addr = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(aml_chan->mbox_fset_addr))
		return PTR_ERR(aml_chan->mbox_fset_addr);

	aml_chan->mbox_fsts_addr = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(aml_chan->mbox_fsts_addr))
		return PTR_ERR(aml_chan->mbox_fsts_addr);

	aml_chan->mbox_wr_addr = devm_platform_ioremap_resource(pdev, 2);
	if (IS_ERR(aml_chan->mbox_wr_addr))
		return PTR_ERR(aml_chan->mbox_wr_addr);

	aml_chan->mbox_rd_addr = devm_platform_ioremap_resource(pdev, 3);
	if (IS_ERR(aml_chan->mbox_rd_addr))
		return PTR_ERR(aml_chan->mbox_rd_addr);

	aml_chan->mbox_irq = platform_get_irq(pdev, 0);
	mutex_init(&aml_chan->mutex);

	mbox_chans = devm_kzalloc(dev, sizeof(*mbox_chans), GFP_KERNEL);
	if (IS_ERR_OR_NULL(mbox_chans))
		return -ENOMEM;
	mbox_chans->con_priv = aml_chan;

	mbox_con = devm_kzalloc(dev, sizeof(*mbox_con), GFP_KERNEL);
	if (IS_ERR(mbox_con))
		return PTR_ERR(mbox_con);

	mbox_con->num_chans = 1;
	mbox_con->ops = &mbox_sec_ops;
	mbox_con->dev = dev;
	mbox_con->chans = mbox_chans;
	mbox_con->txdone_irq = true;

	platform_set_drvdata(pdev, mbox_con);
	if (devm_mbox_controller_register(dev, mbox_con)) {
		dev_err(dev, "failed to register mailbox controller\n");
		return -ENOMEM;
	}
	return 0;
}

static int mbox_sec_remove(struct platform_device *pdev)
{
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id mbox_of_match[] = {
	{ .compatible = "amlogic, mbox-sec" },
	{}
};

static struct platform_driver mbox_sec_driver = {
	.probe = mbox_sec_probe,
	.remove = mbox_sec_remove,
	.driver = {
		.owner		= THIS_MODULE,
		.name = DRIVER_NAME,
		.of_match_table = mbox_of_match,
	},
};

int __init mbox_sec_init(void)
{
	return platform_driver_register(&mbox_sec_driver);
}

void __exit mbox_sec_exit(void)
{
	platform_driver_unregister(&mbox_sec_driver);
}
