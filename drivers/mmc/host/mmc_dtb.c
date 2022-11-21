// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/slab.h>

#include <linux/scatterlist.h>
#include <linux/swap.h>		/* For nr_free_buffer_pages() */
#include <linux/list.h>

#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>
#include <linux/module.h>

#include "core.h"
#include "card.h"
#include "host.h"
#include "bus.h"
#include "mmc_ops.h"
#include <linux/types.h>
#include <linux/stddef.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/scatterlist.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/amlogic/aml_sd.h>
#include "mmc_common.h"

static dev_t amlmmc_dtb_no;
struct cdev amlmmc_dtb;
struct device *dtb_dev;
struct class *amlmmc_dtb_class;
static char *glb_dtb_buf;
struct mmc_card *card_dtb;
struct aml_dtb_info {
	unsigned int stamp[2];
	u8 valid[2];
};

static struct aml_dtb_info dtb_infos = {{0, 0}, {0, 0} };
struct mmc_partitions_fmt *pt_fmt;
struct mmc_dtb {
	struct mmc_card		*card;		/* the host this device belongs to */
	struct device		dev;		/* the device */
};

#define stamp_after(a, b)	((int)(b) - (int)(a)  < 0)
#define CONFIG_DTB_SIZE  (256 * 1024U)
#define DTB_CELL_SIZE   (16 * 1024U)

#define DTB_NAME		"dtb"
#define	SZ_1M			0x00100000
#define	MMC_DTB_PART_OFFSET	(40 * SZ_1M)
#define	EMMC_BLOCK_SIZE		(0x100)
#define	MAX_EMMC_BLOCK_SIZE	(128 * 1024)

#define DTB_RESERVE_OFFSET	(4 * SZ_1M)
#define	DTB_BLK_SIZE		(0x200)
#define	DTB_BLK_CNT		(512)
#define	DTB_SIZE		(DTB_BLK_CNT * DTB_BLK_SIZE)
#define DTB_COPIES		(2)
#define DTB_AREA_BLK_CNT	(DTB_BLK_CNT * DTB_COPIES)
/* pertransfer for internal operations. */
#define MAX_TRANS_BLK		(256)
#define	MAX_TRANS_SIZE		(MAX_TRANS_BLK * DTB_BLK_SIZE)

struct aml_dtb_rsv {
	u8 data[DTB_BLK_SIZE * DTB_BLK_CNT - 4 * sizeof(unsigned int)];
	unsigned int magic;
	unsigned int version;
	unsigned int timestamp;
	unsigned int checksum;
};

static CLASS_ATTR_STRING(emmcdtb, 0644, NULL);

int mmc_dtb_open(struct inode *node, struct file *file)
{
	return 0;
}

static int _dtb_write(struct mmc_card *mmc, int blk, unsigned char *buf)
{
	int ret = 0;
	unsigned char *src = NULL;
	int bit = mmc->csd.read_blkbits;
	int cnt = CONFIG_DTB_SIZE >> bit;

	src = (unsigned char *)buf;

	mmc_claim_host(mmc->host);
	do {
		ret = mmc_write_internal(mmc, blk, MAX_TRANS_BLK, src);
		if (ret) {
			pr_err("%s: save dtb error", __func__);
			ret = -EFAULT;
			break;
		}
		blk += MAX_TRANS_BLK;
		cnt -= MAX_TRANS_BLK;
		src = (unsigned char *)buf + MAX_TRANS_SIZE;
	} while (cnt != 0);
	mmc_release_host(mmc->host);

	return ret;
}

static unsigned int _calc_dtb_checksum(struct aml_dtb_rsv *dtb)
{
	int i = 0;
	int size = sizeof(struct aml_dtb_rsv) - sizeof(unsigned int);
	unsigned int *buffer;
	unsigned int checksum = 0;

	size = size >> 2;
	buffer = (unsigned int *)dtb;
	while (i < size)
		checksum += buffer[i++];

	return checksum;
}

static int _verify_dtb_checksum(struct aml_dtb_rsv *dtb)
{
	unsigned int checksum;

	checksum = _calc_dtb_checksum(dtb);
	pr_debug("calc %x, store %x\n", checksum, dtb->checksum);

	return !(checksum == dtb->checksum);
}

int amlmmc_dtb_write(struct mmc_card *mmc, unsigned char *buf, int len)
{
	int ret = 0, blk;
	int bit = mmc->csd.read_blkbits;
	int cpy, valid;
	struct aml_dtb_rsv *dtb = (struct aml_dtb_rsv *)buf;
	struct aml_dtb_info *info = &dtb_infos;

	if (len > CONFIG_DTB_SIZE) {
		pr_err("%s dtb data len too much", __func__);
		return -EFAULT;
	}
	/* set info */
	valid = info->valid[0] + info->valid[1];
	if (valid == 0) {
		dtb->timestamp = 0;
	} else if (valid == 1) {
		dtb->timestamp = 1 + info->stamp[info->valid[0] ? 0 : 1];
	} else {
		/* both are valid */
		if (info->stamp[0] != info->stamp[1]) {
			pr_info("timestamp are not same %d:%d\n",
				info->stamp[0], info->stamp[1]);
			dtb->timestamp = 1 +
				(stamp_after(info->stamp[1], info->stamp[0]) ?
				info->stamp[1] : info->stamp[0]);
		} else {
			dtb->timestamp = 1 + info->stamp[0];
		}
	}
	/*setting version and magic*/
	dtb->version = 1; /* base version */
	dtb->magic = 0x00447e41; /*A~D\0*/
	dtb->checksum = _calc_dtb_checksum(dtb);
	pr_info("stamp %d, checksum 0x%x, version %d, magic %s\n",
		dtb->timestamp, dtb->checksum,
		dtb->version, (char *)&dtb->magic);
	/* write down... */
	for (cpy = 0; cpy < DTB_COPIES; cpy++) {
		blk = ((get_reserve_partition_off_from_tbl()
					+ DTB_RESERVE_OFFSET) >> bit)
			+ cpy * DTB_BLK_CNT;
		ret |= _dtb_write(mmc, blk, buf);
	}

	return ret;
}

int amlmmc_dtb_read(struct mmc_card *card, unsigned char *buf, int len)
{
	int ret = 0, start_blk, size, blk_cnt;
	int bit = card->csd.read_blkbits;
	unsigned char *dst = NULL;
	unsigned char *buffer = NULL;

	if (len > CONFIG_DTB_SIZE) {
		pr_err("%s dtb data len too much", __func__);
		return -EFAULT;
	}
	memset(buf, 0x0, len);

	start_blk = MMC_DTB_PART_OFFSET;
	buffer = kmalloc(DTB_CELL_SIZE, GFP_KERNEL | __GFP_RECLAIM);
	if (!buffer)
		return -ENOMEM;

	start_blk >>= bit;
	size = CONFIG_DTB_SIZE;
	blk_cnt = size >> bit;
	dst = (unsigned char *)buffer;
	while (blk_cnt != 0) {
		memset(buffer, 0x0, DTB_CELL_SIZE);
		ret = mmc_read_internal(card, start_blk, (DTB_CELL_SIZE >> bit), dst);
		if (ret) {
			pr_err("%s read dtb error", __func__);
			ret = -EFAULT;
			kfree(buffer);
			return ret;
		}
		start_blk += (DTB_CELL_SIZE >> bit);
		blk_cnt -= (DTB_CELL_SIZE >> bit);
		memcpy(buf, dst, DTB_CELL_SIZE);
		buf += DTB_CELL_SIZE;
	}
	kfree(buffer);
	return ret;
}

ssize_t mmc_dtb_read(struct file *file, char __user *buf,
		     size_t count, loff_t *ppos)
{
	unsigned char *dtb_ptr = NULL;
	ssize_t read_size = 0;
	int ret = 0;

	if (*ppos == CONFIG_DTB_SIZE)
		return 0;

	if (*ppos >= CONFIG_DTB_SIZE) {
		pr_err("%s: out of space!", __func__);
		return -EFAULT;
	}

	dtb_ptr = glb_dtb_buf;
	if (!dtb_ptr)
		return -ENOMEM;

	mmc_claim_host(card_dtb->host);
	ret = amlmmc_dtb_read(card_dtb, (unsigned char *)dtb_ptr, CONFIG_DTB_SIZE);
	if (ret) {
		pr_err("%s: read failed:%d", __func__, ret);
		ret = -EFAULT;
		goto exit;
	}
	if ((*ppos + count) > CONFIG_DTB_SIZE)
		read_size = CONFIG_DTB_SIZE - *ppos;
	else
		read_size = count;
	ret = copy_to_user(buf, (dtb_ptr + *ppos), read_size);
	*ppos += read_size;

exit:
	mmc_release_host(card_dtb->host);
	return read_size;
}

ssize_t mmc_dtb_write(struct file *file,
			const char __user *buf, size_t count, loff_t *ppos)
{
	unsigned char *dtb_ptr = NULL;
	ssize_t write_size = 0;
	int ret = 0;

	if (*ppos == CONFIG_DTB_SIZE)
		return 0;
	if (*ppos >= CONFIG_DTB_SIZE) {
		pr_err("%s: out of space!", __func__);
		return -EFAULT;
	}
	dtb_ptr = glb_dtb_buf;
	if (!dtb_ptr)
		return -ENOMEM;

	mmc_claim_host(card_dtb->host);

	if ((*ppos + count) > CONFIG_DTB_SIZE)
		write_size = CONFIG_DTB_SIZE - *ppos;
	else
		write_size = count;

	ret = copy_from_user((dtb_ptr + *ppos), buf, write_size);

	ret = amlmmc_dtb_write(card_dtb,
			       dtb_ptr, CONFIG_DTB_SIZE);
	if (ret) {
		pr_err("%s: write dtb failed", __func__);
		ret = -EFAULT;
		goto exit;
	}

	*ppos += write_size;
exit:
	mmc_release_host(card_dtb->host);
	return write_size;
}

long mmc_dtb_ioctl(struct file *file, unsigned int cmd, unsigned long args)
{
	return 0;
}

static const struct file_operations dtb_ops = {
	.open = mmc_dtb_open,
	.read = mmc_dtb_read,
	.write = mmc_dtb_write,
	.unlocked_ioctl = mmc_dtb_ioctl,
};

int get_reserve_partition_off_from_tbl(void)
{
	return 0x2400000;
}

static int _dtb_read(struct mmc_card *mmc, int blk, unsigned char *buf)
{
	int ret = 0;
	unsigned char *dst = NULL;
	int bit = mmc->csd.read_blkbits;
	int cnt = CONFIG_DTB_SIZE >> bit;

	dst = (unsigned char *)buf;
	mmc_claim_host(mmc->host);
	do {
		ret = mmc_read_internal(mmc, blk, MAX_TRANS_BLK, dst);
		if (ret) {
			pr_err("%s: save dtb error", __func__);
			ret = -EFAULT;
			break;
		}
		blk += MAX_TRANS_BLK;
		cnt -= MAX_TRANS_BLK;
		dst = (unsigned char *)buf + MAX_TRANS_SIZE;
	} while (cnt != 0);
	mmc_release_host(mmc->host);
	return ret;
}

static int _dtb_init(struct mmc_card *mmc)
{
	int ret = 0;
	struct aml_dtb_rsv *dtb;
	struct aml_dtb_info *info = &dtb_infos;
	int cpy = 1, valid = 0;
	int bit = mmc->csd.read_blkbits;
	int blk;

	if (!glb_dtb_buf) {
		glb_dtb_buf = kmalloc(CONFIG_DTB_SIZE, GFP_KERNEL);
		if (!glb_dtb_buf)
			return -ENOMEM;
	}
	dtb = (struct aml_dtb_rsv *)glb_dtb_buf;

	/* read dtb2 1st, for compatibility without checksum. */
	while (cpy >= 0) {
		blk = ((get_reserve_partition_off_from_tbl()
				+ DTB_RESERVE_OFFSET) >> bit)
				+ cpy * DTB_BLK_CNT;
		if (_dtb_read(mmc, blk, (unsigned char *)dtb)) {
			pr_err("%s: block # %#x ERROR!\n", __func__, blk);
		} else {
			ret = _verify_dtb_checksum(dtb);
			if (!ret) {
				info->stamp[cpy] = dtb->timestamp;
				info->valid[cpy] = 1;
			} else {
				pr_debug("cpy %d is not valid\n", cpy);
			}
		}
		valid += info->valid[cpy];
		cpy--;
	}
	pr_debug("total valid %d\n", valid);

	return ret;
}

void amlmmc_dtb_init(struct mmc_card *card, int *retp)
{
	*retp = 0;
	mmc_claim_host(card->host);

	card_dtb = card;
	pr_debug("%s: register dtb chardev", __func__);

	_dtb_init(card);

	*retp = alloc_chrdev_region(&amlmmc_dtb_no, 0, 1, DTB_NAME);
	if (*retp < 0) {
		pr_err("alloc dtb dev_t no failed");
		*retp = -1;
		goto exit;
	}

	cdev_init(&amlmmc_dtb, &dtb_ops);
	amlmmc_dtb.owner = THIS_MODULE;
	*retp = cdev_add(&amlmmc_dtb, amlmmc_dtb_no, 1);
	if (*retp) {
		pr_err("dtb dev add failed");
		*retp = -1;
		goto exit_err1;
	}

	amlmmc_dtb_class = class_create(THIS_MODULE, DTB_NAME);
	if (IS_ERR(amlmmc_dtb_class)) {
		pr_err("dtb dev add failed");
		*retp = -1;
		goto exit_err2;
	}

	*retp = class_create_file(amlmmc_dtb_class, &class_attr_emmcdtb.attr);
	if (*retp) {
		pr_err("dtb dev add failed");
		*retp = -1;
		goto exit_err2;
	}

	dtb_dev = device_create(amlmmc_dtb_class, NULL, amlmmc_dtb_no, NULL, DTB_NAME);
	if (IS_ERR(dtb_dev)) {
		pr_err("dtb dev add failed");
		*retp = -1;
		goto exit_err3;
	}

	pr_info("%s: register dtb chardev OK", __func__);
	goto exit;

exit_err3:
	class_remove_file(amlmmc_dtb_class, &class_attr_emmcdtb.attr);
	class_destroy(amlmmc_dtb_class);
exit_err2:
	cdev_del(&amlmmc_dtb);
exit_err1:
	unregister_chrdev_region(amlmmc_dtb_no, 1);
exit:
	mmc_release_host(card->host);
}

