// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include "host.h"
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/kdebug.h>
#include <linux/notifier.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/of.h>
#include <linux/mailbox_client.h>
#include <linux/amlogic/aml_mbox.h>
#include <dt-bindings/firmware/amlogic,firmware.h>
#include "sysfs.h"
#include "host_poll.h"

struct ring_buffer {
	unsigned int magic;
	unsigned int basepaddr;
	unsigned int size;
	unsigned int head;   /*buffer head: move forward only when tail reaches head*/
	unsigned int headr;  /*read position: move forward only when buffer is read/printed*/
	unsigned int tail;   /*write position:move forward only when writing to buffer*/
	char buffer[4];
};

#define DSP_LOGBUFF_MAGIC 0x1234ABCD
#define BUFF_LEN 256
static unsigned int health_polling_ms = 1000;
static int logbuff_polling_ms = 50;
struct ring_buffer *ring_buffer;

module_param(health_polling_ms, uint, 0644);
module_param(logbuff_polling_ms, uint, 0644);

static bool host_health_hang_check(struct host_module *host)
{
	u32 cur_cnt = 0;
	bool ret = false;

	host->hang = 0;
	cur_cnt = readl(host->health_reg);
	pr_debug("[%s][%s][%u %u]\n", __func__,
		 host->host_data->name,
		 host->pre_cnt, cur_cnt);
	if (cur_cnt == host->pre_cnt) {
		host->hang = 1;
		ret = true;
	}
	host->pre_cnt = cur_cnt;
	host->cur_cnt = cur_cnt;
	return ret;
}

static void host_health_monitor(struct work_struct *work)
{
	char data[20], *envp[] = { data, NULL };
	struct host_module *host_firm = container_of((struct delayed_work *)work,
				struct host_module, host_monitor_work);

	if (host_health_hang_check(host_firm)) {
		snprintf(data, sizeof(data), "ACTION=%s_WTD", host_firm->host_data->name);
		kobject_uevent_env(&host_firm->dev->kobj, KOBJ_CHANGE,
				envp);
		pr_debug("[%s][%s_HANG]\n", __func__, host_firm->host_data->name);
	}

	queue_delayed_work(host_firm->host_wq, &host_firm->host_monitor_work,
		msecs_to_jiffies(health_polling_ms));
}

void host_health_monitor_start(struct host_module *host)
{
	host->pre_cnt = 0;
	host->cur_cnt = 0;

	if (IS_ERR_OR_NULL(host->health_reg))
		host->health_reg = NULL;

	if (host->health_reg) {
		host->host_wq = create_workqueue("host_wq");
		if (host->health_polling_ms)
			health_polling_ms = host->health_polling_ms;

		INIT_DEFERRABLE_WORK(&host->host_monitor_work, host_health_monitor);
		queue_delayed_work(host->host_wq, &host->host_monitor_work,
			   msecs_to_jiffies(health_polling_ms));
	}
}

void host_health_monitor_stop(struct host_module *host)
{
	if (!host->host_wq)
		return;
	dev_dbg(host->dev, "[%s %d]\n", __func__, __LINE__);
	cancel_delayed_work_sync(&host->host_monitor_work);
	flush_workqueue(host->host_wq);
	destroy_workqueue(host->host_wq);
	host->host_wq = NULL;
}

static u32 get_buff_len(struct ring_buffer *rbuf)
{
	if (rbuf->tail < rbuf->headr)
		return rbuf->tail + rbuf->size - rbuf->headr;
	else
		return rbuf->tail - rbuf->headr;
}

static u32 host_get_logbuff(struct ring_buffer *rbuf, char *name)
{
	char buff[BUFF_LEN + 1] = {0};
	u32 idx = 0;
	u32 len = 0;

	len = get_buff_len(ring_buffer);
	if (!len)
		return 0;
	if (len > BUFF_LEN)
		len = BUFF_LEN;

	for (idx = 0; idx < len; idx++) {
		buff[idx] = rbuf->buffer[rbuf->headr];
		idx += 1;
		rbuf->headr += 1;
		rbuf->headr %= rbuf->size;
	}
	buff[len] = '\0';
	pr_info("[%s-%u]%s", name, len, buff);

	return len;
}

static int die_notify(struct notifier_block *nb,
			unsigned long cmd, void *data)
{
	return NOTIFY_DONE;
}

static void host_polling_logbuff(struct work_struct *work)
{
	struct host_module *host_firm = container_of((struct delayed_work *)work,
			struct host_module, host_logbuff_work);

	if (IS_ERR_OR_NULL(ring_buffer))
		return;
	host_get_logbuff(ring_buffer, host_firm->host_data->name);
	queue_delayed_work(host_firm->host_wq, &host_firm->host_logbuff_work,
			msecs_to_jiffies(logbuff_polling_ms));
}

static void host_polling_logbuff_start(struct host_module *host)
{
	if (host->logbuff_polling_ms)
		host->host_wq = create_workqueue("host_wq");

	INIT_DEFERRABLE_WORK(&host->host_monitor_work, host_polling_logbuff);
	queue_delayed_work(host->host_wq, &host->host_monitor_work,
			   msecs_to_jiffies(health_polling_ms));
}

int host_logbuff_start(struct host_module *host)
{
	struct ring_buffer rbuf;
	char data[] = "AP request logbuff";
	int ret;

	if (!(host->logbuff_polling_ms))
		return -EINVAL;

	// wait for remote mcu boot up
	msleep(100);

	ret = aml_mbox_transfer_data(host->mbox_chan, MBOX_CMD_HIFI5_SYSLOG_START,
					data, sizeof(data), &rbuf, sizeof(rbuf),
						MBOX_SYNC);
	if (ret < 0) {
		dev_err(host->dev, "mbox transfer data  error %d\n", ret);
		return ret;
	}

	if (rbuf.magic == DSP_LOGBUFF_MAGIC) {
		if (host->start_pos == DDR_SRAM && host->addr_remap)
			rbuf.basepaddr = (rbuf.basepaddr & 0x000fffff) | host->phys_remap_addr;

		if (pfn_valid(__phys_to_pfn(rbuf.basepaddr)))
			ring_buffer = (struct ring_buffer *)__phys_to_virt(rbuf.basepaddr);
		else
			ring_buffer = (struct ring_buffer *)ioremap_cache(rbuf.basepaddr,
								rbuf.size + sizeof(rbuf) - 4);
		pr_debug("[%s %d][0x%x 0x%x %u %u %u %u]\n", __func__, __LINE__,
			  ring_buffer->magic, ring_buffer->basepaddr,
			  ring_buffer->head, ring_buffer->headr,
			  ring_buffer->tail, ring_buffer->size);
		if (host->logbuff_polling_ms)
			logbuff_polling_ms = host->logbuff_polling_ms;
		host_polling_logbuff_start(host);

		host->nb.notifier_call = die_notify;
		register_die_notifier(&host->nb);
	} else {
		ret = -EINVAL;
		ring_buffer = NULL;
	}
	return ret;
}

void host_logbuff_stop(struct host_module *host)
{
	if (IS_ERR_OR_NULL(ring_buffer))
		return;
	if (!host->host_wq)
		return;

	pr_debug("[%s %d]\n", __func__, __LINE__);
	cancel_delayed_work_sync(&host->host_logbuff_work);
	flush_workqueue(host->host_wq);
	unregister_die_notifier(&host->nb);
	destroy_workqueue(host->host_wq);
	host->host_wq = NULL;
}
