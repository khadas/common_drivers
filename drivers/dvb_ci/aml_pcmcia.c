// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/device.h>

#include "aml_pcmcia.h"
#include "aml_ci.h"

static int aml_pcmcia_debug = 1;

module_param_named(pcmcia_debug, aml_pcmcia_debug, int, 0644);
MODULE_PARM_DESC(pcmcia_debug, "enable verbose debug messages");

static int reset_time_h_t = 2000;
module_param_named(reset_time_h, reset_time_h_t, int, 0644);
MODULE_PARM_DESC(reset_time_h, "reset time h");

static int reset_time_l_t = 2000;
module_param_named(reset_time_l, reset_time_l_t, int, 0644);
MODULE_PARM_DESC(reset_time_l, "reset time l");

#define pr_dbg(fmt, args...)\
do {\
	if (aml_pcmcia_debug)\
		pr_err("aml_pcmcia:" fmt, ## args);\
} while (0)

#define pr_error(fmt, args...) pr_err("aml_pcmcia: " fmt, ## args)

static int pcmcia_plugin(struct aml_pcmcia *pc, int reset)
{
	if (pc->slot_state == MODULE_XTRACTED) {
		pc->pwr(pc, AML_PWR_OPEN);/*hi is open power*/
		pr_dbg("CAM Plugged IN: Adapter(%d) Slot(0)\n", 0);
		//udelay(50);
		usleep_range(50, 60);
		if (pc->io_device_type != AML_DVB_IO_TYPE_CIBUS)
			aml_pcmcia_reset(pc);
		/*wait unplug*/
		pc->init_irq(pc, IRQF_TRIGGER_RISING);
		//udelay(500);
		usleep_range(500, 510);
		pc->slot_state = MODULE_INSERTED;

	} else {
		pr_error("repeat into pcmcia insert \r\n");
		if (reset)
			aml_pcmcia_reset(pc);
	}

	//msleep(1);
	usleep_range(1000, 1010);
	pc->pcmcia_plugin(pc, 1);

	return 0;
}

static int pcmcia_unplug(struct aml_pcmcia *pc)
{
	if (pc->slot_state == MODULE_INSERTED) {
		pr_dbg(" CAM Unplugged: Adapter(%d) Slot(0)\n", 0);
		/*udelay(50);*/
		/*aml_pcmcia_reset(pc);*/
		/*wait plugin*/
		pc->init_irq(pc, IRQF_TRIGGER_FALLING);
		//udelay(500);
		usleep_range(500, 510);
		pc->pwr(pc, AML_PWR_CLOSE);/*hi is open power*/

		pc->slot_state = MODULE_XTRACTED;
	}
	usleep_range(1000, 1010);
	//msleep(1);
	pc->pcmcia_plugin(pc, 0);

	return 0;
}

static irqreturn_t pcmcia_irq_handler(int irq, void *dev_id)
{
	struct aml_pcmcia *pc = (struct aml_pcmcia *)dev_id;

	pr_dbg("%s--into--\r\n", __func__);
	disable_irq_nosync(pc->irq);
	schedule_work(&pc->pcmcia_work);
	enable_irq(pc->irq);
	return IRQ_HANDLED;
}

static void aml_pcmcia_work(struct work_struct *work)
{
	int cd1, cd2;
	struct aml_pcmcia *pc = container_of(work, struct aml_pcmcia, pcmcia_work);

	if (pc->start_work == 0)
		return;
	cd1 = pc->get_cd1(pc);
	cd2 = pc->get_cd2(pc);

	if (cd1 != cd2) {
		pr_error("work CAM card not inerted.\n");
	} else {
		if (!cd1) {
			pr_error("work Adapter(%d) Slot(0): CAM Plugin\n", 0);
			pcmcia_plugin(pc, 0);
		} else {
			pr_error("work Adapter(%d) Slot(0): CAM Unplug\n", 0);
			pcmcia_unplug(pc);
		}
	}
}

void aml_pcmcia_detect_cam(struct aml_pcmcia *pc)
{
	int cd1, cd2;

	if (!pc) {
		pr_error("pc is null\n");
		return;
	}
	if (pc->start_work == 0) {
		pr_error("pc start work is 0\n");
		return;
	}
	cd1 = pc->get_cd1(pc);
	cd2 = pc->get_cd2(pc);

	if (cd1 != cd2) {
		pr_error("CAM card not inerted. check end\n");
	} else {
		if (!cd1) {
			pr_error("Adapter(%d) Slot(0): CAM Plugin\n", 0);
			pcmcia_plugin(pc, 1);
		} else {
			pr_error("Adapter(%d) Slot(0): CAM Unplug\n", 0);
			pcmcia_unplug(pc);
		}
	}
}
EXPORT_SYMBOL(aml_pcmcia_detect_cam);

static struct aml_pcmcia *pc_cur;

int aml_pcmcia_init(struct aml_pcmcia *pc)
{
	int err = 0;
	unsigned long mode;

	pr_dbg("%s start pc->irq=%d\r\n", __func__, pc->irq);
	pc->rst(pc, AML_L);
	/*power on*/
	if (pc->io_device_type != AML_DVB_IO_TYPE_CIBUS)
		pc->pwr(pc, AML_PWR_OPEN);/*hi is open power*/
	/*assuming cam unpluged, config the INT to waiting-for-plugin mode*/
	pc->init_irq(pc, IRQF_TRIGGER_LOW);

	INIT_WORK(&pc->pcmcia_work, aml_pcmcia_work);

	mode = IRQF_ONESHOT;
	if (pc->io_device_type == AML_DVB_IO_TYPE_SPI_T312 ||
		pc->io_device_type == AML_DVB_IO_TYPE_CIBUS) {
		mode = mode | IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;
	}

	err = request_irq(pc->irq,
	pcmcia_irq_handler,
	mode, "aml-pcmcia", pc);
	if (err != 0) {
		pr_error("ERROR: IRQ registration failed ! <%d>", err);
		return -ENODEV;
	}

	pc_cur = pc;
	pr_dbg("%s ok\r\n", __func__);
	if (pc->io_device_type == AML_DVB_IO_TYPE_SPI_T312 ||
		pc->io_device_type == AML_DVB_IO_TYPE_CIBUS) {
		//mcu start very fast,so she can detect cam before soc init end.
		//so we need add detect cam fun for first time.
		aml_pcmcia_detect_cam(pc);
	}
	return 0;
}
EXPORT_SYMBOL(aml_pcmcia_init);

int aml_pcmcia_exit(struct aml_pcmcia *pc)
{
	pc->pwr(pc, AML_PWR_CLOSE);/*hi is open power*/
	free_irq(pc->irq, pc);
	return 0;
}
EXPORT_SYMBOL(aml_pcmcia_exit);

int aml_pcmcia_reset(struct aml_pcmcia *pc)
{
		pr_dbg("CAM RESET-->start\n");
		/* viaccess neotion cam need delay 2000 and 3000 */
		/* smit cam need delay 1000 and 1500 */
		/* need change delay according cam vendor */
		pc->rst(pc, AML_H);/*HI is reset*/
		msleep(reset_time_h_t);
		pc->rst(pc, AML_L);/*default LOW*/
		msleep(reset_time_l_t);
		pr_dbg("CAM RESET--end\n");
	return 0;
}
EXPORT_SYMBOL(aml_pcmcia_reset);
