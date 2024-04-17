// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_irq.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/ctype.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/pm_runtime.h>

#include <linux/amlogic/media/frc/frc_reg.h>
#include <linux/amlogic/media/frc/frc_common.h>
#include <linux/amlogic/media/vout/vinfo.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/amlogic/power_domain.h>

#include "frc_drv.h"
#include "frc_dbg.h"
#include "frc_buf.h"
#include "frc_hw.h"
#include "frc_proc.h"
#include "frc_rdma.h"

int frc_dbg_ctrl;
module_param(frc_dbg_ctrl, int, 0664);
MODULE_PARM_DESC(frc_dbg_ctrl, "frc_dbg_ctrl");

static void frc_debug_parse_param(char *buf_orig, char **parm)
{
	char *ps, *token;
	char delim1[3] = " ";
	char delim2[2] = "\n";
	unsigned int n = 0;

	ps = buf_orig;
	strcat(delim1, delim2);
	while (1) {
		token = strsep(&ps, delim1);
		if (!token)
			break;
		if (*token == '\0')
			continue;
		parm[n++] = token;
	}
}

ssize_t frc_reg_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	pr_frc(0, "read:  echo r reg > /sys/class/frc/reg\n");
	pr_frc(0, "write: echo w reg value > /sys/class/frc/reg\n");
	pr_frc(0, "dump:  echo d reg length > /sys/class/frc/reg\n");
	return 0;
}

ssize_t frc_reg_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	frc_reg_io(buf);
	return count;
}

ssize_t frc_tool_debug_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct tool_debug_s *read_parm = NULL;
	struct frc_dev_s *devp = get_frc_devp();

	read_parm = &devp->tool_dbg;
	return sprintf(buf, "[0x%x] = 0x%x\n",
		read_parm->reg_read, read_parm->reg_read_val);
}

ssize_t frc_tool_debug_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	struct frc_dev_s *devp = get_frc_devp();

	frc_tool_dbg_store(devp, buf);
	return count;
}

ssize_t frc_debug_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct frc_dev_s *devp = get_frc_devp();

	return frc_debug_if_help(devp, buf);
}

ssize_t frc_debug_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	struct frc_dev_s *devp = get_frc_devp();

	frc_debug_if(devp, buf, count);

	return count;
}

ssize_t frc_buf_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct frc_dev_s *devp = get_frc_devp();

	return frc_debug_buf_if_help(devp, buf);
}

ssize_t frc_buf_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	struct frc_dev_s *devp = get_frc_devp();

	frc_debug_buf_if(devp, buf, count);

	return count;
}

ssize_t frc_rdma_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct frc_dev_s *devp = get_frc_devp();

	return frc_debug_rdma_if_help(devp, buf);
}

ssize_t frc_rdma_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	struct frc_dev_s *devp = get_frc_devp();

	frc_debug_rdma_if(devp, buf, count);

	return count;
}

ssize_t frc_param_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct frc_dev_s *devp = get_frc_devp();

	return frc_debug_param_if_help(devp, buf);
}

ssize_t frc_param_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	struct frc_dev_s *devp = get_frc_devp();

	frc_debug_param_if(devp, buf, count);

	return count;
}

ssize_t frc_other_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct frc_dev_s *devp = get_frc_devp();

	return frc_debug_other_if_help(devp, buf);
}

ssize_t frc_other_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	struct frc_dev_s *devp = get_frc_devp();

	frc_debug_other_if(devp, buf, count);

	return count;
}

void frc_status(struct frc_dev_s *devp)
{
	struct frc_fw_data_s *fw_data;
	struct vinfo_s *vinfo = get_current_vinfo();

	fw_data = (struct frc_fw_data_s *)devp->fw_data;
	pr_frc(0, "drv_version= %s\n", FRC_FW_VER);
	pr_frc(0, "ko_version= %s\n", fw_data->frc_alg_ver);
	pr_frc(0, "chip= %d,  vendor= %d\n", fw_data->frc_top_type.chip,
			fw_data->frc_fw_alg_ctrl.frc_algctrl_u8vendor);
	pr_frc(0, "pw_state= %d clk_state= %d, hw_pos= %d(1:after), fw_pause= %d\n",
			devp->power_on_flag, devp->clk_state,
			devp->frc_hw_pos, devp->frc_fw_pause);
	pr_frc(0, "work_state= %d (%s), new= %d\n", devp->frc_sts.state,
			frc_state_ary[devp->frc_sts.state], devp->frc_sts.new_state);
	pr_frc(0, "auto_ctrl= %d\n", devp->frc_sts.auto_ctrl);
	pr_frc(0, "memc_level= %d(%d)\n", fw_data->frc_top_type.frc_memc_level,
			fw_data->frc_top_type.frc_memc_level_1);
	pr_frc(0, "secure_mode= %d, buf secured= %d\n",
			devp->in_sts.secure_mode, devp->buf.secured);
	pr_frc(0, "frc_get_vd_flag= 0x%X(game:0/pc:1/pic:2/hbw:3/limsz:4/vlock:5)\n",
			devp->in_sts.st_flag);
	pr_frc(0, "(in_size_err:6/high_freq:7/zero_freq:8/limfq:9)\n");
	pr_frc(0, "dc_rate(me:%d,mc_y:%d,mc_c:%d,mcdw_y:%d,mcdw_c:%d), real total size:%d\n",
			devp->buf.me_comprate, devp->buf.mc_y_comprate,
			devp->buf.mc_c_comprate, devp->buf.mcdw_y_comprate,
			devp->buf.mcdw_c_comprate, devp->buf.real_total_size);
	pr_frc(0, "mcdw_ratio= 0x%x\n", devp->buf.mcdw_size_rate);
	pr_frc(0, "memc(mcdw)_loss_en=0x%x\n",
			fw_data->frc_top_type.memc_loss_en);
	pr_frc(0, "prot_mode= %d\n", devp->prot_mode);
	pr_frc(0, "high_freq_flash= %d\n", devp->in_sts.high_freq_flash);
	pr_frc(0, "force_en= %d, force_hsize= %d, force_vsize= %d\n",
			devp->force_size.force_en, devp->force_size.force_hsize,
			devp->force_size.force_vsize);
	pr_frc(0, "dbg_en= %d ratio_mode= 0x%x, dbg_hsize= %d, vsize= %d\n",
			devp->dbg_force_en, devp->dbg_in_out_ratio,
			devp->dbg_input_hsize, devp->dbg_input_vsize);
	pr_frc(0, "vf_sts= %d, vf_type= 0x%x, signal_type= 0x%x, source_type= 0x%x\n",
			devp->in_sts.vf_sts,
			devp->in_sts.vf_type, devp->in_sts.signal_type, devp->in_sts.source_type);
	pr_frc(0, "vf_rate= %d (duration= %d)\n", frc_check_vf_rate(devp->in_sts.duration, devp),
				devp->in_sts.duration);
	pr_frc(0, "vpu_int vs_duration= %dus timestamp= %ld\n",
			devp->vs_duration, (ulong)devp->vs_timestamp);
	pr_frc(0, "frc_in vs_duration= %dus timestamp= %ld\n",
	       devp->in_sts.vs_duration, (ulong)devp->in_sts.vs_timestamp);
	pr_frc(0, "frc_in isr vs_cnt= %d, vs_tsk_cnt:%d, inp_err cnt:%d vd_mute cnt:%d\n",
		devp->in_sts.vs_cnt, devp->in_sts.vs_tsk_cnt,
		devp->frc_sts.inp_undone_cnt, devp->frc_sts.video_mute_cnt);
	pr_frc(0, "frc_out vs_duration= %dus timestamp= %ld\n",
	       devp->out_sts.vs_duration, (ulong)devp->out_sts.vs_timestamp);
	pr_frc(0, "frc_out isr vs_cnt= %d, vs_tsk_cnt= %d, err_cnt= (me:%d,mc:%d,vp:%d)\n",
		devp->out_sts.vs_cnt, devp->out_sts.vs_tsk_cnt,
		devp->frc_sts.me_undone_cnt, devp->frc_sts.mc_undone_cnt,
		devp->frc_sts.vp_undone_cnt);
	pr_frc(0, "frc_st vs_cnt:%d vf_repeat_cnt:%d vf_null_cnt:%d\n", devp->frc_sts.vs_cnt,
				devp->in_sts.vf_repeat_cnt, devp->in_sts.vf_null_cnt);
	pr_frc(0, "vout sync_duration_num= %d sync_duration_den= %d out_hz= %d\n",
			vinfo->sync_duration_num, vinfo->sync_duration_den,
			vinfo->sync_duration_num / vinfo->sync_duration_den);
	pr_frc(0, "film_mode= %d\n", frc_check_film_mode(devp));
	pr_frc(0, "mc_fallback= %d\n", fw_data->frc_fw_alg_ctrl.frc_algctrl_u8mcfb);
	pr_frc(0, "frm_buffer_num= %d\n", fw_data->frc_top_type.frc_fb_num);
	pr_frc(0, "n2m_mode= %d\n", devp->in_out_ratio);
	pr_frc(0, "rdma_en= %d\n", fw_data->frc_top_type.rdma_en);
	pr_frc(0, "frc_in hsize= %d vsize= %d\n",
			devp->in_sts.in_hsize, devp->in_sts.in_vsize);
	pr_frc(0, "frc_out hsize= %d vsize= %d\n",
			devp->out_sts.vout_width, devp->out_sts.vout_height);
	pr_frc(0, "vfb(0x1cb4/0x14ca)= %d\n", fw_data->frc_top_type.vfb);
	pr_frc(0, "is_me1mc4= %d\n", fw_data->frc_top_type.is_me1mc4);
	pr_frc(0, "me_hold_line= %d\n", fw_data->holdline_parm.me_hold_line);
	pr_frc(0, "mc_hold_line= %d\n", fw_data->holdline_parm.mc_hold_line);
	pr_frc(0, "inp_hold_line= %d\n", fw_data->holdline_parm.inp_hold_line);
	pr_frc(0, "reg_post_dly_vofst= %d\n", fw_data->holdline_parm.reg_post_dly_vofst);
	pr_frc(0, "reg_mc_dly_vofst0= %d\n", fw_data->holdline_parm.reg_mc_dly_vofst0);
	pr_frc(0, "get_video_latency= %d\n", frc_get_video_latency());
	pr_frc(0, "get_frc_adj_me_out_line= %d\n", devp->out_line);
}

ssize_t frc_debug_if_help(struct frc_dev_s *devp, char *buf)
{
	ssize_t len = 0;

	struct frc_fw_data_s *fw_data;

	fw_data = (struct frc_fw_data_s *)devp->fw_data;

	len += sprintf(buf + len, "frc_debug info:\n");

	len += sprintf(buf + len, "status\t\t=%d\n", devp->frc_sts.state);
	len += sprintf(buf + len, "dbg_level\t=%d\n", frc_dbg_en);//style for tool
	len += sprintf(buf + len, "dbg_mode\t=%d\n", devp->frc_sts.state);
	len += sprintf(buf + len, "test_pattern\t=%d\n", devp->frc_test_ptn);
	len += sprintf(buf + len, "frc_pos\t\t=%d\n", devp->frc_hw_pos);
	len += sprintf(buf + len, "frc_pause\t=%d\n", devp->frc_fw_pause);
	len += sprintf(buf + len, "film_mode\t=%d\n", devp->film_mode);
	len += sprintf(buf + len, "ud_dbg\t\t=%d %d %d %d\n",
		devp->ud_dbg.inp_ud_dbg_en, devp->ud_dbg.meud_dbg_en,
		devp->ud_dbg.mcud_dbg_en, devp->ud_dbg.vpud_dbg_en);
	len += sprintf(buf + len, "auto_ctrl\t=%d\n", devp->frc_sts.auto_ctrl);
	len += sprintf(buf + len, "memc_lossy\t=0x%x\n", fw_data->frc_top_type.memc_loss_en);
	len += sprintf(buf + len, "power_ctrl\t=%d\n", devp->power_on_flag);
	len += sprintf(buf + len, "memc_level\t=%d\n", fw_data->frc_top_type.frc_memc_level);
	len += sprintf(buf + len, "vendor\t\t=%d\n", fw_data->frc_fw_alg_ctrl.frc_algctrl_u8vendor);
	len += sprintf(buf + len, "mcfb\t\t=%d\n", fw_data->frc_fw_alg_ctrl.frc_algctrl_u8mcfb);
	len += sprintf(buf + len, "filmset\t\t=%d\n", fw_data->frc_fw_alg_ctrl.frc_algctrl_u32film);
	len += sprintf(buf + len, "set_n2m\t\t=%d\n", devp->in_out_ratio);
	len += sprintf(buf + len, "auto_n2m\t=%d\n", devp->auto_n2m);
	len += sprintf(buf + len, "set_mcdw\t=(read reg check)\n");
	len += sprintf(buf + len, "test2\t\t=%d\n", devp->test2);

	return len;
}

void frc_debug_if(struct frc_dev_s *devp, const char *buf, size_t count)
{
	char *buf_orig, *parm[47] = {NULL};
	int val1;
	struct frc_fw_data_s *fw_data;

	if (!devp)
		return;

	if (!buf)
		return;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	if (!buf_orig)
		return;

	fw_data = (struct frc_fw_data_s *)devp->fw_data;

	frc_debug_parse_param(buf_orig, (char **)&parm);

	if (!strcmp(parm[0], "status")) {
		frc_status(devp);
	} else if (!strcmp(parm[0], "dbg_level")) {
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_dbg_en = (int)val1;
		pr_frc(0, "frc_dbg_en=%d\n", frc_dbg_en);
	} else if (!strcmp(parm[0], "dbg_mode")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0) {
			if (frc_dbg_ctrl) {
				if (val1 < 100) { //for debug:forbid user-layer call
					pr_frc(0, "ctrl test..\n");
					goto exit;
				}
				val1 = val1 - 100;
			}
			if (val1 < FRC_STATE_NULL) {
				frc_set_mode((u32)val1);
			}
		}
	} else if (!strcmp(parm[0], "test_pattern")) {
		if (!parm[1])
			goto exit;

		if (!strcmp(parm[1], "enable"))
			devp->frc_test_ptn = 1;
		else if (!strcmp(parm[1], "disable"))
			devp->frc_test_ptn = 0;
		frc_pattern_on(devp->frc_test_ptn);
	} else if (!strcmp(parm[0], "frc_pos")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->frc_hw_pos = (u32)val1;
		frc_init_config(devp);
		pr_frc(0, "frc_hw_pos:0x%x (0:before 1:after)\n", devp->frc_hw_pos);
	} else if (!strcmp(parm[0], "frc_pause")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->frc_fw_pause = (u32)val1;
	} else if (!strcmp(parm[0], "film_mode")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->film_mode = val1;
	} else if (!strcmp(parm[0], "force_mode")) {
		if (!parm[3])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->force_size.force_en = val1;
		if (kstrtoint(parm[2], 10, &val1) == 0)
			devp->force_size.force_hsize = val1;
		if (kstrtoint(parm[3], 10, &val1) == 0)
			devp->force_size.force_vsize = val1;
	} else if (!strcmp(parm[0], "ud_dbg")) {
		if (!parm[4]) {
			pr_frc(0, "err:input parameters error!\n");
			goto exit;
		}
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->ud_dbg.inp_ud_dbg_en = val1;
		if (kstrtoint(parm[2], 10, &val1) == 0) {
			devp->ud_dbg.meud_dbg_en = val1;
			devp->ud_dbg.mcud_dbg_en = val1;
			devp->ud_dbg.vpud_dbg_en = val1;
		}
		if (kstrtoint(parm[3], 10, &val1) == 0)
			devp->ud_dbg.inud_time_en = val1;
		if (kstrtoint(parm[4], 10, &val1) == 0)
			devp->ud_dbg.outud_time_en = val1;
	} else if (!strcmp(parm[0], "auto_ctrl")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0) {
			if (frc_dbg_ctrl) {
				if (val1 < 100) { //for debug:forbid user-layer call
					pr_frc(0, "ctrl test..\n");
					goto exit;
				}
				val1 = val1 - 100;
			}
			devp->frc_sts.auto_ctrl = val1;
		}
	} else if (!strcmp(parm[0], "memc_lossy")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0) {
			fw_data->frc_top_type.memc_loss_en  = val1;
			frc_cfg_memc_loss(fw_data->frc_top_type.memc_loss_en);
		}
	} else if (!strcmp(parm[0], "power_ctrl")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_power_domain_ctrl(devp, (u32)val1);
	} else if (!strcmp(parm[0], "memc_level")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0) {
			if (frc_dbg_ctrl) {
				if (val1 < 100) { //for debug:forbid user-layer call
					pr_frc(0, "ctrl test..\n");
					goto exit;
				}
				val1 = val1 - 100;
			}
			frc_memc_set_level((u8)val1);
		}
	} else if (!strcmp(parm[0], "vendor")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_tell_alg_vendor(val1 & 0xFF);
	} else if (!strcmp(parm[0], "mcfb")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_set_memc_fallback(val1 & 0x1F);
	} else if (!strcmp(parm[0], "filmset")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_set_film_support(val1);
	} else if (!strcmp(parm[0], "set_n2m")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_set_n2m(val1);
	} else if (!strcmp(parm[0], "auto_n2m")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->auto_n2m = (val1) ? 1 : 0;
	} else if (!strcmp(parm[0], "set_mcdw")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_cfg_mcdw_loss(val1);
	} else if (!strcmp(parm[0], "force_h2v2")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_set_h2v2(val1);
	} else if (!strcmp(parm[0], "test2")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->test2 = (u8)val1;
	}
exit:
	kfree(buf_orig);
}

ssize_t frc_debug_buf_if_help(struct frc_dev_s *devp, char *buf)
{
	ssize_t len = 0;
	len += sprintf(buf + len, "dump_bufcfg\t: dump buf address, size\n");
	len += sprintf(buf + len, "dump_linkbuf\t: dump link buffer data\n");
	len += sprintf(buf + len, "dump_init_reg\t: dump initial table\n");
	len += sprintf(buf + len, "dump_fixed_reg\t: dump fixed table\n");
	len += sprintf(buf + len, "dump_buf_reg\t: dump buffer register\n");
	len += sprintf(buf + len, "dump_data addr size\t: dump cma buf data\n");
	len += sprintf(buf + len, "buf_num\t\t:%d\n", devp->buf.frm_buf_num);
	len += sprintf(buf + len,
		"dc_set\t: x x x(me:mc_y:mc_c) set frc me,mc_y and mc_c comprate\n");
	len += sprintf(buf + len,
		"dc_mcdw_set\t: x x(mcdw_y:mcdw_c) set mcdw_y and mcdw_c comprate\n");
	len += sprintf(buf + len,
		"mcdw_ratio\t: 0xXX  set mcdw mc_y and mc_c ratio\n");
	len += sprintf(buf + len, "dc_apply\t: reset buffer when frc disable\n");
	return len;
}

void frc_debug_buf_if(struct frc_dev_s *devp, const char *buf, size_t count)
{
	char *buf_orig, *parm[47] = {NULL};
	int val1;
	int val2;

	if (!devp)
		return;

	if (!buf)
		return;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	if (!buf_orig)
		return;

	frc_debug_parse_param(buf_orig, (char **)&parm);

	if (!strcmp(parm[0], "dump_bufcfg")) {
		frc_buf_dump_memory_size_info(devp);
		frc_buf_dump_memory_addr_info(devp);
	} else if (!strcmp(parm[0], "dump_linkbuf")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0) {
			if (val1 < FRC_BUF_MAX_IDX)
				frc_buf_dump_link_tab(devp, (u32)val1);
		}
	} else if (!strcmp(parm[0], "dump_init_reg")) {
		frc_dump_reg_tab();
	} else if (!strcmp(parm[0], "dump_fixed_reg")) {
		frc_dump_fixed_table();
	} else if (!strcmp(parm[0], "dump_buf_reg")) {
		frc_dump_buf_reg();
	} else if (!strcmp(parm[0], "dump_data")) {
		if (kstrtoint(parm[1], 16, &val1))
			goto exit;
		if (kstrtoint(parm[2], 16, &val2))
			goto exit;
		frc_dump_buf_data(devp, (u32)val1, (u32)val2);
	} else if (!strcmp(parm[0], "buf_num")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_set_buf_num((u32)val1);
	} else if (!strcmp(parm[0], "dc_set")) { //(me:mc_y:mc_c)
		if (!parm[3]) {
			pr_frc(0, "err:input me mc_y mc_c\n");
			goto exit;
		}
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->buf.me_comprate = val1;
		if (kstrtoint(parm[2], 10, &val1) == 0)
			devp->buf.mc_y_comprate = val1;
		if (kstrtoint(parm[3], 10, &val1) == 0)
			devp->buf.mc_c_comprate = val1;
	} else if (!strcmp(parm[0], "dc_mcdw_set")) { //(me:mc_y:mc_c)
		if (!parm[2]) {
			pr_frc(0, "err:input mcdw_y mcdw_c\n");
			goto exit;
		}
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->buf.mcdw_y_comprate = val1;
		if (kstrtoint(parm[2], 10, &val1) == 0)
			devp->buf.mcdw_c_comprate = val1;
	} else if (!strcmp(parm[0], "mcdw_ratio")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_set_mcdw_buffer_ratio(val1);
	} else if (!strcmp(parm[0], "dc_apply")) {
		if (devp->frc_sts.state == FRC_STATE_BYPASS) {
			frc_buf_release(devp);
			frc_buf_set(devp);
		}
	}
exit:
	kfree(buf_orig);
}

ssize_t frc_debug_rdma_if_help(struct frc_dev_s *devp, char *buf)
{
	ssize_t len = 0;
	struct frc_fw_data_s *fw_data;
	struct frc_rdma_s *frc_rdma = get_frc_rdma();

	if (!devp)
		return len;

	fw_data = (struct frc_fw_data_s *)devp->fw_data;
	len += sprintf(buf + len, "status\t=show frc rdma status\n");
	// len += sprintf(buf + len, "frc_rdma\t=ctrl or debug frc rdma\n");
	len += sprintf(buf + len, "addr_val\t=addr val(set reg value to rdma table)\n");
	len += sprintf(buf + len, "rdma_en\t\t=%d drv, %d alg\n",
		frc_rdma->rdma_en, fw_data->frc_top_type.rdma_en);
	len += sprintf(buf + len, "rdma_table\t=show frc rdma table\n");
	len += sprintf(buf + len, "trace_en\t=echo 1 > rdma_trace_enable\n");
	len += sprintf(buf + len, "trace_reg\t=echo 0x60 0x61 xx > rdma_trace_reg\n");

	return len;
}

void frc_debug_rdma_if(struct frc_dev_s *devp, const char *buf, size_t count)
{
	u32 val1;
	u32 val2;
	char *buf_orig, *parm[47] = {NULL};
	struct frc_fw_data_s *fw_data;
	struct frc_rdma_s *frc_rdma = get_frc_rdma();


	if (!devp || !buf || !frc_rdma)
		return;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	if (!buf_orig)
		return;

	fw_data = (struct frc_fw_data_s *)devp->fw_data;
	frc_debug_parse_param(buf_orig, (char **)&parm);

	if (!strcmp(parm[0], "status")) {
		frc_rdma_status();
	} else if (!strcmp(parm[0], "addr_val")) {
		if (!parm[2])
			goto exit;
		if (kstrtoint(parm[1], 16, &val1))
			;// val1 = val1 & 0xffff;
		if (kstrtoint(parm[2], 16, &val2))
			;//val2 = val2 & 0xffffffff;
		pr_frc(0, "frc rdma addr:%x, val:%x\n", val1, val2);
		frc_rdma_table_config(val1, val2);
	} else if (!strcmp(parm[0], "rdma_en")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0) {
			// fw_data->frc_top_type.rdma_en = val1;
			pr_frc(2, "input rdma status:%d\n", val1);
			frc_rdma->rdma_en = val1;
			if (val1)
				frc_rdma_init();
			else
				frc_rdma_exit();
		}
	} else if (!strcmp(parm[0], "rdma_table")) {
		frc_read_table(frc_rdma);
	}

exit:
	kfree(buf_orig);
}

ssize_t frc_debug_param_if_help(struct frc_dev_s *devp, char *buf)
{
	ssize_t len = 0;
	struct frc_fw_data_s *fw_data;

	fw_data = (struct frc_fw_data_s *)devp->fw_data;
	len += sprintf(buf + len, "dbg_input_hsize\t=%d\n", devp->dbg_input_hsize);
	len += sprintf(buf + len, "dbg_input_vsize\t=%d\n", devp->dbg_input_vsize);
	len += sprintf(buf + len, "dbg_ratio\t=%d\n", devp->dbg_in_out_ratio);
	len += sprintf(buf + len, "dbg_force\t=%d\n", devp->dbg_force_en);
	len += sprintf(buf + len, "monitor_ireg\t=%d\n", devp->dbg_reg_monitor_i);
	len += sprintf(buf + len, "monitor_oreg\t=%d\n", devp->dbg_reg_monitor_o);
	len += sprintf(buf + len, "monitor_dump\n");
	len += sprintf(buf + len, "monitor_vf\t=%d\n", devp->dbg_vf_monitor);
	len += sprintf(buf + len, "seamless\t=%d\n", devp->in_sts.frc_seamless_en);
	len += sprintf(buf + len, "secure_on\t=[start_addr size] under 32bit ddr\n");
	len += sprintf(buf + len, "secure_off\t=closed frc secure buf\n");
	len += sprintf(buf + len, "set_seg\t\t=(read reg check)\n");
	len += sprintf(buf + len, "set_demo\t=(read reg check)\n");
	len += sprintf(buf + len, "demo_win\t=(read reg check)\n");
	len += sprintf(buf + len, "out_line\t=%d\n", devp->out_line);
	len += sprintf(buf + len, "chk_motion\t=%d\n", devp->ud_dbg.res0_dbg_en);
	len += sprintf(buf + len, "chk_vd\t\t=%d\n", devp->ud_dbg.res1_dbg_en);
	len += sprintf(buf + len, "inp_err\t\t=%d\n", devp->ud_dbg.res1_time_en);
	len += sprintf(buf + len, "dbg_ro\t\t=(check DBG_STAT reg\n");
	len += sprintf(buf + len, "frc_clk_auto\t=%d\n", devp->clk_chg);
	len += sprintf(buf + len, "frc_force_in\t=%d\n",
					(fw_data->frc_top_type.vfp & BIT_4));
	len += sprintf(buf + len, "frc_no_tell\t=%d\n",
					(fw_data->frc_top_type.vfp & BIT_7));
	len += sprintf(buf + len, "frc_dp [0-5]\t=1:red 5:black display pattern\n");
	len += sprintf(buf + len, "frc_ip [0-5]\t=1:red 5:black input pattern\n");
	len += sprintf(buf + len, "chg_patch\t=%d\n", devp->ud_dbg.res2_time_en);
	len += sprintf(buf + len, "osdbit_fcolr\t=(read reg check)\n");
	len += sprintf(buf + len, "prot_mode\t=%d\n", devp->prot_mode);
	len += sprintf(buf + len, "set_urgent\t=(read reg check)\n");
	len += sprintf(buf + len, "no_ko_mode\t=%d\n", devp->no_ko_mode);
	len += sprintf(buf + len, "tell_ready\t=%d\n", devp->other2_flag);
	len += sprintf(buf + len, "chg_slice_num\t=(read reg check)\n");
	return len;
}

void frc_debug_param_if(struct frc_dev_s *devp, const char *buf, size_t count)
{
	char *buf_orig, *parm[47] = {NULL};
	int val1;
	int val2;
	int val3;

	if (!devp)
		return;

	if (!buf)
		return;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	if (!buf_orig)
		return;

	frc_debug_parse_param(buf_orig, (char **)&parm);

	if (!strcmp(parm[0], "dbg_size")) {
		if (!parm[1] || !parm[2])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->dbg_input_hsize = (u32)val1;

		if (kstrtoint(parm[2], 10, &val2) == 0)
			devp->dbg_input_vsize = (u32)val2;
	} else if (!strcmp(parm[0], "dbg_ratio")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->dbg_in_out_ratio = val1;
		pr_frc(0, "dbg_in_out_ratio:0x%x\n", devp->dbg_in_out_ratio);
	} else if (!strcmp(parm[0], "dbg_force")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->dbg_force_en = (u32)val1;
	} else if (!strcmp(parm[0], "monitor_ireg")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0) {
			if (val1 < MONITOR_REG_MAX) {
				if (kstrtoint(parm[2], 16, &val2) == 0) {
					if (val1 < 0x3fff) {
						devp->dbg_in_reg[val1] = val2;
						devp->dbg_reg_monitor_i = 1;
					}
				}
			} else {
				devp->dbg_reg_monitor_i = 0;
			}
		}
	} else if (!strcmp(parm[0], "monitor_oreg")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0) {
			if (val1 < MONITOR_REG_MAX) {
				if (kstrtoint(parm[2], 16, &val2) == 0) {
					if (val1 < 0x3fff) {
						devp->dbg_out_reg[val1] = val2;
						devp->dbg_reg_monitor_o = 1;
					}
				}
			} else {
				devp->dbg_reg_monitor_o = 0;
			}
		}
	} else if (!strcmp(parm[0], "monitor_vf")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0) {
			devp->dbg_vf_monitor = val1;
			devp->dbg_buf_len = 0;
		}
	} else if (!strcmp(parm[0], "monitor_dump")) {
		frc_dump_monitor_data(devp);
	} else if (!strcmp(parm[0], "set_seg")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_set_seg_display((u8)val1, 8, 8, 8);
	} else if (!strcmp(parm[0], "set_demo")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_memc_set_demo((u8)val1);
	} else if (!strcmp(parm[0], "demo_win")) {
		/*Test whether demo window works properly for t3x*/
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			set_frc_demo_window((u8)val1);
	} else if (!strcmp(parm[0], "out_line")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0) {
			devp->out_line = (u32)val1;
			pr_frc(2, "set frc adj me out line is %d\n",
				devp->out_line);
		}
	} else if (!strcmp(parm[0], "chk_motion")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->ud_dbg.res0_dbg_en = val1;
	} else if (!strcmp(parm[0], "chk_vd")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->ud_dbg.res1_dbg_en = val1;
	} else if (!strcmp(parm[0], "inp_err")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->ud_dbg.res1_time_en = val1;
	} else if (!strcmp(parm[0], "dbg_ro")) {
		if (!parm[1]) {
			pr_frc(0, "err: input check\n");
			goto exit;
		}
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_check_hw_stats(devp, val1);
	} else if (!strcmp(parm[0], "frc_clk_auto")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0) {
			devp->clk_chg = val1;
			schedule_work(&devp->frc_clk_work);
		}
	} else if (!strcmp(parm[0], "frc_force_in")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_set_enter_forcefilm(devp, val1);
	} else if (!strcmp(parm[0], "frc_no_tell")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_set_notell_film(devp, val1);
	} else if (!strcmp(parm[0], "frc_dp")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0) {
			if (val1) {
				devp->pat_dbg.pat_en = 1;
				devp->pat_dbg.pat_type |= BIT_1;
				devp->pat_dbg.pat_color = (u8)val1;
			} else {
				devp->pat_dbg.pat_en = 0;
				devp->pat_dbg.pat_type |= ~BIT_1;
				devp->pat_dbg.pat_color = (u8)val1;
			}
			frc_set_output_pattern(val1);
		}
	} else if (!strcmp(parm[0], "frc_ip")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0) {
			if (val1) {
				devp->pat_dbg.pat_en = 1;
				devp->pat_dbg.pat_type |= BIT_0;
				devp->pat_dbg.pat_color = (u8)val1;
			} else {
				devp->pat_dbg.pat_en = 0;
				devp->pat_dbg.pat_type |= ~BIT_0;
				devp->pat_dbg.pat_color = (u8)val1;
			}
			frc_set_input_pattern(val1);
		}
	} else if (!strcmp(parm[0], "seamless")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_set_seamless_proc(val1);
	} else if (!strcmp(parm[0], "chg_patch")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->ud_dbg.res2_time_en = val1;
	} else if (!strcmp(parm[0], "ratio")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->in_out_ratio = val1;
	} else if (!strcmp(parm[0], "osdbit_fcolr")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_osdbit_setfalsecolor(devp, val1);
	} else if (!strcmp(parm[0], "secure_on")) {
		if (!parm[1] || !parm[2])
			goto exit;
		if (kstrtoint(parm[1], 16, &val1) == 0) {
			if (kstrtoint(parm[2], 16, &val2) == 0)
				frc_test_mm_secure_set_on(devp, val1, val2);
		}
	} else if (!strcmp(parm[0], "secure_off")) {
		frc_test_mm_secure_set_off(devp);
	} else if (!strcmp(parm[0], "prot_mode")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->prot_mode = val1;
	} else if (!strcmp(parm[0], "set_urgent")) {
		if (!parm[1] || !parm[2] || !parm[3])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0) {
			if (kstrtoint(parm[2], 10, &val2) == 0)
				if (kstrtoint(parm[3], 10, &val3) == 0)
					frc_set_arb_ugt_cfg(val1, val2, val3);
		}
	} else if (!strcmp(parm[0], "no_ko_mode")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->no_ko_mode = val1;
	} else if (!strcmp(parm[0], "tell_ready")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->other2_flag = val1;
	} else if (!strcmp(parm[0], "chg_slice_num")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_chg_loss_slice_num(val1);
	}
exit:
	kfree(buf_orig);
}

ssize_t frc_debug_other_if_help(struct frc_dev_s *devp, char *buf)
{
	ssize_t len = 0;
	struct frc_fw_data_s *fw_data;

	if (!devp)
		return len;

	fw_data = (struct frc_fw_data_s *)devp->fw_data;

	len += sprintf(buf + len, "crc_read\t=%d\n", devp->frc_crc_data.frc_crc_read);
	len += sprintf(buf + len, "crc_en\t\t=%d %d %d %d\n",
		devp->frc_crc_data.me_wr_crc.crc_en, devp->frc_crc_data.me_rd_crc.crc_en,
		devp->frc_crc_data.mc_wr_crc.crc_en, devp->frc_crc_data.frc_crc_pr);
	len += sprintf(buf + len, "freq_en\t\t=%d\n", devp->in_sts.high_freq_en);
	len += sprintf(buf + len, "inp_adj_en\t=%d\n", devp->in_sts.inp_size_adj_en);
	len += sprintf(buf + len, "crash_int_en\t=(check log)\n");
	len += sprintf(buf + len, "del_120_pth\t=%d\n", devp->ud_dbg.res2_dbg_en);
	len += sprintf(buf + len, "pr_dbg\t\t=%d\n", devp->ud_dbg.pr_dbg);
	len += sprintf(buf + len, "pre_vsync\t=%d\n", devp->use_pre_vsync);
	len += sprintf(buf + len, "mute_en\t\t=%d\t%d\n",
		devp->in_sts.enable_mute_flag, devp->in_sts.mute_vsync_cnt);
	len += sprintf(buf + len, "task_hi_en\t=%d\t%d\n",
		devp->in_sts.hi_en, devp->out_sts.hi_en);
	len += sprintf(buf + len, "timer_ctrl\t=en:%d level:%d interval:%d\n",
			devp->timer_dbg.timer_en, devp->timer_dbg.timer_level,
			devp->timer_dbg.time_interval);
	len += sprintf(buf + len, "frm_seg_en\t=%d\n", devp->in_sts.frm_en);
	len += sprintf(buf + len, "motion_ctrl\t=%d\n",
			fw_data->frc_top_type.motion_ctrl);
	return len;
}

void frc_debug_other_if(struct frc_dev_s *devp, const char *buf, size_t count)
{
	char *buf_orig, *parm[47] = {NULL};
	int val1;
	struct frc_fw_data_s *fw_data;

	if (!devp)
		return;

	if (!buf)
		return;

	fw_data = (struct frc_fw_data_s *)devp->fw_data;
	buf_orig = kstrdup(buf, GFP_KERNEL);
	if (!buf_orig)
		return;

	frc_debug_parse_param(buf_orig, (char **)&parm);

	if (!strcmp(parm[0], "freq_en")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->in_sts.high_freq_en = val1;
	} else if (!strcmp(parm[0], "inp_adj_en")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->in_sts.inp_size_adj_en = val1;
	} else if (!strcmp(parm[0], "crash_int_en")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_set_axi_crash_irq(devp, val1);
	} else if (!strcmp(parm[0], "crc_read")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->frc_crc_data.frc_crc_read = val1;
	} else if (!strcmp(parm[0], "crc_en")) {
		if (!parm[4])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->frc_crc_data.me_wr_crc.crc_en = val1;
		if (kstrtoint(parm[2], 10, &val1) == 0)
			devp->frc_crc_data.me_rd_crc.crc_en = val1;
		if (kstrtoint(parm[3], 10, &val1) == 0)
			devp->frc_crc_data.mc_wr_crc.crc_en = val1;
		if (kstrtoint(parm[4], 10, &val1) == 0)
			devp->frc_crc_data.frc_crc_pr = val1;
		frc_crc_enable(devp);
	} else if (!strcmp(parm[0], "del_120_pth")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0) {
			devp->ud_dbg.res2_dbg_en = val1;
			t3x_revB_patch_apply();
		}
	} else if (!strcmp(parm[0], "pr_dbg")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0) {
			if (devp->ud_dbg.pr_dbg)
				pr_frc(2, "processing, try again later\n");
			else
				devp->ud_dbg.pr_dbg = (u8)val1;
		}
	} else if (!strcmp(parm[0], "clr_mv_buf")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->other1_flag = val1;
	} else if (!strcmp(parm[0], "pre_vsync")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->use_pre_vsync = val1;
	} else if (!strcmp(parm[0], "mvrd_mode")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->dbg_mvrd_mode = val1;
	} else if (!strcmp(parm[0], "mute_dis")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->dbg_mute_disable = val1;
	} else if (!strcmp(parm[0], "frc_sus")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0) {
			if (val1 == 0) {
				devp->frc_sts.auto_ctrl = false;
				PR_FRC("call %s\n", __func__);
				frc_power_domain_ctrl(devp, 0);
				if (devp->power_on_flag)
					devp->power_on_flag = false;
			} else {
				PR_FRC("call %s\n", __func__);
				frc_power_domain_ctrl(devp, 1);
				if (!devp->power_on_flag)
					devp->power_on_flag = true;
				set_frc_bypass(ON);
				devp->frc_sts.auto_ctrl = true;
				devp->frc_sts.re_config = true;
			}
		}
	} else if (!strcmp(parm[0], "mute_en")) {
		if (!parm[2])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0) {
			devp->in_sts.enable_mute_flag = val1;
			if (val1)
				devp->pat_dbg.pat_en = 1;
			else
				devp->pat_dbg.pat_en = 0;
		}
		if (kstrtoint(parm[2], 10, &val1) == 0)
			devp->in_sts.mute_vsync_cnt = val1;
	} else if (!strcmp(parm[0], "align_dbg")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->ud_dbg.align_dbg_en = val1;
	} else if (!strcmp(parm[0], "task_hi_en")) {
		if (!parm[2])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->in_sts.hi_en = val1;
		if (kstrtoint(parm[2], 10, &val1) == 0)
			devp->out_sts.hi_en = val1;
	} else if (!strcmp(parm[0], "timer_ctrl")) {
		if (!parm[3])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->timer_dbg.timer_en = (u8)val1;
		if (kstrtoint(parm[2], 10, &val1) == 0)
			devp->timer_dbg.timer_level = (u16)val1;
		if (kstrtoint(parm[3], 10, &val1) == 0)
			devp->timer_dbg.time_interval =
				(u8)(val1 > 16 ? 16 : val1);
		frc_timer_proc(devp);
	} else if (!strcmp(parm[0], "frm_seg_en")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->in_sts.frm_en = val1;
	} else if (!strcmp(parm[0], "motion_ctrl")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			fw_data->frc_top_type.motion_ctrl = val1;
	}
exit:
	kfree(buf_orig);
}

void frc_reg_io(const char *buf)
{
	char *buf_orig, *parm[8] = {NULL};
	ulong val;
	unsigned int reg;
	unsigned int regvalue;
	unsigned int len;
	int i;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	if (!buf_orig)
		return;
	frc_debug_parse_param(buf_orig, (char **)&parm);

	if (!strcmp(parm[0], "r")) {
		if (!parm[1])
			goto free_buf;
		if (kstrtoul(parm[1], 16, &val) < 0)
			goto free_buf;
		reg = val;
		regvalue = READ_FRC_REG(reg);
		pr_frc(0, "[0x%x] = 0x%x\n", reg, regvalue);
	} else if (!strcmp(parm[0], "w")) {
		if (!parm[1] || !parm[2])
			goto free_buf;
		if (kstrtoul(parm[1], 16, &val) < 0)
			goto free_buf;
		reg = val;
		if (kstrtoul(parm[2], 16, &val) < 0)
			goto free_buf;
		regvalue = val;
		WRITE_FRC_REG_BY_CPU(reg, regvalue);
		pr_frc(0, "[0x%x] = 0x%x\n", reg, regvalue);
	} else if (!strcmp(parm[0], "d")) {
		if (!parm[1] || !parm[2])
			goto free_buf;
		if (kstrtoul(parm[1], 16, &val) < 0)
			goto free_buf;
		reg = val;
		if (kstrtoul(parm[2], 16, &val) < 0)
			goto free_buf;
		len = val;
		for (i = 0; i < len; i++) {
			regvalue = READ_FRC_REG(reg + i);
			pr_frc(0, "[0x%x] = 0x%x\n", reg + i, regvalue);
		}
	}

free_buf:
	kfree(buf_orig);
}

void frc_tool_dbg_store(struct frc_dev_s *devp, const char *buf)
{
	int i, count, flag = 0;
	char *buf_orig, *parm[8] = {NULL};
	ulong val;
	int debug_flag = 32;
	unsigned int reg;
	unsigned int regvalue;
	struct frc_rdma_info *rdma_info;
	struct frc_rdma_s *frc_rdma = get_frc_rdma();

	buf_orig = kstrdup(buf, GFP_KERNEL);
	if (!buf_orig)
		return;
	frc_debug_parse_param(buf_orig, (char **)&parm);
	rdma_info = (struct frc_rdma_info *)frc_rdma->rdma_info[3];

	if (!strcmp(parm[0], "r")) {
		if (!parm[1])
			goto free_buf;
		if (kstrtoul(parm[1], 16, &val) < 0)
			goto free_buf;

		reg = val;
		devp->tool_dbg.reg_read = reg;

		if (is_rdma_enable()) {
			if (rdma_info->rdma_item_count) {
				count = rdma_info->rdma_item_count;
				for (i = 0; i < count; i++) {
					if (rdma_info->rdma_table_addr[i * 2] == reg) {
						devp->tool_dbg.reg_read_val =
							rdma_info->rdma_table_addr[i * 2 + 1];
						flag = 1;
						break;
					}
				}
				if (!flag)
					devp->tool_dbg.reg_read_val = READ_FRC_REG(reg);
			} else {
				devp->tool_dbg.reg_read_val = READ_FRC_REG(reg);
			}
		} else {
			devp->tool_dbg.reg_read_val = READ_FRC_REG(reg);
		}
	} else if (!strcmp(parm[0], "w")) {
		if (!parm[1] || !parm[2])
			goto free_buf;
		if (kstrtoul(parm[1], 16, &val) < 0)
			goto free_buf;
		reg = val;
		if (kstrtoul(parm[2], 16, &val) < 0)
			goto free_buf;
		regvalue = val;

		if (is_rdma_enable()) {
			// debug
			i = rdma_info->rdma_item_count;
			rdma_info->rdma_table_addr[i * 2] = reg;
			rdma_info->rdma_table_addr[i * 2 + 1] = regvalue;
			rdma_info->rdma_item_count++;
			pr_frc(debug_flag, "addr:0x%04x, value:0x%08x\n",
				rdma_info->rdma_table_addr[i * 2],
				rdma_info->rdma_table_addr[i * 2 + 1]);
		} else {
			WRITE_FRC_REG_BY_CPU(reg, regvalue);
		}
	}

free_buf:
	kfree(buf_orig);
}

// timer
static enum hrtimer_restart frc_timer_callback(struct hrtimer *timer)
{
	u8 i, time;
	u16 log;
	u32 reg_val;
	struct frc_dev_s *devp = get_frc_devp();

	log = devp->timer_dbg.timer_level;
	time = devp->timer_dbg.time_interval;

	for (i = 0; i < rdma_trace_num; i++) {
		reg_val = READ_FRC_REG(rdma_trace_reg[i]);
		pr_frc(log, "reg[%04x]=0x%08x %9d\n", rdma_trace_reg[i], reg_val, reg_val);
	}

	hrtimer_forward(&frc_hi_timer,
		hrtimer_cb_get_time(timer), ktime_set(0, time * 1000000)); // unit: ns

	return HRTIMER_RESTART;
}

void frc_timer_proc(struct frc_dev_s *devp)
{
	u8 timer_en, time;

	timer_en = devp->timer_dbg.timer_en;
	time = devp->timer_dbg.time_interval;
	frc_hi_timer.function = frc_timer_callback;

	if (time > 16)
		time = 16;
	else if (time < 1)
		time = 1;

	if (timer_en)
		hrtimer_start(&frc_hi_timer,
			ktime_set(0, time * 1000000), HRTIMER_MODE_REL); // unit: ns
	else
		hrtimer_cancel(&frc_hi_timer);
}

/* column: 1~8, color: 0~7, number: 0~15 */
static void update_seg_7_show(u8 enable, u8 column, u8 color, u8 number)
{
	u8 value;

	value = ((enable & 0x01) << 7) + ((color & 0x07) << 4) + (number & 0x0F);

	// enable flag_number 2_1 ~ 2_8
	if (column == 1)
		UPDATE_FRC_REG_BITS(FRC_MC_SEVEN_FLAG_NUM17_NUM18_NUM21_NUM22,
			value << 8, 0xFF00);
	else if (column == 2)
		UPDATE_FRC_REG_BITS(FRC_MC_SEVEN_FLAG_NUM17_NUM18_NUM21_NUM22,
			value, 0xFF);
	else if (column == 3)
		UPDATE_FRC_REG_BITS(FRC_MC_SEVEN_FLAG_NUM23_NUM24_NUM25_NUM26,
			value << 24, 0xFF000000);
	else if (column == 4)
		UPDATE_FRC_REG_BITS(FRC_MC_SEVEN_FLAG_NUM23_NUM24_NUM25_NUM26,
			value << 16, 0xFF0000);
	else if (column == 5)
		UPDATE_FRC_REG_BITS(FRC_MC_SEVEN_FLAG_NUM23_NUM24_NUM25_NUM26,
			value << 8, 0xFF00);
	else if (column == 6)
		UPDATE_FRC_REG_BITS(FRC_MC_SEVEN_FLAG_NUM23_NUM24_NUM25_NUM26,
			value, 0xFF);
	else if (column == 7)
		UPDATE_FRC_REG_BITS(FRC_MC_SEVEN_FLAG_NUM27_NUM28,
			value << 24, 0xFF000000);
	else if (column == 8)
		UPDATE_FRC_REG_BITS(FRC_MC_SEVEN_FLAG_NUM27_NUM28,
			value << 16, 0xFF0000);
}

void frc_dbg_frame_show(struct frc_dev_s *devp)
{
	u8 i, enable, tmp_cnt;
	static u8 pre_flag;

	if (!devp)
		return;

	enable = devp->in_sts.frm_en;

	if (enable) {
		// in cnt
		tmp_cnt = (devp->in_sts.vs_cnt / 100) % 10;
		update_seg_7_show(1, 1, 1, tmp_cnt);
		tmp_cnt = (devp->in_sts.vs_cnt / 10) % 10;
		update_seg_7_show(1, 2, 1, tmp_cnt);
		tmp_cnt = devp->in_sts.vs_cnt % 10;
		update_seg_7_show(1, 3, 1, tmp_cnt);
		// out cnt
		tmp_cnt = (devp->out_sts.vs_cnt / 100) % 10;
		update_seg_7_show(1, 6, 2, tmp_cnt);
		tmp_cnt = (devp->out_sts.vs_cnt / 10) % 10;
		update_seg_7_show(1, 7, 2, tmp_cnt);
		tmp_cnt = devp->out_sts.vs_cnt % 10;
		update_seg_7_show(1, 8, 2, tmp_cnt);
	} else if (enable == 0 && enable != pre_flag) {
		for (i = 1; i < 9; i++)
			update_seg_7_show(0, i, 0, 0);  // clear
	}

	pre_flag = enable;
}
