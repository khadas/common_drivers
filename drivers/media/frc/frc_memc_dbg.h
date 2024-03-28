/* SPDX-License-Identifier: (GPL-2.0+ OR MIT)
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef __FRC_MEMC_DBG_H__
#define __FRC_MEMC_DBG_H__

ssize_t frc_bbd_ctrl_param_show(struct class *class,
				struct class_attribute *attr, char *buf);
ssize_t frc_bbd_ctrl_param_store(struct class *class,
				struct class_attribute *attr, const char *buf, size_t count);
ssize_t frc_vp_ctrl_param_show(struct class *class, struct class_attribute *attr, char *buf);
ssize_t frc_vp_ctrl_param_store(struct class *class, struct class_attribute *attr,
					const char *buf, size_t count);
ssize_t frc_logo_ctrl_param_show(struct class *class, struct class_attribute *attr, char *buf);
ssize_t frc_logo_ctrl_param_store(struct class *class, struct class_attribute *attr,
					const char *buf, size_t count);
ssize_t frc_iplogo_ctrl_param_show(struct class *class, struct class_attribute *attr, char *buf);
ssize_t frc_iplogo_ctrl_param_store(struct class *class, struct class_attribute *attr,
					const char *buf, size_t count);

ssize_t frc_melogo_ctrl_param_show(struct class *class, struct class_attribute *attr, char *buf);
ssize_t frc_melogo_ctrl_param_store(struct class *class, struct class_attribute *attr,
					const char *buf, size_t count);

ssize_t frc_scene_chg_detect_param_show(struct class *class,
	struct class_attribute *attr, char *buf);
ssize_t frc_scene_chg_detect_param_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t count);

ssize_t frc_fb_ctrl_param_show(struct class *class, struct class_attribute *attr, char *buf);
ssize_t frc_fb_ctrl_param_store(struct class *class, struct class_attribute *attr,
					const char *buf, size_t count);

ssize_t frc_me_ctrl_param_show(struct class *class, struct class_attribute *attr, char *buf);
ssize_t frc_me_ctrl_param_store(struct class *class, struct class_attribute *attr,
					const char *buf, size_t count);

ssize_t frc_search_rang_param_show(struct class *class, struct class_attribute *attr, char *buf);
ssize_t frc_search_rang_param_store(struct class *class, struct class_attribute *attr,
					const char *buf, size_t count);

ssize_t frc_mc_ctrl_param_show(struct class *class, struct class_attribute *attr, char *buf);
ssize_t frc_mc_ctrl_param_store(struct class *class, struct class_attribute *attr,
					const char *buf, size_t count);

ssize_t frc_me_rule_param_show(struct class *class, struct class_attribute *attr, char *buf);
ssize_t frc_me_rule_param_store(struct class *class, struct class_attribute *attr,
					const char *buf, size_t count);

ssize_t frc_film_ctrl_param_show(struct class *class, struct class_attribute *attr, char *buf);
ssize_t frc_film_ctrl_param_store(struct class *class, struct class_attribute *attr,
					const char *buf, size_t count);

ssize_t frc_glb_ctrl_param_show(struct class *class, struct class_attribute *attr, char *buf);
ssize_t frc_glb_ctrl_param_store(struct class *class, struct class_attribute *attr,
					const char *buf, size_t count);

ssize_t frc_bad_edit_ctrl_param_show(struct class *class, struct class_attribute *attr, char *buf);
ssize_t frc_bad_edit_ctrl_param_store(struct class *class, struct class_attribute *attr,
					const char *buf, size_t count);

ssize_t frc_region_fb_ctrl_param_show(struct class *class, struct class_attribute *attr, char *buf);
ssize_t frc_region_fb_ctrl_param_store(struct class *class, struct class_attribute *attr,
					const char *buf, size_t count);

#endif

