/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * S/PDIF match table
 *
 * Copyright (C) 2019 Amlogic, Inc. All rights reserved.
 *
 */

#define SPDIF_A	0
#define SPDIF_B	1

struct spdif_chipinfo {
	unsigned int id;

	/* add ch_cnt to ch_num */
	bool chnum_en;
	/*
	 * axg, clear all irq bits
	 * after axg, such as g12a, clear each bits
	 * Reg_clr_interrupt[7:0] for each bit of irq_status[7:0];
	 */
	bool clr_irq_all_bits;
	/* no PaPb irq */
	bool irq_no_papb;
	/* reg_hold_start_en; 1: add delay to match TDM out when share buff; */
	bool hold_start;
	/* eq/drc */
	bool eq_drc_en;
	/* pc, pd interrupt is separated. */
	bool pcpd_separated;
	/* same source, spdif re-enable */
	bool same_src_spdif_reen;
	/* async fifo */
	bool async_fifo;

	/* from tm2_revb */
	bool separate_tohdmitx_en;
	bool sample_mode_filter_en;
	unsigned int spdifout_lane_mask;
	bool use_arb;
	bool regulator;
	/* offset by the spec */
	int out_reset_reg_offset;
	/* offset by the spec */
	int out_reset_reg_shift;
	/* for input only */
	bool only_capture;
};

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
struct spdif_chipinfo axg_spdif_chipinfo = {
	.id               = SPDIF_A,
	.irq_no_papb      = true,
	.clr_irq_all_bits = true,
	.pcpd_separated   = true,
	.spdifout_lane_mask = SPDIFOUT_LANE_MASK_V1,
	.use_arb          = true,
	.out_reset_reg_offset = 9,
	.out_reset_reg_shift = 15,
};

struct spdif_chipinfo g12a_spdif_a_chipinfo = {
	.id             = SPDIF_A,
	.chnum_en       = true,
	.hold_start     = true,
	.eq_drc_en      = true,
	.pcpd_separated = true,
	.spdifout_lane_mask = SPDIFOUT_LANE_MASK_V1,
	.use_arb        = true,
	.out_reset_reg_offset = 9,
	.out_reset_reg_shift = 15,
};

struct spdif_chipinfo g12a_spdif_b_chipinfo = {
	.id             = SPDIF_B,
	.chnum_en       = true,
	.hold_start     = true,
	.eq_drc_en      = true,
	.pcpd_separated = true,
	.spdifout_lane_mask = SPDIFOUT_LANE_MASK_V1,
	.use_arb        = true,
	.out_reset_reg_offset = 9,
	.out_reset_reg_shift = 16,
};

#ifndef CONFIG_AMLOGIC_REMOVE_OLD
struct spdif_chipinfo tl1_spdif_a_chipinfo = {
	.id           = SPDIF_A,
	.chnum_en     = true,
	.hold_start   = true,
	.eq_drc_en    = true,
	.async_fifo   = true,
	.spdifout_lane_mask = SPDIFOUT_LANE_MASK_V1,
	.use_arb      = true,
	.out_reset_reg_offset = 0xa,
	.out_reset_reg_shift = 15,
};

struct spdif_chipinfo tl1_spdif_b_chipinfo = {
	.id           = SPDIF_B,
	.chnum_en     = true,
	.hold_start   = true,
	.eq_drc_en    = true,
	.async_fifo   = true,
	.spdifout_lane_mask = SPDIFOUT_LANE_MASK_V1,
	.use_arb      = true,
	.out_reset_reg_offset = 0xa,
	.out_reset_reg_shift = 16,
};
#endif

struct spdif_chipinfo sm1_spdif_a_chipinfo = {
	.id           = SPDIF_A,
	.chnum_en     = true,
	.hold_start   = true,
	.eq_drc_en    = true,
	.async_fifo   = true,
	.spdifout_lane_mask = SPDIFOUT_LANE_MASK_V1,
	.use_arb      = true,
	.out_reset_reg_offset = 0xa,
	.out_reset_reg_shift = 15,
};

struct spdif_chipinfo sm1_spdif_b_chipinfo = {
	.id           = SPDIF_B,
	.chnum_en     = true,
	.hold_start   = true,
	.eq_drc_en    = true,
	.async_fifo   = true,
	.spdifout_lane_mask = SPDIFOUT_LANE_MASK_V1,
	.use_arb      = true,
	.out_reset_reg_offset = 0xa,
	.out_reset_reg_shift = 16,
};

struct spdif_chipinfo tm2_spdif_a_chipinfo = {
	.id           = SPDIF_A,
	.chnum_en     = true,
	.hold_start   = true,
	.eq_drc_en    = true,
	.async_fifo   = true,
	.spdifout_lane_mask = SPDIFOUT_LANE_MASK_V1,
	.use_arb      = true,
	.out_reset_reg_offset = 0xa,
	.out_reset_reg_shift = 15,
};

struct spdif_chipinfo tm2_spdif_b_chipinfo = {
	.id           = SPDIF_B,
	.chnum_en     = true,
	.hold_start   = true,
	.eq_drc_en    = true,
	.async_fifo   = true,
	.spdifout_lane_mask = SPDIFOUT_LANE_MASK_V1,
	.use_arb      = true,
	.out_reset_reg_offset = 0xa,
	.out_reset_reg_shift = 16,
};
#endif

struct spdif_chipinfo tm2_revb_spdif_a_chipinfo = {
	.id           = SPDIF_A,
	.chnum_en     = true,
	.hold_start   = true,
	.eq_drc_en    = true,
	.async_fifo   = true,
	.separate_tohdmitx_en = true,
	.sample_mode_filter_en = true,
	.spdifout_lane_mask = SPDIFOUT_LANE_MASK_V2,
	.use_arb      = true,
	.out_reset_reg_offset = 0xa,
	.out_reset_reg_shift = 15,
};

struct spdif_chipinfo tm2_revb_spdif_b_chipinfo = {
	.id           = SPDIF_B,
	.chnum_en     = true,
	.hold_start   = true,
	.eq_drc_en    = true,
	.async_fifo   = true,
	.separate_tohdmitx_en = true,
	.spdifout_lane_mask = SPDIFOUT_LANE_MASK_V2,
	.use_arb      = true,
	.out_reset_reg_offset = 0xa,
	.out_reset_reg_shift = 16,
};

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
struct spdif_chipinfo a5_spdif_a_chipinfo = {
	.id           = SPDIF_A,
	.chnum_en     = true,
	.hold_start   = true,
	.eq_drc_en    = true,
	.async_fifo   = true,
	.spdifout_lane_mask = SPDIFOUT_LANE_MASK_V2,
	.use_arb      = false,
	.regulator    = true,
	.out_reset_reg_offset = 0xa,
	.out_reset_reg_shift = 15,
};

struct spdif_chipinfo a1_spdif_in_chipinfo = {
	.id           = SPDIF_A,
	.chnum_en     = true,
	.hold_start   = true,
	.eq_drc_en    = true,
	.async_fifo   = true,
	.spdifout_lane_mask = SPDIFOUT_LANE_MASK_V1,
	.use_arb      = true,
	.only_capture = true,
};
#endif

static const struct of_device_id aml_spdif_device_id[] = {
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	{
		.compatible = "amlogic, g12a-snd-spdif-a",
		.data       = &g12a_spdif_a_chipinfo,
	},
	{
		.compatible = "amlogic, g12a-snd-spdif-b",
		.data       = &g12a_spdif_b_chipinfo,
	},
#ifndef CONFIG_AMLOGIC_REMOVE_OLD
	{
		.compatible = "amlogic, tl1-snd-spdif-a",
		.data       = &tl1_spdif_a_chipinfo,
	},
	{
		.compatible = "amlogic, tl1-snd-spdif-b",
		.data       = &tl1_spdif_b_chipinfo,
	},
#endif
	{
		.compatible = "amlogic, sm1-snd-spdif-a",
		.data		= &sm1_spdif_a_chipinfo,
	},
	{
		.compatible = "amlogic, sm1-snd-spdif-b",
		.data		= &sm1_spdif_b_chipinfo,
	},
	{
		.compatible = "amlogic, tm2-snd-spdif-a",
		.data		= &tm2_spdif_a_chipinfo,
	},
	{
		.compatible = "amlogic, tm2-snd-spdif-b",
		.data		= &tm2_spdif_b_chipinfo,
	},
#endif
	{
		.compatible = "amlogic, tm2-revb-snd-spdif-a",
		.data		= &tm2_revb_spdif_a_chipinfo,
	},
	{
		.compatible = "amlogic, tm2-revb-snd-spdif-b",
		.data		= &tm2_revb_spdif_b_chipinfo,
	},
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	{
		.compatible = "amlogic, a5-snd-spdif-a",
		.data		= &a5_spdif_a_chipinfo,
	},
	{
		.compatible = "amlogic, axg-snd-spdif",
		.data		= &axg_spdif_chipinfo,
	},
	{
		.compatible = "amlogic, a1-snd-spdif-a",
		.data		= &a1_spdif_in_chipinfo,
	},
#endif
	{},
};
MODULE_DEVICE_TABLE(of, aml_spdif_device_id);
