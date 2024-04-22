/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include "sy602x_reg.h"

enum SY602X_DAP_RAM_ITEM {
	DAP_RAM_EQ_BIQUAD_0	= 0,
	DAP_RAM_EQ_BIQUAD_1	= 1,
	DAP_RAM_EQ_BIQUAD_2	= 2,
	DAP_RAM_EQ_BIQUAD_3	= 3,
	DAP_RAM_EQ_BIQUAD_4	= 4,
	DAP_RAM_EQ_BIQUAD_5	= 5,
	DAP_RAM_EQ_BIQUAD_6	= 6,
	DAP_RAM_EQ_BIQUAD_7	= 7,
	DAP_RAM_EQ_BIQUAD_8	= 8,
	DAP_RAM_EQ_BIQUAD_9	= 9,
	DAP_RAM_EQ_BIQUAD_10	= 10,
	DAP_RAM_EQ_BIQUAD_11	= 11,
	DAP_RAM_EQ_BIQUAD_12	= 12,
	DAP_RAM_EQ_BIQUAD_13	= 13,
	DAP_RAM_EQ_PRE_SCALER   = 14,
	DAP_RAM_SPEQ_COEF_0	= 15,
	DAP_RAM_SPEQ_COEF_1	= 16,
	DAP_RAM_SPEQ_COEF_2	= 17,
	DAP_RAM_SPEQ_COEF_3	= 18,
	DAP_RAM_DRC_BIQUAD_0	= 19,
	DAP_RAM_DRC_BIQUAD_1	= 20,
	DAP_RAM_DRC_BIQUAD_2	= 21,
	DAP_RAM_DRC_BIQUAD_3	= 22,
	DAP_RAM_DRC_BIQUAD_4	= 23,
	DAP_RAM_DRC_BIQUAD_5	= 24,
	DAP_RAM_LOUDNESS_GAIN  = 25,
	DAP_RAM_ITEM_NUM       = 26,
};

struct sy602x_dap_ram_command {
	const enum SY602X_DAP_RAM_ITEM item;
	const unsigned char *const data;
};

struct sy602x_reg {
	uint reg;
	uint val;
};

//Register Table of SY602X
static const u8 sy602x_reg_size[SY602X_MAX_REGISTER + 1] = {
//  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, // 0
	 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, // 1
	 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, // 2
	20, 20, 20, 20, 20, 20,  4, 20, 20, 20, 20, 20, 20, 20, 20, 12, // 3
	12, 12, 12, 12, 12, 12, 12, 20, 20, 20, 20, 20, 20, 12,  3,  3, // 4
	 3,  3,  3,  3,  4,  4,  4,  6,  3,  3,  4,  4,  1, 12, 12,  4, // 5
	 4,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  1, // 6
	 1,  4,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,  4,  4,  4,  1, // 7
	 3,  3,  4,  1,  1,  4,  4,  1,  1,  1,  1,  1,  1,  1,  1,  1 // 8
};

static const struct sy602x_dap_ram_command sy602x_dap_ram_init[] = {
	{.item = DAP_RAM_EQ_PRE_SCALER, .data = "\x36\x08\x00\x00\x00" },
	{.item = DAP_RAM_SPEQ_COEF_0, .data =
		"\x43\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
	{.item = DAP_RAM_SPEQ_COEF_1, .data =
		"\x44\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
	{.item = DAP_RAM_SPEQ_COEF_2, .data =
		"\x45\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
	{.item = DAP_RAM_SPEQ_COEF_3, .data =
		"\x46\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
	{.item = DAP_RAM_DRC_BIQUAD_0, .data =
	"\x47\x00\x00\x00\x00\x00\x7B\x11\xD3\x00\x00\x00\x00\x00\x02\x77\x17\x00\x02\x77\x17" },
	{.item = DAP_RAM_DRC_BIQUAD_1, .data =
	"\x48\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00" },
	{.item = DAP_RAM_DRC_BIQUAD_2, .data =
	"\x49\x00\x00\x00\x00\x00\x7B\x11\xD3\x00\x00\x00\x00\x1F\x82\x77\x17\x00\x7D\x88\xE9" },
	{.item = DAP_RAM_DRC_BIQUAD_3, .data =
	"\x4A\x00\x00\x00\x00\x00\x55\x86\xE1\x00\x00\x00\x00\x00\x15\x3C\x90\x00\x15\x3C\x90" },
	{.item = DAP_RAM_DRC_BIQUAD_4, .data =
	"\x4B\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00" },
	{.item = DAP_RAM_DRC_BIQUAD_5, .data =
	"\x4C\x00\x00\x00\x00\x00\x55\x86\xE1\x00\x00\x00\x00\x1F\x95\x3C\x90\x00\x6A\xC3\x70" },
	{.item = DAP_RAM_LOUDNESS_GAIN, .data =
		"\x4D\x08\x00\x00\x00\x08\x00\x00\x00\x08\x00\x00\x00" },
};

static const struct sy602x_reg sy602x_reg_init_mute[] = {
	{.reg = SY602X_CLK_CTL_REG,	.val = 0x1A },
	{.reg = SY602X_I2S_CONTROL,	.val = 0x10 },
	{.reg = SY602X_SOFT_MUTE,	.val = 0x08 },

};

static const struct sy602x_dap_ram_command sy602x_dap_ram_eq_1[] = {
	{.item = DAP_RAM_EQ_BIQUAD_0, .data =
	"\x30\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_1, .data =
	"\x31\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_2, .data =
	"\x32\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_3, .data =
	"\x33\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_4, .data =
	"\x34\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_5, .data =
	"\x35\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_6, .data =
	"\x37\x1F\x80\xDB\x7C\x00\xFE\xCB\x01\x00\x7F\x5B\x42\x1F\x01\x35\x00\x00\x7F\xC9\x42" },
	{.item = DAP_RAM_EQ_BIQUAD_7, .data =
	"\x38\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_8, .data =
	"\x39\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_9, .data =
	"\x3A\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_10, .data =
	"\x3B\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_11, .data =
	"\x3C\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_12, .data =
	"\x3D\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_13, .data =
	"\x3E\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00" },
};

static const struct sy602x_reg sy602x_reg_eq_1[] = {
	{ .reg = SY602X_CH1_EQ_CTRL1,		.val = 0x3F		}, //CH1_EQ_CTRL1
	{ .reg = SY602X_CH1_EQ_CTRL2,		.val = 0xFF		}, //CH1_EQ_CTRL2
	{ .reg = SY602X_CH2_EQ_CTRL1,		.val = 0x3F		}, //CH2_EQ_CTRL1
	{ .reg = SY602X_CH2_EQ_CTRL2,		.val = 0xFF		}, //CH2_EQ_CTRL2
};

static const struct sy602x_reg sy602x_reg_speq_ctrl[] = {
	{ .reg = SY602X_SPEQ_CTRL_2,		.val = 0x00		}, //SPEQ_CTRL_2
	{ .reg = SY602X_SPEQ_CTRL_3,		.val = 0x00		}, //SPEQ_CTRL_3
};

static const struct sy602x_reg sy602x_reg_init_seq_0[] = {
	{.reg = SY602X_OC_CTRL,	.val = 0x35 },
	{.reg = SY602X_DSP_CTL_REG2,	.val = 0x00 },
	{.reg = SY602X_FAULT_SELECT,	.val = 0x10 },
	{.reg = SY602X_SYS_CTL_REG3,	.val = 0x78 },
	{.reg = SY602X_PROT_SYS_CNTL,	.val = 0x1F },
	{.reg = SY602X_INTER_PRIVATE,	.val = 0x000000F0 },
	{.reg = SY602X_OCDET_WIND_WIDTH,	.val = 0x00000001 },
	{.reg = SY602X_FAULT_OC_TH,	.val = 0x00000001 },
	{.reg = SY602X_PWM_MUX_REG,	.val = 0x00000000 },
	{.reg = SY602X_PWM_OUTFLIP,	.val = 0x40003210 },
	{.reg = SY602X_PWM_CTL_REG,	.val = 0x0000002F },
	{.reg = SY602X_PWM_DC_THRESH,	.val = 0x05 },
	{.reg = SY602X_MOD_LMT_REG,	.val = 0x07 },
	{.reg = SY602X_PWM_DLA_CH1P,	.val = 0x00 },
	{.reg = SY602X_PWM_DLA_CH1N,	.val = 0x00 },
	{.reg = SY602X_PWM_DLA_CH2P,	.val = 0x00 },
	{.reg = SY602X_PWM_DLA_CH2N,	.val = 0x00 },
};

static const struct sy602x_reg sy602x_reg_init_seq_1[] = {
	{.reg = SY602X_LDRC_ENVLP_TC_UP,	.val = 0x01FC05 },
	{.reg = SY602X_LDRC_ENVLP_TC_DN,	.val = 0x7FE900 },
	{.reg = SY602X_MDRC_ENVLP_TC_UP,	.val = 0x01FC05 },
	{.reg = SY602X_MDRC_ENVLP_TC_DN,	.val = 0x7F9E9F },
	{.reg = SY602X_HDRC_ENVLP_TC_UP,	.val = 0x01FC05 },
	{.reg = SY602X_HDRC_ENVLP_TC_DN,	.val = 0x7D5C65 },
	{.reg = SY602X_PDRC_ENVLP_TC_UP,	.val = 0x01FC05 },
	{.reg = SY602X_PDRC_ENVLP_TC_DN,	.val = 0x7FBBCD },

	{.reg = SY602X_LDRC_LMT_CFG1,	.val = 0x3C930C },
	{.reg = SY602X_LDRC_LMT_CFG2,	.val = 0x7FFF51 },
	{.reg = SY602X_LDRC_LMT_CFG3,	.val = 0x7F55C6 },
	{.reg = SY602X_MDRC_LMT_CFG1,	.val = 0x3C530C },
	{.reg = SY602X_MDRC_LMT_CFG2,	.val = 0x7FFF51 },
	{.reg = SY602X_MDRC_LMT_CFG3,	.val = 0x7F1D3B },
	{.reg = SY602X_HDRC_LMT_CFG1,	.val = 0x3C930C },
	{.reg = SY602X_HDRC_LMT_CFG2,	.val = 0x7FFF51 },
	{.reg = SY602X_HDRC_LMT_CFG3,	.val = 0x0FF016 },
	{.reg = SY602X_PDRC_LMT_CFG1,	.val = 0x00030C },
	{.reg = SY602X_PDRC_LMT_CFG2,	.val = 0x7F77C0 },
	{.reg = SY602X_PDRC_LMT_CFG3,	.val = 0x7FFEA2 },

	{.reg = SY602X_DSP_CTL3,	.val = 0x00 },
	{.reg = SY602X_FUNC_DEBUG,	.val = 0x50 },
	{.reg = SY602X_PBQ_CHECKSUM,	.val = 0x08FFB77D },
	{.reg = SY602X_DRC_CHECKSUM,	.val = 0x0000001E },

	{.reg = SY602X_DRC_CTRL,	.val = 0x01000001 },
	{.reg = SY602X_MAS_VOL,	.val = 0xF3 },
	{.reg = SY602X_CH1_VOL,	.val = 0xB2 },
	{.reg = SY602X_CH2_VOL,	.val = 0xB2 },
	{.reg = SY602X_POST_SCALER,	.val = 0x7F },
	{.reg = SY602X_FINE_VOL,	.val = 0x00 },
	{.reg = SY602X_IN_MUX_REG,	.val = 0x00 },
	{.reg = SY602X_DSP_CTL_REG1,	.val = 0x06 },
	{.reg = SY602X_MONITOR_CFG1,	.val = 0x00 },
	{.reg = SY602X_MONITOR_CFG2,	.val = 0x00 },
	{.reg = SY602X_CHECKSUM_CTL,	.val = 0x00 },
	{.reg = SY602X_PWM_CTRL,	.val = 0x03 },
	{.reg = SY602X_SOFT_MUTE,	.val = 0x10 },
};
