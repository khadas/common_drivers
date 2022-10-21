// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/sched/task.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/amlogic/debug_file.h>
#include <linux/moduleparam.h>

#define FILE_CLOSE_FROM_KERNEL		0x00
#define FILE_OPEN_FROM_KERNEL		0x01
#define FILE_CLOSE_READY		0x02

#define	FILE_READ_END			0x04
#define FILE_READ_ERR			0x08
#define	FILE_READ_TO_FIFO_ERR		0x10

#define	FILE_WRITE_GET_FIFO_ERR		0x20
#define	FILE_WRITE_ERR			0x40

static int write_file_num = 1;
static int read_file_num = 1;
module_param(write_file_num, int, 0664);
module_param(read_file_num, int, 0664);

struct write_file {
	struct task_struct *write_task;
	char name[40];
	int id;
};

struct write_file *write_files;

struct read_file {
	struct task_struct *read_task;
	char name[40];
	int id;
};

struct read_file *read_files;

const char *read_file_name[] = {
	"/proc/meminfo",
	"/proc/cpuinfo",
	"/proc/mem_debug",
	"/proc/slabinfo",
	"/proc/partitions",
	"/proc/filesystems",
	"/proc/iomem",
	"/proc/softirqs",
	"/proc/pagetrace",
	"/proc/schedstat"
};

static int write_thread(void *data)
{
	char line[1024];
	int n, i;
	int ret;
	char file_name[40];
	struct debug_file *filp;
	struct write_file *wfp = data;

	sprintf(file_name, "/tmp/data/test%d", wfp->id);
	i = 0;
	while (i < 50) {
		filp = debug_file_open(file_name, O_CREAT | O_APPEND | O_WRONLY, 0664);
		if (!filp) {
			pr_err("%s-%d, failed debug_file_open\n", __func__, __LINE__);
			return -1;
		}
		n = sprintf(line, "%s-%d: thread-%d %d\n", __func__, __LINE__, wfp->id, i);
		ret = debug_file_write(filp, line, n);
		if (ret < 0) {
			pr_err("%s-%d: %d write error %d\n", __func__, __LINE__, i, ret);
			break;
		}
		i++;
		debug_file_close(filp);
	}

	return 0;
}

static int read_thread(void *data)
{
	char line[1024];
	int i;
	int ret;
	struct debug_file *filp;
	struct read_file *rfp = data;

	ssleep(rfp->id + 1);
	filp = debug_file_open(read_file_name[rfp->id], O_RDONLY, 0664);
	if (!filp) {
		pr_err("%s-%d, failed debug_file_open\n", __func__, __LINE__);
		return -1;
	}

	i = 0;
	while (!kthread_should_stop()) {
		memset(line, 0, 1024);
		ret = debug_file_read(filp, line, 1024);
		if (ret < 0) {
			if (ret == -1 * FILE_READ_END)
				pr_info("file %s read end\n", read_file_name[rfp->id]);
			else
				pr_err("%d-%s read err, ret=%d\n", i, read_file_name[rfp->id], ret);
			break;
		}
		if (ret) {
			pr_info("%d-%s: %s", i, read_file_name[rfp->id], line);
			i++;
		}
		msleep(300);
	}

	debug_file_close(filp);
	return 0;
}

static int __init debug_file_test_module_init(void)
{
	int i;

	write_files = kcalloc(write_file_num, sizeof(*write_files), GFP_KERNEL);
	for (i = 0; i < write_file_num; i++) {
		write_files[i].id = i;
		sprintf(write_files[i].name, "write file %d", i);
		write_files[i].write_task = kthread_create(write_thread,
							   &write_files[i],
							   write_files[i].name);
		if (IS_ERR(write_files[i].write_task))
			return PTR_ERR(write_files[i].write_task);
		get_task_struct(write_files[i].write_task);
		wake_up_process(write_files[i].write_task);
	}

	read_files = kcalloc(read_file_num, sizeof(*read_files), GFP_KERNEL);
	for (i = 0; i < read_file_num; i++) {
		read_files[i].id = i;
		sprintf(read_files[i].name, "read file %d", i);
		read_files[i].read_task = kthread_create(read_thread,
							 &read_files[i],
							 read_files[i].name);
		if (IS_ERR(read_files[i].read_task))
			return PTR_ERR(read_files[i].read_task);
		get_task_struct(read_files[i].read_task);
		wake_up_process(read_files[i].read_task);
	}

	return 0;
}

static void __exit debug_file_test_module_exit(void)
{
	int i;

	for (i = 0; i < write_file_num; i++) {
		kthread_stop(write_files[i].write_task);
		put_task_struct(write_files[i].write_task);
		write_files[i].write_task = NULL;
	}
	kfree(write_files);

	for (i = 0; i < read_file_num; i++) {
		kthread_stop(read_files[i].read_task);
		put_task_struct(read_files[i].read_task);
		read_files[i].read_task = NULL;
	}
	kfree(read_files);
}

module_init(debug_file_test_module_init);
module_exit(debug_file_test_module_exit);

MODULE_LICENSE("GPL v2");
