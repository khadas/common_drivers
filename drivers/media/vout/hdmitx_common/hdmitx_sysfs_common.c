// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_dev_common.h>
#include "hdmitx_sysfs_common.h"

/*!!Only one instance supported.*/
static struct hdmitx_common *global_tx_common;
static struct hdmitx_hw_common *global_tx_hw;

/************************common sysfs*************************/
static ssize_t attr_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	int pos = 0;
	char fmt_attr[16];

	hdmitx_get_attr(global_tx_common, fmt_attr);
	pos = snprintf(buf, PAGE_SIZE, "%s\n\r", fmt_attr);

	return pos;
}

static ssize_t attr_store(struct device *dev,
		   struct device_attribute *attr,
		   const char *buf, size_t count)
{
	hdmitx_setup_attr(global_tx_common, buf);
	return count;
}

static DEVICE_ATTR_RW(attr);

static ssize_t hpd_state_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	int pos = 0;

	pos += snprintf(buf + pos, PAGE_SIZE, "%d",
		global_tx_common->hpd_state);
	return pos;
}

static DEVICE_ATTR_RO(hpd_state);

/* rawedid attr */
static ssize_t rawedid_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	int pos = 0;
	int i;
	int num;
	int block_no = 0;

	/* prevent null prt */
	if (!global_tx_common->edid_ptr)
		global_tx_common->edid_ptr = global_tx_common->EDID_buf;

	block_no = global_tx_common->edid_ptr[126];
	if (block_no == 1)
		if (global_tx_common->edid_ptr[128 + 4] == 0xe2 &&
			global_tx_common->edid_ptr[128 + 5] == 0x78)
			block_no = global_tx_common->edid_ptr[128 + 6];	//EEODB
	if (block_no < 8)
		num = (block_no + 1) * 128;
	else
		num = 8 * 128;

	for (i = 0; i < num; i++)
		pos += snprintf(buf + pos, PAGE_SIZE, "%02x",
				global_tx_common->edid_ptr[i]);

	pos += snprintf(buf + pos, PAGE_SIZE, "\n");

	return pos;
}

static DEVICE_ATTR_RO(rawedid);

/*
 * edid_parsing attr
 * If RX edid data are all correct, HEAD(00 ff ff ff ff ff ff 00), checksum,
 * version, etc), then return "ok". Otherwise, "ng"
 * Actually, in some old televisions, EDID is stored in EEPROM.
 * some bits in EEPROM may reverse with time.
 * But it does not affect  edid_parsing.
 * Therefore, we consider the RX edid data are all correct, return "OK"
 */
static ssize_t edid_parsing_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	int pos = 0;

	if (hdmitx_edid_validate(global_tx_common->edid_ptr) == 0)
		pos += snprintf(buf + pos, PAGE_SIZE, "ok\n");
	else
		pos += snprintf(buf + pos, PAGE_SIZE, "ng\n");

	return pos;
}

static DEVICE_ATTR_RO(edid_parsing);

static ssize_t edid_show(struct device *dev,
			 struct device_attribute *attr,
			 char *buf)
{
	return hdmitx_print_sink_cap(global_tx_common, buf, PAGE_SIZE);
}

static ssize_t edid_store(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf, size_t count)
{
	u32 argn = 0;
	char *p = NULL, *para = NULL, *argv[8] = {NULL};
	u32 path_length = 0;
	u32 index = 0, tmp = 0;

	p = kstrdup(buf, GFP_KERNEL);
	if (!p)
		return count;

	do {
		para = strsep(&p, " ");
		if (para) {
			argv[argn] = para;
			argn++;
			if (argn > 7)
				break;
		}
	} while (para);

	if (buf[0] == 'h') {
		int i;

		pr_info("EDID hash value:\n");
		for (i = 0; i < 20; i++)
			pr_info("%02x", global_tx_common->EDID_hash[i]);
		pr_info("\n");
	}
	if (buf[0] == 'd') {
		int ii, jj;
		unsigned long block_idx;
		int ret;

		ret = kstrtoul(buf + 1, 16, &block_idx);
		if (block_idx < EDID_MAX_BLOCK) {
			for (ii = 0; ii < 8; ii++) {
				for (jj = 0; jj < 16; jj++) {
					index = block_idx * 128 + ii * 16 + jj;
					tmp = global_tx_common->EDID_buf1[index];
					pr_info("%02x ", tmp);
				}
				pr_info("\n");
			}
		pr_info("\n");
	}
	}
	if (buf[0] == 'e') {
		int ii, jj;
		unsigned long block_idx;
		int ret;

		ret = kstrtoul(buf + 1, 16, &block_idx);
		if (block_idx < EDID_MAX_BLOCK) {
			for (ii = 0; ii < 8; ii++) {
				for (jj = 0; jj < 16; jj++) {
					index = block_idx * 128 + ii * 16 + jj;
					tmp = global_tx_common->EDID_buf1[index];
					pr_info("%02x ", tmp);
				}
				pr_info("\n");
			}
			pr_info("\n");
		}
	}

	if (!strncmp(argv[0], "save", strlen("save"))) {
		u32 type = 0;

		if (argn != 3) {
			pr_info("[%s] cmd format: save bin/txt edid_file_path\n",
				__func__);
			goto PROCESS_END;
		}
		if (!strncmp(argv[1], "bin", strlen("bin")))
			type = 1;
		else if (!strncmp(argv[1], "txt", strlen("txt")))
			type = 2;

		if (type == 1 || type == 2) {
			/* clean '\n' from file path*/
			path_length = strlen(argv[2]);
			if (argv[2][path_length - 1] == '\n')
				argv[2][path_length - 1] = 0x0;

			hdmitx_save_edid_file(global_tx_common->EDID_buf, argv[2]);
		}
	} else if (!strncmp(argv[0], "load", strlen("load"))) {
		if (argn != 2) {
			pr_info("[%s] cmd format: load edid_file_path\n",
				__func__);
			goto PROCESS_END;
		}

		/* clean '\n' from file path*/
		path_length = strlen(argv[1]);
		if (argv[1][path_length - 1] == '\n')
			argv[1][path_length - 1] = 0x0;
		hdmitx_load_edid_file(argv[1]);
	}

PROCESS_END:
	kfree(p);
	return count;
}

static DEVICE_ATTR_RW(edid);

static ssize_t contenttype_cap_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct rx_cap *prxcap = &global_tx_common->rxcap;

	if (prxcap->cnc0)
		pos += snprintf(buf + pos, PAGE_SIZE, "graphics\n\r");
	if (prxcap->cnc1)
		pos += snprintf(buf + pos, PAGE_SIZE, "photo\n\r");
	if (prxcap->cnc2)
		pos += snprintf(buf + pos, PAGE_SIZE, "cinema\n\r");
	if (prxcap->cnc3)
		pos += snprintf(buf + pos, PAGE_SIZE, "game\n\r");

	return pos;
}

static DEVICE_ATTR_RO(contenttype_cap);

static ssize_t _hdr_cap_show(struct device *dev,
			     struct device_attribute *attr,
			     char *buf,
			     const struct hdr_info *hdr)
{
	int pos = 0;
	unsigned int i, j;
	int hdr10plugsupported = 0;
	const struct cuva_info *cuva = &hdr->cuva_info;
	const struct hdr10_plus_info *hdr10p = &hdr->hdr10plus_info;

	if (hdr10p->ieeeoui == HDR10_PLUS_IEEE_OUI &&
		hdr10p->application_version != 0xFF)
		hdr10plugsupported = 1;
	pos += snprintf(buf + pos, PAGE_SIZE, "HDR10Plus Supported: %d\n",
		hdr10plugsupported);
	pos += snprintf(buf + pos, PAGE_SIZE, "HDR Static Metadata:\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "    Supported EOTF:\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "        Traditional SDR: %d\n",
		!!(hdr->hdr_support & 0x1));
	pos += snprintf(buf + pos, PAGE_SIZE, "        Traditional HDR: %d\n",
		!!(hdr->hdr_support & 0x2));
	pos += snprintf(buf + pos, PAGE_SIZE, "        SMPTE ST 2084: %d\n",
		!!(hdr->hdr_support & 0x4));
	pos += snprintf(buf + pos, PAGE_SIZE, "        Hybrid Log-Gamma: %d\n",
		!!(hdr->hdr_support & 0x8));
	pos += snprintf(buf + pos, PAGE_SIZE, "    Supported SMD type1: %d\n",
		hdr->static_metadata_type1);
	pos += snprintf(buf + pos, PAGE_SIZE, "    Luminance Data\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "        Max: %d\n",
		hdr->lumi_max);
	pos += snprintf(buf + pos, PAGE_SIZE, "        Avg: %d\n",
		hdr->lumi_avg);
	pos += snprintf(buf + pos, PAGE_SIZE, "        Min: %d\n\n",
		hdr->lumi_min);
	pos += snprintf(buf + pos, PAGE_SIZE, "HDR Dynamic Metadata:");

	for (i = 0; i < 4; i++) {
		if (hdr->dynamic_info[i].type == 0)
			continue;
		pos += snprintf(buf + pos, PAGE_SIZE,
			"\n    metadata_version: %x\n",
			hdr->dynamic_info[i].type);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"        support_flags: %x\n",
			hdr->dynamic_info[i].support_flags);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"        optional_fields:");
		for (j = 0; j <
			(hdr->dynamic_info[i].of_len - 3); j++)
			pos += snprintf(buf + pos, PAGE_SIZE, " %x",
				hdr->dynamic_info[i].optional_fields[j]);
	}

	pos += snprintf(buf + pos, PAGE_SIZE, "\n\ncolorimetry_data: %x\n",
		hdr->colorimetry_support);
	if (cuva->ieeeoui == CUVA_IEEEOUI) {
		pos += snprintf(buf + pos, PAGE_SIZE, "CUVA supported: 1\n");
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  system_start_code: %u\n", cuva->system_start_code);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  version_code: %u\n", cuva->version_code);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  display_maximum_luminance: %u\n",
			cuva->display_max_lum);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  display_minimum_luminance: %u\n",
			cuva->display_min_lum);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  monitor_mode_support: %u\n", cuva->monitor_mode_sup);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  rx_mode_support: %u\n", cuva->rx_mode_sup);
		for (i = 0; i < (cuva->length + 1); i++)
			pos += snprintf(buf + pos, PAGE_SIZE, "%02x",
				cuva->rawdata[i]);
		pos += snprintf(buf + pos, PAGE_SIZE, "\n");
	}
	return pos;
}

static ssize_t hdr_cap_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	int pos = 0;
	const struct hdr_info *info = &global_tx_common->rxcap.hdr_info;

	if (global_tx_common->hdr_priority == 2) {
		pos += snprintf(buf + pos, PAGE_SIZE,
			"mask rx hdr capability\n");
		return pos;
	}

	return _hdr_cap_show(dev, attr, buf, info);
}

static DEVICE_ATTR_RO(hdr_cap);

static ssize_t hdr_cap2_show(struct device *dev,
			    struct device_attribute *attr,
			    char *buf)
{
	const struct hdr_info *info2 = &global_tx_common->rxcap.hdr_info2;

	return _hdr_cap_show(dev, attr, buf, info2);
}

static DEVICE_ATTR_RO(hdr_cap2);

static ssize_t _show_dv_cap(struct device *dev,
			    struct device_attribute *attr,
			    char *buf,
			    const struct dv_info *dv)
{
	int pos = 0;
	int i;

	if (dv->ieeeoui != DV_IEEE_OUI || dv->block_flag != CORRECT) {
		pos += snprintf(buf + pos, PAGE_SIZE,
			"The Rx don't support DolbyVision\n");
		return pos;
	}
	pos += snprintf(buf + pos, PAGE_SIZE,
		"DolbyVision RX support list:\n");

	if (dv->ver == 0) {
		pos += snprintf(buf + pos, PAGE_SIZE,
			"VSVDB Version: V%d\n", dv->ver);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"2160p%shz: 1\n",
			dv->sup_2160p60hz ? "60" : "30");
		pos += snprintf(buf + pos, PAGE_SIZE,
			"Support mode:\n");
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  DV_RGB_444_8BIT\n");
		if (dv->sup_yuv422_12bit)
			pos += snprintf(buf + pos, PAGE_SIZE,
				"  DV_YCbCr_422_12BIT\n");
	}
	if (dv->ver == 1) {
		pos += snprintf(buf + pos, PAGE_SIZE,
			"VSVDB Version: V%d(%d-byte)\n",
			dv->ver, dv->length + 1);
		if (dv->length == 0xB) {
			pos += snprintf(buf + pos, PAGE_SIZE,
				"2160p%shz: 1\n",
				dv->sup_2160p60hz ? "60" : "30");
		pos += snprintf(buf + pos, PAGE_SIZE,
			"Support mode:\n");
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  DV_RGB_444_8BIT\n");
		if (dv->sup_yuv422_12bit)
			pos += snprintf(buf + pos, PAGE_SIZE,
			"  DV_YCbCr_422_12BIT\n");
		if (dv->low_latency == 0x01)
			pos += snprintf(buf + pos, PAGE_SIZE,
				"  LL_YCbCr_422_12BIT\n");
		}

		if (dv->length == 0xE) {
			pos += snprintf(buf + pos, PAGE_SIZE,
				"2160p%shz: 1\n",
				dv->sup_2160p60hz ? "60" : "30");
			pos += snprintf(buf + pos, PAGE_SIZE,
				"Support mode:\n");
			pos += snprintf(buf + pos, PAGE_SIZE,
				"  DV_RGB_444_8BIT\n");
			if (dv->sup_yuv422_12bit)
				pos += snprintf(buf + pos, PAGE_SIZE,
				"  DV_YCbCr_422_12BIT\n");
		}
	}
	if (dv->ver == 2) {
		pos += snprintf(buf + pos, PAGE_SIZE,
			"VSVDB Version: V%d\n", dv->ver);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"2160p%shz: 1\n",
			dv->sup_2160p60hz ? "60" : "30");
		pos += snprintf(buf + pos, PAGE_SIZE,
			"Support mode:\n");
		if (dv->Interface != 0x00 && dv->Interface != 0x01) {
			pos += snprintf(buf + pos, PAGE_SIZE,
				"  DV_RGB_444_8BIT\n");
			if (dv->sup_yuv422_12bit)
				pos += snprintf(buf + pos, PAGE_SIZE,
					"  DV_YCbCr_422_12BIT\n");
		}
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  LL_YCbCr_422_12BIT\n");
		if (dv->Interface == 0x01 || dv->Interface == 0x03) {
			if (dv->sup_10b_12b_444 == 0x1) {
				pos += snprintf(buf + pos, PAGE_SIZE,
					"  LL_RGB_444_10BIT\n");
			}
			if (dv->sup_10b_12b_444 == 0x2) {
				pos += snprintf(buf + pos, PAGE_SIZE,
					"  LL_RGB_444_12BIT\n");
			}
		}
	}
	pos += snprintf(buf + pos, PAGE_SIZE,
		"IEEEOUI: 0x%06x\n", dv->ieeeoui);
	pos += snprintf(buf + pos, PAGE_SIZE,
		"EMP: %d\n", dv->dv_emp_cap);
	pos += snprintf(buf + pos, PAGE_SIZE, "VSVDB: ");
	for (i = 0; i < (dv->length + 1); i++)
		pos += snprintf(buf + pos, PAGE_SIZE, "%02x",
		dv->rawdata[i]);
	pos += snprintf(buf + pos, PAGE_SIZE, "\n");
	return pos;
}

static ssize_t dv_cap_show(struct device *dev,
			   struct device_attribute *attr,
			   char *buf)
{
	int pos = 0;
	const struct dv_info *dv = &global_tx_common->rxcap.dv_info;

	if (dv->ieeeoui != DV_IEEE_OUI || global_tx_common->hdr_priority) {
		pos += snprintf(buf + pos, PAGE_SIZE,
			"The Rx don't support DolbyVision\n");
		return pos;
	}
	return _show_dv_cap(dev, attr, buf, dv);
}

static DEVICE_ATTR_RO(dv_cap);

static ssize_t dv_cap2_show(struct device *dev,
			    struct device_attribute *attr,
			    char *buf)
{
	const struct dv_info *dv2 = &global_tx_common->rxcap.dv_info2;

	return _show_dv_cap(dev, attr, buf, dv2);
}

static DEVICE_ATTR_RO(dv_cap2);

static ssize_t frac_rate_policy_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{
	int val = 0;

	if (isdigit(buf[0])) {
		val = buf[0] - '0';
		pr_info("set frac_rate_policy as %d\n", val);
		if (val == 0 || val == 1)
			global_tx_common->frac_rate_policy = val;
		else
			pr_info("only accept as 0 or 1\n");
	}

	return count;
}

static ssize_t frac_rate_policy_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int pos = 0;

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n",
		global_tx_common->frac_rate_policy);

	return pos;
}

static DEVICE_ATTR_RW(frac_rate_policy);

static ssize_t avmute_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret = 0;
	int pos = 0;

	ret = global_tx_hw->cntlmisc(global_tx_hw, MISC_READ_AVMUTE_OP, 0);
	pos += snprintf(buf + pos, PAGE_SIZE, "%d", ret);

	return pos;
}

/*
 *  1: set avmute
 * -1: clear avmute
 *  0: off avmute
 */
static ssize_t avmute_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int cmd = OFF_AVMUTE;
	static int mask0;
	static int mask1;
	static DEFINE_MUTEX(avmute_mutex);
	/*
	 *unsigned int mute_us =
	 *	hdmitx_device.debug_param.avmute_frame * hdmitx_get_frame_duration();
	 */
	pr_info("%s %s\n", __func__, buf);
	mutex_lock(&avmute_mutex);
	if (strncmp(buf, "-1", 2) == 0) {
		cmd = CLR_AVMUTE;
		mask0 = -1;
	} else if (strncmp(buf, "0", 1) == 0) {
		cmd = OFF_AVMUTE;
		mask0 = 0;
	} else if (strncmp(buf, "1", 1) == 0) {
		cmd = SET_AVMUTE;
		mask0 = 1;
	}
	if (strncmp(buf, "r-1", 3) == 0) {
		cmd = CLR_AVMUTE;
		mask1 = -1;
	} else if (strncmp(buf, "r0", 2) == 0) {
		cmd = OFF_AVMUTE;
		mask1 = 0;
	} else if (strncmp(buf, "r1", 2) == 0) {
		cmd = SET_AVMUTE;
		mask1 = 1;
	}
	if (mask0 == 1 || mask1 == 1)
		cmd = SET_AVMUTE;
	else if ((mask0 == -1) && (mask1 == -1))
		cmd = CLR_AVMUTE;

	hdmitx_hw_avmute(global_tx_hw, cmd);
	/*
	 *if (cmd == SET_AVMUTE && hdmitx_device.debug_param.avmute_frame > 0)
	 *	msleep(mute_us / 1000);
	 */
	mutex_unlock(&avmute_mutex);

	return count;
}

static DEVICE_ATTR_RW(avmute);

/*
 *  1: enable hdmitx phy
 *  0: disable hdmitx phy
 */
static ssize_t phy_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	int cmd = TMDS_PHY_ENABLE;

	pr_info("%s %s\n", __func__, buf);

	if (strncmp(buf, "0", 1) == 0)
		cmd = TMDS_PHY_DISABLE;
	else if (strncmp(buf, "1", 1) == 0)
		cmd = TMDS_PHY_ENABLE;
	else
		pr_info("set phy wrong: %s\n", buf);

	global_tx_hw->cntlmisc(global_tx_hw, MISC_TMDS_PHY_OP, cmd);
	return count;
}

static ssize_t phy_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int pos = 0;
	int state = 0;

	state = global_tx_hw->getstate(global_tx_hw, STAT_TX_PHY, 0);
	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n", state);

	return pos;
}

static DEVICE_ATTR_RW(phy);

static ssize_t contenttype_mode_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int pos = 0;
	static char * const ct_names[] = {
		"off",
		"game",
		"graphics",
		"photo",
		"cinema",
	};

	if (global_tx_common->ct_mode >= 0 &&
		global_tx_common->ct_mode < ARRAY_SIZE(ct_names))
		pos += snprintf(buf + pos, PAGE_SIZE, "%s\n\r",
					ct_names[global_tx_common->ct_mode]);

	return pos;
}

static inline int com_str(const char *buf, const char *str)
{
	return strncmp(buf, str, strlen(str)) == 0;
}

static ssize_t contenttype_mode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	u32 ct_mode = SET_CT_OFF;

	pr_info("hdmitx: store contenttype_mode as %s\n", buf);

	if (global_tx_common->allm_mode == 1) {
		global_tx_common->allm_mode = 0;
		hdmitx_dev_setup_vsif_packet(global_tx_common, global_tx_hw, VT_ALLM, 0, NULL);
	}
	hdmitx_dev_setup_vsif_packet(global_tx_common, global_tx_hw, VT_HDMI14_4K, 1, NULL);

	if (com_str(buf, "0") || com_str(buf, "off"))
		ct_mode = SET_CT_OFF;
	else if (com_str(buf, "1") || com_str(buf, "game"))
		ct_mode = SET_CT_GAME;
	else if (com_str(buf, "2") || com_str(buf, "graphics"))
		ct_mode = SET_CT_GRAPHICS;
	else if (com_str(buf, "3") || com_str(buf, "photo"))
		ct_mode = SET_CT_PHOTO;
	else if (com_str(buf, "4") || com_str(buf, "cinema"))
		ct_mode = SET_CT_CINEMA;

	global_tx_hw->cntlconfig(global_tx_hw, CONF_CT_MODE, ct_mode);
	global_tx_common->ct_mode = ct_mode;

	return count;
}

static DEVICE_ATTR_RW(contenttype_mode);

/*************************tx20 sysfs*************************/

/*************************tx21 sysfs*************************/

int hdmitx_sysfs_common_create(struct device *dev,
		struct hdmitx_common *tx_comm,
		struct hdmitx_hw_common *tx_hw)
{
	int ret = 0;

	global_tx_common = tx_comm;
	global_tx_hw = tx_hw;

	ret = device_create_file(dev, &dev_attr_attr);
	ret = device_create_file(dev, &dev_attr_hpd_state);
	ret = device_create_file(dev, &dev_attr_frac_rate_policy);

	ret = device_create_file(dev, &dev_attr_rawedid);
	ret = device_create_file(dev, &dev_attr_edid_parsing);
	ret = device_create_file(dev, &dev_attr_edid);

	ret = device_create_file(dev, &dev_attr_contenttype_cap);
	ret = device_create_file(dev, &dev_attr_hdr_cap);
	ret = device_create_file(dev, &dev_attr_hdr_cap2);
	ret = device_create_file(dev, &dev_attr_dv_cap);
	ret = device_create_file(dev, &dev_attr_dv_cap2);

	ret = device_create_file(dev, &dev_attr_avmute);
	ret = device_create_file(dev, &dev_attr_phy);

	ret = device_create_file(dev, &dev_attr_contenttype_mode);

	return ret;
}

int hdmitx_sysfs_common_destroy(struct device *dev)
{
	device_remove_file(dev, &dev_attr_attr);
	device_remove_file(dev, &dev_attr_hpd_state);
	device_remove_file(dev, &dev_attr_frac_rate_policy);

	device_remove_file(dev, &dev_attr_rawedid);
	device_remove_file(dev, &dev_attr_edid_parsing);
	device_remove_file(dev, &dev_attr_edid);

	device_remove_file(dev, &dev_attr_contenttype_cap);
	device_remove_file(dev, &dev_attr_hdr_cap);
	device_remove_file(dev, &dev_attr_hdr_cap2);
	device_remove_file(dev, &dev_attr_dv_cap);
	device_remove_file(dev, &dev_attr_dv_cap2);

	device_remove_file(dev, &dev_attr_avmute);
	device_remove_file(dev, &dev_attr_phy);

	device_remove_file(dev, &dev_attr_contenttype_mode);

	global_tx_common = 0;
	global_tx_hw = 0;

	return 0;
}

