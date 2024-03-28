/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#ifndef VIDEO_PP_COMMON_H
#define VIDEO_PP_COMMON_H

typedef void (*set_debug_flag_func)(const char *module, int debug_mode);
struct module_debug_node {
	const char *name;
	set_debug_flag_func set_debug_flag_notify;
};

void set_vc_config(const char *module, const char *debug, int len);
void set_vq_config(const char *module, const char *debug, int len);

#endif
