/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __CPUCORE_COOLING_H__
#define __CPUCORE_COOLING_H__

#include <linux/thermal.h>
#include <linux/cpumask.h>

enum hotplug_mode {
	CPU_PLUG,
	CPU_UNPLUG,
	CPU_MODE_MAX
};

/*cpunum: cpu number which coolingdevice can plug&unplug most.
 *cluster_num: clusters which coolingdevice can work on.
 *cluster_core_num: number of cores which coolingdevice can plug&unplug.
 *online: cpumask of coolingdevice can unplug, set when plug, clear when unplug.
 *offline: cpumask of coolingdevice can plug back, set when unplug, clear when plug.
 *hotstep: cpu number which need to offline for current temperature:0-->trip~trip+hyst,
 *1-->trip+hyst~trip+2*hyst,2-->trip+2*hyst~trip+3*hyst......
 *setstep: step last set.
 */
struct cpucore_cooling_device {
	int cpunum;
	int cluster_num;
	int *cluster_core_num;
	cpumask_var_t *online;
	cpumask_var_t *offline;
	int mode; /*plug or unplug*/
	struct thermal_cooling_device *cool_dev;
	int hotstep;
	int setstep;
};

struct thermal_cooling_device *cpucore_cooling_register(struct device_node *np,
				struct device_node *child);

void cpucore_cooling_unregister(struct thermal_cooling_device *cdev);
#endif /* __CPU_COOLING_H__ */
