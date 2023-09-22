// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/vmalloc.h>
#include <linux/kfifo.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_tracer.h>

#define HDMI_LOG_SIZE (BIT(12)) /* 4k */

struct hdmitx_tracer {
	int previous_error_event;
	struct kfifo log_fifo;
};

const char *hdmitx_event_to_str(enum hdmitx_event_log_bits event)
{
	switch (event) {
	case HDMITX_HPD_PLUGOUT:
		return "hdmitx_hpd_plugout\n";
	case HDMITX_HPD_PLUGIN:
		return "hdmitx_hpd_plugin\n";
	case HDMITX_EDID_HDMI_DEVICE:
		return "hdmitx_edid_hdmi_device\n";
	case HDMITX_EDID_DVI_DEVICE:
		return "hdmitx_edid_dvi_device\n";
	case HDMITX_EDID_HEAD_ERROR:
		return "hdmitx_edid_head_error\n";
	case HDMITX_EDID_CHECKSUM_ERROR:
		return "hdmitx_edid_checksum_error\n";
	case HDMITX_EDID_I2C_ERROR:
		return "hdmitx_edid_i2c_error\n";
	case HDMITX_HDCP_AUTH_SUCCESS:
		return "hdmitx_hdcp_auth_success\n";
	case HDMITX_HDCP_AUTH_FAILURE:
		return "hdmitx_hdcp_auth_failure\n";
	case HDMITX_HDCP_HDCP_1_ENABLED:
		return "hdmitx_hdcp_hdcp1_enabled\n";
	case HDMITX_HDCP_HDCP_2_ENABLED:
		return "hdmitx_hdcp_hdcp2_enabled\n";
	case HDMITX_HDCP_NOT_ENABLED:
		return "hdmitx_hdcp_not_enabled\n";
	case HDMITX_HDCP_DEVICE_NOT_READY_ERROR:
		return "hdmitx_hdcp_device_not_ready_error\n";
	case HDMITX_HDCP_AUTH_NO_14_KEYS_ERROR:
		return "hdmitx_hdcp_auth_no_14_keys_error\n";
	case HDMITX_HDCP_AUTH_NO_22_KEYS_ERROR:
		return "hdmitx_hdcp_auth_no_22_keys_error\n";
	case HDMITX_HDCP_AUTH_READ_BKSV_ERROR:
		return "hdmitx_hdcp_auth_read_bksv_error\n";
	case HDMITX_HDCP_AUTH_VI_MISMATCH_ERROR:
		return "hdmitx_hdcp_auth_vi_mismatch_error\n";
	case HDMITX_HDCP_AUTH_TOPOLOGY_ERROR:
		return "hdmitx_hdcp_auth_topology_error\n";
	case HDMITX_HDCP_AUTH_R0_MISMATCH_ERROR:
		return "hdmitx_hdcp_auth_r0_mismatch_error\n";
	case HDMITX_HDCP_AUTH_REPEATER_DELAY_ERROR:
		return "hdmitx_hdcp_auth_repeater_delay_error\n";
	case HDMITX_HDCP_I2C_ERROR:
		return "hdmitx_hdcp_i2c_error\n";
	default:
		return "Unknown event\n";
	}
}

struct hdmitx_tracer *hdmitx_tracer_create(void)
{
	struct hdmitx_tracer *instance = vmalloc(sizeof(*instance));
	int ret = 0;

	if (!instance) {
		pr_err("%s FAIL\n", __func__);
	} else {
		instance->previous_error_event = 0;
		ret = kfifo_alloc(&instance->log_fifo, HDMI_LOG_SIZE, GFP_KERNEL);
		if (ret)
			pr_err("hdmitx: alloc hdmi_log_kfifo fail [%d]\n", ret);
	}

	return instance;
}

int hdmitx_tracer_destroy(struct hdmitx_tracer *tracer)
{
	if (tracer) {
		kfifo_free(&tracer->log_fifo);
		vfree(tracer);
	}

	return 0;
}

int hdmitx_tracer_write_event(struct hdmitx_tracer *tracer,
	enum hdmitx_event_log_bits event)
{
	const char *log_str;
	int ret = 0;

	/*reset error event when hpd*/
	if (event == HDMITX_HPD_PLUGOUT || event == HDMITX_HPD_PLUGIN)
		tracer->previous_error_event = 0;

	if (event & HDMITX_HDMI_ERROR_MASK) {
		if (event & tracer->previous_error_event) {
			// Found, skip duplicate logging.
			// For example, UEvent spamming of HDCP support (b/220687552).
			return 0;
		}
		pr_info("Record HDMI error: %s\n", hdmitx_event_to_str(event));
		tracer->previous_error_event |= event;
	}

	log_str = hdmitx_event_to_str(event);
	ret = kfifo_in(&tracer->log_fifo, log_str, strlen(log_str));
	if (!ret)
		pr_err("%s fifo error %d\n", __func__, ret);

	return ret;
}

int hdmitx_tracer_read_event(struct hdmitx_tracer *tracer,
	char *buf, int read_len)
{
	return kfifo_out(&tracer->log_fifo, buf, read_len);
}

