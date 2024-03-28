// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * drivers/amlogic/media/video_processor/common/video_pp_common.c
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

#include <linux/types.h>
#include <linux/amlogic/media/ge2d/ge2d.h>
#include <linux/amlogic/cpu_version.h>
#include <linux/amlogic/media/vfm/vframe.h>
#include "video_pp_common.h"
#ifdef CONFIG_AMLOGIC_VIDEO_COMPOSER
#include "../video_composer/video_composer.h"
#endif
#ifdef CONFIG_AMLOGIC_VIDEOQUEUE
#include "../videoqueue/videoqueue.h"
#endif

#define MODULE_VC    0
#define MODULE_VQ    1
#define VC_TITLE    "Display_VC"
#define VQ_TITLE    "Display_VQ"
#define DEFAULT_TITLE    "default"
#define DEBUG_LEVEL    "debuglevel"
#define VC_DEBUG_NODES    4
#define VQ_DEBUG_NODES    3
#define CMD_LEN    32
#define MODULE_LEN    128

#ifdef CONFIG_AMLOGIC_VIDEO_COMPOSER
static int vc_print_array[3] = {0, 34, 255};
static struct module_debug_node vc_debug_nodes[] = {
	{
		.name = "print_flag",
		.set_debug_flag_notify = debug_vc_print_flag,
	},
	{
		.name = "transform",
		.set_debug_flag_notify = debug_vc_transform,
	},
	{
		.name = "force_composer",
		.set_debug_flag_notify = debug_vc_force_composer,
	},
	{
		.name = "total_get_count",
		.set_debug_flag_notify = debug_vc_get_count,
	},
};
#endif

#ifdef CONFIG_AMLOGIC_VIDEOQUEUE
static int vq_print_array[3] = {0, 19, 31};
static struct module_debug_node vq_debug_nodes[] = {
	{
		.name = "print_flag",
		.set_debug_flag_notify = debug_vq_print_flag,
	},
	{
		.name = "game_mode",
		.set_debug_flag_notify = debug_vq_game_mode,
	},
	{
		.name = "vframe_get_delay",
		.set_debug_flag_notify = debug_vq_vframe_delay,
	},
};
#endif

int get_source_type(struct vframe_s *vf)
{
	enum videocom_source_type ret;
	int interlace_mode;

	interlace_mode = vf->type & VIDTYPE_TYPEMASK;
	if (vf->source_type == VFRAME_SOURCE_TYPE_HDMI ||
	    vf->source_type == VFRAME_SOURCE_TYPE_CVBS) {
		if ((vf->bitdepth & BITDEPTH_Y10) &&
		    (!(vf->type & VIDTYPE_COMPRESS)) &&
		    (get_cpu_type() >= MESON_CPU_MAJOR_ID_TXL))
			ret = VDIN_10BIT_NORMAL;
		else
			ret = VDIN_8BIT_NORMAL;
	} else {
		if ((vf->bitdepth & BITDEPTH_Y10) &&
		    (!(vf->type & VIDTYPE_COMPRESS)) &&
		    (get_cpu_type() >= MESON_CPU_MAJOR_ID_TXL)) {
			if (interlace_mode == VIDTYPE_INTERLACE_TOP)
				ret = DECODER_10BIT_TOP;
			else if (interlace_mode == VIDTYPE_INTERLACE_BOTTOM)
				ret = DECODER_10BIT_BOTTOM;
			else
				ret = DECODER_10BIT_NORMAL;
		} else {
			if (interlace_mode == VIDTYPE_INTERLACE_TOP)
				ret = DECODER_8BIT_TOP;
			else if (interlace_mode == VIDTYPE_INTERLACE_BOTTOM)
				ret = DECODER_8BIT_BOTTOM;
			else
				ret = DECODER_8BIT_NORMAL;
		}
	}
	return ret;
}

int get_ge2d_input_format(struct vframe_s *vf)
{
	int format = GE2D_FORMAT_M24_YUV420;
	enum videocom_source_type soure_type;

	soure_type = get_source_type(vf);
	switch (soure_type) {
	case DECODER_8BIT_NORMAL:
		if (vf->type & VIDTYPE_VIU_422)
			format = GE2D_FORMAT_S16_YUV422;
		else if (vf->type & VIDTYPE_VIU_NV21)
			format = GE2D_FORMAT_M24_NV21;
		else if (vf->type & VIDTYPE_VIU_444)
			format = GE2D_FORMAT_S24_YUV444;
		else
			format = GE2D_FORMAT_M24_YUV420;
		break;
	case DECODER_8BIT_BOTTOM:
		if (vf->type & VIDTYPE_VIU_422)
			format = GE2D_FORMAT_S16_YUV422
				| (GE2D_FORMAT_S16_YUV422B & (3 << 3));
		else if (vf->type & VIDTYPE_VIU_NV21)
			format = GE2D_FORMAT_M24_NV21
				| (GE2D_FORMAT_M24_NV21B & (3 << 3));
		else if (vf->type & VIDTYPE_VIU_444)
			format = GE2D_FORMAT_S24_YUV444
				| (GE2D_FORMAT_S24_YUV444B & (3 << 3));
		else
			format = GE2D_FORMAT_M24_YUV420
				| (GE2D_FMT_M24_YUV420B & (3 << 3));
		break;
	case DECODER_8BIT_TOP:
		if (vf->type & VIDTYPE_VIU_422)
			format = GE2D_FORMAT_S16_YUV422
				| (GE2D_FORMAT_S16_YUV422T & (3 << 3));
		else if (vf->type & VIDTYPE_VIU_NV21)
			format = GE2D_FORMAT_M24_NV21
				| (GE2D_FORMAT_M24_NV21T & (3 << 3));
		else if (vf->type & VIDTYPE_VIU_444)
			format = GE2D_FORMAT_S24_YUV444
				| (GE2D_FORMAT_S24_YUV444T & (3 << 3));
		else
			format = GE2D_FORMAT_M24_YUV420
				| (GE2D_FMT_M24_YUV420T & (3 << 3));
		break;
	case DECODER_10BIT_NORMAL:
		if (vf->type & VIDTYPE_VIU_422) {
			if (vf->bitdepth & FULL_PACK_422_MODE)
				format = GE2D_FORMAT_S16_10BIT_YUV422;
			else
				format = GE2D_FORMAT_S16_12BIT_YUV422;
		}
		break;
	case DECODER_10BIT_BOTTOM:
		if (vf->type & VIDTYPE_VIU_422) {
			if (vf->bitdepth & FULL_PACK_422_MODE)
				format = GE2D_FORMAT_S16_10BIT_YUV422
					| (GE2D_FORMAT_S16_10BIT_YUV422B
					& (3 << 3));
			else
				format = GE2D_FORMAT_S16_12BIT_YUV422
					| (GE2D_FORMAT_S16_12BIT_YUV422B
					& (3 << 3));
		}
		break;
	case DECODER_10BIT_TOP:
		if (vf->type & VIDTYPE_VIU_422) {
			if (vf->bitdepth & FULL_PACK_422_MODE)
				format = GE2D_FORMAT_S16_10BIT_YUV422
					| (GE2D_FORMAT_S16_10BIT_YUV422T
					& (3 << 3));
			else
				format = GE2D_FORMAT_S16_12BIT_YUV422
					| (GE2D_FORMAT_S16_12BIT_YUV422T
					& (3 << 3));
		}
		break;
	case VDIN_8BIT_NORMAL:
		if (vf->type & VIDTYPE_VIU_422)
			format = GE2D_FORMAT_S16_YUV422;
		else if (vf->type & VIDTYPE_VIU_NV21)
			format = GE2D_FORMAT_M24_NV21;
		else if (vf->type & VIDTYPE_VIU_444)
			format = GE2D_FORMAT_S24_YUV444;
		else
			format = GE2D_FORMAT_M24_YUV420;
		break;
	case VDIN_10BIT_NORMAL:
		if (vf->type & VIDTYPE_VIU_422) {
			if (vf->bitdepth & FULL_PACK_422_MODE)
				format = GE2D_FORMAT_S16_10BIT_YUV422;
			else
				format = GE2D_FORMAT_S16_12BIT_YUV422;
		}
		break;
	default:
		format = GE2D_FORMAT_M24_YUV420;
	}
	return format;
}
EXPORT_SYMBOL(get_ge2d_input_format);

static int get_module_config(const char *configs, const char *name, char *cmd)
{
	char *module_str;
	char *str_end;
	int cmd_len;

	if (!configs || !name)
		return -1;

	module_str = strstr(configs, name);

	if (module_str) {
		if (module_str > configs && module_str[-1] != ';')
			return -1;

		module_str += strlen(name);
		if (module_str[0] != ':' ||  module_str[1] == '\0')
			return -1;

		module_str += 1;
		str_end = strchr(module_str, ';');
		if (str_end) {
			strncpy(cmd, module_str, str_end - module_str);
			cmd_len = str_end - module_str;
			if (cmd_len > MODULE_LEN - 1) {
				pr_info("%s: module_len is too long\n", name);
				return -1;
			}

			cmd[str_end - module_str] = '\0';
		} else {
			return -1;
		}

	} else {
		return -1;
	}
	return 0;
}

static int set_pipeline_debug_flag(char *module_str, char *cmd_str, int type)
{
	char *node_str;
	char *str_end;
	int ret = 0;
	int node;
	int node_value;
	int cmd_len;
	int nodes_num = 0;
	int module_enable_flag = 0;
	struct module_debug_node *debug_nodes;

#ifdef CONFIG_AMLOGIC_VIDEO_COMPOSER
	if (type == MODULE_VC) {
		module_enable_flag = 1;
		debug_nodes = vc_debug_nodes;
		nodes_num = VC_DEBUG_NODES;
	}
#endif
#ifdef CONFIG_AMLOGIC_VIDEOQUEUE
	if (type == MODULE_VQ) {
		module_enable_flag = 2;
		debug_nodes = vq_debug_nodes;
		nodes_num = VQ_DEBUG_NODES;
	}
#endif
	if (module_enable_flag == 0)
		return -1;

	for (node = 0; node < nodes_num; node++) {
		node_str = strstr(module_str, debug_nodes[node].name);
		if (!node_str)
			continue;

		if (node_str > module_str && node_str[-1] != ',')
			return -1;

		node_str += strlen(debug_nodes[node].name);
		if (node_str[0] != ':' || node_str[1] == '\0')
			return -1;

		node_str += 1;
		str_end = strstr(node_str, ",");
		if (str_end)
			cmd_len = str_end - node_str;
		else
			cmd_len = strlen(node_str);
		if (cmd_len > CMD_LEN - 1) {
			pr_info("cmd len is too long\n");
			return -1;
		}

		strncpy(cmd_str, node_str, cmd_len);
		cmd_str[cmd_len] = '\0';
		ret = kstrtoint(cmd_str, 0, &node_value);
		if (ret != 0) {
			pr_info("%s: failed to parse value\n", __func__);
			return -1;
		}
		debug_nodes[node].set_debug_flag_notify(debug_nodes[node].name, node_value);
	}
	return 0;
}

static void set_module_print_flag(int type, int loglevel)
{
	int print_level = 0;
	int *print_level_array;
	int module_enable_flag = 0;

#ifdef CONFIG_AMLOGIC_VIDEO_COMPOSER
	if (type == MODULE_VC) {
		print_level_array = vc_print_array;
		module_enable_flag = 1;
	}
#endif
#ifdef CONFIG_AMLOGIC_VIDEOQUEUE
	if (type == MODULE_VQ) {
		print_level_array = vq_print_array;
		module_enable_flag = 2;
	}
#endif
	if (module_enable_flag == 0)
		return;

	switch (loglevel) {
	case 0:
		print_level = print_level_array[0];
		break;
	case 1:
	case 2:
	case 3:
	case 4:
		print_level = print_level_array[1];
		break;
	default:
		print_level = print_level_array[2];
		break;
	}

#ifdef CONFIG_AMLOGIC_VIDEO_COMPOSER
	if (type == MODULE_VC)
		debug_vc_print_flag(NULL, print_level);
#endif
#ifdef CONFIG_AMLOGIC_VIDEOQUEUE
	if (type == MODULE_VQ)
		debug_vq_print_flag(NULL, print_level);
#endif
}

static int set_default_debug_flag(char *default_str, char *cmd_str, int type)
{
	char *node_str;
	char *str_end;
	int loglevel = 0;
	int ret = 0;
	int cmd_len;

	node_str = strstr(default_str, DEBUG_LEVEL);
	if (!node_str)
		return -1;

	if (node_str > default_str && node_str[-1] != ',')
		return -1;

	node_str += strlen(DEBUG_LEVEL);
	if (node_str[0] != ':' || node_str[1] == '\0')
		return -1;

	node_str += 1;
	str_end = strstr(node_str, ",");
	if (str_end)
		cmd_len = str_end - node_str + 1;
	else
		cmd_len = strlen(node_str);
	if (cmd_len > CMD_LEN - 1) {
		pr_info("cmd len is too long\n");
		return -1;
	}

	strncpy(cmd_str, node_str, cmd_len);
	cmd_str[cmd_len] = '\0';
	ret = kstrtoint(cmd_str, 10, &loglevel);
	if (ret != 0) {
		pr_info("%s:get loglevel failed\n", __func__);
		return -1;
	}
	set_module_print_flag(type, loglevel);
	return 0;
}

void set_vc_config(const char *module, const char *debug, int len)
{
	char *default_str = kzalloc(sizeof(char) * 128, GFP_KERNEL);
	char *module_str = kzalloc(sizeof(char) * 128, GFP_KERNEL);
	char *cmd_str = kzalloc(sizeof(char) * 32, GFP_KERNEL);
	int ret;

	if (get_module_config(debug, VC_TITLE, module_str) == 0) {
		pr_info("%s: Display_VC:%s\n", __func__, module_str);
		ret = set_pipeline_debug_flag(module_str, cmd_str, MODULE_VC);
		if (ret != 0)
			pr_info("set debug flag fail.\n");
	} else if (get_module_config(debug, DEFAULT_TITLE, default_str) == 0) {
		pr_info("%s: default:%s\n", __func__, default_str);
		ret = set_default_debug_flag(default_str, cmd_str, MODULE_VC);
		if (ret != 0)
			pr_info("set default debug flag fail.\n");
	}

	kfree(default_str);
	kfree(module_str);
	kfree(cmd_str);
}
EXPORT_SYMBOL(set_vc_config);

void set_vq_config(const char *module, const char *debug, int len)
{
	char *default_str = kzalloc(sizeof(char) * 128, GFP_KERNEL);
	char *module_str = kzalloc(sizeof(char) * 128, GFP_KERNEL);
	char *cmd_str = kzalloc(sizeof(char) * 32, GFP_KERNEL);
	int ret;

	if (get_module_config(debug, VQ_TITLE, module_str) == 0) {
		pr_info("%s: Display_VQ:%s\n", __func__, module_str);
		ret = set_pipeline_debug_flag(module_str, cmd_str, MODULE_VQ);
		if (ret != 0)
			pr_info("set debug flag fail.\n");
	} else if (get_module_config(debug, DEFAULT_TITLE, default_str) == 0) {
		pr_info("%s: default:%s", __func__, default_str);
		ret = set_default_debug_flag(default_str, cmd_str, MODULE_VQ);
		if (ret != 0)
			pr_info("set default debug flag fail.\n");
	}

	kfree(default_str);
	kfree(module_str);
	kfree(cmd_str);
}
EXPORT_SYMBOL(set_vq_config);

