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
	pr_frc(0, "frc_get_vd_flag= 0x%X(game:0/pc:1/pic:2/hbw:3/limsz:4/vlock:5/in_size_err:6)\n",
				devp->in_sts.st_flag);
	pr_frc(0, "dc_rate(me:%d,mc_y:%d,mc_c:%d,mcdw_y:%d,mcdw_c:%d), real total size:%d\n",
		devp->buf.me_comprate, devp->buf.mc_y_comprate,
		devp->buf.mc_c_comprate, devp->buf.mcdw_y_comprate,
		devp->buf.mcdw_c_comprate, devp->buf.real_total_size);
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
	pr_frc(0, "frc_in isr vs_cnt= %d, vs_tsk_cnt:%d, inp_err cnt= %d\n",
		devp->in_sts.vs_cnt, devp->in_sts.vs_tsk_cnt,
		devp->frc_sts.inp_undone_cnt);
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

	len += sprintf(buf + len, "status:%d\t: (%s)\n", devp->frc_sts.state,
				frc_state_ary[devp->frc_sts.state]);
	len += sprintf(buf + len, "dbg_level=%d\n", frc_dbg_en);//style for tool
	len += sprintf(buf + len, "dbg_mode\t: 0:disable 1:enable 2:bypass\n");
	len += sprintf(buf + len, "test_pattern disable or enable\t:\n");
	len += sprintf(buf + len, "frc_pos 0,1\t: 0:before postblend; 1:after postblend\n");
	len += sprintf(buf + len, "frc_pause 0/1 \t: 0: fw work 1:fw not work\n");
	len += sprintf(buf + len, "film_mode val\t: 0:video 1:22 2:32 3:3223 4:2224\n");
	len += sprintf(buf + len, " \t\t 5:32322 6:44\n");
	len += sprintf(buf + len, "force_mode en(0/1) hize vsize\n");
	len += sprintf(buf + len, "ud_dbg 0/1 0/1 0/1 0/1\t: meud_en,mcud_en,in,out alg time\n");
	len += sprintf(buf + len, "auto_ctrl 0/1 \t: frc auto on off work mode\n");
	len += sprintf(buf + len, "memc_lossy 0/1/2 \t: 0:off 1:mc_en,2:me_en,3:memc_en\n");
	len += sprintf(buf + len, "power_ctrl 0/1: power down/on memc\n");
	len += sprintf(buf + len, "memc_level x: memc_dejudder (0-10)\n");
	len += sprintf(buf + len, "vendor    : alg vendor(0x0:ref...)\n");
	len += sprintf(buf + len, "mcfb      : set memc fallback (0-20)\n");
	len += sprintf(buf + len, "filmset   : set memc film mode\n");
	len += sprintf(buf + len, "set_n2m   : manual set n2m\n");
	len += sprintf(buf + len, "auto_n2m  : auto set n2m\n");
	len += sprintf(buf + len, "set_mcdw  : set mcdw\n");
	len += sprintf(buf + len, "force_h2v2: force h2v2\n");

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
	len += sprintf(buf + len,
		"buf_num val\t: val(1 - 16) frc and logo frame buffer number\n");
	len += sprintf(buf + len,
		"dc_set\t: x x x(me:mc_y:mc_c) set frc me,mc_y and mc_c comprate\n");
	len += sprintf(buf + len,
		"dc_mcdw_set\t: x x(mc_y:mc_c) set mcdw mc_y and mc_c comprate\n");
	len += sprintf(buf + len, "dc_apply\t: reset buffer when frc bypass\n");
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

	len += sprintf(buf + len, "frc_rdma\t:  ctrl or debug frc rdma\n");
	len += sprintf(buf + len, "addr_val addr val\t: set reg value to rdma table\n");
	len += sprintf(buf + len, "rdma_en\t: 0/1 closed or open frc rdma\n");
	return len;
}

void frc_debug_rdma_if(struct frc_dev_s *devp, const char *buf, size_t count)
{
	char *buf_orig, *parm[47] = {NULL};
	struct frc_fw_data_s *fw_data;
	int val1;
	int val2;

	if (!devp)
		return;

	if (!buf)
		return;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	if (!buf_orig)
		return;

	fw_data = (struct frc_fw_data_s *)devp->fw_data;
	frc_debug_parse_param(buf_orig, (char **)&parm);

	if (!strcmp(parm[0], "frc_rdma")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0) {
			pr_frc(0, "frc rdma test start, val1:%d\n", val1);
			frc_rdma_process(val1);
		}
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
		if (kstrtoint(parm[1], 10, &val1) == 0)
			fw_data->frc_top_type.rdma_en = val1;
	}

exit:
	kfree(buf_orig);
}

ssize_t frc_debug_param_if_help(struct frc_dev_s *devp, char *buf)
{
	ssize_t len = 0;

	len += sprintf(buf + len, "dbg_size [h v]\t: set debug hsize and vsize\n");
	len += sprintf(buf + len, "ratio val [val]\t: 0:112 1:213 2:2-5 3:5-6 6:1-1\n");
	len += sprintf(buf + len, "dbg_ratio [val]\t: set debug input ratio\n");
	len += sprintf(buf + len, "dbg_force\t: force debug mode\n");
	len += sprintf(buf + len, "monitor_ireg [idx reg_addr]\t: monitor frc register (max 6)\n");
	len += sprintf(buf + len, "monitor_oreg [idx reg_addr]\t: monitor frc register (max 6)\n");
	len += sprintf(buf + len, "monitor_dump\n");
	len += sprintf(buf + len, "monitor_vf [1/0]\t: monitor current vf on, off\n");
	len += sprintf(buf + len, "seamless\t: [0/1] 0:disable 1:enable\n");
	len += sprintf(buf + len, "secure_on\t: [start_addr size] under 32bit ddr\n");
	len += sprintf(buf + len, "secure_off\t: closed frc secure buf\n");
	len += sprintf(buf + len, "set_seg\t:\n");
	len += sprintf(buf + len, "set_demo\t:\n");
	len += sprintf(buf + len, "demo_win\t:\n");
	len += sprintf(buf + len, "out_line\t: adjust frm delay\n");
	len += sprintf(buf + len, "chk_motion\t:\n");
	len += sprintf(buf + len, "chk_vd\t:\n");
	len += sprintf(buf + len, "inp_err\t:\n");
	len += sprintf(buf + len, "dbg_ro\t:\n");
	len += sprintf(buf + len, "frc_clk_auto\t:\n");
	len += sprintf(buf + len, "frc_force_in\t:\n");
	len += sprintf(buf + len, "frc_no_tell\t:\n");
	len += sprintf(buf + len, "frc_dp [0-5]\t: 1:red 5:black display pattern\n");
	len += sprintf(buf + len, "frc_ip [0-5]\t: 1:red 5:black input pattern\n");
	len += sprintf(buf + len, "mcdw_ratio\t:\n");
	len += sprintf(buf + len, "chg_patch\t:\n");
	len += sprintf(buf + len, "osdbit_fcolr\t:\n");
	len += sprintf(buf + len, "prot_mode\t:\n");
	len += sprintf(buf + len, "set_urgent\t:\n");
	return len;
}

void frc_debug_param_if(struct frc_dev_s *devp, const char *buf, size_t count)
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
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_set_output_pattern(val1);
	} else if (!strcmp(parm[0], "frc_ip")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_set_input_pattern(val1);
	} else if (!strcmp(parm[0], "seamless")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_set_seamless_proc(val1);
	} else if (!strcmp(parm[0], "mcdw_ratio")) {
		if (!parm[1])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0)
			frc_set_mcdw_buffer_ratio(val1);
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
		if (!parm[1] || !parm[2])
			goto exit;
		if (kstrtoint(parm[1], 10, &val1) == 0) {
			if (kstrtoint(parm[2], 16, &val2) == 0)
				frc_set_urgent_cfg(val1, val2);
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

	len += sprintf(buf + len,
		"crc_read [0/1]\t: 0: disable crc read, 1: enable crc read\n");
	len += sprintf(buf + len,
		"crc_en [0/1 0/1 0/1 0/1]\t: mewr/merd/mcwr/vs_print crc enable\n");
	len += sprintf(buf + len, "freq_en [0/1]\t: high frequent word flash\n");
	len += sprintf(buf + len,
		"inp_adj_en [0/1]\t: size adjust when input size not standard\n");
	len += sprintf(buf + len, "crash_int_en\t:\n");
	len += sprintf(buf + len, "del_120_pth\t:\n");
	len += sprintf(buf + len, "pr_dbg 0/1\t: print reg table\n");
	return len;
}

void frc_debug_other_if(struct frc_dev_s *devp, const char *buf, size_t count)
{
	char *buf_orig, *parm[47] = {NULL};
	int val1;

	if (!devp)
		return;

	if (!buf)
		return;

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
		if (kstrtoint(parm[1], 10, &val1) == 0)
			devp->ud_dbg.res2_dbg_en = val1;
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
	struct frc_rdma_info *frc_rdma2 = frc_get_rdma_info_2();

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
		devp->tool_dbg.reg_read = reg;

		if (is_rdma_enable()) {
			if (frc_rdma2->rdma_item_count) {
				count = frc_rdma2->rdma_item_count;
				for (i = 0; i < count; i++) {
					if (frc_rdma2->rdma_table_addr[i * 2] == reg) {
						devp->tool_dbg.reg_read_val =
							frc_rdma2->rdma_table_addr[i * 2 + 1];
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
			i = frc_rdma2->rdma_item_count;
			frc_rdma2->rdma_table_addr[i * 2] = reg;
			frc_rdma2->rdma_table_addr[i * 2 + 1] = regvalue;
			frc_rdma2->rdma_item_count++;
			pr_frc(debug_flag, "addr:0x%04x, value:0x%08x\n",
				frc_rdma2->rdma_table_addr[i * 2],
				frc_rdma2->rdma_table_addr[i * 2 + 1]);
		} else {
			WRITE_FRC_REG_BY_CPU(reg, regvalue);
		}
	}

free_buf:
	kfree(buf_orig);
}
