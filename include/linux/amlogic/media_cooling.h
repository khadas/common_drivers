/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __MEDIA_COOLING_H__
#define __MEDIA_COOLING_H__

#include <linux/thermal.h>

/*hotstep: 0-->trip~trip+hyst,
 *1-->trip+hyst~trip+2*hyst,2-->trip+2*hyst~trip+3*hyst.....
 *setstep: step last set
 */
struct media_cooling_device {
	struct thermal_cooling_device *cool_dev;
	int (*set_media_cooling_state)(int state);
	u32 hotstep; /*0:no limit;1:low level limit;2:high level limit*/
	u32 setstep;
	int maxstep; /*the max hotstep which can be setted by the cooling device*/
	int bindtrip; /*trip id of the cooling device*/
};

struct media_cooling_para {
	struct device_node *np;
	int maxstep;
	int bindtrip;
};

#ifdef CONFIG_AMLOGIC_MEDIA_THERMAL
struct thermal_cooling_device *media_cooling_register(struct media_cooling_device *mcdev);
void media_cooling_unregister(struct thermal_cooling_device *cdev);
int setup_media_para(const char *node_name);
#else
static inline struct thermal_cooling_device *
media_cooling_register(struct media_cooling_device *mcdev)
{
	return NULL;
}

static inline
void media_cooling_unregister(struct thermal_cooling_device *cdev)
{
}

static inline int setup_media_para(const char *node_name)
{
	return 0;
}
#endif
#endif /* __MEDIA_COOLING_H__ */
