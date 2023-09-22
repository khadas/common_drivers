/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_TRACER_H
#define __HDMITX_TRACER_H

/* For Debug:
 * write event to fifo, and userspace can read by sysfs.
 */

enum hdmitx_event_log_bits {
	HDMITX_HPD_PLUGOUT                      = BIT(0),
	HDMITX_HPD_PLUGIN                       = BIT(1),
	/* EDID states */
	HDMITX_EDID_HDMI_DEVICE                 = BIT(2),
	HDMITX_EDID_DVI_DEVICE                  = BIT(3),
	/* HDCP states */
	HDMITX_HDCP_AUTH_SUCCESS                = BIT(4),
	HDMITX_HDCP_AUTH_FAILURE                = BIT(5),
	HDMITX_HDCP_HDCP_1_ENABLED              = BIT(6),
	HDMITX_HDCP_HDCP_2_ENABLED              = BIT(7),
	HDMITX_HDCP_NOT_ENABLED                 = BIT(8),

	/* EDID errors */
	HDMITX_EDID_HEAD_ERROR                  = BIT(16),
	HDMITX_EDID_CHECKSUM_ERROR              = BIT(17),
	HDMITX_EDID_I2C_ERROR                   = BIT(18),
	/* HDCP errors */
	HDMITX_HDCP_DEVICE_NOT_READY_ERROR      = BIT(19),
	HDMITX_HDCP_AUTH_NO_14_KEYS_ERROR       = BIT(20),
	HDMITX_HDCP_AUTH_NO_22_KEYS_ERROR       = BIT(21),
	HDMITX_HDCP_AUTH_READ_BKSV_ERROR        = BIT(22),
	HDMITX_HDCP_AUTH_VI_MISMATCH_ERROR      = BIT(23),
	HDMITX_HDCP_AUTH_TOPOLOGY_ERROR         = BIT(24),
	HDMITX_HDCP_AUTH_R0_MISMATCH_ERROR      = BIT(25),
	HDMITX_HDCP_AUTH_REPEATER_DELAY_ERROR   = BIT(26),
	HDMITX_HDCP_I2C_ERROR                   = BIT(27),

	HDMITX_HDMI_ERROR_MASK	=	GENMASK(31, 16),
};

struct hdmitx_tracer;

int hdmitx_tracer_destroy(struct hdmitx_tracer *tracer);

/*write event to fifo, return 0 if write success, else return errno.*/
int hdmitx_tracer_write_event(struct hdmitx_tracer *tracer,
	enum hdmitx_event_log_bits event);
/*read events log in fifo, return read buffer len.*/
int hdmitx_tracer_read_event(struct hdmitx_tracer *tracer,
	char *buf, int read_len);

#endif
