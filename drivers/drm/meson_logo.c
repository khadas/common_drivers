// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/of_graph.h>
#include <linux/dma-map-ops.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <uapi/linux/sched/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/notifier.h>
#include <linux/clk.h>
#include <linux/cma.h>
#include <linux/of_address.h>

#include <drm/drmP.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_atomic_uapi.h>
#include <drm/drm_flip_work.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_rect.h>
#include <drm/drm_fb_helper.h>

#ifdef CONFIG_AMLOGIC_DRM_USE_ION
#include "meson_gem.h"
#include "meson_fb.h"
#endif
#include "meson_drv.h"
#include "meson_vpu.h"
#include "meson_vpu_pipeline.h"
#include "meson_crtc.h"
#include "meson_logo.h"
#include "meson_hdmi.h"
#include "meson_plane.h"

#ifdef CONFIG_AMLOGIC_MEDIA_FB
#include <linux/amlogic/media/osd/osd_logo.h>
#endif
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/amlogic/gki_module.h>
#include <linux/amlogic/aml_free_reserved.h>

#ifdef CONFIG_CMA
struct cma *cma_logo;
#endif

static char *strmode;
struct am_meson_logo logo;
static struct platform_device *gp_dev;
static unsigned long gem_mem_start, gem_mem_size;
static struct resource osd_mem_res;
static bool is_cma;

#ifdef MODULE
MODULE_PARM_DESC(outputmode, "outputmode");
module_param_named(outputmode, logo.outputmode_t, charp, 0600);

#else
core_param(fb_width, logo.width, uint, 0644);
core_param(fb_height, logo.height, uint, 0644);
core_param(display_bpp, logo.bpp, uint, 0644);
core_param(outputmode, logo.outputmode_t, charp, 0644);
#endif

#ifndef CONFIG_AMLOGIC_MEDIA_FB
static u32 drm_logo_bpp = 16;
static u32 drm_logo_width = 1920;
static u32 drm_logo_height = 1080;

static int drm_logo_bpp_setup(char *str)
{
	int ret;

	ret = kstrtoint(str, 0, &drm_logo_bpp);
	return ret;
}
__setup("display_bpp=", drm_logo_bpp_setup);

static u32 drm_logo_get_display_bpp(void)
{
	return drm_logo_bpp;
}

static int drm_logo_width_setup(char *str)
{
	int ret;

	ret = kstrtoint(str, 0, &drm_logo_width);
	return ret;
}
__setup("fb_width=", drm_logo_width_setup);

static u32 drm_logo_get_fb_width(void)
{
	return drm_logo_width;
}

static int drm_logo_height_setup(char *str)
{
	int ret;

	ret = kstrtoint(str, 0, &drm_logo_height);
	return ret;
}
__setup("fb_height=", drm_logo_height_setup);

static u32 drm_logo_get_fb_height(void)
{
	return drm_logo_height;
}

#define OSD_INVALID_INFO 0xffffffff
#define OSD_FIRST_GROUP_START 1
#define OSD_SECOND_GROUP_START 4
#define OSD_END 7

static inline  int str2lower(char *str)
{
	while (*str != '\0') {
		*str = tolower(*str);
		str++;
	}
	return 0;
}

static struct osd_info_s osd_info = {
	.index = 0,
	.osd_reverse = 0,
};

static struct para_osd_info_s para_osd_info[OSD_END + 2] = {
	/* head */
	{
		"head", OSD_INVALID_INFO,
		OSD_END + 1, 1,
		0, OSD_END + 1
	},
	/* dev */
	{
		"osd0",	DEV_OSD0,
		OSD_FIRST_GROUP_START - 1, OSD_FIRST_GROUP_START + 1,
		OSD_FIRST_GROUP_START, OSD_SECOND_GROUP_START - 1
	},
	{
		"osd1",	DEV_OSD1,
		OSD_FIRST_GROUP_START, OSD_FIRST_GROUP_START + 2,
		OSD_FIRST_GROUP_START, OSD_SECOND_GROUP_START - 1
	},
	{
		"all", DEV_ALL,
		OSD_FIRST_GROUP_START + 1, OSD_FIRST_GROUP_START + 3,
		OSD_FIRST_GROUP_START, OSD_SECOND_GROUP_START - 1
	},
	/* reverse_mode */
	{
		"false", REVERSE_FALSE,
		OSD_SECOND_GROUP_START - 1, OSD_SECOND_GROUP_START + 1,
		OSD_SECOND_GROUP_START, OSD_END
	},
	{
		"true", REVERSE_TRUE,
		OSD_SECOND_GROUP_START, OSD_SECOND_GROUP_START + 2,
		OSD_SECOND_GROUP_START, OSD_END
	},
	{
		"x_rev", REVERSE_X,
		OSD_SECOND_GROUP_START + 1, OSD_SECOND_GROUP_START + 3,
		OSD_SECOND_GROUP_START, OSD_END
	},
	{
		"y_rev", REVERSE_Y,
		OSD_SECOND_GROUP_START + 2, OSD_SECOND_GROUP_START + 4,
		OSD_SECOND_GROUP_START, OSD_END
	},
	{
		"tail", OSD_INVALID_INFO, OSD_END,
		0, 0,
		OSD_END + 1
	},
};

static inline int install_osd_reverse_info(struct osd_info_s *init_osd_info,
					   char *para)
{
	u32 i = 0;
	static u32 tail = OSD_END + 1;
	u32 first = para_osd_info[0].next_idx;

	for (i = first; i < tail; i = para_osd_info[i].next_idx) {
		if (strcmp(para_osd_info[i].name, para) == 0) {
			u32 group_start = para_osd_info[i].cur_group_start;
			u32 group_end = para_osd_info[i].cur_group_end;
			u32	prev = para_osd_info[group_start].prev_idx;
			u32  next = para_osd_info[group_end].next_idx;

			switch (para_osd_info[i].cur_group_start) {
			case OSD_FIRST_GROUP_START:
				init_osd_info->index = para_osd_info[i].info;
				break;
			case OSD_SECOND_GROUP_START:
				init_osd_info->osd_reverse =
					para_osd_info[i].info;
				break;
			}
			para_osd_info[prev].next_idx = next;
			para_osd_info[next].prev_idx = prev;
			return 0;
		}
	}
	return 0;
}

static int drm_logo_reverse_setup(char *str)
{
	char	*ptr = str;
	char	sep[2];
	char	*option;
	int count = 2;
	char find = 0;
	struct osd_info_s *init_osd_info;

	if (!str)
		return -EINVAL;

	init_osd_info = &osd_info;
	memset(init_osd_info, 0, sizeof(struct osd_info_s));
	do {
		if (!isalpha(*ptr) && !isdigit(*ptr)) {
			find = 1;
			break;
		}
	} while (*++ptr != '\0');
	if (!find)
		return -EINVAL;
	sep[0] = *ptr;
	sep[1] = '\0';
	while ((count--) && (option = strsep(&str, sep))) {
		str2lower(option);
		install_osd_reverse_info(init_osd_info, option);
	}
	return 0;
}
__setup("osd_reverse=", drm_logo_reverse_setup);

void drm_logo_get_osd_reverse(u32 *index, u32 *reverse_type)
{
	*index = osd_info.index;
	*reverse_type = osd_info.osd_reverse;
}

#endif

struct para_pair_s {
	char *name;
	int value;
};

#ifdef CONFIG_AMLOGIC_MEMORY_EXTEND
#ifdef CONFIG_64BIT
static void free_reserved_highmem(unsigned long start, unsigned long end)
{
}
#else
static void free_reserved_highmem(unsigned long start, unsigned long end)
{
	for (; start < end; ) {
		free_highmem_page(phys_to_page(start));
	start += PAGE_SIZE;
	}
}
#endif

static void free_reserved_mem(unsigned long start, unsigned long size)
{
	unsigned long end = PAGE_ALIGN(start + size);
	struct page *page, *epage;

	pr_info("%s %d logo start_addr=%lx, end=%lx\n", __func__, __LINE__, start, end);
	page = phys_to_page(start);
	if (PageHighMem(page)) {
		free_reserved_highmem(start, end);
	} else {
		epage = phys_to_page(end);
		if (!PageHighMem(epage)) {
			aml_free_reserved_area(__va(start),
					   __va(end), 0, "fb-memory");
		} else {
			/* reserved area cross zone */
			struct zone *zone;
			unsigned long bound;

			zone  = page_zone(page);
			bound = zone_end_pfn(zone);
			aml_free_reserved_area(__va(start),
					   __va(bound << PAGE_SHIFT),
					   0, "fb-memory");
			zone  = page_zone(epage);
			bound = zone->zone_start_pfn;
			free_reserved_highmem(bound << PAGE_SHIFT, end);
		}
	}
}
#endif

void am_meson_free_logo_memory(void)
{
	if (is_cma) {
		phys_addr_t logo_addr = page_to_phys(logo.logo_page);

		if (logo.size > 0 && logo.alloc_flag) {
#ifdef CONFIG_CMA
			DRM_INFO("%s, free cma memory: addr:0x%pa,size:0x%x\n",
				 __func__, &logo_addr, logo.size);

			cma_release(cma_logo, logo.logo_page, logo.size >> PAGE_SHIFT);
#endif
		}
	} else {
#ifdef CONFIG_AMLOGIC_MEMORY_EXTEND
		free_reserved_mem(logo.start, logo.size);
		DRM_INFO("%s, free none_cma memory: addr:0x%pa,size:0x%x\n",
				 __func__, &logo.start, logo.size);
#endif
	}

	logo.alloc_flag = 0;
}

static int am_meson_logo_info_update(struct meson_drm *priv)
{
	if (is_cma)
		logo.start = page_to_phys(logo.logo_page);

	logo.alloc_flag = 1;
	/*config 1080p logo as default*/
	if (!logo.width || !logo.height) {
		logo.width = 1920;
		logo.height = 1080;
	}
	if (!logo.bpp)
		logo.bpp = 16;
	if (!logo.outputmode_t) {
		strcpy(logo.outputmode, "1080p60hz");
	} else {
		strncpy(logo.outputmode, logo.outputmode_t, VMODE_NAME_LEN_MAX);
		logo.outputmode[VMODE_NAME_LEN_MAX - 1] = '\0';
	}
	priv->logo = &logo;

	return 0;
}

static int am_meson_logo_init_fb(struct drm_device *dev,
		struct drm_framebuffer *fb, int idx)
{
	struct am_meson_fb *meson_fb;
	struct am_meson_logo *slogo;
	struct meson_drm *priv = dev->dev_private;

	slogo = kzalloc(sizeof(*slogo), GFP_KERNEL);
	if (!slogo)
		return -EFAULT;

	memcpy(slogo, &logo, sizeof(struct am_meson_logo));
	if (idx == VPP0) {
#ifdef CONFIG_AMLOGIC_VOUT_SERVE
		strcpy(slogo->outputmode, get_vout_mode_uboot());
#endif
	} else if (idx == VPP1) {
#ifdef CONFIG_AMLOGIC_VOUT2_SERVE
		strcpy(slogo->outputmode, get_vout2_mode_uboot());
#endif
	} else if (idx == VPP2) {
#ifdef CONFIG_AMLOGIC_VOUT3_SERVE
		strcpy(slogo->outputmode, get_vout3_mode_uboot());
#endif
	}

	if (!strcmp("null", slogo->outputmode) ||
		!strcmp("dummy_l", slogo->outputmode)) {
		DRM_DEBUG("NULL MODE or DUMMY MODE, nothing to do.");
		kfree(slogo);
		return -EINVAL;
	}

	slogo->logo_page = logo.logo_page;
	slogo->vaddr = logo.vaddr;
	slogo->start = logo.start;
	slogo->panel_index = priv->primary_plane_index[idx];
	slogo->vpp_index = idx;

	DRM_INFO("logo%d width=%d,height=%d,start_addr=0x%pa,size=%d\n",
		 idx, slogo->width, slogo->height, &slogo->start, slogo->size);
	DRM_DEBUG("bpp=%d,alloc_flag=%d, osd_reverse=%d\n",
		 slogo->bpp, slogo->alloc_flag, slogo->osd_reverse);
	DRM_DEBUG("outputmode=%s\n", slogo->outputmode);

	meson_fb = to_am_meson_fb(fb);
	meson_fb->logo = slogo;

	return 0;
}

/*copy from update_output_state,
 *TODO:sync with update_output_state
 */
static int am_meson_update_output_state(struct drm_atomic_state *state,
					struct drm_mode_set *set)
{
	struct drm_device *dev = set->crtc->dev;
	struct drm_crtc *crtc;
	struct drm_crtc_state *new_crtc_state;
	struct drm_connector *connector;
	struct drm_connector_state *new_conn_state;
	int ret, i;
	char hdmitx_attr[16];
	struct am_hdmitx_connector_state *hdmitx_state;

	ret = drm_modeset_lock(&dev->mode_config.connection_mutex,
			       state->acquire_ctx);
	if (ret)
		return ret;

	/* First disable all connectors on the target crtc. */
	ret = drm_atomic_add_affected_connectors(state, set->crtc);
	if (ret)
		return ret;

	for_each_new_connector_in_state(state, connector, new_conn_state, i) {
		if (new_conn_state->crtc == set->crtc) {
			ret = drm_atomic_set_crtc_for_connector(new_conn_state,
								NULL);
			if (ret)
				return ret;

			/* Make sure legacy setCrtc always re-trains */
			new_conn_state->link_status = DRM_LINK_STATUS_GOOD;
		}
	}

	/* Then set all connectors from set->connectors on the target crtc */
	for (i = 0; i < set->num_connectors; i++) {
		new_conn_state =
			drm_atomic_get_connector_state(state,
						       set->connectors[i]);
		if (IS_ERR(new_conn_state))
			return PTR_ERR(new_conn_state);

		ret = drm_atomic_set_crtc_for_connector(new_conn_state,
							set->crtc);
		if (ret)
			return ret;

		if (new_conn_state->connector->connector_type == DRM_MODE_CONNECTOR_HDMIA) {
			/*read attr from hdmitx, its from uboot*/
			am_hdmi_info.hdmitx_dev->get_attr(hdmitx_attr);
			hdmitx_state = to_am_hdmitx_connector_state(new_conn_state);
			convert_attrstr(hdmitx_attr, &hdmitx_state->color_attr_para);
			hdmitx_state->pref_hdr_policy = am_hdmi_info.hdmitx_dev->get_hdr_priority();
		}
	}

	for_each_new_crtc_in_state(state, crtc, new_crtc_state, i) {
		/* Don't update ->enable for the CRTC in the set_config request,
		 * since a mismatch would indicate a bug in the upper layers.
		 * The actual modeset code later on will catch any
		 * inconsistencies here.
		 */
		if (crtc == set->crtc)
			continue;

		if (!new_crtc_state->connector_mask) {
			ret = drm_atomic_set_mode_prop_for_crtc(new_crtc_state,
								NULL);
			if (ret < 0)
				return ret;

			new_crtc_state->active = false;
		}
	}

	return 0;
}

static int _am_meson_occupy_plane_config(struct drm_atomic_state *state,
					struct drm_mode_set *set)
{
	struct drm_crtc *crtc = set->crtc;
	struct meson_drm *private = crtc->dev->dev_private;
	struct am_osd_plane *osd_plane;
	struct drm_plane_state *plane_state;
	int i, hdisplay, vdisplay, ret;

	for (i = 0; i < MESON_MAX_OSD; i++) {
		osd_plane = private->osd_planes[i];
		if (!osd_plane || osd_plane->osd_occupied)
			break;
	}

	if (!osd_plane || !osd_plane->osd_occupied)
		return 0;

	plane_state = drm_atomic_get_plane_state(state, &osd_plane->base);
	if (IS_ERR(plane_state))
		return PTR_ERR(plane_state);

	ret = drm_atomic_set_crtc_for_plane(plane_state, crtc);
	if (ret != 0)
		return ret;

	drm_mode_get_hv_timing(set->mode, &hdisplay, &vdisplay);
	drm_atomic_set_fb_for_plane(plane_state, set->fb);
	plane_state->crtc_x = 0;
	plane_state->crtc_y = 0;
	plane_state->crtc_w = hdisplay;
	plane_state->crtc_h = vdisplay;
	plane_state->src_x = 0;
	plane_state->src_y = 0;
	plane_state->src_w = 1280/*set->fb->width*/ << 16;
	plane_state->src_h = 720/*set->fb->height*/ << 16;
	plane_state->alpha = 1;
	plane_state->zpos = 128;

	return 0;
}

/*similar with __drm_atomic_helper_set_config,
 *TODO:sync with __drm_atomic_helper_set_config
 */
int __am_meson_drm_set_config(struct drm_mode_set *set,
			      struct drm_atomic_state *state, int idx)
{
	struct drm_crtc_state *crtc_state;
	struct drm_plane_state *plane_state;
	struct drm_crtc *crtc = set->crtc;
	struct meson_drm *private = crtc->dev->dev_private;
	struct am_meson_crtc_state *meson_crtc_state;
	struct am_osd_plane *osd_plane;
	struct am_meson_fb *meson_fb;
	int hdisplay, vdisplay, ret;
	unsigned int zpos = OSD_PLANE_BEGIN_ZORDER;

	crtc_state = drm_atomic_get_crtc_state(state, crtc);
	if (IS_ERR(crtc_state))
		return PTR_ERR(crtc_state);

	meson_fb = to_am_meson_fb(set->fb);
	meson_crtc_state = to_am_meson_crtc_state(crtc_state);
	if (meson_fb->logo->vpp_index == VPP0) {
#ifdef CONFIG_AMLOGIC_VOUT_SERVE
		meson_crtc_state->uboot_mode_init = get_vout_mode_uboot_state();
#endif
	} else if (meson_fb->logo->vpp_index == VPP1) {
#ifdef CONFIG_AMLOGIC_VOUT2_SERVE
		meson_crtc_state->uboot_mode_init = get_vout2_mode_uboot_state();
#endif
	} else if (meson_fb->logo->vpp_index == VPP2) {
#ifdef CONFIG_AMLOGIC_VOUT3_SERVE
		meson_crtc_state->uboot_mode_init = get_vout3_mode_uboot_state();
#endif
	}
	DRM_DEBUG("uboot_mode_init=%d\n", meson_crtc_state->uboot_mode_init);

	osd_plane = private->osd_planes[idx];
	if (!osd_plane)
		return -EINVAL;

	plane_state = drm_atomic_get_plane_state(state, &osd_plane->base);
	if (IS_ERR(plane_state))
		return PTR_ERR(plane_state);

	if (!set->mode) {
		WARN_ON(set->fb);
		WARN_ON(set->num_connectors);

		ret = drm_atomic_set_mode_for_crtc(crtc_state, NULL);
		if (ret != 0)
			return ret;

		crtc_state->active = false;

		ret = drm_atomic_set_crtc_for_plane(plane_state, NULL);
		if (ret != 0)
			return ret;

		drm_atomic_set_fb_for_plane(plane_state, NULL);

		goto commit;
	}

	WARN_ON(!set->fb);
	WARN_ON(!set->num_connectors);

	ret = drm_atomic_set_mode_for_crtc(crtc_state, set->mode);
	if (ret != 0)
		return ret;

	crtc_state->active = true;

	ret = drm_atomic_set_crtc_for_plane(plane_state, crtc);
	if (ret != 0)
		return ret;

	drm_mode_get_hv_timing(set->mode, &hdisplay, &vdisplay);
	drm_atomic_set_fb_for_plane(plane_state, set->fb);
	plane_state->crtc_x = 0;
	plane_state->crtc_y = 0;
	plane_state->crtc_w = hdisplay;
	plane_state->crtc_h = vdisplay;
	plane_state->src_x = set->x << 16;
	plane_state->src_y = set->y << 16;
	plane_state->zpos = zpos + osd_plane->plane_index;

	switch (logo.osd_reverse) {
	case 1:
		plane_state->rotation = DRM_MODE_REFLECT_MASK;
		break;
	case 2:
		plane_state->rotation = DRM_MODE_REFLECT_X;
		break;
	case 3:
		plane_state->rotation = DRM_MODE_REFLECT_Y;
		break;
	default:
		plane_state->rotation = DRM_MODE_ROTATE_0;
		break;
	}

	if (drm_rotation_90_or_270(plane_state->rotation)) {
		if (private->ui_config.ui_h)
			plane_state->src_w = private->ui_config.ui_h << 16;
		else
			plane_state->src_w = set->fb->height << 16;
		if (private->ui_config.ui_w)
			plane_state->src_h = private->ui_config.ui_w << 16;
		else
			plane_state->src_h = set->fb->width << 16;
	} else {
		if (private->ui_config.ui_w)
			plane_state->src_w = private->ui_config.ui_w << 16;
		else
			plane_state->src_w = set->fb->width << 16;
		if (private->ui_config.ui_h)
			plane_state->src_h = private->ui_config.ui_h << 16;
		else
			plane_state->src_h = set->fb->height << 16;
	}

	if (meson_fb->logo->vpp_index == VPP0)
		_am_meson_occupy_plane_config(state, set);

commit:
	ret = am_meson_update_output_state(state, set);
	if (ret)
		return ret;

	return 0;
}

/*copy from drm_atomic_helper_set_config,
 *TODO:sync with drm_atomic_helper_set_config
 */
static int am_meson_drm_set_config(struct drm_mode_set *set,
				   struct drm_modeset_acquire_ctx *ctx, int idx)
{
	struct drm_atomic_state *state;
	struct drm_crtc *crtc = set->crtc;
	int ret = 0;

	state = drm_atomic_state_alloc(crtc->dev);
	if (!state)
		return -ENOMEM;

	state->acquire_ctx = ctx;
	ret = __am_meson_drm_set_config(set, state, idx);
	if (ret != 0)
		goto fail;

	ret = drm_atomic_commit(state);

fail:
	drm_atomic_state_put(state);
	return ret;
}

static void am_meson_load_logo(struct drm_device *dev,
	struct drm_framebuffer *fb, int idx)
{
	struct drm_mode_set set;
	struct drm_display_mode *mode;
	struct drm_connector **connector_set;
	struct drm_connector *connector;
	struct drm_modeset_acquire_ctx *ctx;
	struct meson_drm *private = dev->dev_private;
	struct am_meson_fb *meson_fb;
	u32 found, num_modes;

	DRM_DEBUG("%s idx[%d]\n", __func__, idx);

	if (!logo.alloc_flag) {
		DRM_INFO("%s: logo memory is not alloc\n", __func__);
		return;
	}

	if (am_meson_logo_init_fb(dev, fb, idx)) {
		DRM_DEBUG("vout%d logo is disabled!\n", idx + 1);
		return;
	}

	meson_fb = to_am_meson_fb(fb);
	/*init all connector and found matched uboot mode.*/
	found = 0;
	list_for_each_entry(connector, &dev->mode_config.connector_list, head) {
		drm_modeset_lock_all(dev);
		if (drm_modeset_is_locked(&dev->mode_config.connection_mutex))
			drm_modeset_unlock(&dev->mode_config.connection_mutex);
		num_modes = connector->funcs->fill_modes(connector,
							 dev->mode_config.max_width,
							 dev->mode_config.max_height);
		drm_modeset_unlock_all(dev);

		if (num_modes) {
			list_for_each_entry(mode, &connector->modes, head) {
				if (!strcmp(mode->name, meson_fb->logo->outputmode)) {
					found = 1;
					break;
				}
			}
			if (found)
				break;
		}

		DRM_DEBUG("Connector[%d] status[%d]\n",
			connector->connector_type, connector->status);
	}

	if (found) {
		DRM_INFO("Found Connector[%d] mode[%s]\n",
			connector->connector_type, mode->name);
		if (!strcmp("null", mode->name)) {
			DRM_INFO("NULL MODE, nothing to do.");
			return;
		}
	} else {
		connector = NULL;
		mode = NULL;
		return;
	}

	connector_set = kmalloc_array(1, sizeof(struct drm_connector *),
				      GFP_KERNEL);
	if (!connector_set)
		return;

	DRM_DEBUG("mode flag %x\n", mode->flags);

	connector_set[0] = connector;
	set.crtc = &private->crtcs[idx]->base;
	set.x = 0;
	set.y = 0;
	set.mode = mode;
	set.crtc->mode = *mode;
	set.connectors = connector_set;
	set.num_connectors = 1;
	set.fb = &meson_fb->base;

	drm_modeset_lock_all(dev);
	ctx = dev->mode_config.acquire_ctx;
	if (am_meson_drm_set_config(&set, ctx, meson_fb->logo->panel_index))
		DRM_INFO("[%s]am_meson_drm_set_config fail\n", __func__);
	drm_modeset_unlock_all(dev);

	kfree(connector_set);
}

static int parse_reserve_mem_resource(struct device_node *np,
				       struct resource *res)
{
	int ret;

	ret = of_address_to_resource(np, 0, res);
	of_node_put(np);

	return ret;
}

void am_meson_logo_init(struct drm_device *dev)
{
	struct drm_mode_fb_cmd2 mode_cmd;
	struct drm_framebuffer *fb;
	struct meson_drm *private = dev->dev_private;
	struct platform_device *pdev = to_platform_device(private->dev);
#ifdef CONFIG_CMA
	struct reserved_mem *rmem = NULL;
	struct device_node *np, *mem_node;
#endif
	u32 reverse_type, osd_index;
	int i, ret;
	const char *compatible;
	const int *reusable;

	DRM_DEBUG("%s in[%d]\n", __func__, __LINE__);

	gp_dev = pdev;
	np = gp_dev->dev.of_node;
	mem_node = of_parse_phandle(np, "memory-region", 0);
	compatible = of_get_property(mem_node, "compatible", NULL);
	reusable = of_get_property(mem_node, "reusable", NULL);

	if (!strcmp(compatible, "shared-dma-pool") && reusable)
		is_cma = true;
	else
		is_cma = false;

	if (is_cma) {
		DRM_INFO("allocate cmd mem\n");
		ret = of_reserved_mem_device_init(&gp_dev->dev);
		if (ret != 0) {
			DRM_ERROR("failed to init reserved memory\n");
		} else {
#ifdef CONFIG_CMA
			if (mem_node) {
				rmem = of_reserved_mem_lookup(mem_node);
				of_node_put(mem_node);
				if (rmem) {
					logo.size = rmem->size;
					DRM_DEBUG("of read %s reservememsize=0x%x, base %pa\n",
						rmem->name, logo.size, &rmem->base);
				}
			} else {
				DRM_ERROR("no memory-region\n");
			}

			cma_logo = dev_get_cma_area(&gp_dev->dev);

			if (cma_logo) {
				if (logo.size > 0) {
#if CONFIG_AMLOGIC_KERNEL_VERSION >= 14515
					logo.logo_page = cma_alloc(cma_logo,
							ALIGN(logo.size, PAGE_SIZE) >> PAGE_SHIFT,
							0, GFP_KERNEL);
#else
					logo.logo_page = cma_alloc(cma_logo,
							ALIGN(logo.size, PAGE_SIZE) >> PAGE_SHIFT,
							0, false);
#endif
					if (!logo.logo_page)
						DRM_ERROR("allocate buffer failed\n");
					else
						am_meson_logo_info_update(private);

					DRM_INFO(" cma_alloc from %s start page %px-%px size %x\n",
						cma_get_name(cma_logo),
						logo.logo_page,
						(void *)logo.start,
						logo.size);
				}
			}
#endif
			if (gem_mem_start) {
				dma_declare_coherent_memory(dev->dev,
								gem_mem_start,
								gem_mem_start,
								gem_mem_size);
				DRM_INFO("meson drm mem_start = 0x%x, size = 0x%x\n",
					(u32)gem_mem_start, (u32)gem_mem_size);
			}
		}
	} else {
		DRM_INFO("allocate reserved mem\n");
		if (mem_node) {
			ret = parse_reserve_mem_resource(mem_node, &osd_mem_res);

			if (ret != 0) {
				DRM_ERROR("failed to init none_cma memory\n");
			} else {
				logo.size = resource_size(&osd_mem_res);
				logo.start = osd_mem_res.start;
				DRM_INFO("of read reservememsize=0x%x--0x%x\n",
						logo.size, (u32)logo.start);
			}

			if (logo.size > 0) {
				logo.vaddr = memremap(logo.start, logo.size,
						MEMREMAP_WB);

				if (!logo.vaddr)
					DRM_ERROR("allocate buffer failed\n");
				else
					am_meson_logo_info_update(private);
			}

		} else {
			DRM_ERROR("no memory-region\n");
		}
	}

#ifdef CONFIG_AMLOGIC_MEDIA_FB
	get_logo_osd_reverse(&osd_index, &reverse_type);
	logo.osd_reverse = reverse_type;
	logo.width = get_logo_fb_width();
	logo.height = get_logo_fb_height();
	logo.bpp = get_logo_display_bpp();
#else
	drm_logo_get_osd_reverse(&osd_index, &reverse_type);
	logo.osd_reverse = reverse_type;
	logo.width = drm_logo_get_fb_width();
	logo.height = drm_logo_get_fb_height();
	logo.bpp = drm_logo_get_display_bpp();
#endif
	if (!logo.bpp)
		logo.bpp = 16;

	if (logo.bpp == 16)
		mode_cmd.pixel_format = DRM_FORMAT_RGB565;
	else if (logo.bpp == 24)
		mode_cmd.pixel_format = DRM_FORMAT_RGB888;
	else
		mode_cmd.pixel_format = DRM_FORMAT_XRGB8888;

	mode_cmd.offsets[0] = 0;
	mode_cmd.width = logo.width;
	mode_cmd.height = logo.height;
	mode_cmd.modifier[0] = DRM_FORMAT_MOD_LINEAR;
	/*ToDo*/
	mode_cmd.pitches[0] = ALIGN(mode_cmd.width * logo.bpp, 32) / 8;
	fb = am_meson_fb_alloc(dev, &mode_cmd, NULL);
	if (IS_ERR_OR_NULL(fb)) {
		DRM_ERROR("drm fb allocate failed\n");
		private->logo_show_done = true;
		return;
	}

	/*Todo: the condition may need change according to the boot args*/
	if (strmode && !strcmp("4", strmode))
		DRM_INFO("current is strmode\n");
	else
		for (i = 0; i < MESON_MAX_CRTC; i++)
			am_meson_load_logo(dev, fb, i);

	if (drm_framebuffer_read_refcount(fb) > 1)
		drm_framebuffer_put(fb);

	DRM_DEBUG("drm_fb[id:%d,ref:%d]\n", fb->base.id, kref_read(&fb->base.refcount));

	private->logo_show_done = true;

	DRM_DEBUG("%s end[%d]\n", __func__, __LINE__);
}

static int gem_mem_device_init(struct reserved_mem *rmem, struct device *dev)
{
	s32 ret = 0;

	if (!rmem) {
		pr_info("Can't get reverse mem!\n");
		ret = -EFAULT;
		return ret;
	}
	gem_mem_start = rmem->base;
	gem_mem_size = rmem->size;
	DRM_INFO("init gem memory source addr:0x%x size:0x%x\n",
		(u32)gem_mem_start, (u32)gem_mem_size);

	return 0;
}

static const struct reserved_mem_ops rmem_gem_ops = {
	.device_init = gem_mem_device_init,
};

static int __init gem_mem_setup(struct reserved_mem *rmem)
{
	rmem->ops = &rmem_gem_ops;
	DRM_INFO("gem mem setup\n");
	return 0;
}

RESERVEDMEM_OF_DECLARE(gem, "amlogic, gem_memory", gem_mem_setup);

