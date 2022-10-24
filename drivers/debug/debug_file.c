// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/workqueue.h>
#include <linux/amlogic/debug_file.h>

#define FILE_CLOSE_FROM_KERNEL		0x00
#define FILE_OPEN_FROM_KERNEL		0x01
#define FILE_CLOSE_READY		0x02

#define	FILE_READ_END			0x04
#define FILE_READ_ERR			0x08
#define	FILE_READ_TO_FIFO_ERR		0x10

#define	FILE_WRITE_GET_FIFO_ERR		0x20
#define	FILE_WRITE_ERR			0x40

static struct kfifo files_info;
static struct proc_dir_entry *proc_debug_file;
static struct proc_dir_entry *proc_files_info;
static LIST_HEAD(files_list);
static LIST_HEAD(files_release_list);
static DEFINE_MUTEX(files_info_lock);
static DEFINE_MUTEX(file_debug_lock);

static atomic_t file_id = ATOMIC_INIT(0);

static DECLARE_WAIT_QUEUE_HEAD(queue_wait_file_info);
static unsigned int file_info_to_fifo(struct kfifo *fifo, void *buf,
					unsigned int len)
{
	unsigned int rlen;

	mutex_lock(&files_info_lock);
	rlen = kfifo_in(fifo, buf, len);
	mutex_unlock(&files_info_lock);
	wake_up(&queue_wait_file_info);

	return rlen;
}

static unsigned int file_debug_to_fifo(struct debug_file *filp, const void *buf,
					unsigned int len)
{
	unsigned int rlen;

	rlen = kfifo_in(&filp->kfifo_buf, buf, len);
	wake_up(&filp->wq);

	return rlen;
}

static ssize_t files_info_read(struct file *file, char __user *buf,
						size_t count, loff_t *ppos)
{
	int ret;
	unsigned int copied;

	mutex_lock(&files_info_lock);
	ret = kfifo_to_user(&files_info, buf, count, &copied);
	mutex_unlock(&files_info_lock);
	if (ret)
		return ret;

	return copied;
}

static ssize_t files_info_write(struct file *file, const char __user *buf,
						size_t count, loff_t *ppos)
{
	struct debug_file *filp;
	struct debug_file_param param;

	if (copy_from_user(&param, buf, sizeof(struct debug_file_param)))
		return -EFAULT;

	mutex_lock(&file_debug_lock);
	list_for_each_entry(filp, &files_list, list) {
		if (filp->param.id == param.id) {
			filp->param.status = param.status;
			break;
		}
	}
	mutex_unlock(&file_debug_lock);

	return count;
}

static __poll_t files_info_poll(struct file *file, poll_table *wait)
{
	__poll_t mask = 0;

	poll_wait(file, &queue_wait_file_info, wait);
	if (!kfifo_is_empty(&files_info))
		mask = EPOLLIN | EPOLLRDNORM;

	return mask;
}

static const struct proc_ops files_info_proc_ops = {
	.proc_read	= files_info_read,
	.proc_write	= files_info_write,
	.proc_lseek	= noop_llseek,
	.proc_poll	= files_info_poll
};

static ssize_t file_read(struct file *file, char __user *buf,
						size_t count, loff_t *ppos)
{
	int ret;
	unsigned int copied;
	struct debug_file *filp = PDE_DATA(file_inode(file));

	ret = kfifo_to_user(&filp->kfifo_buf, buf, count, &copied);
	if (ret)
		return ret;

	return copied;
}

static ssize_t file_write(struct file *file, const char __user *buf,
						size_t count, loff_t *ppos)
{
	int ret;
	unsigned int copied;
	struct debug_file *filp = PDE_DATA(file_inode(file));

	ret = kfifo_from_user(&filp->kfifo_buf, buf, count, &copied);

	if (ret)
		return ret;

	return copied;
}

static int file_close(struct inode *inode, struct file *file)
{
	struct debug_file *filp = PDE_DATA(inode);

	file_info_to_fifo(&files_info, &filp->param, sizeof(struct debug_file_param));

	mutex_lock(&file_debug_lock);
	if (filp->param.status == FILE_CLOSE_READY) {
		list_del(&filp->list);
		list_add(&filp->list, &files_release_list);
		schedule_delayed_work(&filp->release_work, msecs_to_jiffies(100));
	}
	mutex_unlock(&file_debug_lock);

	return 0;
}

static __poll_t file_poll(struct file *file, poll_table *wait)
{
	__poll_t mask = 0;
	struct debug_file *filp = PDE_DATA(file_inode(file));

	poll_wait(file, &filp->wq, wait);
	if (!kfifo_is_empty(&filp->kfifo_buf))
		mask = EPOLLIN | EPOLLRDNORM;

	return mask;
}

static const struct proc_ops file_proc_ops = {
	.proc_read	= file_read,
	.proc_write	= file_write,
	.proc_lseek	= noop_llseek,
	.proc_release	= file_close,
	.proc_poll	= file_poll
};

static void release_debug_file(struct debug_file *filp)
{
	pr_debug("file %s closed, id=%d\n", filp->param.filename, filp->param.id);
	mutex_lock(&file_debug_lock);
	list_del(&filp->list);
	proc_remove(filp->proc_file);
	kfifo_free(&filp->kfifo_buf);
	kfree(filp);
	mutex_unlock(&file_debug_lock);
}

static void release_work(struct work_struct *work)
{
	struct debug_file *filp;

	filp = container_of(work, struct debug_file, release_work.work);
	release_debug_file(filp);
}

struct debug_file *debug_file_open(const char *filename, int flags, umode_t mode)
{
	int ret, exit_flag;
	struct debug_file *filp, *test_filp;

	if (flags & O_RDWR)
		return NULL;

	if (kfifo_avail(&files_info) < sizeof(struct debug_file_param) * 10)
		return NULL;

	filp = kzalloc(sizeof(*filp), GFP_KERNEL);
	if (!filp)
		return NULL;

	strncpy(filp->param.filename, filename, DEBUG_FILE_NAME_LEN - 1);
	filp->param.flags = flags;
	filp->param.mode = mode;
	filp->param.status = FILE_OPEN_FROM_KERNEL;

	while (1) {
		exit_flag = 0;
		mutex_lock(&file_debug_lock);
		list_for_each_entry(test_filp, &files_list, list) {
			if (!strcmp(test_filp->param.filename, filp->param.filename)) {
				exit_flag = 1;
				break;
			}
		}

		list_for_each_entry(test_filp, &files_release_list, list) {
			if (!strcmp(test_filp->param.filename, filp->param.filename)) {
				exit_flag = 1;
				break;
			}
		}
		mutex_unlock(&file_debug_lock);
		if (!exit_flag)
			break;

		mdelay(100);
	}

	INIT_DELAYED_WORK(&filp->release_work, release_work);
	mutex_init(&filp->mutex);
	ret = kfifo_alloc(&filp->kfifo_buf, (2 * PAGE_SIZE), GFP_KERNEL);
	if (ret) {
		pr_err("error kfifo_alloc for %s\n", filename);
		goto err1;
	}
	filp->proc_file = proc_create_data(kbasename(filp->param.filename),
						     0664,
						     proc_debug_file,
						     &file_proc_ops,
						     (void *)filp);
	if (!filp->proc_file) {
		pr_err("failed to create files_info proc file\n");
		goto err2;
	}

	mutex_lock(&file_debug_lock);
	filp->param.id = atomic_read(&file_id);
	atomic_inc(&file_id);
	list_add(&filp->list, &files_list);
	init_waitqueue_head(&filp->wq);
	mutex_unlock(&file_debug_lock);

	pr_debug("open file %s, id=%d\n", filename, filp->param.id);
	file_info_to_fifo(&files_info, &filp->param, sizeof(struct debug_file_param));

	return filp;

err2:
	kfifo_free(&filp->kfifo_buf);
err1:
	kfree(filp);

	return NULL;
}
EXPORT_SYMBOL(debug_file_open);

ssize_t debug_file_read(struct debug_file *filp, void *buf, size_t count)
{
	int ret = kfifo_out(&filp->kfifo_buf, buf, count);

	if (!ret && filp->param.status != FILE_OPEN_FROM_KERNEL)
		return filp->param.status * -1;

	return ret;				// return read bytes
}
EXPORT_SYMBOL(debug_file_read);

ssize_t debug_file_write(struct debug_file *filp, const void *buf, size_t count)
{
	if (filp->param.status != FILE_OPEN_FROM_KERNEL)
		return filp->param.status * -1;

	return file_debug_to_fifo(filp, buf, count);
}
EXPORT_SYMBOL(debug_file_write);

int debug_file_close(struct debug_file *filp)
{
	mutex_lock(&file_debug_lock);
	if (filp->param.status == FILE_OPEN_FROM_KERNEL) {
		filp->param.status = FILE_CLOSE_FROM_KERNEL;
		mutex_unlock(&file_debug_lock);

		file_info_to_fifo(&files_info, &filp->param, sizeof(struct debug_file_param));
	} else {
		if (filp->param.status != FILE_CLOSE_FROM_KERNEL &&
			filp->param.status != FILE_CLOSE_READY) {
			list_del(&filp->list);
			list_add(&filp->list, &files_release_list);
			schedule_delayed_work(&filp->release_work, msecs_to_jiffies(1));
		}
		mutex_unlock(&file_debug_lock);
	}

	return 0;
}
EXPORT_SYMBOL(debug_file_close);

int __init debug_file_init(void)
{
	int ret;

	ret = kfifo_alloc(&files_info, sizeof(struct debug_file_param) * 100, GFP_KERNEL);
	if (ret) {
		pr_err("error kfifo_alloc files_info\n");
		return ret;
	}

	proc_debug_file = proc_mkdir("debug_file", NULL);
	if (!proc_debug_file) {
		pr_err("failed to create debug_file proc entry\n");
		goto err1;
	}

	proc_files_info = proc_create("files_info", 0444, proc_debug_file, &files_info_proc_ops);
	if (!proc_files_info) {
		pr_err("failed to create files_info proc file\n");
		goto err2;
	}

	return 0;

err2:
	proc_remove(proc_debug_file);
err1:
	kfifo_free(&files_info);

	return -1;
}

void __exit debug_file_exit(void)
{
	while (1) {
		if (list_empty(&files_list) && list_empty(&files_release_list))
			break;
		ssleep(1);
	}

	proc_remove(proc_debug_file);
	kfifo_free(&files_info);
}
