// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
//#define DEBUG
#include <linux/module.h>
#include <linux/thermal.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include "cpucore_cooling.h"
#include <linux/cpumask.h>
#include <linux/amlogic/meson_cooldev.h>
#include "thermal_core.h"

/**
 * cpucore_get_max_state - callback function to get the max cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: fill this variable with the max cooling state.
 *
 * Callback for the thermal cooling device to return the cpucore
 * max cooling state.
 *
 * Return: 0 on success, an error code otherwise.
 */
static int cpucore_get_max_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	struct cpucore_cooling_device *cpucore_dev = cdev->devdata;
	*state = cpucore_dev->cpunum;
	pr_debug("max cpu core=%ld\n", *state);
	return 0;
}

/**
 * cpucore_get_cur_state - callback function to get the current cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: fill this variable with the current cooling state.
 *
 * Callback for the thermal cooling device to return the cpucore
 * current cooling state.
 *
 * Return: 0 on success, an error code otherwise.
 */
static int cpucore_get_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	struct cpucore_cooling_device *cpucore_dev = cdev->devdata;
	*state = cpucore_dev->setstep;
	pr_debug("current state=%ld\n", *state);
	return 0;
}

/**
 * cpucore_set_cur_state - callback function to set the current cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: set this variable to the current cooling state.
 *
 * Callback for the thermal cooling device to change the cpucore
 * current cooling state.
 *
 * Return: 0 on success, an error code otherwise.
 */
static int cpucore_set_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long state)
{
	struct cpucore_cooling_device *cpucore_dev = cdev->devdata;
	int i, cpu;

	if (WARN_ON(state > cpucore_dev->cpunum))
		return -EINVAL;

	switch (cpucore_dev->mode) {
	case CPU_PLUG:
		for (i = 0; i < cpucore_dev->cluster_num; i++) {
			if (!cpumask_weight(cpucore_dev->offline[i]))
				continue;
			cpu = cpumask_last(cpucore_dev->offline[i]);
			pr_debug("[%s]online cpu%d\n", cdev->type, cpu);
			add_cpu(cpu);
			cpumask_clear_cpu(cpu, cpucore_dev->offline[i]);
			cpumask_set_cpu(cpu, cpucore_dev->online[i]);
			break;
		}
		break;
	case CPU_UNPLUG:
		for (i = 0; i < cpucore_dev->cluster_num; i++) {
			if (!cpumask_weight(cpucore_dev->online[i]))
				continue;
			cpu = cpumask_any_and(cpucore_dev->online[i], cpu_online_mask);
			if (cpu == nr_cpu_ids)
				continue;
			pr_debug("[%s]offline cpu%d\n", cdev->type, cpu);
			remove_cpu(cpu);
			cpumask_set_cpu(cpu, cpucore_dev->offline[i]);
			cpumask_clear_cpu(cpu, cpucore_dev->online[i]);
			break;
		}
		break;
	default:
		break;
	}

	return 0;
}

static int calculate_hotstep(struct thermal_instance *instance)
{
	struct thermal_zone_device *tz;
	struct thermal_cooling_device *cdev;
	struct cpucore_cooling_device *cpucore_dev;
	int hyst = 0, trip_temp;

	if (!instance)
		return -EINVAL;

	tz = instance->tz;
	cdev = instance->cdev;

	if (!tz || !cdev)
		return -EINVAL;

	cpucore_dev = cdev->devdata;

	tz->ops->get_trip_hyst(tz, instance->trip, &hyst);
	tz->ops->get_trip_temp(tz, instance->trip, &trip_temp);

	if (tz->temperature >= (trip_temp + (cpucore_dev->hotstep + 1) * hyst)) {
		cpucore_dev->hotstep++;
		cpucore_dev->mode = CPU_UNPLUG;
		pr_debug("[%s]temp:%d increase,trip:%d,hotstep:%d\n", cdev->type, tz->temperature,
			trip_temp, cpucore_dev->hotstep);
	}
	if (tz->temperature < (trip_temp + cpucore_dev->hotstep * hyst) && cpucore_dev->hotstep) {
		cpucore_dev->hotstep--;
		cpucore_dev->mode = CPU_PLUG;
		pr_debug("[%s]temp:%d decrease,trip:%d,hotstep:%d\n", cdev->type, tz->temperature,
			trip_temp, cpucore_dev->hotstep);
	}

	return cpucore_dev->hotstep;
}

/*
 * return the cooling device hotstep, witch is constrained by instance->upper.
 */
static int cpucore_get_requested_power(struct thermal_cooling_device *cdev,
				       u32 *power)
{
	struct thermal_instance *instance;

	mutex_lock(&cdev->lock);
	list_for_each_entry(instance, &cdev->thermal_instances, cdev_node) {
		if (!cdev->ops || !cdev->ops->set_cur_state)
			continue;
		*power = (u32)calculate_hotstep(instance);
		if (*power > instance->upper)
			*power = instance->upper;
	}
	mutex_unlock(&cdev->lock);

	return 0;
}

static int cpucore_state2power(struct thermal_cooling_device *cdev,
			       unsigned long state, u32 *power)
{
	*power = 0;

	return 0;
}

static int cpucore_power2state(struct thermal_cooling_device *cdev,
			       u32 power, unsigned long *state)
{
	cdev->ops->get_cur_state(cdev, state);
	return 0;
}

/* Bind cpucore callbacks to thermal cooling device ops */
static struct thermal_cooling_device_ops const cpucore_cooling_ops = {
	.get_max_state = cpucore_get_max_state,
	.get_cur_state = cpucore_get_cur_state,
	.set_cur_state = cpucore_set_cur_state,
	.state2power   = cpucore_state2power,
	.power2state   = cpucore_power2state,
	.get_requested_power = cpucore_get_requested_power,
};

static int setup_cooling_params(struct cpucore_cooling_device *cdev,
	struct device_node *child)
{
	int i, j, cpu, offset;

	cdev->cpunum = 0;
	cdev->hotstep = 0;
	cdev->setstep = 0;
	cdev->mode = CPU_MODE_MAX;
	cdev->cluster_num = of_property_count_u32_elems(child, "cluster_core_num");
	if (cdev->cluster_num < 1)
		return -EINVAL;
	cdev->cluster_core_num = kcalloc(cdev->cluster_num, sizeof(u32), GFP_KERNEL);
	if (!cdev->cluster_core_num)
		return -ENOMEM;
	cdev->online = kcalloc(cdev->cluster_num, sizeof(cpumask_var_t), GFP_KERNEL);
	if (!cdev->online)
		return -ENOMEM;
	cdev->offline = kcalloc(cdev->cluster_num, sizeof(cpumask_var_t), GFP_KERNEL);
	if (!cdev->offline)
		return -ENOMEM;
	if (of_property_read_u32_array(child, "cluster_core_num",
		cdev->cluster_core_num, cdev->cluster_num))
		return -EINVAL;

	for (i = 0; i < cdev->cluster_num; i++) {
		offset = i == 0 ? 0 : cdev->cluster_core_num[i - 1];
		for (j = 0; j < cdev->cluster_core_num[i]; j++) {
			of_property_read_u32_index(child, "cluster_cores", j + offset,
				&cpu);
			cpumask_set_cpu(cpu, cdev->online[i]);
			cdev->cpunum++;
		}
		pr_debug("cluster%d[%d %*pbl]\n", i, cdev->cluster_core_num[i],
			cpumask_pr_args(cdev->online[i]));
	}

	return 0;
}

/**
 * cpucore_cooling_register - function to create cpucore cooling device.
 *
 * This interface function registers the cpucore cooling device with the name
 * "thermal-cpucore-%x". This api can support multiple instances of cpucore
 * cooling devices.
 *
 * Return: a valid struct thermal_cooling_device pointer on success,
 * on failure, it returns a corresponding ERR_PTR().
 */
struct thermal_cooling_device *
cpucore_cooling_register(struct device_node *np,
	struct device_node *child)
{
	struct thermal_cooling_device *cool_dev;
	struct cpucore_cooling_device *cpucore_cdev;

	cpucore_cdev = kzalloc(sizeof(*cpucore_cdev), GFP_KERNEL);
	if (!cpucore_cdev)
		return ERR_PTR(-EINVAL);

	if (setup_cooling_params(cpucore_cdev, child))
		goto free;

	cool_dev = thermal_of_cooling_device_register(np, "thermal-cpucore-0", cpucore_cdev,
						      &cpucore_cooling_ops);
	if (!cool_dev) {
		pr_err("cpucore cooling device register fail\n");
		goto free;
	}
	cpucore_cdev->cool_dev = cool_dev;

	return cool_dev;
free:
	kfree(cpucore_cdev->cluster_core_num);
	kfree(cpucore_cdev->online);
	kfree(cpucore_cdev->offline);
	kfree(cpucore_cdev);
	return ERR_PTR(-EINVAL);
}
EXPORT_SYMBOL_GPL(cpucore_cooling_register);

/**
 * cpucore_cooling_unregister - function to remove cpucore cooling device.
 * @cdev: thermal cooling device pointer.
 *
 * This interface function unregisters the "thermal-cpucore-%x" cooling device.
 */
void cpucore_cooling_unregister(struct thermal_cooling_device *cdev)
{
	struct cpucore_cooling_device *cpucore_dev;

	if (!cdev)
		return;

	cpucore_dev = cdev->devdata;

	thermal_cooling_device_unregister(cpucore_dev->cool_dev);
	kfree(cpucore_dev);
}
EXPORT_SYMBOL_GPL(cpucore_cooling_unregister);
