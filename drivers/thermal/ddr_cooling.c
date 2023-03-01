// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/thermal.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/amlogic/ddr_cooling.h>
#include <linux/amlogic/meson_cooldev.h>
#include <linux/io.h>
#include "thermal_core.h"

static int ddrfreqcd_id;
static DEFINE_MUTEX(cooling_ddr_lock);
static DEFINE_MUTEX(cooling_list_lock);
static LIST_HEAD(ddr_dev_list);

static void ddr_coolingdevice_id_get(int *id)
{
	mutex_lock(&cooling_ddr_lock);
	*id = ddrfreqcd_id++;
	mutex_unlock(&cooling_ddr_lock);
}

static void ddr_coolingdevice_id_put(void)
{
	mutex_lock(&cooling_ddr_lock);
	ddrfreqcd_id--;
	mutex_unlock(&cooling_ddr_lock);
}

static int ddr_get_max_state(struct thermal_cooling_device *cdev,
			     unsigned long *state)
{
	struct ddr_cooling_device *ddr_device = cdev->devdata;

	*state = ddr_device->ddr_status - 1;
	return 0;
}

static int ddr_get_cur_state(struct thermal_cooling_device *cdev,
			     unsigned long *state)
{
	struct ddr_cooling_device *ddr_device = cdev->devdata;
	u32 val, mask;
	u32 start_bit, end_bit;
	int i;

	start_bit = ddr_device->ddr_bits[0];
	end_bit = ddr_device->ddr_bits[1];
	mask = GENMASK(end_bit, start_bit);
	val = readl_relaxed(ddr_device->vddr_reg);
	val = (val & mask) >> start_bit;

	for (i = 0; i < ddr_device->ddr_status; i++) {
		if (val == ddr_device->ddr_data[i])
			break;
	}

	if (i == ddr_device->ddr_status) {
		dev_warn(&ddr_device->cool_dev->device,
				"Not matching any states!\n");
		*state = -EINVAL;
	} else {
		*state = i;
	}

	return 0;
}

static int ddr_set_cur_state(struct thermal_cooling_device *cdev,
			     unsigned long state)
{
	struct ddr_cooling_device *ddr_device = cdev->devdata;
	u32 val, mask;
	u32 start_bit, end_bit;

	if (!(state < ddr_device->ddr_status)) {
		dev_warn(&ddr_device->cool_dev->device,
				"Not matching any states!\n");
		return -EINVAL;
	}

	start_bit = ddr_device->ddr_bits[0];
	end_bit = ddr_device->ddr_bits[1];
	mask = GENMASK(end_bit, start_bit);
	val = readl_relaxed(ddr_device->vddr_reg);
	val = (val & (~mask)) | (ddr_device->ddr_data[state] << start_bit);

	writel_relaxed(val, ddr_device->vddr_reg);

	return 0;
}

static unsigned long cdev_calc_next_state_by_temp(struct thermal_instance *instance,
	int temperature)
{
	struct thermal_instance *ins = instance;
	struct thermal_zone_device *tz;
	struct thermal_cooling_device *cdev;
	struct ddr_cooling_device *ddr_device;
	int i, hyst = 0, trip_temp, max;

	if (!ins)
		return -EINVAL;

	tz = ins->tz;
	cdev = ins->cdev;

	if (!tz || !cdev)
		return -EINVAL;

	ddr_device = cdev->devdata;
	max = ddr_device->ddr_status - 1;

	tz->ops->get_trip_hyst(tz, instance->trip, &hyst);
	tz->ops->get_trip_temp(tz, instance->trip, &trip_temp);

	for (i = 0; i < ddr_device->ddr_status; i++) {
		if (temperature < (trip_temp + (i + 1) * hyst))
			break;
	}

	return i > max ? max : i;
}

static int ddr_get_requested_power(struct thermal_cooling_device *cdev,
				   u32 *power)
{
	struct thermal_instance *instance;
	struct thermal_zone_device *tz;

	mutex_lock(&cdev->lock);
	list_for_each_entry(instance, &cdev->thermal_instances, cdev_node) {
		tz = instance->tz;
		if (cdev->ops && cdev->ops->set_cur_state && instance->trip == THERMAL_TRIP_HOT)
			*power = (u32)cdev_calc_next_state_by_temp(instance, tz->temperature);
	}
	mutex_unlock(&cdev->lock);

	return 0;
}

static int ddr_state2power(struct thermal_cooling_device *cdev,
			   unsigned long state, u32 *power)
{
	*power = 0;
	return 0;
}

static int ddr_power2state(struct thermal_cooling_device *cdev,
			   u32 power, unsigned long *state)
{
	cdev->ops->get_cur_state(cdev, state);
	return 0;
}

static struct thermal_cooling_device_ops const ddr_cooling_ops = {
	.get_max_state = ddr_get_max_state,
	.get_cur_state = ddr_get_cur_state,
	.set_cur_state = ddr_set_cur_state,
	.state2power   = ddr_state2power,
	.power2state   = ddr_power2state,
	.get_requested_power = ddr_get_requested_power,
};

struct thermal_cooling_device *
ddr_cooling_register(struct device_node *np, struct cool_dev *cool)
{
	struct thermal_cooling_device *cool_dev;
	struct ddr_cooling_device *ddr_dev = NULL;
	struct thermal_instance *pos = NULL;
	char dev_name[THERMAL_NAME_LENGTH];
	int i;

	ddr_dev = kmalloc(sizeof(*ddr_dev), GFP_KERNEL);
	if (!ddr_dev)
		return ERR_PTR(-ENOMEM);

	ddr_coolingdevice_id_get(&ddr_dev->id);

	ddr_dev->vddr_reg = ioremap(cool->ddr_reg, 4);
	if (!ddr_dev->vddr_reg) {
		pr_err("thermal ddr cdev: ddr reg0 ioremap fail.\n");
		ddr_coolingdevice_id_put();
		kfree(ddr_dev);
		return ERR_PTR(-EINVAL);
	}

	snprintf(dev_name, sizeof(dev_name), "thermal-ddr-%d",
		 ddr_dev->id);

	ddr_dev->ddr_reg = cool->ddr_reg;
	ddr_dev->ddr_status = cool->ddr_status;
	for (i = 0; i < 2; i++)
		ddr_dev->ddr_bits[i] = cool->ddr_bits[i];
	ddr_dev->ddr_bits_keep = ~(0xffffffff << (ddr_dev->ddr_bits[1] + 1));
	ddr_dev->ddr_bits_keep = ~((ddr_dev->ddr_bits_keep >> ddr_dev->ddr_bits[0])
				   << ddr_dev->ddr_bits[0]);
	for (i = 0; i < cool->ddr_status; i++)
		ddr_dev->ddr_data[i] = cool->ddr_data[i];

	cool_dev = thermal_of_cooling_device_register(np, dev_name, ddr_dev,
						      &ddr_cooling_ops);

	if (!cool_dev) {
		ddr_coolingdevice_id_put();
		kfree(ddr_dev);
		return ERR_PTR(-EINVAL);
	}
	ddr_dev->cool_dev = cool_dev;

	list_for_each_entry(pos, &cool_dev->thermal_instances, cdev_node) {
		if (!strncmp(pos->cdev->type, dev_name, sizeof(dev_name))) {
			pr_err("Notice!!! The notify interface has been removed.\n");
			break;
		}
	}

	mutex_lock(&cooling_list_lock);
	list_add(&ddr_dev->node, &ddr_dev_list);
	mutex_unlock(&cooling_list_lock);

	return cool_dev;
}
EXPORT_SYMBOL_GPL(ddr_cooling_register);

/**
 * cpucore_cooling_unregister - function to remove cpucore cooling device.
 * @cdev: thermal cooling device pointer.
 *
 * This interface function unregisters the "thermal-cpucore-%x" cooling device.
 */
void ddr_cooling_unregister(struct thermal_cooling_device *cdev)
{
	struct ddr_cooling_device *ddr_dev;

	if (!cdev)
		return;

	ddr_dev = cdev->devdata;
	iounmap(ddr_dev->vddr_reg);

	mutex_lock(&cooling_list_lock);
	list_del(&ddr_dev->node);
	mutex_unlock(&cooling_list_lock);

	thermal_cooling_device_unregister(ddr_dev->cool_dev);
	ddr_coolingdevice_id_put();
	kfree(ddr_dev);
}
EXPORT_SYMBOL_GPL(ddr_cooling_unregister);
