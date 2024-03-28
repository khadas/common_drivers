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
#include <linux/time.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/ctype.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <asm/div64.h>
#include <linux/sched/clock.h>
#include <linux/amlogic/media/utils/am_com.h>

#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/amlogic/media/vpu/vpu.h>
#include <linux/amlogic/media/vfm/vframe.h>
#include <linux/amlogic/media/vfm/vframe_provider.h>
#include <linux/amlogic/media/vfm/vframe_receiver.h>
#include <linux/amlogic/media/frame_sync/timestamp.h>
#include <linux/amlogic/media/frame_sync/tsync.h>
#include <linux/amlogic/media/frc/frc_reg.h>
#include <linux/amlogic/media/frc/frc_common.h>
#include <linux/amlogic/tee.h>
#include <linux/clk.h>
#include <linux/amlogic/media/video_sink/video.h>

#include "frc_drv.h"
#include "frc_proc.h"
#include "frc_hw.h"
#include "frc_rdma.h"
#include "frc_dbg.h"
#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
#include <linux/amlogic/aml_iotrace.h>
#endif

int frc_enable_cnt = 1;
module_param(frc_enable_cnt, int, 0664);
MODULE_PARM_DESC(frc_enable_cnt, "frc enable counter");

int frc_disable_cnt = 1;
module_param(frc_disable_cnt, int, 0664);
MODULE_PARM_DESC(frc_disable_cnt, "frc disable counter");

int frc_re_cfg_cnt;/*need bigger than frc_disable_cnt 3, 15*/
module_param(frc_re_cfg_cnt, int, 0664);
MODULE_PARM_DESC(frc_re_cfg_cnt, "frc reconfig counter");

int sec_flag;
module_param(sec_flag, int, 0664);
MODULE_PARM_DESC(sec_flag, "frc debug flag");

u32 secure_tee_handle;

void frc_fw_initial(struct frc_dev_s *devp)
{
	if (!devp)
		return;

	devp->in_sts.vs_cnt = 0;
	devp->in_sts.vs_tsk_cnt = 0;
	devp->in_sts.vs_timestamp = sched_clock();
	devp->in_sts.vf_repeat_cnt = 0;
	devp->in_sts.vf_null_cnt = 0;

	devp->out_sts.vs_cnt = 0;
	devp->out_sts.vs_tsk_cnt = 0;
	devp->out_sts.vs_timestamp = sched_clock();
	devp->in_sts.vf = NULL;
	// devp->frc_sts.vs_cnt = 0;
	devp->vs_timestamp = sched_clock();

	devp->frc_sts.inp_undone_cnt = 0;
	devp->frc_sts.me_undone_cnt = 0;
	devp->frc_sts.mc_undone_cnt = 0;
	devp->frc_sts.vp_undone_cnt = 0;
	devp->little_win = 0;
}

static void frc_mute_config(void)
{
	struct frc_dev_s *devp;

	devp = get_frc_devp();
	/*if not receive vpu mute information, frc doesn't open mute function*/
	if (devp->in_sts.in_hsize < WIDTH_2K ||
		devp->in_sts.in_vsize < HEIGHT_1K ||
		!get_video_mute())
		devp->in_sts.enable_mute_flag = 0;
	else
		devp->in_sts.enable_mute_flag = 1;

	if (devp->in_sts.duration > 4005)
		devp->in_sts.mute_vsync_cnt = 45;
	else if (devp->in_sts.duration > 1605)
		devp->in_sts.mute_vsync_cnt = 15;
	else
		devp->in_sts.mute_vsync_cnt = 55;

	pr_frc(2, "enable_mute_flag = %d,mute_vsync_cnt = %d\n",
	devp->in_sts.enable_mute_flag, devp->in_sts.mute_vsync_cnt);
}

void frc_hw_initial(struct frc_dev_s *devp)
{
	frc_fw_initial(devp);
	frc_mtx_set(devp);
	frc_top_init(devp);
	t3x_verB_set_cfg(0, devp);
	frc_input_size_align_check(devp);
	if (devp->test2)
		frc_mute_config();
	frc_pattern_dbg_ctrl(devp);
	return;
}

void frc_in_reg_monitor(struct frc_dev_s *devp)
{
	u32 i;
	u32 reg;
	//char *buf = devp->dbg_buf;

	for (i = 0; i < MONITOR_REG_MAX; i++) {
		reg = devp->dbg_in_reg[i];
		if (reg != 0 && reg < 0x3fff) {
			if (devp->dbg_buf_len > 300) {
				devp->dbg_reg_monitor_i = 0;
				devp->dbg_buf_len = 0;
				return;
			}
			devp->dbg_buf_len++;
			pr_info("ivs:%d 0x%x=0x%08x\n", devp->in_sts.vs_cnt,
				reg, READ_FRC_REG(reg));
		}
	}
}

void frc_vf_monitor(struct frc_dev_s *devp)
{
	if (devp->dbg_buf_len > 300) {
		devp->dbg_vf_monitor = 0;
		return;
	}
	devp->dbg_buf_len++;
	pr_info("ivs:%d 0x%lx\n", devp->frc_sts.vs_cnt, (ulong)devp->in_sts.vf);
}

void frc_out_reg_monitor(struct frc_dev_s *devp)
{
	u32 i;
	u32 reg;

	for (i = 0; i < MONITOR_REG_MAX; i++) {
		reg = devp->dbg_out_reg[i];
		if (reg != 0 && reg < 0x3fff) {
			if (devp->dbg_buf_len > 300) {
				devp->dbg_reg_monitor_o = 0;
				devp->dbg_buf_len = 0;
				return;
			}
			devp->dbg_buf_len++;
			pr_info("ovs:%d 0x%x=0x%08x\n", devp->out_sts.vs_cnt,
				reg, READ_FRC_REG(reg));
		}
	}
}

void frc_dump_monitor_data(struct frc_dev_s *devp)
{
	//char *buf = devp->dbg_buf;
	//pr_info("%d, %s\n", devp->dbg_buf_len, buf);
	devp->dbg_buf_len = 0;
}

/*frc-fw input task execution time*/
void frc_in_task_print(u64 timer)
{
	static u64 in_tsk_inx, in_tsk_min, in_tsk_max, in_tsk_sum;

	in_tsk_inx++;
	in_tsk_sum += timer;
	if (in_tsk_min > timer)
		in_tsk_min = timer;
	if (in_tsk_max < timer)
		in_tsk_max = timer;

	if (in_tsk_inx == 60) {
		pr_frc(0, "in_tsk_time  min = %lld max = %lld avg = %lld\n",
			in_tsk_min, in_tsk_max, div64_u64(in_tsk_sum, 60));
		in_tsk_min = in_tsk_max;
		in_tsk_inx = 0;
		in_tsk_sum = 0;
		in_tsk_max = 0;
	}
}

/*frc-fw output task execution time*/
void frc_out_task_print(u64 timer)
{
	static u64 out_tsk_inx, out_tsk_min, out_tsk_max, out_tsk_sum;

	out_tsk_inx++;
	out_tsk_sum += timer;
	if (out_tsk_min > timer)
		out_tsk_min = timer;
	if (out_tsk_max < timer)
		out_tsk_max = timer;

	if (out_tsk_inx == 60) {
		pr_frc(0, "out_tsk_time min = %lld max = %lld avg = %lld\n",
			out_tsk_min, out_tsk_max, div64_u64(out_tsk_sum, 60));
		out_tsk_min = out_tsk_max;
		out_tsk_inx = 0;
		out_tsk_sum = 0;
		out_tsk_max = 0;
	}
}

/*use to frc "status" debug*/
void frc_isr_print_zero(struct frc_dev_s *devp)
{
	if (devp->frc_sts.changed_flag == 0)
		return;
	devp->in_sts.vs_duration = 0;
	devp->in_sts.vs_timestamp = 0;
	devp->in_sts.vs_cnt = 0;
	devp->in_sts.vs_tsk_cnt = 0;

	devp->out_sts.vs_duration = 0;
	devp->out_sts.vs_timestamp = 0;
	devp->out_sts.vs_cnt = 0;
	devp->out_sts.vs_tsk_cnt = 0;
	devp->frc_sts.vs_cnt = 0;
	devp->frc_sts.video_mute_cnt = 0;
	devp->frc_sts.vs_data_cnt = 0;
	WRITE_FRC_REG_BY_CPU(0x6d, 0x0);
	WRITE_FRC_REG_BY_CPU(0x6e, 0x0);

	devp->frc_sts.changed_flag = 0;
	devp->ud_dbg.align_dbg_en = 0;
}

irqreturn_t frc_input_isr(int irq, void *dev_id)
{
	struct frc_dev_s *devp = (struct frc_dev_s *)dev_id;

	u64 timestamp = sched_clock();

	if (!devp->probe_ok || !devp->power_on_flag)
		return IRQ_HANDLED;
	if (devp->clk_state == FRC_CLOCK_OFF)
		return IRQ_HANDLED;

	devp->in_sts.vs_cnt++;

	/*update vs time*/
	timestamp = div64_u64(timestamp, 1000);
	devp->in_sts.vs_duration = timestamp - devp->in_sts.vs_timestamp;
	devp->in_sts.vs_timestamp = timestamp;

	// t3x_verB_set_cfg(1);
	inp_undone_read(devp);
	if (devp->dbg_reg_monitor_i)
		frc_in_reg_monitor(devp);
	if (devp->in_sts.vs_cnt == devp->dbg_mvrd_mode)
		if ((READ_FRC_REG(FRC_MC_MVRD_CTRL) & BIT_0) == BIT_0)
			WRITE_FRC_REG_BY_CPU(FRC_MC_MVRD_CTRL, 0x100);
	if (devp->in_sts.hi_en)
		tasklet_hi_schedule(&devp->input_tasklet);
	else
		tasklet_schedule(&devp->input_tasklet);

	FRC_RDMA_WR_REG_IN(FRC_REG_TOP_RESERVE13, devp->in_sts.vs_cnt);
	frc_rdma_config(1, 0);

	return IRQ_HANDLED;
}

void frc_input_tasklet_pro(unsigned long arg)
{
	struct frc_dev_s *devp = (struct frc_dev_s *)arg;
	struct frc_fw_data_s *pfw_data;
	u64 timestamp;

	pfw_data = (struct frc_fw_data_s *)devp->fw_data;
	if (!pfw_data)
		return;
	if (!devp->probe_ok)
		return;
	if (!devp->power_on_flag) {
		// devp->power_off_flag++;
		return;
	}
	if (devp->clk_state == FRC_CLOCK_OFF)
		return;
#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
	iotrace_misc_record_write(RECORD_TYPE_FRC_INPUT_IN, 0, 0, 0);
#endif
	devp->in_sts.vs_tsk_cnt++;
	if (!devp->frc_fw_pause) {
		timestamp = sched_clock();
		if (pfw_data->memc_in_irq_handler)
			pfw_data->memc_in_irq_handler(pfw_data);
		// if (!devp->power_on_flag)
		// devp->power_off_flag++;
		if (devp->ud_dbg.inud_time_en)
			frc_in_task_print(sched_clock() - timestamp);
	}
#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
	iotrace_misc_record_write(RECORD_TYPE_FRC_INPUT_OUT, 0, 0, 0);
#endif
}

irqreturn_t frc_output_isr(int irq, void *dev_id)
{
	struct frc_dev_s *devp = (struct frc_dev_s *)dev_id;
	if (!devp->probe_ok || !devp->power_on_flag)
		return IRQ_HANDLED;
	if (devp->clk_state == FRC_CLOCK_OFF)
		return IRQ_HANDLED;

	u64 timestamp = sched_clock();
	// struct frc_rdma_info *frc_rdma = frc_get_rdma_info();

	devp->out_sts.vs_cnt++;
	/*update vs time*/
	timestamp = div64_u64(timestamp, 1000);
	devp->out_sts.vs_duration = timestamp - devp->out_sts.vs_timestamp;
	devp->out_sts.vs_timestamp = timestamp;

	me_undone_read(devp);
	mc_undone_read(devp);
	vp_undone_read(devp);

	get_vout_info(devp);

	if (devp->dbg_reg_monitor_o)
		frc_out_reg_monitor(devp);
	if (devp->out_sts.hi_en)
		tasklet_hi_schedule(&devp->output_tasklet);
	else
		tasklet_schedule(&devp->output_tasklet);

	// frc_rdma->rdma_item_count = 0;
	// rdma trigger 0 manual, 1-7 auto path
	FRC_RDMA_WR_REG_OUT(FRC_REG_TOP_RESERVE14, devp->out_sts.vs_cnt);
	frc_rdma_config(2, 0);

	if (devp->ud_dbg.pr_dbg)
		frc_debug_print(devp);

	return IRQ_HANDLED;
}

void frc_output_tasklet_pro(unsigned long arg)
{
	struct frc_dev_s *devp = (struct frc_dev_s *)arg;
	struct frc_fw_data_s *pfw_data;
	u64 timestamp;

	pfw_data = (struct frc_fw_data_s *)devp->fw_data;
	if (!pfw_data)
		return;
	if (!devp->probe_ok)
		return;
	if (!devp->power_on_flag)
		return;
	if (devp->clk_state == FRC_CLOCK_OFF)
		return;

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
	iotrace_misc_record_write(RECORD_TYPE_FRC_OUTPUT_IN, 0, 0, 0);
#endif
	devp->out_sts.vs_tsk_cnt++;
	if (devp->in_sts.enable_mute_flag == 1 &&
		// devp->frc_sts.vs_data_cnt == devp->in_sts.mute_vsync_cnt) {
		devp->out_sts.vs_cnt == devp->in_sts.mute_vsync_cnt) {
		frc_set_output_pattern(0); // unmute
		pr_frc(1, "%s: unmute after enable wait %d(%d) frames",
			__func__, devp->out_sts.vs_cnt,
			devp->frc_sts.vs_data_cnt);
	}
	if (!devp->frc_fw_pause) {
		timestamp = sched_clock();
		if (pfw_data->memc_out_irq_handler)
			pfw_data->memc_out_irq_handler(pfw_data);
		if (pfw_data->frc_fw_ctrl_if)
			pfw_data->frc_fw_ctrl_if(pfw_data);
		// if (!devp->power_on_flag)
		// devp->power_off_flag++;
		if (devp->ud_dbg.outud_time_en)
			frc_out_task_print(sched_clock() - timestamp);
	}
	frc_dbg_frame_show(devp);
#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
	iotrace_misc_record_write(RECORD_TYPE_FRC_OUTPUT_OUT, 0, 0, 0);
#endif
}

irqreturn_t frc_axi_crash_isr(int irq, void *dev_id)
{
	struct frc_dev_s *devp = (struct frc_dev_s *)dev_id;
	u32 tmp1, tmp2;

	pr_frc(0, "%s: crash occur!", __func__);
	if (!devp->probe_ok || !devp->power_on_flag)
		return IRQ_HANDLED;
	if (devp->clk_state == FRC_CLOCK_OFF)
		return IRQ_HANDLED;

	pr_frc(1, "apb_crash_addr:%X, axi_stats:0x%X,0x%X\n",
			READ_FRC_REG(FRC_APB_CRASH_ADDR),
			READ_FRC_REG(FRC_RDAXI0_PROT_STAT),
			READ_FRC_REG(FRC_WRAXI0_PROT_STAT));
	tmp1 = READ_FRC_REG(FRC_RDAXI0_PROT_CTRL);
	WRITE_FRC_REG_BY_CPU(FRC_RDAXI0_PROT_CTRL, tmp1 & 0xFFFFFFFD);
	WRITE_FRC_REG_BY_CPU(FRC_RDAXI0_PROT_CTRL, tmp1 | 0x02);

	tmp1 = READ_FRC_REG(FRC_RDAXI1_PROT_CTRL);
	WRITE_FRC_REG_BY_CPU(FRC_RDAXI1_PROT_CTRL, tmp1 & 0xFFFFFFFD);
	WRITE_FRC_REG_BY_CPU(FRC_RDAXI1_PROT_CTRL, tmp1 | 0x02);

	tmp1 = READ_FRC_REG(FRC_RDAXI2_PROT_CTRL);
	WRITE_FRC_REG_BY_CPU(FRC_RDAXI2_PROT_CTRL, tmp1 & 0xFFFFFFFD);
	WRITE_FRC_REG_BY_CPU(FRC_RDAXI2_PROT_CTRL, tmp1 | 0x02);

	tmp1 = READ_FRC_REG(FRC_WRAXI0_PROT_CTRL);
	WRITE_FRC_REG_BY_CPU(FRC_WRAXI0_PROT_CTRL, tmp1 & 0xFFFFFFFD);
	WRITE_FRC_REG_BY_CPU(FRC_WRAXI0_PROT_CTRL, tmp1 | 0x02);

	tmp1 = READ_FRC_REG(FRC_WRAXI1_PROT_CTRL);
	WRITE_FRC_REG_BY_CPU(FRC_WRAXI1_PROT_CTRL, tmp1 & 0xFFFFFFFD);
	WRITE_FRC_REG_BY_CPU(FRC_WRAXI1_PROT_CTRL, tmp1 | 0x02);

	tmp2 = READ_FRC_REG(FRC_ARB_BAK_CTRL);
	WRITE_FRC_REG_BY_CPU(FRC_ARB_BAK_CTRL, tmp2 & 0xFFFFFFDF);
	WRITE_FRC_REG_BY_CPU(FRC_ARB_BAK_CTRL, tmp2 | 0x20);

	pr_frc(0, "%s: try to clear!", __func__);
	return IRQ_HANDLED;
}

void frc_change_to_state(enum frc_state_e state)
{
	struct frc_dev_s *devp = get_frc_devp();

	if (devp->in_sts.vf_sts == 0 && state == FRC_STATE_ENABLE) {
		devp->frc_sts.frame_cnt = 0;
		devp->frc_sts.state_transing = false;
		pr_frc(0, "%s %d->%d, no video, can't change\n", __func__,
				devp->frc_sts.state, state);
	} else if (devp->frc_sts.state_transing) {
		pr_frc(0, "%s state_transing busy(%d:%d->%d,frm:%d)!\n", __func__,
				devp->frc_sts.state, devp->frc_sts.new_state,
						state, devp->frc_sts.frame_cnt);
		if (state != devp->frc_sts.new_state) {
			devp->frc_sts.state = devp->frc_sts.new_state;
			devp->frc_sts.new_state = state;
			devp->frc_sts.frame_cnt = 0;
			devp->frc_sts.state_transing = false;
			pr_frc(0, "busy broken:%s %d->%d\n", __func__,
					devp->frc_sts.state, state);
		}
	} else if (devp->frc_sts.state != state) {
		devp->frc_sts.new_state = state;
		devp->frc_sts.state_transing = true;
		pr_frc(2, "%s %d->%d(frm=%d)\n", __func__, devp->frc_sts.state,
				state, devp->frc_sts.frame_cnt);
	}
}

static bool frc_osd_window_en(struct st_frc_in_sts *cur_in_sts)
{
	struct frc_dev_s *devp = get_frc_devp();
	if (get_chip_type() != ID_T3X)
		return false;

	if ((cur_in_sts->in_hsize < HSIZE_MAX_4K && cur_in_sts->in_hsize > HSIZE_MIN_4K) ||
		(cur_in_sts->in_vsize < VSIZE_MAX_2K && cur_in_sts->in_vsize > VSIZE_MIN_2K))
		return true;
	else if ((devp->out_sts.vout_width == WIDTH_2K && devp->out_sts.vout_height == HEIGHT_1K) &&
		((cur_in_sts->in_hsize < HSIZE_MAX_2K && cur_in_sts->in_hsize > HSIZE_MIN_2K) ||
		(cur_in_sts->in_vsize < VSIZE_MAX_1K && cur_in_sts->in_vsize > VSIZE_MIN_1K)))
		return true;

	return false;
}

static void frc_fast_disable_process(void)
{
	struct frc_dev_s *devp;
//	u32 tmp_frm_size;

	devp = get_frc_devp();
	devp->in_sts.vf_sts = 0;
	devp->in_sts.no_vf_cnt = 0;
	devp->frc_sts.out_put_mode_changed = FRC_EVENT_VF_CHG_IN_SIZE;

	set_frc_enable(false);
//	frc_change_to_state(FRC_STATE_DISABLE);
	frc_change_to_state(FRC_STATE_BYPASS);
	devp->st_change = 0;
	devp->next_frame = 1;
	devp->need_bypass = 1;
	frc_state_change_finish(devp);
	pr_frc(2, "0x3217 = %x  0x35 = %x\n", vpu_reg_read(0x3217),
		READ_FRC_REG(FRC_REG_OUT_INT_FLAG));
}

static void frc_disable_deal_diff_win(bool window)
{
	struct frc_dev_s *devp = get_frc_devp();

	if (window) {
		if (devp->st_change == 1 || devp->st_change == 2) {
			return;
		} else if (devp->frc_sts.state == FRC_STATE_ENABLE ||
			devp->frc_sts.new_state == FRC_STATE_ENABLE) {
			pr_frc(2, "start disable frc : %d", window);
			frc_fast_disable_process();
		}
		devp->in_sts.have_vf_cnt = 0;
	} else {
		if (devp->st_change == 1 || devp->st_change == 2) {
			return;
		} else if (devp->frc_sts.state == FRC_STATE_ENABLE ||
			devp->frc_sts.new_state == FRC_STATE_ENABLE) {
			pr_frc(2, "start disable frc : %d", window);
			frc_fast_disable_process();
		}
	}
}

const char * const frc_state_ary[] = {
	"FRC_STATE_DISABLE",
	"FRC_STATE_ENABLE",
	"FRC_STATE_BYPASS",
};

int frc_update_in_sts(struct frc_dev_s *devp, struct st_frc_in_sts *frc_in_sts,
				struct vframe_s *vf, struct vpp_frame_par_s *cur_video_sts)
{
	struct frc_fw_data_s *pfw_data;

	if (!vf || !cur_video_sts) {
		frc_in_sts->in_hsize = 0;
		frc_in_sts->in_vsize = 0;
		return -1;
	}

	pfw_data = (struct frc_fw_data_s *)devp->fw_data;
	frc_in_sts->vf_type = vf->type;
	frc_in_sts->duration = vf->duration;
	frc_in_sts->signal_type = vf->signal_type;
	frc_in_sts->source_type = vf->source_type;
	frc_in_sts->vf = vf;
	frc_in_sts->vf_index = vf->omx_index;

	if (frc_in_sts->duration > 0 && devp->in_out_ratio != FRC_RATIO_1_1) {
		pfw_data->frc_top_type.frc_in_frm_rate =
			(1000000 / devp->in_sts.vs_duration);
		pfw_data->frc_top_type.video_duration = (u16)(frc_in_sts->duration);
	} else if (frc_in_sts->duration > 0 &&
				devp->in_out_ratio == FRC_RATIO_1_1) {
		pfw_data->frc_top_type.frc_in_frm_rate =
				pfw_data->frc_top_type.frc_out_frm_rate;
	} else {
		pfw_data->frc_top_type.frc_in_frm_rate = 0;
	}

	if (!frc_in_sts->vf_sts) {
		frc_in_sts->in_hsize = 0;
		frc_in_sts->in_vsize = 0;
	} else if (devp->dbg_force_en && devp->dbg_input_hsize &&
					devp->dbg_input_vsize) {
		frc_in_sts->in_hsize = devp->dbg_input_hsize;
		frc_in_sts->in_vsize = devp->dbg_input_vsize;
	} else {
		if (devp->frc_hw_pos == FRC_POS_AFTER_POSTBLEND) {
			frc_in_sts->in_hsize = devp->out_sts.vout_width;
			frc_in_sts->in_vsize = devp->out_sts.vout_height;
		} else {
			//frc_in_sts->in_hsize = cur_video_sts->nnhf_input_w;
			//frc_in_sts->in_vsize = cur_video_sts->nnhf_input_h;
			frc_in_sts->in_hsize = cur_video_sts->frc_h_size;
			frc_in_sts->in_vsize = cur_video_sts->frc_v_size;
		}
	}
//	if (devp->frc_sts.vs_cnt < 100)
//		pr_frc(2, "vs_cnt %d: get in size(%d,%d) vd_sr_out(%d,%d)\n",
//			devp->frc_sts.vs_cnt,
//			frc_in_sts->in_hsize, frc_in_sts->in_vsize,
//			cur_video_sts->frc_h_size, cur_video_sts->frc_v_size);

	if (frc_in_sts->frc_hd_start_lines != cur_video_sts->VPP_hd_start_lines_ ||
		frc_in_sts->frc_hd_end_lines != cur_video_sts->VPP_hd_end_lines_) {
		frc_in_sts->frc_hd_start_lines = cur_video_sts->VPP_hd_start_lines_;
		frc_in_sts->frc_hd_end_lines = cur_video_sts->VPP_hd_end_lines_;
	}
	if (frc_in_sts->frc_vd_start_lines != cur_video_sts->VPP_vd_start_lines_ ||
		frc_in_sts->frc_vd_end_lines != cur_video_sts->VPP_vd_end_lines_) {
		frc_in_sts->frc_vd_start_lines = cur_video_sts->VPP_vd_start_lines_;
		frc_in_sts->frc_vd_end_lines = cur_video_sts->VPP_vd_end_lines_;
	}

	if (frc_in_sts->frc_vsc_startp != cur_video_sts->VPP_vsc_startp ||
		frc_in_sts->frc_hsc_startp != cur_video_sts->VPP_hsc_startp) {
		frc_in_sts->frc_vsc_startp = cur_video_sts->VPP_vsc_startp;
		frc_in_sts->frc_hsc_startp = cur_video_sts->VPP_hsc_startp;
	}
	return 0;
}

enum efrc_event frc_input_sts_check(struct frc_dev_s *devp,
						struct st_frc_in_sts *cur_in_sts)
{
	/* check change */
	enum efrc_event sts_change = FRC_EVENT_NO_EVENT;
	//enum frc_state_e cur_state = devp->frc_sts.state;
	u32 cur_sig_in;
	u32 tmpvalue;
	bool is_osd_window;
	struct frc_fw_data_s *pfw_data;
	struct frc_top_type_s *frc_top;
	struct vinfo_s *vinfo = get_current_vinfo();
	static u16 seamless_cnt;

	is_osd_window = frc_osd_window_en(cur_in_sts);
	pfw_data = (struct frc_fw_data_s *)devp->fw_data;
	frc_top = &pfw_data->frc_top_type;

	/*back up*/
	devp->in_sts.vf_type = cur_in_sts->vf_type;
	devp->in_sts.duration = cur_in_sts->duration;
	devp->in_sts.signal_type = cur_in_sts->signal_type;
	devp->in_sts.source_type = cur_in_sts->source_type;
	devp->in_sts.frc_vsc_startp = cur_in_sts->frc_vsc_startp;
	devp->in_sts.frc_hsc_startp = cur_in_sts->frc_hsc_startp;
	devp->in_sts.vf_index =	cur_in_sts->vf_index;
	frc_top->inp_padding_xofst = devp->in_sts.frc_hsc_startp;
	frc_top->inp_padding_yofst = devp->in_sts.frc_vsc_startp;

	if (devp->next_frame == 1) {
		devp->need_bypass = 0;
		devp->next_frame++;
		pr_frc(2, "0x3217 = %x  0x35 = %x\n", vpu_reg_read(0x3217),
		READ_FRC_REG(FRC_REG_OUT_INT_FLAG));
	} else if (devp->next_frame == 2) {
		devp->next_frame = 0;
		pr_frc(2, "0x3217 = %x  0x35 = %x\n", vpu_reg_read(0x3217),
		READ_FRC_REG(FRC_REG_OUT_INT_FLAG));
	}

	/* check h size change */
	devp->in_sts.size_chged = 0;
	devp->in_sts.t3x_proc_size_chg = 0;
	if (devp->in_sts.in_hsize != cur_in_sts->in_hsize) {
		pr_frc(1, "hsize change (%d - %d)\n",
			devp->in_sts.in_hsize, cur_in_sts->in_hsize);
		devp->in_sts.in_hsize = cur_in_sts->in_hsize;
		devp->in_sts.t3x_proc_size_chg = 1;
		if (get_chip_type() == ID_T3X) {
			frc_disable_deal_diff_win(is_osd_window);
		} else if (devp->frc_sts.state == FRC_STATE_ENABLE && get_chip_type() >= ID_T5M) {
			pr_frc(2, "%s start disable frc", __func__);
			set_frc_enable(false);
			set_frc_bypass(true);
			// frc_change_to_state(FRC_STATE_DISABLE);
			frc_change_to_state(FRC_STATE_BYPASS);
			frc_state_change_finish(devp);
		}
		if (devp->in_sts.frc_seamless_en) {
			if (devp->in_sts.in_hsize == 0) {
				/*need reconfig*/
				devp->frc_sts.re_cfg_cnt = frc_re_cfg_cnt;
				sts_change |= FRC_EVENT_VF_CHG_IN_SIZE;
			} else if  (devp->frc_sts.state == FRC_STATE_ENABLE) {
				devp->in_sts.size_chged = 1;
			}
		} else {
			/*need reconfig*/
			devp->frc_sts.re_cfg_cnt = frc_re_cfg_cnt;
			sts_change |= FRC_EVENT_VF_CHG_IN_SIZE;
		}
	}
	/* check v size change */
	if (devp->in_sts.in_vsize != cur_in_sts->in_vsize) {
		pr_frc(1, "vsize change (%d - %d)\n",
			devp->in_sts.in_vsize, cur_in_sts->in_vsize);
		devp->in_sts.in_vsize = cur_in_sts->in_vsize;
		devp->in_sts.t3x_proc_size_chg = 1;
		if (get_chip_type() == ID_T3X) {
			frc_disable_deal_diff_win(is_osd_window);
		} else if (devp->frc_sts.state == FRC_STATE_ENABLE && get_chip_type() >= ID_T5M) {
			pr_frc(2, "%s start disable frc", __func__);
			set_frc_enable(false);
			set_frc_bypass(true);
			// frc_change_to_state(FRC_STATE_DISABLE);
			frc_change_to_state(FRC_STATE_BYPASS);
			frc_state_change_finish(devp);
		}
		if (devp->in_sts.frc_seamless_en) {
			if (devp->in_sts.in_vsize == 0) {
				/*need reconfig*/
				devp->frc_sts.re_cfg_cnt = frc_re_cfg_cnt;
				sts_change |= FRC_EVENT_VF_CHG_IN_SIZE;
			} else if (devp->frc_sts.state == FRC_STATE_ENABLE) {
				devp->in_sts.size_chged = 1;
			}
		} else {
			/*need reconfig*/
			devp->frc_sts.re_cfg_cnt = frc_re_cfg_cnt;
			sts_change |= FRC_EVENT_VF_CHG_IN_SIZE;
		}
	}
	if (devp->in_sts.frc_seamless_en && !devp->in_sts.frc_is_tvin) {
		if (seamless_cnt == 1) {
			frc_input_init(devp, frc_top);
			if (get_chip_type() == ID_T3X) {
				if (frc_top->frc_ratio_mode == FRC_RATIO_1_2)
					tmpvalue = (frc_top->hsize + 15) & 0xFFF0;
				else
					tmpvalue = frc_top->hsize;
				tmpvalue |= (frc_top->vsize) << 16;
				WRITE_FRC_REG_BY_CPU(FRC_FRAME_SIZE, tmpvalue);
			} else {
				tmpvalue = frc_top->hsize;
				tmpvalue |= (frc_top->vsize) << 16;
				WRITE_FRC_REG_BY_CPU(FRC_FRAME_SIZE, tmpvalue);
			}
			frc_top->is_me1mc4 = 1;/*me:mc 1:4*/
			WRITE_FRC_REG_BY_CPU(FRC_INPUT_SIZE_ALIGN, 0x3);   //16*16 align
			WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL27,
				(cur_in_sts->frc_hsc_startp) << 13 |
				(cur_in_sts->frc_vsc_startp));

			pr_frc(2, "manual set frc size:v(x):0x%x, h(y)0x%x\n",
				cur_in_sts->frc_hd_start_lines, cur_in_sts->frc_vd_start_lines);

			if (pfw_data->frc_input_cfg)
				pfw_data->frc_input_cfg(devp->fw_data);
			devp->in_sts.size_chged = 0;
			pr_frc(2, " %s size changed, frc not reopen\n", __func__);

			seamless_cnt = 0;
			pr_frc(2, "seamless frm 2\n");
			return sts_change;
		} else if (devp->in_sts.size_chged) {
			if (!seamless_cnt || seamless_cnt > 1)
				seamless_cnt = 1; // frame cnt 1
			pr_frc(2, "seamless frm 1 seamless_cnt:%d\n", seamless_cnt);
		} else if (!devp->in_sts.size_chged) {
			WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL27,
				(cur_in_sts->frc_hsc_startp) << 13 |
				(cur_in_sts->frc_vsc_startp));
			seamless_cnt = 0;
			// pr_frc(2, "seamless frm 3\n");
		}
	} else {
		frc_top->inp_padding_xofst = 0;
		frc_top->inp_padding_yofst = 0;
	}

	if (devp->frc_sts.out_put_mode_changed || devp->frc_sts.re_config) {
		pr_frc(1, "out_put_mode_changed 0x%x re_config:%d\n",
			devp->frc_sts.out_put_mode_changed,
			devp->frc_sts.re_config);
		if (devp->frc_sts.out_put_mode_changed ==
			FRC_EVENT_VF_CHG_IN_SIZE) {
			devp->frc_sts.re_cfg_cnt = 5;
		} else if (devp->frc_sts.out_put_mode_changed ==
			FRC_EVENT_VOUT_CHG) {
			if (devp->out_sts.vout_width != vinfo->width ||
				devp->out_sts.vout_height != vinfo->height)
				devp->frc_sts.re_cfg_cnt = 3;
			pr_frc(1, "out_chg-w(%d->%d)-h(%d->%d)-d(%d->%d)\n",
				devp->out_sts.vout_width, vinfo->width,
				devp->out_sts.vout_height, vinfo->height,
				devp->out_sts.out_framerate,
				(vinfo->sync_duration_num * 100 / vinfo->sync_duration_den) / 100);
			if (devp->out_sts.vout_width == vinfo->width &&
				devp->out_sts.vout_height == vinfo->height &&
				devp->frc_sts.state == FRC_STATE_ENABLE &&
				!devp->in_sts.size_chged) {
				devp->frc_sts.re_cfg_cnt = 0;
				pr_frc(1, "%s rate chg\n", __func__);
			}
			//devp->out_sts.vout_height = vinfo->height;
			//devp->out_sts.vout_width = vinfo->width;
		} else {
			devp->frc_sts.re_cfg_cnt = frc_re_cfg_cnt;
		}
		sts_change |= FRC_EVENT_VOUT_CHG;
		devp->frc_sts.out_put_mode_changed = 0;
		devp->frc_sts.re_config = 0;
	}

	/* check is same vframe */
	pr_frc(dbg_sts, "vf (0x%lx, 0x%lx)\n", (ulong)devp->in_sts.vf, (ulong)cur_in_sts->vf);
	if (devp->in_sts.vf == cur_in_sts->vf && cur_in_sts->vf_sts)
		devp->in_sts.vf_repeat_cnt++;

	devp->in_sts.vf = cur_in_sts->vf;

	if (devp->frc_sts.re_cfg_cnt) {
		devp->frc_sts.re_cfg_cnt--;
		cur_sig_in = false;
	} else if (devp->in_sts.in_hsize < FRC_H_LIMIT ||
			devp->in_sts.in_vsize < FRC_V_LIMIT) {
		cur_sig_in = false;
	} else {
		cur_sig_in = cur_in_sts->vf_sts;
	}

	pr_frc(dbg_sts, "vf_sts: %d, cur_sig_in:0x%x have_cnt:%d no_cnt:%d re_cfg_cnt:%d\n",
		devp->in_sts.vf_sts, cur_sig_in,
		devp->in_sts.have_vf_cnt, devp->in_sts.no_vf_cnt, devp->frc_sts.re_cfg_cnt);

	switch (devp->in_sts.vf_sts) {
	case VFRAME_NO:
		if (cur_sig_in == VFRAME_HAVE) {
			if (devp->in_sts.have_vf_cnt++ >=
				(frc_enable_cnt + (is_osd_window ? WINDOW_DELAY_CNT : 0))) {
				devp->in_sts.vf_sts = cur_sig_in;
				//if (FRC_EVENT_VF_IS_GAME)
				sts_change |= FRC_EVENT_VF_CHG_TO_HAVE;
				devp->in_sts.have_vf_cnt = 0;
				pr_frc(1, "FRC_EVENT_VF_CHG_TO_HAVE\n");
			}
		} else {
			devp->in_sts.have_vf_cnt = 0;
			if (devp->frc_sts.state == FRC_STATE_DISABLE &&
				devp->in_sts.vf_null_cnt == 200) {
				devp->frc_sts.retrycnt = 0;
				frc_change_to_state(FRC_STATE_BYPASS);
				pr_frc(1, "no_frm->chg bypass\n");
			} else if (devp->frc_sts.state == FRC_STATE_BYPASS &&
					devp->in_sts.vf_null_cnt == 1200 &&
					devp->clk_state == FRC_CLOCK_NOR) {
				devp->clk_state = FRC_CLOCK_NOR2XXX;
				schedule_work(&devp->frc_clk_work);
				pr_frc(1, "no_frm->disable clk\n");
			}
		}
		break;
	case VFRAME_HAVE:
		if (cur_sig_in == VFRAME_NO) {
			if (devp->in_sts.no_vf_cnt++ >= frc_disable_cnt) {
				devp->in_sts.vf_sts = cur_sig_in;
				devp->in_sts.no_vf_cnt = 0;
				pr_frc(1, "FRC_EVENT_VF_CHG_TO_NO\n");
				sts_change |= FRC_EVENT_VF_CHG_TO_NO;
			}
		} else {
			devp->in_sts.no_vf_cnt = 0;
		}
		break;
	}

	/* even attach , mode change */
	if (devp->frc_sts.auto_ctrl && sts_change) {
		if (sts_change & FRC_EVENT_VF_CHG_TO_NO)
			frc_change_to_state(FRC_STATE_DISABLE);
			//frc_change_to_state(FRC_STATE_BYPASS);
		else if (sts_change & FRC_EVENT_VF_CHG_TO_HAVE)
			frc_change_to_state(FRC_STATE_ENABLE);
	}
	if (devp->dbg_vf_monitor)
		frc_vf_monitor(devp);

	return sts_change;
}

/*video_input_w
 * input vframe and display mode handle
 *
 */
void frc_input_vframe_handle(struct frc_dev_s *devp, struct vframe_s *vf,
					struct vpp_frame_par_s *cur_video_sts)
{
	struct st_frc_in_sts cur_in_sts;
	u32 no_input = false, vd_en_flag;// vd_regval;
	enum efrc_event frc_event = FRC_EVENT_NO_EVENT;

	if (!devp)
		return;

	if (!devp->probe_ok || !devp->power_on_flag)
		return;

	frc_in_sts_init(&cur_in_sts);
	vd_en_flag = get_video_enabled(0);
	// vd_regval = vpu_reg_read(0x1dfb);
	if (devp->ud_dbg.res1_dbg_en == 1)
		pr_frc(1, "get_vd_en=%2d\n", vd_en_flag);
	if (!vf || !cur_video_sts || vd_en_flag == 0) {
		devp->in_sts.vf_null_cnt++;
		no_input = true;
	} //else if ((vd_regval & (BIT_0 | BIT_8)) == 0) {
		//devp->in_sts.vf_null_cnt++;
		//no_input = true;
	//}

	if (vf) {
		if ((vf->flag & VFRAME_FLAG_GAME_MODE)  ==
					VFRAME_FLAG_GAME_MODE) {
			if ((devp->in_sts.st_flag & FRC_FLAG_GAME_MODE) !=
						FRC_FLAG_GAME_MODE) {
				devp->in_sts.st_flag =
				devp->in_sts.st_flag | FRC_FLAG_GAME_MODE;
				pr_frc(1, "video = game_mode");
			}
			no_input = true;
		} else if ((vf->flag & VFRAME_FLAG_PC_MODE)  ==
					VFRAME_FLAG_PC_MODE) {
			if ((devp->in_sts.st_flag & FRC_FLAG_PC_MODE) !=
						FRC_FLAG_PC_MODE) {
				devp->in_sts.st_flag =
					devp->in_sts.st_flag | FRC_FLAG_PC_MODE;
				pr_frc(1, "video = pc_mode");
			}
			no_input = true;
		} else if ((vf->flag & VFRAME_FLAG_HIGH_BANDWIDTH) ==
				VFRAME_FLAG_HIGH_BANDWIDTH) {
			if ((devp->in_sts.st_flag & FRC_FLAG_HIGH_BW) !=
						FRC_FLAG_HIGH_BW) {
				devp->in_sts.st_flag =
					devp->in_sts.st_flag | FRC_FLAG_HIGH_BW;
				pr_frc(1, "video = high_bw");
			}
			no_input = true;
		} else {
			devp->in_sts.st_flag =
				devp->in_sts.st_flag &
					(~(FRC_FLAG_GAME_MODE +
						FRC_FLAG_HIGH_BW +
						FRC_FLAG_PC_MODE));
		}

		if ((vf->type & VIDTYPE_PIC) == VIDTYPE_PIC) {
			if ((devp->in_sts.st_flag & FRC_FLAG_PIC_MODE) !=
						FRC_FLAG_PIC_MODE) {
				devp->in_sts.st_flag =
				devp->in_sts.st_flag | FRC_FLAG_PIC_MODE;
				pr_frc(1, "video = pic_mode");
			}
			no_input = true;
		} else {
			devp->in_sts.st_flag =
				devp->in_sts.st_flag & (~FRC_FLAG_PIC_MODE);
		}

		if (!cur_video_sts) {
			pr_frc(1, "vpp_frame_par_s is NULL");
			no_input = true;
		} else if (cur_video_sts->frc_h_size < FRC_H_LIMIT ||
				cur_video_sts->frc_v_size < FRC_V_LIMIT) {
			if ((devp->in_sts.st_flag & FRC_FLAG_INSIZE_ERR) !=
						FRC_FLAG_INSIZE_ERR) {
				devp->in_sts.st_flag =
				devp->in_sts.st_flag | FRC_FLAG_INSIZE_ERR;
				pr_frc(1, "video = err_insize");
			}
			no_input = true;
		} else if (cur_video_sts->frc_h_size > 3840 + 8 ||
				cur_video_sts->frc_v_size > 2160 + 8) {
			if ((devp->in_sts.st_flag & FRC_FLAG_INSIZE_ERR) !=
						FRC_FLAG_INSIZE_ERR) {
				devp->in_sts.st_flag =
				devp->in_sts.st_flag | FRC_FLAG_INSIZE_ERR;
				pr_frc(1, "video = err_insize");
			}
			no_input = true;
		} else {
			devp->in_sts.st_flag =
				devp->in_sts.st_flag & (~FRC_FLAG_INSIZE_ERR);
		}

		if (devp->out_sts.out_framerate == FRC_VD_FPS_144 ||
			devp->out_sts.out_framerate == FRC_VD_FPS_288) {
			if ((devp->in_sts.st_flag & FRC_FLAG_LIMIT_FREQ) !=
						FRC_FLAG_LIMIT_FREQ) {
				devp->in_sts.st_flag =
				devp->in_sts.st_flag | FRC_FLAG_LIMIT_FREQ;
				pr_frc(1, "video = limit_freq");
			}
			no_input = true;
		} else {
			devp->in_sts.st_flag =
				devp->in_sts.st_flag & (~FRC_FLAG_LIMIT_FREQ);
		}

		if (get_video_mute()) {
			if ((devp->in_sts.st_flag & FRC_FLAG_MUTE_ST) !=
						FRC_FLAG_MUTE_ST) {
				devp->in_sts.st_flag =
				devp->in_sts.st_flag | FRC_FLAG_MUTE_ST;
				pr_frc(1, "video = video mute");
			}
			devp->frc_sts.video_mute_cnt++;
			if (devp->out_sts.out_framerate < 70)
				no_input = devp->dbg_mute_disable;
			pr_frc(1, "video_mute_cnt = %d", devp->frc_sts.video_mute_cnt);
		} else {
			devp->in_sts.st_flag =
				devp->in_sts.st_flag & (~FRC_FLAG_MUTE_ST);
		}

		if ((vf->flag & VFRAME_FLAG_VIDEO_SECURE) ==
				 VFRAME_FLAG_VIDEO_SECURE) {
			devp->in_sts.secure_mode = true;
			/*for test secure mode disable memc*/
			//no_input = true;
		} else {
			devp->in_sts.secure_mode = false;
		}
		//input security check
		// frc_check_secure_mode(vf, devp); /*get secure mode will delay 10 frames*/
		/*check vd status change*/
		if (!no_input) {
			frc_chk_vd_sts_chg(devp, vf);
			if (!devp->in_sts.frc_is_tvin)
				frc_char_flash_check();
			else
				frc_set_seamless_proc(0); // tvin close seamless
		}

		if (devp->in_sts.frc_vf_rate == FRC_VD_FPS_100 ||
			devp->in_sts.frc_vf_rate == FRC_VD_FPS_120) {
			if ((devp->in_sts.st_flag & FRC_FLAG_HIGH_FREQ) !=
						FRC_FLAG_HIGH_FREQ) {
				devp->in_sts.st_flag =
					devp->in_sts.st_flag | FRC_FLAG_HIGH_FREQ;
				pr_frc(1, "high freq\n");
			}
			no_input = true;
		} else {
			devp->in_sts.st_flag =
				devp->in_sts.st_flag & (~FRC_FLAG_HIGH_FREQ);
		}

		if (devp->in_sts.frc_vf_rate == FRC_VD_FPS_00) {
			if ((devp->in_sts.st_flag & FRC_FLAG_ZERO_FREQ) !=
						FRC_FLAG_ZERO_FREQ) {
				devp->in_sts.st_flag =
					devp->in_sts.st_flag | FRC_FLAG_ZERO_FREQ;
				pr_frc(1, "zero freq\n");
			}
			if (vf->duration <= 1500)
				no_input = true;
		} else {
			devp->in_sts.st_flag =
				devp->in_sts.st_flag & (~FRC_FLAG_ZERO_FREQ);
		}
	}

	/*secure mode*/
	if (devp->in_sts.secure_mode != devp->buf.secured &&
		devp->buf.cma_mem_alloced) {
		if (devp->in_sts.secure_mode == 0 &&
			devp->buf.secured == 1) {
			no_input = true;
			frc_re_cfg_cnt = 0;  // need reopen instantly
		}
		schedule_work(&devp->frc_secure_work);
		//pr_frc(2, "frc_re_cfg_cnt:%d pre_secure_mode:%d\n",
			//frc_re_cfg_cnt, devp->buf.secured);
	} else {
		frc_re_cfg_cnt = FRC_RE_CFG_CNT;
	}

	if (devp->frc_hw_pos == FRC_POS_AFTER_POSTBLEND)
		cur_in_sts.vf_sts = true;
	else
		cur_in_sts.vf_sts = no_input ? false : true;

	if (cur_in_sts.vf_sts) {
		devp->frc_sts.vs_data_cnt++;
		if (devp->frc_sts.vs_data_cnt < 50)
			pr_frc(2, "vpp vsync (data) idx =%d, 0x1a1c =0x%x, 0x3f01 =0x%x\n",
				devp->frc_sts.vs_data_cnt,
				vpu_reg_read(devp->vpu_byp_frc_reg_addr),
				READ_FRC_REG(0x3f01));
	}
	frc_update_in_sts(devp, &cur_in_sts, vf, cur_video_sts);

	/* check input is change */
	frc_event = frc_input_sts_check(devp, &cur_in_sts);

	if (frc_event)
		pr_frc(1, "event = 0x%08x\n", frc_event);
}

void frc_state_change_finish(struct frc_dev_s *devp)
{
	if (devp->frc_sts.state == FRC_STATE_ENABLE &&
		(devp->frc_sts.new_state == FRC_STATE_BYPASS ||
		devp->frc_sts.new_state == FRC_STATE_DISABLE))
		devp->frc_sts.changed_flag = 1;

	devp->frc_sts.state = devp->frc_sts.new_state;
	devp->frc_sts.state_transing = false;
	devp->frc_sts.frame_cnt = 0;
}

void frc_test_mm_secure_set_off(struct frc_dev_s *devp)
{
	if (!tee_enabled()) {
		pr_frc(0, "tee is not enable\n");
		return;
	}

	if (secure_tee_handle) {
		tee_unprotect_mem(secure_tee_handle);
		pr_frc(0, "%s handl:%d\n", __func__, secure_tee_handle);
		secure_tee_handle = 0;
	}
}

void frc_test_mm_secure_set_on(struct frc_dev_s *devp, u32 start, u32 size)
{
	if (!tee_enabled()) {
		pr_frc(0, "tee is not enable\n");
		return;
	}

	if (!secure_tee_handle) {
		tee_protect_mem_by_type(TEE_MEM_TYPE_FRC, start, size, &secure_tee_handle);
		pr_frc(0, "%s handl:%d start:0x%x size:0x%x\n", __func__,
			secure_tee_handle, start, size);
	}
}

void frc_mm_secure_set(struct frc_dev_s *devp)
{
	phys_addr_t addr_start;
	u32 addr_size;
	enum frc_state_e new_state;

	if (!tee_enabled()) {
		pr_frc(0, "tee is not enable\n");
		return;
	}
	if (!devp)
		return;
	if (!devp->probe_ok || !devp->power_on_flag)
		return;

	/*data buffer set to secure mode*/
	addr_start = devp->buf.secure_start;
	// data buf size: 0x9a30000
	addr_size = devp->buf.secure_size;

	/*data buffer, me/mc info and link buffer set to secure mode*/
	//addr_start = devp->buf.cma_mem_paddr_start + devp->buf.lossy_mc_y_info_buf_paddr;
	//addr_size = devp->buf.norm_hme_data_buf_paddr[0] - devp->buf.lossy_mc_y_info_buf_paddr;

	new_state = devp->frc_sts.new_state;

	/*secure mode check*/
	if (!devp->in_sts.secure_mode) {
		if (devp->buf.secured) {
			devp->buf.secured = false;
			/*call secure api to exit secure mode*/
			tee_unprotect_mem(secure_tee_handle);
			frc_force_secure(false);
			pr_frc(1, "%s tee_unprotect_mem %d\n", __func__,
				secure_tee_handle);
			secure_tee_handle = 0;
		}
	} else {
		/*need set mm to secure mode*/
		if (new_state == FRC_STATE_ENABLE) {
			if (!devp->buf.secured) {
				devp->buf.secured = true;
				/*call secure api enter secure mode*/
				tee_protect_mem_by_type(TEE_MEM_TYPE_FRC, addr_start,
							addr_size, &secure_tee_handle);
				frc_force_secure(true);
				pr_frc(1, "%s handl:%d addr_start:0x%lx addr_size:0x%x\n",
					__func__, secure_tee_handle,
					(ulong)addr_start, addr_size);
			}
		} else {
			if (devp->buf.secured) {
				devp->buf.secured = false;
				/*call secure api to exit secure mode*/
				tee_unprotect_mem(secure_tee_handle);
				frc_force_secure(false);
				pr_frc(1, "%s tee_unprotect_mem %d\n", __func__,
					secure_tee_handle);
				secure_tee_handle = 0;
			}
		}
	}
}

void frc_state_handle(struct frc_dev_s *devp)
{
	enum frc_state_e cur_state;
	enum frc_state_e new_state;
	struct frc_fw_data_s *pfw_data;
	u32 state_changed = 0;
	u32 frame_cnt = 0;
	static u8 forceidx;
	static u8 bypasscnt;
	static u8 freezecnt;
	static u8 off2on_cnt;
	u8 frc_input_fid = 0;
	u32 read0x60 = 0;
	u8 chg_flag = 0;
	u32 log = 1;

	cur_state = devp->frc_sts.state;
	new_state = devp->frc_sts.new_state;
	frame_cnt = devp->frc_sts.frame_cnt;
	chg_flag = devp->ud_dbg.res2_time_en;
	pfw_data = (struct frc_fw_data_s *)devp->fw_data;

	if (cur_state != new_state) {
		state_changed = 1;
		pr_frc(log, "stat_chg(%d->%d), frm_cnt:%d\n", cur_state,
			new_state, frame_cnt);
		if (chg_flag < 0x1F && frame_cnt < 20 &&
					devp->clk_state == FRC_CLOCK_NOR)
			pr_frc(log, "frm:%d,reg_0x102:0x%8X,0x113:0x%8X,0x146:0x%8X,0x147:0x%8X\n",
				frame_cnt,
				READ_FRC_REG(FRC_REG_PAT_POINTER),
				READ_FRC_REG(FRC_REG_OUT_FID),
				READ_FRC_REG(FRC_REG_FWD_PHS),
				READ_FRC_REG(FRC_REG_FWD_FID));
		pr_frc(log, "in_cnt:%X,out_cnt:%X,DBG1:0x%08X,DBG2:0x%08X,DBG3:0x%08X\n",
			READ_FRC_REG(0x6d),
			READ_FRC_REG(0x6e),
			READ_FRC_REG(FRC_REG_INP_HS_DBG1),
			READ_FRC_REG(FRC_REG_INP_HS_DBG2),
			READ_FRC_REG(FRC_REG_INP_HS_DBG3));
		pr_frc(log, "INP_MCDW_CTRL=0x%x, MCDW_CTRL=0x%x vs_cnt=%d\n",
			READ_FRC_REG(FRC_INP_MCDW_CTRL),
			READ_FRC_REG(FRC_MCDW_PATH_CTRL),
			devp->frc_sts.vs_cnt);
	}
	switch (cur_state) {
	case FRC_STATE_DISABLE:
	if (state_changed) {
		if (new_state == FRC_STATE_BYPASS) {
			set_frc_bypass(ON);
			schedule_work(&devp->frc_secure_work);
			devp->frc_sts.frame_cnt = 0;
			pr_frc(log, "stat_chg %s -> %s done\n",
					frc_state_ary[cur_state],
					frc_state_ary[new_state]);
			frc_state_change_finish(devp);
			off2on_cnt = 0;
		} else if (new_state == FRC_STATE_ENABLE) {
			if (devp->frc_sts.frame_cnt == 0) {
				if (devp->clk_state != FRC_CLOCK_NOR &&
					devp->clk_state != FRC_CLOCK_XXX2NOR) {
					devp->clk_state = FRC_CLOCK_XXX2NOR;
					schedule_work(&devp->frc_clk_work);
				} else if (devp->clk_state == FRC_CLOCK_NOR &&
					devp->buf.cma_mem_alloced) {
					// frc_mm_secure_set(devp);
					frc_clr_badedit_effect_before_enable();
					schedule_work(&devp->frc_secure_work);
					get_vout_info(devp);
					frc_hw_initial(devp);
					//first : set bypass off
					set_frc_bypass(OFF);
					read0x60 = READ_FRC_REG(FRC_REG_TOP_RESERVE0) & 0xFF;
					bypasscnt = (read0x60 & 0xf0) >> 4;
					freezecnt = read0x60 & 0x0f;
					frc_get_film_base_vf(devp);
					if (freezecnt < (pfw_data->frc_top_type.vfp & 0x0f))
						freezecnt = pfw_data->frc_top_type.vfp & 0x0f;
					else
						pfw_data->frc_top_type.vfp |= (freezecnt & 0x0f);
					if (pfw_data->frc_input_cfg)
						pfw_data->frc_input_cfg(devp->fw_data);
					//second: set frc enable on
					frc_memc_clr_vbuffer(devp, 1);
					set_frc_enable(ON);
					devp->frc_fw_pause = 0;
					pr_frc(log, "frc_bypass_cnt:%d,freeze_cnt:%d\n",
							bypasscnt, freezecnt);
					devp->frc_sts.frame_cnt++;
				}
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt < bypasscnt) {
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
				if (devp->frc_sts.frame_cnt == bypasscnt && is_rdma_enable())
					t3x_verB_set_cfg(1, devp);
			} else if (devp->frc_sts.frame_cnt == bypasscnt) {
				if (!is_rdma_enable())
					t3x_verB_set_cfg(1, devp);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt == bypasscnt + 1) {
				forceidx = frc_frame_forcebuf_enable(1);
				frc_frame_forcebuf_count(forceidx);
				if (chg_flag == 0)
					frc_memc_120hz_patch_2(devp);
				pr_frc(log, "d-e_freeze idx:%d, frm:%d, chg:%d\n",
					forceidx, devp->frc_sts.frame_cnt, chg_flag);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt > bypasscnt + 1 &&
						devp->frc_sts.frame_cnt <
						freezecnt + bypasscnt + 1) {
				frc_frame_forcebuf_count(forceidx);
				if (devp->frc_sts.frame_cnt == chg_flag + bypasscnt + 1)
					frc_memc_120hz_patch_2(devp);
				frc_input_fid =
				READ_FRC_REG(FRC_REG_PAT_POINTER) >> 4 & 0xF;
				pr_frc(log, "d-e_freezing readidx:%d, frm:%d, chg:%d\n",
					frc_input_fid, devp->frc_sts.frame_cnt, chg_flag);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt ==
						freezecnt + bypasscnt + 1) {
				frc_frame_forcebuf_enable(0);
				frc_memc_120hz_patch_3(devp);
				frc_memc_clr_vbuffer(devp, 0);
				pr_frc(log, "d-e_freezed to open, frm:%d\n",
					devp->frc_sts.frame_cnt);
				// frc_state_change_finish(devp);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (frc_check_film_mode(devp) != 0 || off2on_cnt > 29) {
				pr_frc(log, "d-e_stat_chg %s -> %s[%d] done, used_frm:%d[%d]\n",
						frc_state_ary[cur_state],
						frc_state_ary[new_state],
						frc_check_film_mode(devp),
						off2on_cnt,
						devp->frc_sts.frame_cnt);
				frc_state_change_finish(devp);
				off2on_cnt = 0;
			} else if (devp->frc_sts.frame_cnt > bypasscnt + freezecnt + 1 &&
					 (frc_check_film_mode(devp) == 0)) {
				pr_frc(log, "d-e_detecting film[%d], frm: %d\n",
						frc_check_film_mode(devp),
						devp->frc_sts.frame_cnt);
				off2on_cnt++;
				devp->frc_sts.frame_cnt++;
			}
		} else {
			pr_frc(0, "err new state %d\n", new_state);
		}
	}
	break;
	case FRC_STATE_ENABLE:
	if (state_changed) {
		if (new_state == FRC_STATE_DISABLE) {
			if (devp->frc_sts.frame_cnt == 0) {
				schedule_work(&devp->frc_secure_work);
				frc_frame_forcebuf_enable(0);
				devp->frc_fw_pause = 1;
				set_frc_enable(OFF);
				devp->frc_sts.frame_cnt++;
			} else {
				devp->frc_sts.frame_cnt = 0;
				pr_frc(log, "stat_chg %s -> %s done\n",
					frc_state_ary[cur_state],
					frc_state_ary[new_state]);
				frc_state_change_finish(devp);
			}
		} else if (new_state == FRC_STATE_BYPASS) {
			//first frame set enable off
			if (devp->frc_sts.frame_cnt == 0) {
				schedule_work(&devp->frc_secure_work);
				frc_frame_forcebuf_enable(0);
				devp->frc_fw_pause = 1;
				set_frc_enable(OFF);
				devp->frc_sts.frame_cnt++;
			} else {
				//second frame set bypass on
				set_frc_bypass(ON);
				devp->frc_sts.frame_cnt = 0;
				pr_frc(log, "stat_chg %s->%s done\n",
				       frc_state_ary[cur_state],
				       frc_state_ary[new_state]);
				frc_state_change_finish(devp);
			}
		} else {
			pr_frc(0, "err new state %d\n", new_state);
		}
	}
	break;
	case FRC_STATE_BYPASS:
	if (state_changed) {
		if (new_state == FRC_STATE_DISABLE) {
			if (devp->frc_sts.frame_cnt == 0) {
				if (devp->clk_state != FRC_CLOCK_NOR &&
					devp->clk_state != FRC_CLOCK_XXX2NOR)  {
					devp->clk_state = FRC_CLOCK_XXX2NOR;
					schedule_work(&devp->frc_clk_work);
				} else if (devp->clk_state == FRC_CLOCK_NOR) {
					devp->frc_sts.frame_cnt++;
				}
			} else {
				set_frc_bypass(OFF);
				devp->frc_fw_pause = 1;
				set_frc_enable(OFF);
				devp->frc_sts.frame_cnt = 0;
				pr_frc(log, "stat_chg %s -> %s done\n",
				frc_state_ary[cur_state],
				frc_state_ary[new_state]);
				frc_state_change_finish(devp);
			}
		} else if (new_state == FRC_STATE_ENABLE) {
			if (devp->frc_sts.frame_cnt == 0) {
				if (devp->clk_state != FRC_CLOCK_NOR &&
					devp->clk_state != FRC_CLOCK_XXX2NOR)  {
					devp->clk_state = FRC_CLOCK_XXX2NOR;
					schedule_work(&devp->frc_clk_work);
				} else if (devp->clk_state == FRC_CLOCK_NOR &&
					devp->buf.cma_mem_alloced) {
					//first frame set bypass off
					frc_clr_badedit_effect_before_enable();
					schedule_work(&devp->frc_secure_work);
					frc_memc_clr_vbuffer(devp, 1);
					get_vout_info(devp);
					frc_hw_initial(devp);
					set_frc_bypass(OFF);
					devp->frc_sts.frame_cnt++;
				}
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt == 1) {
				//second frame set enable on
				read0x60 = READ_FRC_REG(FRC_REG_TOP_RESERVE0) & 0xFF;
				bypasscnt = (read0x60 & 0xf0) >> 4;
				freezecnt = read0x60 & 0x0f;
				frc_get_film_base_vf(devp);
				if (freezecnt < (pfw_data->frc_top_type.vfp & 0x0f))
					freezecnt = pfw_data->frc_top_type.vfp & 0x0f;
				else
					pfw_data->frc_top_type.vfp |= (freezecnt & 0x0f);
				if (pfw_data->frc_input_cfg)
					pfw_data->frc_input_cfg(devp->fw_data);
				devp->frc_fw_pause = 0;
				set_frc_enable(ON);
				pr_frc(log, "frc_bypass_cnt:%d,freeze_cnt:%d",
						bypasscnt, freezecnt);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt < bypasscnt + 1) {
				pr_frc(log, "b-e_bypassing frm:%d\n",
					devp->frc_sts.frame_cnt);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
				if (devp->frc_sts.frame_cnt > bypasscnt && is_rdma_enable())
					t3x_verB_set_cfg(1, devp);
			} else if (devp->frc_sts.frame_cnt == bypasscnt + 1) {
				if (!is_rdma_enable())
					t3x_verB_set_cfg(1, devp);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt == bypasscnt + 2) {
				// frc_memc_120hz_patch_3(devp);
				forceidx = frc_frame_forcebuf_enable(1);
				frc_frame_forcebuf_count(forceidx);
				if (chg_flag == 0)
					frc_memc_120hz_patch_2(devp);
				pr_frc(log, "b-e_freeze start, rd_idx:%d, frm:%d, chg:%d\n",
					forceidx, devp->frc_sts.frame_cnt, chg_flag);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt > bypasscnt + 2 &&
					devp->frc_sts.frame_cnt <
					 bypasscnt + freezecnt + 2) {
				frc_frame_forcebuf_count(forceidx);
				if (devp->frc_sts.frame_cnt == chg_flag + bypasscnt + 2)
					frc_memc_120hz_patch_2(devp);
				frc_input_fid =
				READ_FRC_REG(FRC_REG_PAT_POINTER) >> 4 & 0xF;
				pr_frc(log, "b-e_freezing readidx %d, frm: %d, chg:%d\n",
					frc_input_fid, devp->frc_sts.frame_cnt, chg_flag);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt ==
					bypasscnt + freezecnt + 2) {
				frc_frame_forcebuf_enable(0);
				frc_memc_120hz_patch_3(devp);
				frc_memc_clr_vbuffer(devp, 0);
				pr_frc(log, "b-e_freezed to open, frm:%d\n",
					devp->frc_sts.frame_cnt);
				// frc_state_change_finish(devp);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (frc_check_film_mode(devp) != 0 || devp->frc_sts.frame_cnt > 29) {
				pr_frc(log, "b-e_stat_chg %s -> %s[%d] done, used frm:%d[%d]\n",
						frc_state_ary[cur_state],
						frc_state_ary[new_state],
						frc_check_film_mode(devp),
						off2on_cnt,
						devp->frc_sts.frame_cnt);
				frc_state_change_finish(devp);
				off2on_cnt = 0;
			} else if (devp->frc_sts.frame_cnt > bypasscnt + freezecnt + 2 &&
					  (frc_check_film_mode(devp) == 0)) {
				pr_frc(log, "b-e_detecting film[%d], frm: %d\n",
						frc_check_film_mode(devp),
						devp->frc_sts.frame_cnt);
				off2on_cnt++;
				devp->frc_sts.frame_cnt++;
			}
		} else {
			pr_frc(0, "err new state %d\n", new_state);
		}
	}
	break;

	default:
		pr_frc(0, "err state %d\n", cur_state);
		break;
	}
}

void frc_state_handle_new(struct frc_dev_s *devp)
{
	enum frc_state_e cur_state;
	enum frc_state_e new_state;
	struct frc_fw_data_s *pfw_data;
	u32 state_changed = 0;
	u32 frame_cnt = 0;
	u32 vframe_idx = 0;
	static u8 forceidx;
	static u8 bypasscnt;
	static u8 freezecnt;
	static u8 off2on_cnt;
	u8 frc_input_fid = 0;
	u32 read0x60 = 0;
	u8 chg_flag = 0;
	u32 log = 1;

	u32 tmp_frm_size;
	enum chip_id chip;
	struct frc_top_type_s *frc_top;

	chip = get_chip_type();

	cur_state = devp->frc_sts.state;
	new_state = devp->frc_sts.new_state;
	frame_cnt = devp->frc_sts.frame_cnt;
	chg_flag = devp->ud_dbg.res2_time_en;
	vframe_idx = devp->in_sts.vf_index;

	pfw_data = (struct frc_fw_data_s *)devp->fw_data;
	frc_top = &pfw_data->frc_top_type;
	if (cur_state != new_state) {
		state_changed = 1;
		pr_frc(log, "stat_chg(%d->%d), frm_cnt:%d, vd_frm:%d\n", cur_state,
			new_state, frame_cnt, vframe_idx);
		if (chg_flag < 0x1F && frame_cnt < 30 &&
					devp->clk_state == FRC_CLOCK_NOR) {
			pr_frc(log, "frm:%d,reg_0x102:0x%8X,0x113:0x%8X,0x146:0x%8X,0x147:0x%8X\n",
				frame_cnt,
				READ_FRC_REG(FRC_REG_PAT_POINTER),
				READ_FRC_REG(FRC_REG_OUT_FID),
				READ_FRC_REG(FRC_REG_FWD_PHS),
				READ_FRC_REG(FRC_REG_FWD_FID));
			pr_frc(log, "0x3217 = %x  0x1d21 = %x  0x3218 = %x  0x35 = %x\n",
				vpu_reg_read(0x3217), vpu_reg_read(0x1d21),
				vpu_reg_read(0x3218), READ_FRC_REG(FRC_REG_OUT_INT_FLAG));
			pr_frc(2, "0x3f05 = %x\n", READ_FRC_REG(FRC_FRAME_SIZE));
			pr_frc(2, "%s chk INP_MCDW=0x%x,MCDW_CTRL=0x%x, SRCH_RNG_MODE=%x\n",
				__func__,
				READ_FRC_REG(FRC_INP_MCDW_CTRL),
				READ_FRC_REG(FRC_MCDW_PATH_CTRL),
				READ_FRC_REG(FRC_SRCH_RNG_MODE)
				);
		}
	}
	switch (cur_state) {
	case FRC_STATE_DISABLE:
	if (state_changed) {
		if (new_state == FRC_STATE_BYPASS) {
			if (devp->frc_sts.frame_cnt == 0) {
				schedule_work(&devp->frc_secure_work);
				devp->frc_sts.frame_cnt = 0;
				/*notify vpu bypass frc   default:0  bypass_frc:1 not_bypass_frc:2*/
				devp->need_bypass = 1;
				devp->frc_sts.frame_cnt++;
			} else {
				devp->need_bypass = 0;
				pr_frc(log, "stat_chg %s -> %s done\n",
					frc_state_ary[cur_state],
					frc_state_ary[new_state]);
				frc_state_change_finish(devp);
				off2on_cnt = 0;
			}
		} else if (new_state == FRC_STATE_ENABLE) {
			if (devp->frc_sts.frame_cnt == 0) {
				if (devp->clk_state != FRC_CLOCK_NOR &&
					devp->clk_state != FRC_CLOCK_XXX2NOR) {
					devp->clk_state = FRC_CLOCK_XXX2NOR;
					schedule_work(&devp->frc_clk_work);
				} else if (devp->clk_state == FRC_CLOCK_NOR &&
					devp->buf.cma_mem_alloced) {
					devp->frc_sts.frame_cnt++;
					devp->need_bypass = 2;
					frc_clr_badedit_effect_before_enable();
				}
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt == 1) {
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
				// frc_mm_secure_set(devp);
				schedule_work(&devp->frc_secure_work);
				get_vout_info(devp);
				frc_hw_initial(devp);
				//first : set bypass off
				read0x60 = READ_FRC_REG(FRC_REG_TOP_RESERVE0) & 0xFFFF;
				bypasscnt = (read0x60 & 0xfff0) >> 4;
				freezecnt = read0x60 & 0xf;
				//frc_get_film_base_vf(devp);
				if (pfw_data->frc_input_cfg)
					pfw_data->frc_input_cfg(devp->fw_data);
				//second: set frc enable on
				frc_memc_clr_vbuffer(devp, 1);
				frc_win_align_set(devp, 1);
				frc_win_size_align();
				set_frc_enable(ON);
				// devp->frc_fw_pause = 0;
				pr_frc(log, "frc_bypass_cnt:%d,freeze_cnt:%d\n",
						bypasscnt, freezecnt);
				devp->st_change = 1;
				devp->need_bypass = 0;
			} else if (devp->frc_sts.frame_cnt == 2) {
				if (devp->in_sts.t3x_proc_size_chg) {
					frc_input_init(devp, frc_top);
					tmp_frm_size = frc_top->hsize;
					tmp_frm_size |= (frc_top->vsize) << 16;
					FRC_RDMA_WR_REG_IN(FRC_FRAME_SIZE, tmp_frm_size);
				}
				devp->st_change = 3;
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt == 3) {
				pr_frc(2, "post 0x3f05 = %x\n", READ_FRC_REG(FRC_FRAME_SIZE));

				if (pfw_data->frc_input_cfg && devp->in_sts.t3x_proc_size_chg)
					pfw_data->frc_input_cfg(devp->fw_data);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt < bypasscnt) {
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
				if (devp->frc_sts.frame_cnt == bypasscnt && is_rdma_enable())
					t3x_verB_set_cfg(1, devp);
			} else if (devp->frc_sts.frame_cnt == bypasscnt) {
				if (!is_rdma_enable())
					t3x_verB_set_cfg(1, devp);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt == bypasscnt + 1) {
				forceidx = frc_frame_forcebuf_enable(1);
				frc_frame_forcebuf_count(forceidx);
				if (chg_flag == 0)
					frc_memc_120hz_patch_2(devp);
				pr_frc(log, "d-e_freeze idx:%d, frm:%d, chg:%d\n",
					forceidx, devp->frc_sts.frame_cnt, chg_flag);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt > bypasscnt + 1 &&
						devp->frc_sts.frame_cnt <
						freezecnt + bypasscnt + 1) {
				frc_frame_forcebuf_count(forceidx);
				if (devp->frc_sts.frame_cnt == chg_flag + bypasscnt + 1)
					frc_memc_120hz_patch_2(devp);
				frc_input_fid =
				READ_FRC_REG(FRC_REG_PAT_POINTER) >> 4 & 0xF;
				pr_frc(log, "d-e_freezing readidx:%d, frm:%d, chg:%d\n",
					frc_input_fid, devp->frc_sts.frame_cnt, chg_flag);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt ==
						freezecnt + bypasscnt + 1) {
				frc_frame_forcebuf_enable(0);
				frc_memc_120hz_patch_3(devp);
				frc_memc_clr_vbuffer(devp, 0);
				pr_frc(log, "d-e_freezed to open, frm:%d\n",
					devp->frc_sts.frame_cnt);
				// frc_state_change_finish(devp);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (frc_check_film_mode(devp) != 0 || off2on_cnt > 29) {
				pr_frc(log, "d-e_stat_chg %s -> %s[%d] done, used_frm:%d[%d]\n",
						frc_state_ary[cur_state],
						frc_state_ary[new_state],
						frc_check_film_mode(devp),
						off2on_cnt,
						devp->frc_sts.frame_cnt);
				frc_state_change_finish(devp);
				off2on_cnt = 0;
			} else if (devp->frc_sts.frame_cnt > bypasscnt + freezecnt + 1 &&
					 (frc_check_film_mode(devp) == 0)) {
				pr_frc(log, "d-e_detecting film[%d], frm: %d\n",
						frc_check_film_mode(devp),
						devp->frc_sts.frame_cnt);
				off2on_cnt++;
				devp->frc_sts.frame_cnt++;
			}
		} else {
			pr_frc(0, "err new state %d\n", new_state);
		}
	}
	break;
	case FRC_STATE_ENABLE:
	if (state_changed) {
		if (new_state == FRC_STATE_DISABLE) {
			if (devp->frc_sts.frame_cnt == 0) {
				schedule_work(&devp->frc_secure_work);
				frc_frame_forcebuf_enable(0);
				//devp->frc_fw_pause = 1;
				set_frc_enable(OFF);
				devp->st_change = 2;
				devp->frc_sts.frame_cnt++;
			} else {
				frc_clr_badedit_effect_before_enable();
				devp->frc_sts.frame_cnt = 0;
				devp->st_change = 0;
				pr_frc(log, "stat_chg %s -> %s done\n",
					frc_state_ary[cur_state],
					frc_state_ary[new_state]);
				frc_state_change_finish(devp);
			}
		} else if (new_state == FRC_STATE_BYPASS) {
			//first frame set enable off
			if (devp->frc_sts.frame_cnt == 0) {
				schedule_work(&devp->frc_secure_work);
				frc_frame_forcebuf_enable(0);
				//devp->frc_fw_pause = 1;
				set_frc_enable(OFF);
				devp->st_change = 2;
				devp->frc_sts.frame_cnt++;
			} else if (devp->frc_sts.frame_cnt == 1) {
				frc_clr_badedit_effect_before_enable();
				devp->st_change = 0;
				devp->need_bypass = 1;
				devp->frc_sts.frame_cnt++;
			} else if (devp->frc_sts.frame_cnt == 2) {
				//second frame set bypass on
				//set_frc_bypass(ON);
				devp->need_bypass = 0;
				devp->frc_sts.frame_cnt++;
			} else {
				devp->frc_sts.frame_cnt = 0;
				pr_frc(log, "stat_chg %s->%s done\n",
				       frc_state_ary[cur_state],
				       frc_state_ary[new_state]);
				frc_state_change_finish(devp);
			}
		} else {
			pr_frc(0, "err new state %d\n", new_state);
		}
	}
	break;
	case FRC_STATE_BYPASS:
	if (state_changed) {
		if (new_state == FRC_STATE_DISABLE) {
			if (devp->frc_sts.frame_cnt == 0) {
				if (devp->clk_state != FRC_CLOCK_NOR &&
					devp->clk_state != FRC_CLOCK_XXX2NOR)  {
					devp->clk_state = FRC_CLOCK_XXX2NOR;
					schedule_work(&devp->frc_clk_work);
				} else if (devp->clk_state == FRC_CLOCK_NOR) {
					devp->frc_sts.frame_cnt++;
				}
			} else {
				set_frc_bypass(OFF);
				//devp->frc_fw_pause = 1;
				set_frc_enable(OFF);
				devp->frc_sts.frame_cnt = 0;
				pr_frc(log, "stat_chg %s -> %s done\n",
				frc_state_ary[cur_state],
				frc_state_ary[new_state]);
				frc_state_change_finish(devp);
			}
		} else if (new_state == FRC_STATE_ENABLE) {
			if (devp->frc_sts.frame_cnt == 0) {
				if (devp->clk_state != FRC_CLOCK_NOR &&
					devp->clk_state != FRC_CLOCK_XXX2NOR)  {
					devp->clk_state = FRC_CLOCK_XXX2NOR;
					schedule_work(&devp->frc_clk_work);
				} else if (devp->clk_state == FRC_CLOCK_NOR &&
					devp->buf.cma_mem_alloced) {
					//first frame set bypass off
					devp->need_bypass = 2;
					devp->frc_sts.frame_cnt++;
					frc_clr_badedit_effect_before_enable();
				}
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt == 1) {
				schedule_work(&devp->frc_secure_work);
				frc_memc_clr_vbuffer(devp, 1);
				get_vout_info(devp);
				frc_hw_initial(devp);
				//second frame set enable on
				read0x60 = READ_FRC_REG(FRC_REG_TOP_RESERVE0) & 0xFFFF;
				bypasscnt = (read0x60 & 0xfff0) >> 4;
				freezecnt = read0x60 & 0x0f;
				//frc_get_film_base_vf(devp);
				if (pfw_data->frc_input_cfg)
					pfw_data->frc_input_cfg(devp->fw_data);
				//devp->frc_fw_pause = 0;
				frc_win_align_set(devp, 1);
				frc_win_size_align();
				set_frc_enable(ON);
				devp->st_change = 1;
				devp->need_bypass = 0;
				pr_frc(log, "frc_bypass_cnt:%d,freeze_cnt:%d",
						bypasscnt, freezecnt);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt == 2) {
				if (devp->in_sts.t3x_proc_size_chg) {
					frc_input_init(devp, frc_top);
					tmp_frm_size = frc_top->hsize;
					tmp_frm_size |= (frc_top->vsize) << 16;
					FRC_RDMA_WR_REG_IN(FRC_FRAME_SIZE, tmp_frm_size);
				}
				devp->st_change = 3;
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt == 3) {
				if (pfw_data->frc_input_cfg && devp->in_sts.t3x_proc_size_chg)
					pfw_data->frc_input_cfg(devp->fw_data);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt < bypasscnt + 1) {
				pr_frc(log, "b-e_bypassing frm:%d\n",
					devp->frc_sts.frame_cnt);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
				if (devp->frc_sts.frame_cnt > bypasscnt && is_rdma_enable())
					t3x_verB_set_cfg(1, devp);
			} else if (devp->frc_sts.frame_cnt == bypasscnt + 1) {
				if (!is_rdma_enable())
					t3x_verB_set_cfg(1, devp);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt == bypasscnt + 2) {
				// frc_memc_120hz_patch_3(devp);
				forceidx = frc_frame_forcebuf_enable(1);
				frc_frame_forcebuf_count(forceidx);
				if (chg_flag == 0)
					frc_memc_120hz_patch_2(devp);
				pr_frc(log, "b-e_freeze start, rd_idx:%d, frm:%d, chg:%d\n",
					forceidx, devp->frc_sts.frame_cnt, chg_flag);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt > bypasscnt + 2 &&
					devp->frc_sts.frame_cnt <
					 bypasscnt + freezecnt + 2) {
				frc_frame_forcebuf_count(forceidx);
				if (devp->frc_sts.frame_cnt == chg_flag + bypasscnt + 2)
					frc_memc_120hz_patch_2(devp);
				frc_input_fid =
				READ_FRC_REG(FRC_REG_PAT_POINTER) >> 4 & 0xF;
				pr_frc(log, "b-e_freezing readidx %d, frm: %d, chg:%d\n",
					frc_input_fid, devp->frc_sts.frame_cnt, chg_flag);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (devp->frc_sts.frame_cnt ==
					bypasscnt + freezecnt + 2) {
				frc_frame_forcebuf_enable(0);
				frc_memc_120hz_patch_3(devp);
				frc_memc_clr_vbuffer(devp, 0);
				pr_frc(log, "b-e_freezed to open, frm:%d\n",
					devp->frc_sts.frame_cnt);
				// frc_state_change_finish(devp);
				devp->frc_sts.frame_cnt++;
				off2on_cnt++;
			} else if (frc_check_film_mode(devp) != 0 || devp->frc_sts.frame_cnt > 29) {
				pr_frc(log, "b-e_stat_chg %s -> %s[%d] done, used frm:%d[%d]\n",
						frc_state_ary[cur_state],
						frc_state_ary[new_state],
						frc_check_film_mode(devp),
						off2on_cnt,
						devp->frc_sts.frame_cnt);
				frc_state_change_finish(devp);
				off2on_cnt = 0;
			} else if (devp->frc_sts.frame_cnt > bypasscnt + freezecnt + 2 &&
					  (frc_check_film_mode(devp) == 0)) {
				pr_frc(log, "b-e_detecting film[%d], frm: %d\n",
						frc_check_film_mode(devp),
						devp->frc_sts.frame_cnt);
				off2on_cnt++;
				devp->frc_sts.frame_cnt++;
			}
		} else {
			pr_frc(0, "err new state %d\n", new_state);
		}
	}
	break;

	default:
		pr_frc(0, "err state %d\n", cur_state);
		break;
	}
}

int frc_memc_set_level(u8 level)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *pfw_data;

	if (!devp || !devp->probe_ok || !devp->fw_data)
		return 0;
	pfw_data = (struct frc_fw_data_s *)devp->fw_data;
	pr_frc(1, "set_memc_level:%d\n", level);
	if (level != pfw_data->frc_top_type.frc_memc_level) {
		pfw_data->frc_top_type.frc_memc_level = level;
		if (pfw_data->frc_memc_level)
			pfw_data->frc_memc_level(pfw_data);
	}
	return 1;
}

int frc_fpp_memc_set_level(u8 level, u8 num)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *pfw_data;
	int flag = 0;

	if (!devp || !devp->probe_ok || !devp->fw_data)
		return 0;
	pr_frc(1, "fpp_set_memc_level:%d[%d]\n", level, num);
	pfw_data = (struct frc_fw_data_s *)devp->fw_data;
	if (level != pfw_data->frc_top_type.frc_memc_level) {
		pfw_data->frc_top_type.frc_memc_level = level;
		flag = 1;
	}
	if (num != pfw_data->frc_top_type.frc_memc_level_1) {
		pfw_data->frc_top_type.frc_memc_level_1 = num;
		flag = 1;
	}
	if (pfw_data->frc_memc_level && flag)
		pfw_data->frc_memc_level(pfw_data);
	return 1;
}

int frc_lge_memc_set_level(struct v4l2_ext_memc_motion_comp_info comp_info)
{
	u8 memc_type;
	u32 judder_level = 0;
	unsigned char temp_level[1];
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *pfw_data;

	if (!devp || !devp->probe_ok || !devp->fw_data)
		return 0;
	pfw_data = (struct frc_fw_data_s *)devp->fw_data;
	memc_type = (u8)comp_info.memc_type;

	pr_frc(1, "comp_info.memc_type:%d, judder_level:%d, blur_level:%d\n",
		(u8)comp_info.memc_type, comp_info.judder_level, comp_info.blur_level);

	if (memc_type == V4L2_EXT_MEMC_OFF) {
		pfw_data->frc_top_type.frc_memc_level = 0;  // off
	} else if (memc_type == V4L2_EXT_MEMC_NATURAL) {
		pfw_data->frc_top_type.frc_memc_level = 6;  // low level
	} else if (memc_type == V4L2_EXT_MEMC_SMOOTH) {
		pfw_data->frc_top_type.frc_memc_level = 10;  // mid level
	} else if (memc_type == V4L2_EXT_MEMC_CINEMA_CLEAR) {
		pfw_data->frc_top_type.frc_memc_level = 3;  // mid level
	} else if (memc_type == V4L2_EXT_MEMC_TYPE_USER) {
		temp_level[0] = comp_info.judder_level;
		if (kstrtoint(temp_level, 10, &judder_level) == 0) {
			// char type
			if (judder_level <= 10)
				pfw_data->frc_top_type.frc_memc_level = judder_level;
			else
				pr_frc(0, "ioc memc-level char-value is unsupported\n");
		} else {
			// int type
			judder_level = comp_info.judder_level;
			if (judder_level <= 10)
				pfw_data->frc_top_type.frc_memc_level = judder_level;
			else
				pr_frc(0, "ioc memc-level int-value is unsupported\n");
		}
	} else if (memc_type == V4L2_EXT_MEMC_TYPE_55_PULLDOWN) {
		pr_frc(1, "24p film mode\n");
		// need to discuss
	}

	pr_frc(1, "ioc lge_set_memc_level:%d\n",
		pfw_data->frc_top_type.frc_memc_level);
	if (pfw_data->frc_memc_level)
		pfw_data->frc_memc_level(pfw_data);

	return 0;
}

void frc_lge_memc_get_level(struct v4l2_ext_memc_motion_comp_info *comp_info)
{
	enum v4l2_ext_memc_type memc_type;
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *pfw_data;

	pfw_data = (struct frc_fw_data_s *)devp->fw_data;

	if (pfw_data->frc_top_type.frc_memc_level == 0) {
		memc_type = (enum v4l2_ext_memc_type)V4L2_EXT_MEMC_OFF;
	} else if (pfw_data->frc_top_type.frc_memc_level == 3) {
		memc_type = (enum v4l2_ext_memc_type)V4L2_EXT_MEMC_CINEMA_CLEAR;
	} else if (pfw_data->frc_top_type.frc_memc_level == 6) {
		memc_type = (enum v4l2_ext_memc_type)V4L2_EXT_MEMC_NATURAL;
	} else if (pfw_data->frc_top_type.frc_memc_level == 10) {
		memc_type = (enum v4l2_ext_memc_type)V4L2_EXT_MEMC_SMOOTH;
	} else {
		//user setting
		memc_type = (enum v4l2_ext_memc_type)V4L2_EXT_MEMC_TYPE_USER;
	}
	comp_info->memc_type = memc_type;
	comp_info->judder_level = '1';
	comp_info->blur_level = '1';
}

void frc_lge_memc_init(void)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *pfw_data;

	if (!devp || !devp->probe_ok || !devp->fw_data)
		return;
	pfw_data = (struct frc_fw_data_s *)devp->fw_data;

	// reserve frc buf init

	//frc status and fw_pause ready
	// devp->frc_fw_pause = true;
	devp->frc_sts.auto_ctrl = true;
	frc_change_to_state(FRC_STATE_ENABLE);
	// devp->frc_sts.re_config = true;
	pr_frc(0, "frc lge memc init done\n");
}

int frc_memc_set_deblur(u8 level)
{
	struct frc_dev_s *devp = get_frc_devp();
	// struct frc_fw_alg_ctrl_s *pfrc_fw_alg_ctrl;
	struct frc_fw_data_s *pfw_data;

	if (!devp || !devp->probe_ok || !devp->fw_data)
		return 0;
	pfw_data = (struct frc_fw_data_s *)devp->fw_data;
	pr_frc(1, "set_deblur_level:%d\n", level);
	if (level != pfw_data->frc_top_type.frc_deblur_level) {
		pfw_data->frc_top_type.frc_deblur_level = level;
		if (pfw_data->frc_memc_level)
			pfw_data->frc_memc_level(pfw_data);
	}
	return 1;
}

int frc_memc_set_demo(u8 setdemo)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *pfw_data;
	u32 tmpstart = 0, tmpend = 0;

	if (!devp)
		return 0;
	if (!devp->probe_ok)
		return 0;
	if (!devp->fw_data)
		return 0;
	pfw_data = (struct frc_fw_data_s *)devp->fw_data;
	pr_frc(1, "set_demo_mode:%d\n", setdemo);
	pr_frc(1, "in_hsize:%4d\n",  pfw_data->frc_top_type.hsize);
	pr_frc(1, "in_vsize:%4d\n",  pfw_data->frc_top_type.vsize);
	pr_frc(1, "out_hsize:%4d\n",  pfw_data->frc_top_type.out_hsize);
	pr_frc(1, "out_vsize:%4d\n",  pfw_data->frc_top_type.out_vsize);
	if (setdemo == 0) {
		WRITE_FRC_BITS(FRC_MC_DEMO_WINDOW, 0, 3, 1);
		WRITE_FRC_BITS(FRC_REG_MC_DEBUG1, 0, 17, 1);
	} else if (setdemo < 3) {
		tmpstart = pfw_data->frc_top_type.hsize / 2 << 16;
		pr_frc(1, "demo_win_start:%4d\n", tmpstart);
		WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW1_XYXY_ST, tmpstart);
		tmpend = ((pfw_data->frc_top_type.hsize - 1) << 16) +
				(pfw_data->frc_top_type.vsize - 1);
		pr_frc(1, "demo_win_end:%4d\n", tmpend);
		WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW1_XYXY_ED, tmpend);
		WRITE_FRC_BITS(FRC_REG_MC_DEBUG1, (setdemo - 1), 17, 1);
		WRITE_FRC_BITS(FRC_MC_DEMO_WINDOW, 1, 3, 1);
	} else if (setdemo < 5) {
		WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW1_XYXY_ST, 0);
		tmpend = ((pfw_data->frc_top_type.hsize / 2 - 1) << 16) +
				(pfw_data->frc_top_type.vsize - 1);
		pr_frc(1, "demo_win_end:%4d\n", tmpend);
		WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW1_XYXY_ED, tmpend);
		WRITE_FRC_BITS(FRC_REG_MC_DEBUG1, (setdemo - 3), 17, 1);
		WRITE_FRC_BITS(FRC_MC_DEMO_WINDOW, 1, 3, 1);
	}
	return 1;
}

int frc_init_out_line(void)
{
	u32 vfb = 0;
	enum chip_id chip;

	chip = get_chip_type();
	if (chip == ID_T3X)
		vfb = (vpu_reg_read(ENCL_VIDEO_VAVON_BLINE_T3X) >> 16) & 0xffff;
	else
		vfb = vpu_reg_read(ENCL_VIDEO_VAVON_BLINE);

	if (vfb > 0 && vfb < 120) {   // need more check 500 is correct or not
		vfb = (vfb / 4) * 3;  // 3/4 point of front vblank, default
	} else {
		vfb = 51;
		PR_ERR("%s read back vfb:%d\n", __func__, vfb);
	}

	return vfb;
}

void frc_vpp_vs_ir_chk_film(struct frc_dev_s *frc_devp)
{
	if (!frc_devp->probe_ok || !frc_devp->power_on_flag)
		return;
	if (frc_devp->ud_dbg.res0_dbg_en == 1) {
		if (!frc_devp->frc_fw_pause)
			frc_devp->frc_fw_pause = 1;
		pr_frc(6, "vscnt=%7d, glb=%10d, cntglb=%7d\n",
			frc_devp->frc_sts.vs_cnt,
			READ_FRC_REG(FRC_FD_DIF_GL),
			READ_FRC_REG(FRC_FD_DIF_COUNT_GL));
	} // restore prev setting ,need first set fw_pause=0 by
	// echo frc_pause 0 > /sys/class/frc/debug
}

int frc_tell_alg_vendor(u8 vendor_info)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_alg_ctrl_s *pfrc_fw_alg_ctrl;
	struct frc_fw_data_s *pfw_data;

	if (!devp)
		return 0;
	if (!devp->probe_ok)
		return 0;
	if (!devp->fw_data)
		return 0;
	pfw_data = (struct frc_fw_data_s *)devp->fw_data;
	pfrc_fw_alg_ctrl = (struct frc_fw_alg_ctrl_s *)&pfw_data->frc_fw_alg_ctrl;
	pr_frc(1, "tell_alg_vendor:0x%x\n", vendor_info);
	if (pfrc_fw_alg_ctrl->frc_algctrl_u8vendor != vendor_info)
		pfrc_fw_alg_ctrl->frc_algctrl_u8vendor = vendor_info;
	// if (pfw_data->frc_fw_ctrl_if)  // change to output isr
	//	pfw_data->frc_fw_ctrl_if(pfw_data);
	return 1;
}

int frc_set_memc_fallback(u8 fbvale)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_alg_ctrl_s *pfrc_fw_alg_ctrl;
	struct frc_fw_data_s *pfw_data;

	if (!devp)
		return 0;
	if (!devp->probe_ok)
		return 0;
	if (!devp->fw_data)
		return 0;
	pfw_data = (struct frc_fw_data_s *)devp->fw_data;
	pfrc_fw_alg_ctrl = (struct frc_fw_alg_ctrl_s *)&pfw_data->frc_fw_alg_ctrl;
	pr_frc(1, "set mc fallback:0x%x\n", fbvale);
	pfrc_fw_alg_ctrl->frc_algctrl_u8mcfb = (fbvale > 20) ? 20 : fbvale;
	if (pfw_data->frc_fw_ctrl_if)
		pfw_data->frc_fw_ctrl_if(pfw_data);
	return 1;
}

int frc_set_film_support(u32 filmcnt)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_alg_ctrl_s *pfrc_fw_alg_ctrl;
	struct frc_fw_data_s *pfw_data;

	if (!devp)
		return 0;
	if (!devp->probe_ok)
		return 0;
	if (!devp->fw_data)
		return 0;
	pfw_data = (struct frc_fw_data_s *)devp->fw_data;
	pfrc_fw_alg_ctrl = (struct frc_fw_alg_ctrl_s *)&pfw_data->frc_fw_alg_ctrl;
	pr_frc(1, "set support film:0x%x\n", filmcnt);
	pfrc_fw_alg_ctrl->frc_algctrl_u32film = filmcnt;
	if (pfw_data->frc_fw_ctrl_if)
		pfw_data->frc_fw_ctrl_if(pfw_data);
	return 1;
}

static int notify_frc_signal_to_amvideo(int *char_flash_check)
{
	static int pre_char_flash_check;

//#ifdef CONFIG_AMLOGIC_MEDIA_VIDEO
	if (pre_char_flash_check != *char_flash_check) {
		pr_frc(2, "char_flash_check = %d\n", *char_flash_check);
		pre_char_flash_check = *char_flash_check;
		amvideo_notifier_call_chain
			(AMVIDEO_UPDATE_FRC_CHAR_FLASH,
			(void *)char_flash_check);
	}
//#endif
	return 0;
}

// fix frc char flashing
void frc_char_flash_check(void)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *pfw_data;
	static u16 match_count;
	int char_flash_check;
	int temp, temp2;

	if (!devp || !devp->probe_ok || !devp->fw_data)
		return;
	if (devp->in_sts.high_freq_en) {
		if (devp->in_sts.high_freq_flash) {
			char_flash_check = 0;
			devp->in_sts.high_freq_flash = char_flash_check;
			notify_frc_signal_to_amvideo(&char_flash_check);
		}
		return;
	}
	pfw_data = (struct frc_fw_data_s *)devp->fw_data;
	/*4K input/output && memc on && glb_motion && hist*/
	if (pfw_data->frc_top_type.hsize == 3840 &&
		pfw_data->frc_top_type.vsize == 2160 &&
		devp->frc_sts.state == 1) {
		temp = READ_FRC_REG(FRC_BBD_RO_MAX1_HIST_CNT);
		temp2 = READ_FRC_REG(FRC_BBD_RO_MAX2_HIST_CNT);
		temp = temp + temp2;
		if ((temp > MIN_HIST1 && temp < MAX_HIST1) ||
			(temp > MIN_HIST2 && temp < MAX_HIST2)) {
			match_count++;
			if (match_count > LIMIT * 2)
				match_count = LIMIT * 2;
		} else {
			match_count--;
			if (match_count < 1)
				match_count = 1;
		}

		if (match_count > LIMIT)
			char_flash_check = 1;
		else
			char_flash_check = 0;
		devp->in_sts.high_freq_flash = char_flash_check;
		notify_frc_signal_to_amvideo(&char_flash_check);
	}
}

/*
 * input vf devp
 * output status(is tvin? source changed? fps)
 */
void frc_chk_vd_sts_chg(struct frc_dev_s *devp, struct vframe_s *vf)
{
	static u8 frc_is_tvin_s, frc_source_chg_s;
	struct frc_fw_data_s *pfw_data;
	bool vlock_locked;

	pfw_data = (struct frc_fw_data_s *)devp->fw_data;

	if (!vf)
		return;
	if (vf->source_type == VFRAME_SOURCE_TYPE_HDMI ||
		vf->source_type == VFRAME_SOURCE_TYPE_CVBS ||
		vf->source_type == VFRAME_SOURCE_TYPE_TUNER)
		devp->in_sts.frc_is_tvin = true;
	else
		devp->in_sts.frc_is_tvin = false;

	if (frc_is_tvin_s != devp->in_sts.frc_is_tvin) {
		frc_is_tvin_s = devp->in_sts.frc_is_tvin;
		if (devp->in_sts.frc_is_tvin) {
			WRITE_FRC_BITS(FRC_TOP_MISC_CTRL, 1, 0, 1);
			WRITE_FRC_BITS(FRC_REG_TOP_CTRL25, 0, 31, 1);
			WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL27, 0);
		}
		pr_frc(1, "input change %d. (1:tvin)\n", frc_is_tvin_s);
	}

	if (vf->vc_private) {
		if (vf->vc_private->flag & VC_FLAG_FIRST_FRAME)
			devp->in_sts.frc_source_chg = true;
		else
			devp->in_sts.frc_source_chg = false;

		if (frc_source_chg_s != devp->in_sts.frc_source_chg) {
			pr_frc(1, "input source change [%d->%d]. (1:changed)\n",
				frc_source_chg_s, devp->in_sts.frc_source_chg);
			frc_source_chg_s = devp->in_sts.frc_source_chg;
		}
	}

	if (frc_source_chg_s != devp->in_sts.frc_source_chg) {
		pr_frc(1, "input source change [%d->%d]. (1:changed)\n",
			frc_source_chg_s, devp->in_sts.frc_source_chg);
		frc_source_chg_s = devp->in_sts.frc_source_chg;
	}

	if (vf->vc_private) {
		pr_frc(6, "last vf disp vsync count =%d\n",
			vf->vc_private->last_disp_count);
		devp->in_sts.frc_last_disp_count =
			vf->vc_private->last_disp_count;
	}
	// every vframe detect frame rate
	frc_check_vf_rate(vf->duration, devp);

	if (devp->in_sts.frc_is_tvin) {
		if (vf->duration == 4000 || vf->duration == 4004)
			vlock_locked = vlock_get_vlock_flag();
		else
			vlock_locked = vlock_get_phlock_flag() &&
					vlock_get_vlock_flag();
		if (vlock_locked) {
			devp->in_sts.st_flag |= FRC_FLAG_VLOCK_ST;
			pfw_data->frc_top_type.vfp |= BIT_8;
		} else {
			devp->in_sts.st_flag &= (~FRC_FLAG_VLOCK_ST);
			pfw_data->frc_top_type.vfp &= (~BIT_8);
		}
	}
}

u16 frc_check_film_mode(struct frc_dev_s *frc_devp)
{
	struct frc_fw_data_s *fw_data;
	struct frc_top_type_s *frc_top;

	fw_data = (struct frc_fw_data_s *)frc_devp->fw_data;
	frc_top = &fw_data->frc_top_type;

	//if (frc_devp->frc_sts.state == FRC_STATE_ENABLE)
		frc_top->film_mode  = READ_FRC_REG(FRC_REG_PHS_TABLE) >> 8 & 0xFF;
	//else
	//	frc_top->film_mode  = EN_DRV_VIDEO;
	return (u16)(frc_top->film_mode);
}

void frc_check_secure_mode(struct vframe_s *vf, struct frc_dev_s *devp)
{
	u32 temp;
	enum chip_id chip;
	static int secure_mode;

	chip = get_chip_type();

	if (chip == ID_T3) {
		if ((vf->flag & VFRAME_FLAG_VIDEO_SECURE) ==
			VFRAME_FLAG_VIDEO_SECURE)
			devp->in_sts.secure_mode = true;
		else
			devp->in_sts.secure_mode = false;
	} else if (chip == ID_T5M || chip == ID_T3X) {
		if (!sec_flag) {
			temp = READ_FRC_REG(FRC_RO_FRM_SEC_STAT);
			temp = (temp >> 16) & 0xf; // 1: input frame is security
			if (temp)
				devp->in_sts.secure_mode = true;
			else
				devp->in_sts.secure_mode = false;
		} else {
			if ((vf->flag & VFRAME_FLAG_VIDEO_SECURE) ==
				VFRAME_FLAG_VIDEO_SECURE)
				devp->in_sts.secure_mode = true;
			else
				devp->in_sts.secure_mode = false;
		}
	}

	if (secure_mode != devp->in_sts.secure_mode) {
		pr_frc(0, "frc secure sts:%d, sec_flag:%d, chip:%d\n",
			devp->in_sts.secure_mode, sec_flag, chip);
		secure_mode = devp->in_sts.secure_mode;
	}
}

void frc_win_align_set(struct frc_dev_s *devp, u8 align_set)
{
	if (!devp)
		return;
	if (align_set == devp->ud_dbg.align_dbg_en)
		return;

	devp->ud_dbg.align_dbg_en = align_set;

	if (devp->ud_dbg.align_dbg_en == FRC_WIN_ALIGN_AUTO) { //auto
		regdata_inpholdctl_0002 = READ_FRC_REG(FRC_INP_HOLD_CTRL);
		frc_config_reg_value(BIT_20, BIT_20, &regdata_inpholdctl_0002);
		WRITE_FRC_REG_BY_CPU(FRC_INP_HOLD_CTRL, regdata_inpholdctl_0002);
		WRITE_FRC_BITS(FRC_TOP_MISC_CTRL, 1, 0, 1);
		WRITE_FRC_BITS(FRC_REG_TOP_CTRL25, 0, 31, 1);
	} else if (devp->ud_dbg.align_dbg_en == FRC_WIN_ALIGN_MANU) { // manual
		regdata_inpholdctl_0002 = READ_FRC_REG(FRC_INP_HOLD_CTRL);
		frc_config_reg_value(BIT_20, BIT_20, &regdata_inpholdctl_0002);
		WRITE_FRC_REG_BY_CPU(FRC_INP_HOLD_CTRL, regdata_inpholdctl_0002);
		WRITE_FRC_BITS(FRC_TOP_MISC_CTRL, 0, 0, 1);
		WRITE_FRC_BITS(FRC_REG_TOP_CTRL25, 1, 31, 1);
	} else if (devp->ud_dbg.align_dbg_en == FRC_WIN_ALIGN_DGB1) { // debug
		regdata_inpholdctl_0002 = READ_FRC_REG(FRC_INP_HOLD_CTRL);
		frc_config_reg_value(BIT_20, BIT_20, &regdata_inpholdctl_0002);
		WRITE_FRC_REG_BY_CPU(FRC_INP_HOLD_CTRL, regdata_inpholdctl_0002);
	}
	pr_frc(2, "0x2 = %x  0x50 = %x  0x26 = %x\n", READ_FRC_REG(FRC_INP_HOLD_CTRL),
			READ_FRC_REG(FRC_TOP_MISC_CTRL), READ_FRC_REG(FRC_REG_TOP_CTRL25));
}

void frc_win_size_align(void)
{
	u32 tmp_frm_size;
	u32 tmp_proc_size;
	u32 shift_bits = 0x1f;
	u32 mask_bits = ~shift_bits;
	u32 tmp_ofst_value;
	enum chip_id chip;
	struct frc_dev_s *devp;
	struct frc_top_type_s *frc_top;
	struct frc_fw_data_s *pfw_data;

	chip = get_chip_type();
	devp = get_frc_devp();
	pfw_data = (struct frc_fw_data_s *)devp->fw_data;
	frc_top = &pfw_data->frc_top_type;

	if (devp->ud_dbg.align_dbg_en == FRC_WIN_ALIGN_AUTO) {
		if (chip == ID_T3X && devp->in_out_ratio == FRC_RATIO_1_2) {
			tmp_frm_size = (frc_top->hsize + shift_bits) & mask_bits;
			tmp_frm_size |= (frc_top->vsize) << 16;
		} else {
			tmp_frm_size = frc_top->hsize;
			tmp_frm_size |= (frc_top->vsize) << 16;
		}
		tmp_proc_size = devp->out_sts.vout_width;
		tmp_proc_size |= (devp->out_sts.vout_height) << 16;

		tmp_ofst_value = 0;

		WRITE_FRC_REG_BY_CPU(FRC_FRAME_SIZE, tmp_frm_size);
		WRITE_FRC_REG_BY_CPU(FRC_PROC_SIZE, tmp_proc_size);
		if (chip != ID_T3)
			WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL27, tmp_ofst_value);
	} else if (devp->ud_dbg.align_dbg_en == FRC_WIN_ALIGN_MANU) {
		tmp_proc_size = (devp->in_sts.in_hsize + shift_bits) & mask_bits;
		tmp_proc_size |= ((devp->in_sts.in_vsize + 0xF) & 0xFFFFFFF0) << 16;
//		tmp_proc_size |= devp->in_sts.in_vsize << 16;

		tmp_ofst_value = 0;

		if (chip == ID_T3X)
			tmp_ofst_value |= 0 << 13;
		else if (devp->in_sts.in_hsize & shift_bits)
			tmp_ofst_value |= (((frc_top->hsize & shift_bits) ^ shift_bits) + 1) << 13;
		else
			tmp_ofst_value |= 0 << 13;
		WRITE_FRC_REG_BY_CPU(FRC_PROC_SIZE, tmp_proc_size);
		if (chip != ID_T3)
			WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL27, tmp_ofst_value);
		pr_frc(2, "0x51 = %x  0x28 = %x\n", READ_FRC_REG(FRC_PROC_SIZE),
				READ_FRC_REG(FRC_REG_TOP_CTRL27));
	}
}

void frc_input_size_align_check(struct frc_dev_s *devp)
{
	u8 reg_win_en;
	u8 reg_auto_align_en;
	u8 reg_inp_padding_en;
	u16 in_vsize = 0;
	u16 in_hsize = 0;

	/*only for T5M*/
	if (get_chip_type() != ID_T5M || devp->in_sts.frc_seamless_en)
		return;

	reg_win_en = (READ_FRC_REG(FRC_INP_HOLD_CTRL) >> 20) & 0x1; // bit:20
	reg_auto_align_en = READ_FRC_REG(FRC_TOP_MISC_CTRL) & 0x1; // bit:0
	reg_inp_padding_en = (READ_FRC_REG(FRC_REG_TOP_CTRL25) >> 31) & 0x1; //bit:31

	pr_frc(2, "reg_win_en:%d,reg_auto_align_en:%d,reg_inp_padding_en:%d\n",
		reg_win_en, reg_auto_align_en, reg_inp_padding_en);
	pr_frc(2, "in_sts.in_hsize:%d devp->in_sts.in_vsize:%d\n",
		devp->in_sts.in_hsize, devp->in_sts.in_vsize);
	pr_frc(2, "devp->out_sts.vout_width:%d devp->out_sts.vout_height:%d\n",
		devp->out_sts.vout_width, devp->out_sts.vout_height);

	WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL27, 0x0); // clear align mothod
	WRITE_FRC_REG_BY_CPU(FRC_PROC_SIZE,	       // restore default fhd value
			(devp->out_sts.vout_height & 0x3fff) << 16 |
			(devp->out_sts.vout_width & 0x3fff));

	if (reg_win_en && reg_auto_align_en && !reg_inp_padding_en) {
		if (devp->out_sts.vout_width == 1920 &&
			devp->out_sts.vout_height == 1080) {
			if (devp->in_sts.in_hsize > IN_W_SIZE_FHD_90 &&
				devp->in_sts.in_hsize < devp->out_sts.vout_width &&
				devp->in_sts.in_hsize % 8)
				in_hsize = 8 - (devp->in_sts.in_hsize % 8);
			if (devp->in_sts.in_vsize > IN_H_SIZE_FHD_90 &&
				devp->in_sts.in_vsize < devp->out_sts.vout_height &&
				devp->in_sts.in_vsize % 8)
				in_vsize = 8 - (devp->in_sts.in_vsize % 8);
		} else if (devp->out_sts.vout_width == 3840 &&
			devp->out_sts.vout_height == 2160) {
			if (devp->in_sts.in_hsize > IN_W_SIZE_UHD_90 &&
				devp->in_sts.in_hsize < devp->out_sts.vout_width &&
				devp->in_sts.in_hsize % 16)
				in_hsize = 16 - (devp->in_sts.in_hsize % 16);
			if (devp->in_sts.in_vsize > IN_H_SIZE_UHD_90 &&
				devp->in_sts.in_vsize < devp->out_sts.vout_height &&
				devp->in_sts.in_vsize % 16)
				in_vsize = 16 - (devp->in_sts.in_vsize % 16);
		}
		WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL27,
			(in_hsize & 0x1fff) << 13 | (in_vsize & 0x1ff));
	}
	pr_frc(2, "align_vsize:%d align_hsize:%d\n", in_vsize, in_hsize);
}

void frc_set_seamless_proc(u32 seamless)
{
	static u32 cur_status;
	struct frc_dev_s *devp = get_frc_devp();

	if (!devp || get_chip_type() == ID_T3 || cur_status == seamless)
		return;

	if (devp->frc_sts.state == FRC_STATE_ENABLE)
		devp->frc_sts.re_config = true;

	if (seamless) {
		WRITE_FRC_BITS(FRC_TOP_MISC_CTRL, 0, 0, 1); //auto_align_en
		WRITE_FRC_BITS(FRC_REG_TOP_CTRL25, 1, 31, 1); //inp_padding_en
	} else {
		WRITE_FRC_BITS(FRC_TOP_MISC_CTRL, 1, 0, 1);
		WRITE_FRC_BITS(FRC_REG_TOP_CTRL25, 0, 31, 1);
	}
	devp->in_sts.frc_seamless_en = (u8)seamless;
	cur_status = seamless;
	pr_frc(2, "seamless is %d.\n", seamless);
}

/* Test whether demo window works properly for t3x
 * input: dome window num (1/2/3/4)
 * output: number of demo window
 */
void set_frc_demo_window(u8 demo_num)
{
	u32 tmpstart, tmpend;
	static u8 demo_style;
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *pfw_data;

	if (!devp)
		return;
	if (!devp->probe_ok)
		return;
	if (!devp->fw_data)
		return;
	pfw_data = (struct frc_fw_data_s *)devp->fw_data;

	if (get_chip_type() == ID_T3X) {
		if (demo_num == 0) {
			WRITE_FRC_BITS(FRC_MC_DEMO_WINDOW, 0, 0, 5);
			WRITE_FRC_BITS(FRC_REG_MC_DEBUG1, 0, 17, 4);
			demo_style = 0;
		} else if (demo_num == 1) {
			if (!demo_style) {
				tmpstart = (pfw_data->frc_top_type.hsize / 2) << 16;
				tmpend = (pfw_data->frc_top_type.hsize) << 16 |
					pfw_data->frc_top_type.vsize;
			} else if (demo_style == 1) {
				tmpstart = 0;
				tmpend = (pfw_data->frc_top_type.hsize / 2) << 16 |
					pfw_data->frc_top_type.vsize;
			} else {
				tmpstart = (pfw_data->frc_top_type.hsize / 4) << 16 |
					pfw_data->frc_top_type.vsize / 4;
				tmpend = (pfw_data->frc_top_type.hsize / 4) * 3 << 16 |
					pfw_data->frc_top_type.vsize / 4 * 3;
			}
			demo_style++;
			if (demo_style > 2)
				demo_style = 0;
			// set demo window-1 position
			WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW1_XYXY_ST, tmpstart);
			WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW1_XYXY_ED, tmpend);
			// enable demo window1
			WRITE_FRC_BITS(FRC_REG_MC_DEBUG1, 0x1, 17, 4);
			WRITE_FRC_BITS(FRC_MC_DEMO_WINDOW, 0x8, 0, 5);
		} else if (demo_num == 2) {
			if (!demo_style) {
				tmpstart = 0;
				tmpend = (pfw_data->frc_top_type.hsize / 2) << 16 |
					pfw_data->frc_top_type.vsize / 2;
			} else {
				tmpstart = (pfw_data->frc_top_type.hsize / 2) << 16;
				tmpend = (pfw_data->frc_top_type.hsize) << 16 |
					pfw_data->frc_top_type.vsize / 2;
			}
			// set demo window position
			WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW1_XYXY_ST, tmpstart);
			WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW1_XYXY_ED, tmpend);
			if (!demo_style) {
				tmpstart = (pfw_data->frc_top_type.hsize / 2) << 16 |
					pfw_data->frc_top_type.vsize / 2;
				tmpend = pfw_data->frc_top_type.hsize << 16 |
					pfw_data->frc_top_type.vsize;
			} else {
				tmpstart = pfw_data->frc_top_type.vsize / 2;
				tmpend = (pfw_data->frc_top_type.hsize / 2) << 16 |
					pfw_data->frc_top_type.vsize;
			}
			WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW2_XYXY_ST, tmpstart);
			WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW2_XYXY_ED, tmpend);

			demo_style++;
			if (demo_style > 1)
				demo_style = 0;
			// enable demo window
			WRITE_FRC_BITS(FRC_REG_MC_DEBUG1, 0x3, 17, 4);
			WRITE_FRC_BITS(FRC_MC_DEMO_WINDOW, 0xc, 0, 5);
		} else if (demo_num == 3) {
			tmpstart = 0;
			tmpend = (pfw_data->frc_top_type.hsize / 10) * 3 << 16 |
				pfw_data->frc_top_type.vsize;
			// set demo window1 position
			WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW1_XYXY_ST, tmpstart);
			WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW1_XYXY_ED, tmpend);
			tmpstart = (pfw_data->frc_top_type.hsize / 10) * 3 << 16;
			tmpend = (pfw_data->frc_top_type.hsize / 10) * 7 << 16 |
				pfw_data->frc_top_type.vsize;
			// set demo window2 position
			WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW2_XYXY_ST, tmpstart);
			WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW2_XYXY_ED, tmpend);
			tmpstart = (pfw_data->frc_top_type.hsize / 10) * 7 << 16;
			tmpend = (pfw_data->frc_top_type.hsize) << 16 |
				pfw_data->frc_top_type.vsize;
			// set demo window3 position
			WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW3_XYXY_ST, tmpstart);
			WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW3_XYXY_ED, tmpend);

			// enable demo window
			WRITE_FRC_BITS(FRC_REG_MC_DEBUG1, 0x7, 17, 4);
			WRITE_FRC_BITS(FRC_MC_DEMO_WINDOW, 0xe, 0, 5);
		} else if (demo_num == 4) {
			tmpstart = (pfw_data->frc_top_type.hsize / 10) << 16 |
				pfw_data->frc_top_type.vsize / 10;
			tmpend = (pfw_data->frc_top_type.hsize / 10) * 4 << 16 |
				(pfw_data->frc_top_type.vsize / 10) * 4;
			// set demo window1 position
			WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW1_XYXY_ST, tmpstart);
			WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW1_XYXY_ED, tmpend);
			tmpstart = (pfw_data->frc_top_type.hsize / 10) * 6 << 16 |
				(pfw_data->frc_top_type.vsize / 10);
			tmpend = (pfw_data->frc_top_type.hsize / 10) * 9 << 16 |
				(pfw_data->frc_top_type.vsize / 10) * 4;
			// set demo window2 position
			WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW2_XYXY_ST, tmpstart);
			WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW2_XYXY_ED, tmpend);

			tmpstart = (pfw_data->frc_top_type.hsize / 10) << 16 |
				(pfw_data->frc_top_type.vsize / 10) * 6;
			tmpend = (pfw_data->frc_top_type.hsize / 10) * 4 << 16 |
				(pfw_data->frc_top_type.vsize / 10) * 9;
			// set demo window3 position
			WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW3_XYXY_ST, tmpstart);
			WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW3_XYXY_ED, tmpend);
			tmpstart = (pfw_data->frc_top_type.hsize / 10) * 6 << 16 |
				(pfw_data->frc_top_type.vsize / 10) * 6;
			tmpend = (pfw_data->frc_top_type.hsize / 10) * 9 << 16 |
				(pfw_data->frc_top_type.vsize / 10) * 9;
			// set demo window4 position
			WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW4_XYXY_ST, tmpstart);
			WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW4_XYXY_ED, tmpend);

			// enable demo window
			WRITE_FRC_BITS(FRC_REG_MC_DEBUG1, 0xf, 17, 4);
			WRITE_FRC_BITS(FRC_MC_DEMO_WINDOW, 0xf, 0, 5);
		}
		pr_frc(0, "FRC_REG_MC_DEBUG1 value = 0x%x\n", READ_FRC_REG(FRC_REG_MC_DEBUG1));
		pr_frc(0, "FRC_MC_DEMO_WINDOW value = 0x%x\n", READ_FRC_REG(FRC_MC_DEMO_WINDOW));
	}
}

/*bug: t3x 120Hz boot video flash*/
void frc_boot_timestamp_check(struct frc_dev_s *devp)
{
	u64 timestamp;

	if (get_chip_type() != ID_T3X ||
		devp->in_sts.boot_check_finished)
		return;

	timestamp = sched_clock();
	timestamp = div64_u64(timestamp, 1000000000); // sec
	if (timestamp > FRC_BOOT_TIMESTAMP &&
		devp->in_sts.boot_timestamp_en) {
		if (devp->in_sts.auto_ctrl_reserved) {
			devp->frc_sts.auto_ctrl = true;
			frc_change_to_state(FRC_STATE_ENABLE);
		}
		devp->in_sts.boot_check_finished = 1;
		pr_frc(0, "boot check finished, auto-ctrl:%d\n",
			devp->in_sts.auto_ctrl_reserved);
	}
}
