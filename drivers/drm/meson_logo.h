/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 */

#ifndef __AM_MESON_LOGO_H
#define __AM_MESON_LOGO_H
#include <stdarg.h>

#define VMODE_NAME_LEN_MAX    64

struct am_meson_logo {
	struct page *logo_page;
	void *vaddr;
	phys_addr_t start;
	int panel_index;
	int vpp_index;
	u32 size;
	u32 width;
	u32 height;
	u32 bpp;
	u32 alloc_flag;
	u32 info_loaded_mask;
	u32 osd_reverse;
	u32 vmode;
	u32 debug;
	u32 loaded;
	char *outputmode_t;
	char outputmode[VMODE_NAME_LEN_MAX];
};

enum osd_dev_e {
	DEV_OSD0 = 0,
	DEV_OSD1,
	DEV_OSD2,
	DEV_OSD3,
	DEV_ALL,
	DEV_MAX
};

#ifndef CONFIG_AMLOGIC_MEDIA_FB
struct osd_info_s {
	u32 index;
	u32 osd_reverse;
};

struct para_osd_info_s {
	char *name;
	u32 info;
	u32 prev_idx;
	u32 next_idx;
	u32 cur_group_start;
	u32 cur_group_end;
};

enum reverse_info_e {
	REVERSE_FALSE = 0,
	REVERSE_TRUE,
	REVERSE_X,
	REVERSE_Y,
	REVERSE_MAX
};

void drm_logo_get_osd_reverse(u32 *index, u32 *reverse_type);
#endif

void am_meson_logo_init(struct drm_device *dev);
void am_meson_free_logo_memory(void);

#endif

