// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <drm/drmP.h>
#include <linux/component.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#include "hdmi_tx_module.h"
#include <drm/amlogic/meson_connector_dev.h>
#include <linux/arm-smccc.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include "hw/hdmi_tx_ddc.h"
#include "meson_hdcp.h"
#include "meson_drm_hdmitx.h"

/* ioctl numbers */
enum {
	TEE_HDCP_START,
	TEE_HDCP_END,
	HDCP_DAEMON_LOAD_END,
	HDCP_DAEMON_REPORT,
	HDCP_EXE_VER_SET,
	HDCP_TX_VER_REPORT,
	HDCP_DOWNSTR_VER_REPORT,
	HDCP_EXE_VER_REPORT
};

#define TEE_HDCP_IOC_START    _IOW('P', TEE_HDCP_START, int)
#define TEE_HDCP_IOC_END    _IOW('P', TEE_HDCP_END, int)
#define HDCP_DAEMON_IOC_LOAD_END    _IOW('P', HDCP_DAEMON_LOAD_END, int)
#define HDCP_DAEMON_IOC_REPORT    _IOR('P', HDCP_DAEMON_REPORT, int)
#define HDCP_EXE_VER_IOC_SET    _IOW('P', HDCP_EXE_VER_SET, int)
#define HDCP_TX_VER_IOC_REPORT    _IOR('P', HDCP_TX_VER_REPORT, int)
#define HDCP_DOWNSTR_VER_IOC_REPORT    _IOR('P', HDCP_DOWNSTR_VER_REPORT, int)
#define HDCP_EXE_VER_IOC_REPORT    _IOR('P', HDCP_EXE_VER_REPORT, int)

enum {
	HDCP_TX22_DISCONNECT = 0,
	HDCP_TX22_START,
	HDCP_TX22_STOP
};

enum {
	HDCP22_DAEMON_LOADING = 0,
	HDCP22_DAEMON_DONE,
	HDCP22_DAEMON_TIMEOUT
};

#define HDCP_AUTH_TIMEOUT (40) /*40*200ms = 8s*/
#define HDCP22_LOAD_TIMEOUT (160)
#define TIMER_CHECK	(1 * HZ / 2)
#define TIMER_CHK_CNT 60

struct meson_hdmitx_hdcp {
	struct miscdevice hdcp_comm_device;
	wait_queue_head_t hdcp_comm_queue;
	unsigned int hdcp_tx_type;/*bit0:hdcp14 bit 1:hdcp22*/
	unsigned int hdcp_downstream_type;/*bit0:hdcp14 bit 1:hdcp22*/
	unsigned int hdcp_execute_type;/*0: null hdcp 1: hdcp14 2: hdcp22*/
	unsigned int hdcp_debug_type;/*0: null hdcp 1: hdcp14 2: hdcp22*/

	unsigned int hdcp_en;
	int hdcp_poll_report;
	int hdcp_auth_result;
	int hdcp_fail_cnt;
	int hdcp_report;
	int hdcp22_daemon_state;

	struct connector_hdcp_cb drm_hdcp_cb;
	struct timer_list daemon_load_timer;
	unsigned int key_chk_cnt;
	struct delayed_work notify_work;
};

static struct meson_hdmitx_hdcp meson_hdcp;

static unsigned int get_hdcp_downstream_ver(void)
{
	unsigned int hdcp_downstream_type = meson_hdcp_get_rx_cap();

	HDMITX_INFO("downstream support hdcp14: %d\n",
		 hdcp_downstream_type & 0x1);
	HDMITX_INFO("downstream support hdcp22: %d\n",
		 (hdcp_downstream_type & 0x2) >> 1);

	return hdcp_downstream_type;
}

static unsigned int meson_hdcp_get_tx_key_version(void)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (!hdev || hdev->hdmi_init != 1)
		return 0;

	if (hdev->lstore < 0x10) {
		hdev->lstore = 0;
		if (hdmitx_hw_cntl_ddc(&hdev->tx_hw.base, DDC_HDCP_14_LSTORE, 0))
			hdev->lstore += 1;
		if (hdmitx_hw_cntl_ddc(&hdev->tx_hw.base, DDC_HDCP_22_LSTORE, 0))
			hdev->lstore += 2;
	}

	HDMITX_INFO("hdmitx support hdcp14: %d, hdcp22: %d\n",
		hdev->lstore & 0x1, (hdev->lstore & 0x2) >> 1);
	return hdev->lstore & 0x3;
}

int meson_hdcp_get_valid_type(int request_type_mask)
{
	int type;
	unsigned int hdcp_tx_type = meson_hdcp_get_tx_cap();

	HDMITX_INFO("%s usr_type: %d, key_store: %d\n",
		 __func__, request_type_mask, hdcp_tx_type);
	if (hdcp_tx_type)
		meson_hdcp.hdcp_downstream_type = get_hdcp_downstream_ver();
	switch (hdcp_tx_type & 0x3) {
	case 0x3:
		if ((meson_hdcp.hdcp_downstream_type & 0x2) &&
			(request_type_mask & 0x2))
			type = HDCP_MODE22;
		else if ((meson_hdcp.hdcp_downstream_type & 0x1) &&
			 (request_type_mask & 0x1))
			type = HDCP_MODE14;
		else
			type = HDCP_NULL;
		break;
	case 0x2:
		if ((meson_hdcp.hdcp_downstream_type & 0x2) &&
			(request_type_mask & 0x2))
			type = HDCP_MODE22;
		else
			type = HDCP_NULL;
		break;
	case 0x1:
		if ((meson_hdcp.hdcp_downstream_type & 0x1) &&
			(request_type_mask & 0x1))
			type = HDCP_MODE14;
		else
			type = HDCP_NULL;
		break;
	default:
		type = HDCP_NULL;
		HDMITX_INFO("[%s]: TX no hdcp key\n", __func__);
		break;
	}
	return type;
}

static int am_get_hdcp_exe_type(void)
{
	return meson_hdcp_get_valid_type(meson_hdcp.hdcp_debug_type);
}

void meson_hdcp_disable(void)
{
	if (!meson_hdcp.hdcp_en)
		return;

	DRM_INFO("[%s]: %d\n", __func__, meson_hdcp.hdcp_execute_type);
	if (meson_hdcp.hdcp_execute_type == HDCP_MODE22) {
		/* when switch mode under hdcp1.4, should not
		 * trigger hdcp_tx22 daemon to change status.
		 * otherwise, it may exit idle state and
		 * enter polling(driver hdcp2.2 start) per 5ms
		 * which cause high cpu CPU utilization
		 */
		meson_hdcp.hdcp_report = HDCP_TX22_DISCONNECT;
		/* wait for tx22 to enter unconnected state */
		msleep(200);
		meson_hdcp.hdcp_report = HDCP_TX22_STOP;
		/* wakeup hdcp_tx22 to stop hdcp22 */
		wake_up(&meson_hdcp.hdcp_comm_queue);
		/* wait for hdcp_tx22 stop hdcp22 done */
		msleep(200);
		drm_hdmitx_disable_hdcp_mode(HDCP_MODE22);
	} else if (meson_hdcp.hdcp_execute_type  == HDCP_MODE14) {
		drm_hdmitx_disable_hdcp_mode(HDCP_MODE14);
	}
	meson_hdcp.hdcp_execute_type = HDCP_NULL;
	meson_hdcp.hdcp_auth_result = HDCP_AUTH_UNKNOWN;
	meson_hdcp.hdcp_en = 0;
	meson_hdcp.hdcp_fail_cnt = 0;
}

void meson_hdcp_disconnect(void)
{
	/* if (meson_hdcp.hdcp_execute_type == HDCP_MODE22) */
		/* drm_hdmitx_disable_hdcp_mode(HDCP_MODE22); */
	/* else if (meson_hdcp.hdcp_execute_type  == HDCP_MODE14) */
		/* drm_hdmitx_disable_hdcp_mode(HDCP_MODE14); */
	meson_hdcp.hdcp_report = HDCP_TX22_DISCONNECT;
	meson_hdcp.hdcp_downstream_type = 0;
	/* TODO: for suspend/resume, need to stop/start hdcp22
	 * need to keep exe type
	 */
	meson_hdcp.hdcp_execute_type = HDCP_NULL;
	meson_hdcp.hdcp_auth_result = HDCP_AUTH_UNKNOWN;
	meson_hdcp.hdcp_en = 0;
	meson_hdcp.hdcp_fail_cnt = 0;
	HDMITX_INFO("[%s]: HDCP_TX22_DISCONNECT\n", __func__);
	wake_up(&meson_hdcp.hdcp_comm_queue);
}

static void meson_hdmitx_hdcp_cb(void *data, int auth)
{
	struct meson_hdmitx_hdcp *hdcp_data = (struct meson_hdmitx_hdcp *)data;
	int hdcp_auth_result = HDCP_AUTH_UNKNOWN;

	if (hdcp_data->hdcp_en &&
		hdcp_data->hdcp_auth_result == HDCP_AUTH_UNKNOWN) {
		if (auth == 1) {
			hdcp_auth_result = HDCP_AUTH_OK;
		} else if (auth == 0) {
			hdcp_data->hdcp_fail_cnt++;

			if (hdcp_data->hdcp_fail_cnt > HDCP_AUTH_TIMEOUT)
				hdcp_auth_result = HDCP_AUTH_FAIL;
		}

		HDMITX_INFO("HDCP cb %d vs %d\n", hdcp_data->hdcp_auth_result, auth);

		if (hdcp_auth_result != hdcp_data->hdcp_auth_result) {
			hdcp_data->hdcp_auth_result = hdcp_auth_result;
			if (meson_hdcp.drm_hdcp_cb.hdcp_notify)
				meson_hdcp.drm_hdcp_cb.hdcp_notify
					(meson_hdcp.drm_hdcp_cb.data,
					hdcp_data->hdcp_execute_type,
					hdcp_data->hdcp_auth_result);
		}
	}
}

void meson_hdcp_reg_result_notify(struct connector_hdcp_cb *cb)
{
	if (meson_hdcp.drm_hdcp_cb.hdcp_notify)
		HDMITX_INFO("Register hdcp notify again!?\n");

	meson_hdcp.drm_hdcp_cb.hdcp_notify = cb->hdcp_notify;
	meson_hdcp.drm_hdcp_cb.data = cb->data;
}

void meson_hdcp_enable(int hdcp_type)
{
	if (hdcp_type == HDCP_NULL)
		return;

	/* hdcp enabled, but may not start auth as key not ready */
	meson_hdcp.hdcp_en = 1;
	meson_hdcp.hdcp_fail_cnt = 0;
	meson_hdcp.hdcp_auth_result = HDCP_AUTH_UNKNOWN;
	meson_hdcp.hdcp_execute_type = hdcp_type;

	if (meson_hdcp.hdcp_execute_type == HDCP_MODE22) {
		if (meson_hdcp.hdcp22_daemon_state != HDCP22_DAEMON_DONE) {
			HDMITX_INFO("[%s]: hdcp_tx22 not ready, delay hdcp auth\n", __func__);
			return;
		}
		drm_hdmitx_enable_hdcp_mode(2);
		meson_hdcp.hdcp_report = HDCP_TX22_START;
		msleep(50);
		wake_up(&meson_hdcp.hdcp_comm_queue);
	} else if (meson_hdcp.hdcp_execute_type == HDCP_MODE14) {
		drm_hdmitx_enable_hdcp_mode(1);
	}
	HDMITX_INFO("[%s]: report=%d, use_type=%u, execute=%u\n",
		 __func__, meson_hdcp.hdcp_report,
		 meson_hdcp.hdcp_debug_type, meson_hdcp.hdcp_execute_type);
}

static long hdcp_comm_ioctl(struct file *file,
	unsigned int cmd,
	unsigned long arg)
{
	int rtn_val;
	unsigned int out_size;
	int hdcp_type = HDCP_NULL;

	switch (cmd) {
	case TEE_HDCP_IOC_START:
		/* notify by TEE, hdcp key ready, echo 2 > hdcp_mode */
		rtn_val = 0;
		meson_hdcp.hdcp_tx_type = meson_hdcp_get_tx_key_version();
		if (meson_hdcp.hdcp_tx_type & 0x2) {
			/* when bootup, if hdcp22 init after hdcp14 auth,
			 * hdcp path will switch to hdcp22. need to delay
			 * hdcp auth to covery this issue.
			 */
			drm_hdmitx_hdcp22_init();
		}
		HDMITX_INFO("hdcp key load ready\n");
		break;
	case TEE_HDCP_IOC_END:
		rtn_val = 0;
		break;
	case HDCP_DAEMON_IOC_LOAD_END:
		/* hdcp_tx22 load ready (after TEE key ready) */
		HDMITX_INFO("IOC_LOAD_END %d, %d\n",
			 meson_hdcp.hdcp_report, meson_hdcp.hdcp_poll_report);
		if (meson_hdcp.hdcp22_daemon_state == HDCP22_DAEMON_TIMEOUT)
			HDMITX_INFO("hdcp22 key load late than TIMEOUT.\n");
		meson_hdcp.hdcp22_daemon_state = HDCP22_DAEMON_DONE;
		meson_hdcp.hdcp_poll_report = meson_hdcp.hdcp_report;
		rtn_val = 0;
		break;
	case HDCP_DAEMON_IOC_REPORT:
		rtn_val = copy_to_user((void __user *)arg,
				       (void *)&meson_hdcp.hdcp_report,
				       sizeof(meson_hdcp.hdcp_report));
		if (rtn_val != 0) {
			out_size = sizeof(meson_hdcp.hdcp_report);
			HDMITX_INFO("out_size: %u, leftsize: %u\n",
				 out_size, rtn_val);
			rtn_val = -1;
		}
		break;
	case HDCP_EXE_VER_IOC_SET:
		if (arg > 2) {
			rtn_val = -1;
		} else {
			meson_hdcp.hdcp_debug_type = arg;
			rtn_val = 0;
			if (hdmitx_hpd_hw_op(HPD_READ_HPD_GPIO)) {
				meson_hdcp_disable();
				hdcp_type = am_get_hdcp_exe_type();
				meson_hdcp_enable(hdcp_type);
			}
		}
		break;
	case HDCP_TX_VER_IOC_REPORT:
		rtn_val = copy_to_user((void __user *)arg,
				       (void *)&meson_hdcp.hdcp_tx_type,
				       sizeof(meson_hdcp.hdcp_tx_type));
		if (rtn_val != 0) {
			out_size = sizeof(meson_hdcp.hdcp_tx_type);
			HDMITX_INFO("out_size: %u, leftsize: %u\n",
				 out_size, rtn_val);
			rtn_val = -1;
		}
		break;
	case HDCP_DOWNSTR_VER_IOC_REPORT:
		rtn_val = copy_to_user((void __user *)arg,
				       (void *)&meson_hdcp.hdcp_downstream_type,
				       sizeof(meson_hdcp.hdcp_downstream_type));
		if (rtn_val != 0) {
			out_size = sizeof(meson_hdcp.hdcp_downstream_type);
			HDMITX_INFO("out_size: %u, leftsize: %u\n",
				 out_size, rtn_val);
			rtn_val = -1;
		}
		break;
	case HDCP_EXE_VER_IOC_REPORT:
		rtn_val = copy_to_user((void __user *)arg,
				       (void *)&meson_hdcp.hdcp_execute_type,
				       sizeof(meson_hdcp.hdcp_execute_type));
		if (rtn_val != 0) {
			out_size = sizeof(meson_hdcp.hdcp_execute_type);
			HDMITX_INFO("out_size: %u, leftsize: %u\n",
				 out_size, rtn_val);
			rtn_val = -1;
		}
		break;
	default:
		rtn_val = -EPERM;
	}
	return rtn_val;
}

static unsigned int hdcp_comm_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	HDMITX_INFO("hdcp_poll %d, %d\n", meson_hdcp.hdcp_report, meson_hdcp.hdcp_poll_report);
	poll_wait(file, &meson_hdcp.hdcp_comm_queue, wait);
	if (meson_hdcp.hdcp_report != meson_hdcp.hdcp_poll_report) {
		mask = POLLIN | POLLRDNORM;
		meson_hdcp.hdcp_poll_report = meson_hdcp.hdcp_report;
	}
	return mask;
}

static const struct file_operations hdcp_comm_file_operations = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = hdcp_comm_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = hdcp_comm_ioctl,
#endif
	.poll = hdcp_comm_poll,
};

/***** debug interface begin *****/
static void am_hdmitx_set_hdcp_mode(unsigned int user_type)
{
	meson_hdcp.hdcp_debug_type = user_type;
	HDMITX_INFO("set_hdcp_mode: %d manually\n", user_type);
}

static void am_hdmitx_set_hdmi_mode(void)
{
	enum vmode_e vmode = get_current_vmode();

	if (vmode == VMODE_HDMI) {
		HDMITX_INFO("set_hdmi_mode manually\n");
	} else {
		HDMITX_INFO("set_hdmi_mode manually fail! vmode:%d\n", vmode);
		return;
	}

	/* TODO: sync meson_hdmi.c meson_hdmitx_encoder_atomic_enable() */
	set_vout_mode_pre_process(vmode);
	set_vout_vmode(vmode);
	set_vout_mode_post_process(vmode);
}

/* set hdmi+hdcp mode */
static void am_hdmitx_set_out_mode(void)
{
	enum vmode_e vmode = get_current_vmode();
	struct hdmitx_dev *hdmitx_dev = get_hdmitx_device();
	int last_hdcp_mode = HDCP_NULL;

	if (vmode == VMODE_HDMI) {
		HDMITX_INFO("set_out_mode\n");
	} else {
		HDMITX_INFO("set_out_mode fail! vmode:%d\n", vmode);
		return;
	}

	if (hdmitx_dev->tx_comm.hdcp_ctl_lvl > 0) {
		hdmitx_common_avmute_locked(&hdmitx_dev->tx_comm,
			SET_AVMUTE, AVMUTE_PATH_HDMITX);
		last_hdcp_mode = meson_hdcp.hdcp_execute_type;
		meson_hdcp_disable();
	}
	/* TODO: sync meson_hdmi.c meson_hdmitx_encoder_atomic_enable() */
	set_vout_mode_pre_process(vmode);
	set_vout_vmode(vmode);
	set_vout_mode_post_process(vmode);
	/* msleep(1000); */
	if (hdmitx_dev->tx_comm.hdcp_ctl_lvl > 0) {
		hdmitx_common_avmute_locked(&hdmitx_dev->tx_comm,
			CLR_AVMUTE, AVMUTE_PATH_HDMITX);
		meson_hdcp_enable(last_hdcp_mode);
	}
}

static void am_hdmitx_hdcp_disable(void)
{
	struct hdmitx_dev *hdmitx_dev = get_hdmitx_device();

	if (hdmitx_dev->tx_comm.hdcp_ctl_lvl >= 1)
		meson_hdcp_disable();
	HDMITX_INFO("hdcp disable manually\n");
}

static void am_hdmitx_hdcp_enable(void)
{
	struct hdmitx_dev *hdmitx_dev = get_hdmitx_device();
	int hdcp_type = HDCP_NULL;

	if (hdmitx_dev->tx_comm.hdcp_ctl_lvl >= 1) {
		hdcp_type = am_get_hdcp_exe_type();
		meson_hdcp_enable(hdcp_type);
	}
	HDMITX_INFO("hdcp enable manually\n");
}

static void am_hdmitx_hdcp_disconnect(void)
{
	struct hdmitx_dev *hdmitx_dev = get_hdmitx_device();

	if (hdmitx_dev->tx_comm.hdcp_ctl_lvl >= 1)
		meson_hdcp_disconnect();
	HDMITX_INFO("hdcp disconnect manually\n");
}

/***** debug interface end *****/

static void meson_hdcp_key_notify(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct meson_hdmitx_hdcp *meson_hdcp =
		container_of(dwork, struct meson_hdmitx_hdcp,
			     notify_work);

	meson_hdcp->drm_hdcp_cb.hdcp_notify(meson_hdcp->drm_hdcp_cb.data,
		HDCP_KEY_UPDATE, HDCP_AUTH_UNKNOWN);
}

void hdcp_key_check(struct timer_list *timer)
{
	struct meson_hdmitx_hdcp *meson_hdcp =
		container_of(timer, struct meson_hdmitx_hdcp,
			     daemon_load_timer);

	/*send notify when key load finished or timeout.*/
	if (meson_hdcp->hdcp22_daemon_state == HDCP22_DAEMON_DONE) {
		HDMITX_INFO("hdcp_tx22 load ready, stop timer\n");
		/*meson_hdcp->key_chk_cnt = 0;*/
		schedule_delayed_work(&meson_hdcp->notify_work, HZ / 100);
	} else if (meson_hdcp->key_chk_cnt++ < TIMER_CHK_CNT) {
		meson_hdcp->daemon_load_timer.expires = jiffies + TIMER_CHECK;
		add_timer(&meson_hdcp->daemon_load_timer);
	} else {
		HDMITX_INFO("hdcp_tx22 load timeout\n");
		meson_hdcp->hdcp22_daemon_state = HDCP22_DAEMON_TIMEOUT;
		/*meson_hdcp->key_chk_cnt = 0;*/
		schedule_delayed_work(&meson_hdcp->notify_work, 0);
	}
}

bool hdcp_tx22_daemon_ready(void)
{
	bool ret = false;

	if (meson_hdcp.hdcp22_daemon_state != HDCP22_DAEMON_DONE)
		ret = false;
	else
		ret = true;
	return ret;
}

static int drm_hdmitx_register_hdcp_cb(struct drm_hdmitx_hdcp_cb *hdcp_cb)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	hdev->drm_hdcp_cb.callback = hdcp_cb->callback;
	hdev->drm_hdcp_cb.data = hdcp_cb->data;
	return 0;
}

void meson_hdcp_init(void)
{
	int ret;
	struct drm_hdmitx_hdcp_cb hdcp_cb;
	struct hdmitx_dev *hdmitx_dev;

	meson_hdcp.hdcp_debug_type = 0x3;
	meson_hdcp.hdcp_report = HDCP_TX22_DISCONNECT;
	meson_hdcp.hdcp_en = 0;
	meson_hdcp.hdcp22_daemon_state = HDCP22_DAEMON_LOADING;
	init_waitqueue_head(&meson_hdcp.hdcp_comm_queue);
	meson_hdcp.hdcp_comm_device.minor = MISC_DYNAMIC_MINOR;
	meson_hdcp.hdcp_comm_device.name = "tee_comm_hdcp";
	meson_hdcp.hdcp_comm_device.fops = &hdcp_comm_file_operations;
	meson_hdcp.drm_hdcp_cb.hdcp_notify = NULL;
	meson_hdcp.drm_hdcp_cb.data = NULL;

	ret = misc_register(&meson_hdcp.hdcp_comm_device);
	if (ret < 0)
		HDMITX_INFO("%s [ERROR] misc_register fail\n", __func__);

	hdcp_cb.callback = meson_hdmitx_hdcp_cb;
	hdcp_cb.data = &meson_hdcp;
	drm_hdmitx_register_hdcp_cb(&hdcp_cb);

	hdmitx_dev = get_hdmitx_device();
	hdmitx_dev->hwop.am_hdmitx_set_hdcp_mode = am_hdmitx_set_hdcp_mode;
	hdmitx_dev->hwop.am_hdmitx_set_hdmi_mode = am_hdmitx_set_hdmi_mode;
	hdmitx_dev->hwop.am_hdmitx_set_out_mode = am_hdmitx_set_out_mode;
	hdmitx_dev->hwop.am_hdmitx_hdcp_disable = am_hdmitx_hdcp_disable;
	hdmitx_dev->hwop.am_hdmitx_hdcp_enable = am_hdmitx_hdcp_enable;
	hdmitx_dev->hwop.am_hdmitx_hdcp_disconnect = am_hdmitx_hdcp_disconnect;

	INIT_DELAYED_WORK(&meson_hdcp.notify_work, meson_hdcp_key_notify);
	meson_hdcp.key_chk_cnt = 0;
	timer_setup(&meson_hdcp.daemon_load_timer, hdcp_key_check, 0);
	meson_hdcp.daemon_load_timer.expires = jiffies + TIMER_CHECK;
	add_timer(&meson_hdcp.daemon_load_timer);
}

void meson_hdcp_exit(void)
{
	misc_deregister(&meson_hdcp.hdcp_comm_device);
	del_timer_sync(&meson_hdcp.daemon_load_timer);
	cancel_delayed_work(&meson_hdcp.notify_work);
}

/* bit[1]: hdcp22, bit[0]: hdcp14 */
/* apart from hdcp key, it will also check hdcp2.2 daemon status */
unsigned int meson_hdcp_get_tx_cap(void)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	unsigned int hdcp_tx_type = meson_hdcp_get_tx_key_version();

	/* check hdcp daemon: android or daemon is running */
	if (hdev->tx_comm.hdcp_ctl_lvl > 0 && !hdcp_tx22_daemon_ready())
		hdcp_tx_type &= 0x1;

	return hdcp_tx_type & 0x3;
}

/* bit[1]: hdcp22, bit[0]: hdcp14 */
unsigned int meson_hdcp_get_rx_cap(void)
{
	unsigned int ver = 0x0;
	struct hdmitx_dev *hdev = get_hdmitx_device();

	/* note that during hdcp1.4 authentication, read hdcp version
	 * of connected TV set(capable of hdcp2.2) may cause TV
	 * switch its hdcp mode, and flash screen. should not
	 * read hdcp version of sink during hdcp1.4 authentication.
	 * if hdcp1.4 authentication currently, force return hdcp1.4
	 */
	if (hdev->tx_comm.hdcp_mode == 1)
		return 0x1;
	/* if TX don't have HDCP22 key, skip RX hdcp22 ver */
	if (hdmitx_hw_cntl_ddc(&hdev->tx_hw.base,
		DDC_HDCP_22_LSTORE, 0) == 0)
		return 0x1;

	/* Detect RX support HDCP22 */
	ver = hdcp_rd_hdcp22_ver();
	/* Here, must assume RX support HDCP14, otherwise affect 1A-03 */
	if (ver)
		return 0x3;
	else
		return 0x1;
}

/* after TEE hdcp key valid, do hdcp22 init before tx22 start */
void drm_hdmitx_hdcp22_init(void)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	hdmitx_hdcp_do_work(hdev);
	hdmitx_hw_cntl_ddc(&hdev->tx_hw.base,
		DDC_HDCP_MUX_INIT, 2);
}

/* echo 1/2 > hdcp_mode */
void drm_hdmitx_enable_hdcp_mode(unsigned int content_type)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	enum hdmi_vic vic = HDMI_0_UNKNOWN;

	vic = hdmitx_hw_get_state(&hdev->tx_hw.base, STAT_VIDEO_VIC, 0);
	hdmitx_hw_cntl_ddc(&hdev->tx_hw.base, DDC_HDCP_GET_AUTH, 0);

	if (content_type == 1) {
		hdmitx_hw_cntl_ddc(&hdev->tx_hw.base, DDC_HDCP_MUX_INIT, 1);
		if (vic == HDMI_17_720x576p50_4x3 || vic == HDMI_18_720x576p50_16x9)
			usleep_range(500000, 500010);
		hdev->tx_comm.hdcp_mode = 1;
		hdmitx_hdcp_do_work(hdev);
		hdmitx_hw_cntl_ddc(&hdev->tx_hw.base,
			DDC_HDCP_OP, HDCP14_ON);
	} else if (content_type == 2) {
		hdev->tx_comm.hdcp_mode = 2;
		hdmitx_hdcp_do_work(hdev);
		/* for drm hdcp_tx22, esm init only once
		 * don't do HDCP22 IP reset after init done!
		 */
		hdmitx_hw_cntl_ddc(&hdev->tx_hw.base,
			DDC_HDCP_MUX_INIT, 3);
	}
}

/* echo -1 > hdcp_mode;echo stop14/22 > hdcp_ctrl */
void drm_hdmitx_disable_hdcp_mode(unsigned int content_type)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	hdmitx_hw_cntl_ddc(&hdev->tx_hw.base, DDC_HDCP_MUX_INIT, 1);
	hdmitx_hw_cntl_ddc(&hdev->tx_hw.base, DDC_HDCP_GET_AUTH, 0);

	if (content_type == 1) {
		hdmitx_hw_cntl_ddc(&hdev->tx_hw.base,
			DDC_HDCP_OP, HDCP14_OFF);
	} else if (content_type == 2) {
		hdmitx_hw_cntl_ddc(&hdev->tx_hw.base,
			DDC_HDCP_OP, HDCP22_OFF);
	}

	hdev->tx_comm.hdcp_mode = 0;
	hdmitx_hdcp_do_work(hdev);
	hdmitx_current_status(HDMITX_HDCP_NOT_ENABLED);
}

