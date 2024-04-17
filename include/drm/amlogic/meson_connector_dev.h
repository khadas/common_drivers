/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 */

#ifndef MESON_CONNECTOR_DEV_H_
#define MESON_CONNECTOR_DEV_H_
#include <drm/drm_connector.h>
#include <drm/drm_modes.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_format_para.h>
#include <uapi/amlogic/drm/meson_drm.h>

#define MESON_CONNECTOR_TYPE_PROP_NAME "meson.connector_type"

enum {
	MESON_DRM_CONNECTOR_V10 = 0,
};

struct drm_hdmitx_timing_para {
	char name[DRM_DISPLAY_MODE_LEN];
	int pi_mode;
	u32 pix_repeat_factor;
	u32 h_pol;
	u32 v_pol;

	u32 pixel_freq;
	u32 h_active;
	u32 h_front;
	u32 h_sync;
	u32 h_total;
	u32 v_active;
	u32 v_front;
	u32 v_sync;
	u32 v_total;
};

struct meson_connector_dev {
	int ver;

	/*copy from vout_op_s*/
	struct vinfo_s *(*get_vinfo)(void *data);
	int (*set_vmode)(enum vmode_e vmode, void *data);
	enum vmode_e (*validate_vmode)(char *name, unsigned int frac,
				       void *data);
	int (*check_same_vmodeattr)(char *name, void *data);
	int (*vmode_is_supported)(enum vmode_e vmode, void *data);
	int (*disable)(enum vmode_e vmode, void *data);
	int (*set_state)(int state, void *data);
	int (*clr_state)(int state, void *data);
	int (*get_state)(void *data);
	int (*get_disp_cap)(char *buf, void *data);
	int (*set_vframe_rate_hint)(int policy, void *data);
	int (*get_vframe_rate_hint)(void *data);
	void (*set_bist)(unsigned int num, void *data);
	int (*vout_suspend)(void *data);
	int (*vout_resume)(void *data);
	int (*vout_shutdown)(void *data);
};

/*hdmitx specified struct*/
/*hdcp*/
enum {
	HDCP_NULL = 0,
	HDCP_MODE14 = 1 << 0,
	HDCP_MODE22 = 1 << 1,
	HDCP_KEY_UPDATE = 1 << 2
};

enum {
	HDCP_AUTH_FAIL = 0,
	HDCP_AUTH_OK = 1,
	HDCP_AUTH_UNKNOWN = 0xff,
};

struct connector_hpd_cb {
	void (*callback)(void *data);
	void *data;
};

struct connector_hdcp_cb {
	void (*hdcp_notify)(void *data, int type, int auth_result);
	void *data;
};

struct meson_hdmitx_dev {
	struct meson_connector_dev base;
	struct hdmitx_common *hdmitx_common;
	struct hdmitx_hw_common *hw_common;

	/*hdcp apis*/
	void (*hdcp_init)(void);
	void (*hdcp_exit)(void);
	void (*hdcp_enable)(int hdcp_type);
	void (*hdcp_disable)(void);
	void (*hdcp_disconnect)(void);

	unsigned int (*get_tx_hdcp_cap)(void);
	unsigned int (*get_rx_hdcp_cap)(void);
	void (*register_hdcp_notify)(struct connector_hdcp_cb *cb);

	/*vrr apis*/
	bool (*get_vrr_cap)(void);
	int (*get_vrr_mode_group)(struct drm_vrr_mode_group *groups, int max_group);
	int (*set_vframe_rate_hint)(int duration, void *data);

	int (*get_hdmi_hdr_status)(void);
};

#define to_meson_hdmitx_dev(x)	container_of(x, struct meson_hdmitx_dev, base)

/*hdmitx specified struct end.*/

/*cvbs specified struct*/
struct meson_cvbs_dev {
	struct meson_connector_dev base;
};

/*cvbs specified struct*/

/*panel specified struct*/
struct meson_panel_dev {
	struct meson_connector_dev base;
	int (*get_modes)(struct meson_panel_dev *panel, struct drm_display_mode **modes, int *num);
	int (*get_modes_vrr_range)(struct meson_panel_dev *panel, void *range, int max, int *num);
};

/*dummy_l specified struct*/
struct meson_dummyl_dev {
	struct meson_connector_dev base;
};

/*dummy_l specified type*/
#define DRM_MODE_CONNECTOR_MESON_DUMMY_L  0x200

/*dummy_p specified struct*/
struct meson_dummyp_dev {
	struct meson_connector_dev base;
};

/*dummy_p specified type*/
#define DRM_MODE_CONNECTOR_MESON_DUMMY_P  0x201

#define to_meson_panel_dev(x)	container_of(x, struct meson_panel_dev, base)
#define to_meson_dummyl_dev(x)	container_of(x, struct meson_dummyl_dev, base)
#define to_meson_dummyp_dev(x)	container_of(x, struct meson_dummyp_dev, base)

/*lcd specified struct*/

/*amlogic extend connector type: for original type is not enough.
 *start from: 0xff,
 *extend connector: 0x100 ~ 0x1ff,
 *legacy panel type for non-drm: 0x1000 ~
 */
#define DRM_MODE_MESON_CONNECTOR_PANEL_START 0xff
#define DRM_MODE_MESON_CONNECTOR_PANEL_END   0x1ff

enum {
	DRM_MODE_CONNECTOR_MESON_LVDS_A = 0x100,
	DRM_MODE_CONNECTOR_MESON_LVDS_B = 0x101,
	DRM_MODE_CONNECTOR_MESON_LVDS_C = 0x102,

	DRM_MODE_CONNECTOR_MESON_VBYONE_A = 0x110,
	DRM_MODE_CONNECTOR_MESON_VBYONE_B = 0x111,

	DRM_MODE_CONNECTOR_MESON_MIPI_A = 0x120,
	DRM_MODE_CONNECTOR_MESON_MIPI_B = 0x121,

	DRM_MODE_CONNECTOR_MESON_EDP_A = 0x130,
	DRM_MODE_CONNECTOR_MESON_EDP_B = 0x131,
};

#endif
