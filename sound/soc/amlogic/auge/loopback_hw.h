/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __AML_LOOPBACK_HW_H__
#define __AML_LOOPBACK_HW_H__

#include <linux/types.h>

struct data_cfg {
	/*
	 * 0: extend bits as "0"
	 * 1: extend bits as "msb"
	 */
	unsigned int ext_signed;
	/* channel number */
	unsigned int chnum;
	/* channel selected */
	unsigned int chmask;
	/* combined data */
	unsigned int type;
	/* the msb positioin in data */
	unsigned int m;
	/* the lsb position in data */
	unsigned int n;

	/* loopback datalb src */
	unsigned int src;

	unsigned int datalb_src;

	/* channel and mask in new ctrol register */
	bool ch_ctrl_switch;

	/* enable resample B for loopback*/
	unsigned int resample_enable;
};

void tdminlb_set_clk(int datatlb_src,
		     int sclk_div, int ratio, bool enable);

void tdminlb_set_format(int i2s_fmt);

void tdminlb_set_ctrl(int src);

void tdminlb_enable(int tdm_index, int in_enable);

void tdminlb_fifo_enable(int is_enable);

void tdminlb_set_format(int i2s_fmt);
void tdminlb_set_lanemask_and_chswap(int swap, int lane_mask);

void tdminlb_set_src(int src);
void lb_set_datain_src(int id, int src);

void lb_set_datain_cfg(int id, struct data_cfg *datain_cfg);
void lb_set_datalb_cfg(int id, struct data_cfg *datalb_cfg);

void lb_enable(int id, bool enable);
void lb_debug(int id, bool enable);

#endif
