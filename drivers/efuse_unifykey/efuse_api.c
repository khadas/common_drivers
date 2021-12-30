// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/arm-smccc.h>
#include <linux/highmem.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/amlogic/efuse.h>
#include <linux/kallsyms.h>
#include "efuse.h"
#include <linux/amlogic/secmon.h>

static ssize_t meson_efuse_fn_smc(struct efuse_hal_api_arg *arg)
{
	long ret;
	unsigned int cmd, offset, size;
	unsigned long *retcnt = (unsigned long *)(arg->retcnt);
	struct arm_smccc_res res;

	if (arg->cmd == EFUSE_HAL_API_READ)
		cmd = efuse_cmd.read_cmd;
	else if (arg->cmd == EFUSE_HAL_API_WRITE)
		cmd = efuse_cmd.write_cmd;
	else
		return -1;

	offset = arg->offset;
	size = arg->size;

	meson_sm_mutex_lock();

	if (arg->cmd == EFUSE_HAL_API_WRITE)
		memcpy((void *)get_meson_sm_input_base(),
		       (const void *)arg->buffer, size);

	arm_smccc_smc(cmd, offset, size, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	if (!ret) {
		meson_sm_mutex_unlock();
		return -1;
	}

	*retcnt = res.a0;

	if (arg->cmd == EFUSE_HAL_API_READ)
		memcpy((void *)arg->buffer,
		       (const void *)get_meson_sm_output_base(), ret);

	meson_sm_mutex_unlock();

	return 0;
}

static ssize_t meson_trustzone_efuse(struct efuse_hal_api_arg *arg)
{
	ssize_t ret;
	struct cpumask task_cpumask;

	if (!arg)
		return -1;

	cpumask_copy(&task_cpumask, current->cpus_ptr);
	set_cpus_allowed_ptr(current, cpumask_of(0));

	ret = meson_efuse_fn_smc(arg);
	set_cpus_allowed_ptr(current, &task_cpumask);

	return ret;
}

static unsigned long efuse_data_process(unsigned long type,
					unsigned long buffer,
					unsigned long length,
					unsigned long option)
{
	struct arm_smccc_res res;

	meson_sm_mutex_lock();

	memcpy((void *)get_meson_sm_input_base(),
	       (const void *)buffer, length);

	do {
		arm_smccc_smc((unsigned long)AML_DATA_PROCESS,
			      (unsigned long)type,
			      (unsigned long)get_secmon_phy_input_base(),
			      (unsigned long)length,
			      (unsigned long)option,
			      0, 0, 0, &res);
	} while (0);

	meson_sm_mutex_unlock();

	return res.a0;
}

int efuse_amlogic_cali_item_read(unsigned int item)
{
	struct arm_smccc_res res;

	/* range check */
	if (item < EFUSE_CALI_SUBITEM_WHOBURN ||
		item > EFUSE_CALI_SUBITEM_BC)
		return -EINVAL;

	meson_sm_mutex_lock();

	do {
		arm_smccc_smc((unsigned long)EFUSE_READ_CALI_ITEM,
			(unsigned long)item,
			0, 0, 0, 0, 0, 0, &res);
	} while (0);

	meson_sm_mutex_unlock();
	return res.a0;
}
EXPORT_SYMBOL_GPL(efuse_amlogic_cali_item_read);

/*
 *return: 1: wrote, 0: not write, -1: fail or not support
 */
int efuse_amlogic_check_lockable_item(unsigned int item)
{
	struct arm_smccc_res res;

	/* range check */
	if (item < EFUSE_LOCK_SUBITEM_BASE ||
		item > EFUSE_LOCK_SUBITEM_MAX)
		return -EINVAL;

	meson_sm_mutex_lock();

	do {
		arm_smccc_smc((unsigned long)EFUSE_READ_CALI_ITEM,
			(unsigned long)item,
			0, 0, 0, 0, 0, 0, &res);
	} while (0);

	meson_sm_mutex_unlock();
	return res.a0;
}
EXPORT_SYMBOL_GPL(efuse_amlogic_check_lockable_item);

unsigned long efuse_amlogic_set(char *buf, size_t count)
{
	unsigned long ret;
	struct cpumask task_cpumask;

	cpumask_copy(&task_cpumask, current->cpus_ptr);
	set_cpus_allowed_ptr(current, cpumask_of(0));

	ret = efuse_data_process(AML_D_P_W_EFUSE_AMLOGIC,
				 (unsigned long)buf, (unsigned long)count, 0);
	set_cpus_allowed_ptr(current, &task_cpumask);

	return ret;
}

static ssize_t meson_trustzone_efuse_get_max(struct efuse_hal_api_arg *arg)
{
	ssize_t ret;
	unsigned int cmd;
	struct arm_smccc_res res;

	if (arg->cmd != EFUSE_HAL_API_USER_MAX)
		return -1;

	meson_sm_mutex_lock();
	cmd = efuse_cmd.get_max_cmd;

	arm_smccc_smc(cmd, 0, 0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	meson_sm_mutex_unlock();

	if (!ret)
		return -1;

	return ret;
}

ssize_t efuse_get_max(void)
{
	struct efuse_hal_api_arg arg;
	ssize_t ret;
	struct cpumask task_cpumask;

	arg.cmd = EFUSE_HAL_API_USER_MAX;

	cpumask_copy(&task_cpumask, current->cpus_ptr);
	set_cpus_allowed_ptr(current, cpumask_of(0));

	ret = meson_trustzone_efuse_get_max(&arg);
	set_cpus_allowed_ptr(current, &task_cpumask);

	return ret;
}

static ssize_t _efuse_read(char *buf, size_t count, loff_t *ppos)
{
	unsigned int pos = *ppos;

	struct efuse_hal_api_arg arg;
	unsigned int retcnt;
	ssize_t ret;

	arg.cmd = EFUSE_HAL_API_READ;
	arg.offset = pos;
	arg.size = count;
	arg.buffer = (unsigned long)buf;
	arg.retcnt = (unsigned long)&retcnt;
	ret = meson_trustzone_efuse(&arg);
	if (ret == 0) {
		*ppos += retcnt;
		return retcnt;
	}

	return ret;
}

static ssize_t _efuse_write(const char *buf, size_t count, loff_t *ppos)
{
	unsigned int pos = *ppos;

	struct efuse_hal_api_arg arg;
	unsigned int retcnt;
	ssize_t ret;

	arg.cmd = EFUSE_HAL_API_WRITE;
	arg.offset = pos;
	arg.size = count;
	arg.buffer = (unsigned long)buf;
	arg.retcnt = (unsigned long)&retcnt;

	ret = meson_trustzone_efuse(&arg);
	if (ret == 0) {
		*ppos = retcnt;
		return retcnt;
	}

	return ret;
}

ssize_t efuse_read_usr(char *buf, size_t count, loff_t *ppos)
{
	char *pdata = NULL;
	ssize_t ret;
	loff_t pos;

	pdata = kmalloc(count, GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	pos = *ppos;

	ret = _efuse_read(pdata, count, (loff_t *)&pos);

	memcpy(buf, pdata, count);
	kfree(pdata);

	return ret;
}

ssize_t efuse_write_usr(char *buf, size_t count, loff_t *ppos)
{
	char *pdata = NULL;
	ssize_t ret;
	loff_t pos;

	pdata = kmalloc(count, GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	memcpy(pdata, buf, count);
	pos = *ppos;

	ret = _efuse_write(pdata, count, (loff_t *)&pos);
	kfree(pdata);

	return ret;
}
