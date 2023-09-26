// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/thermal.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/amlogic/media_cooling.h>
#include <linux/amlogic/meson_cooldev.h>
#include <linux/io.h>
#include "thermal_core.h"

struct device_node *media_np;

int setup_media_para(const char *node_name)
{
	media_np = of_find_node_by_name(NULL, node_name);
	if (!media_np) {
		pr_err("thermal: can't find node %s\n", node_name);
		return -EINVAL;
	}

	return 0;
}

static int media_get_max_state(struct thermal_cooling_device *cdev,
				unsigned long *max)
{
	struct media_cooling_device *media_dev = cdev->devdata;

	*max = media_dev->maxstep ? media_dev->maxstep : 0;

	return 0;
}

static int media_get_cur_state(struct thermal_cooling_device *cdev,
			     unsigned long *state)
{
	struct media_cooling_device *media_dev = cdev->devdata;
	*state = media_dev->setstep;
	return 0;
}

static int media_set_cur_state(struct thermal_cooling_device *cdev,
			     unsigned long state)
{
	struct media_cooling_device *media_dev = cdev->devdata;

	media_dev->set_media_cooling_state(state);

	return 0;
}

static unsigned long calculate_hotstep(struct thermal_instance *instance)
{
	struct thermal_zone_device *tz;
	struct thermal_cooling_device *cdev;
	struct media_cooling_device *media_dev;
	int hyst = 0, trip_temp;

	if (!instance)
		return -EINVAL;

	tz = instance->tz;
	cdev = instance->cdev;

	if (!tz || !cdev)
		return -EINVAL;

	media_dev = cdev->devdata;

	tz->ops->get_trip_hyst(tz, instance->trip, &hyst);
	tz->ops->get_trip_temp(tz, instance->trip, &trip_temp);

	if (tz->temperature >= (trip_temp + (media_dev->hotstep + 1) * hyst)) {
		media_dev->hotstep++;
		pr_debug("[%s]temp:%d increase,trip:%d,hotstep:%d\n", cdev->type, tz->temperature,
			trip_temp, media_dev->hotstep);
	}
	if (tz->temperature < (trip_temp + media_dev->hotstep * hyst) && media_dev->hotstep) {
		media_dev->hotstep--;
		pr_debug("[%s]temp:%d decrease,trip:%d,hotstep:%d\n", cdev->type, tz->temperature,
			trip_temp, media_dev->hotstep);
	}

	return media_dev->hotstep;
}

/*
 *return the cooling device hotstep, witch is constrained by instance->upper
 *and media_cdev->maxstep.
 */
static int media_get_requested_power(struct thermal_cooling_device *cdev,
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

static const struct thermal_cooling_device_ops media_cooling_ops = {
	.get_max_state = media_get_max_state,
	.get_cur_state = media_get_cur_state,
	.set_cur_state = media_set_cur_state,
	.get_requested_power = media_get_requested_power,
};

struct thermal_cooling_device *
media_cooling_register(struct media_cooling_device *mcdev)
{
	struct thermal_cooling_device *cool_dev;

	mcdev->hotstep = 0;
	mcdev->setstep = 0;

	cool_dev = thermal_of_cooling_device_register(media_np, "thermal-media", mcdev,
						      &media_cooling_ops);
	if (!cool_dev || !mcdev->set_media_cooling_state)
		goto out;

	mcdev->cool_dev = cool_dev;

	return cool_dev;
out:
	kfree(mcdev);
	return ERR_PTR(-EINVAL);
}
EXPORT_SYMBOL_GPL(media_cooling_register);

void media_cooling_unregister(struct thermal_cooling_device *cdev)
{
	struct media_cooling_device *media_dev;

	if (!cdev)
		return;

	media_dev = cdev->devdata;

	thermal_cooling_device_unregister(media_dev->cool_dev);
	kfree(media_dev);
}
EXPORT_SYMBOL_GPL(media_cooling_unregister);
