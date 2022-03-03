// SPDX-License-Identifier: GPL-2.0
/*
 * PDM match talbe
 *
 * Copyright (C) 2019 Amlogic, Inc. All rights reserved.
 *
 */

static struct pdm_chipinfo g12a_pdm_chipinfo = {
	.mute_fn         = true,
	.truncate_data   = false,
};

static struct pdm_chipinfo tl1_pdm_chipinfo = {
	.mute_fn         = true,
	.truncate_data   = false,
};

static struct pdm_chipinfo sm1_pdm_chipinfo = {
	.mute_fn         = true,
	.truncate_data   = false,
	.train           = true,
};

static struct pdm_chipinfo tm2_pdm_chipinfo = {
	.mute_fn         = true,
	.truncate_data   = false,
	.train           = true,
};

static struct pdm_chipinfo a1_pdm_chipinfo = {
	.mute_fn         = true,
	.truncate_data   = false,
	.train           = true,
	.vad_top         = true,
};

static const struct of_device_id aml_pdm_device_id[] = {
	{
		.compatible = "amlogic, axg-snd-pdm",
	},
	{
		.compatible = "amlogic, g12a-snd-pdm",
		.data       = &g12a_pdm_chipinfo,
	},
	{
		.compatible = "amlogic, tl1-snd-pdm",
		.data       = &tl1_pdm_chipinfo,
	},
	{
		.compatible = "amlogic, sm1-snd-pdm",
		.data		= &sm1_pdm_chipinfo,
	},
	{
		.compatible = "amlogic, tm2-snd-pdm",
		.data		= &tm2_pdm_chipinfo,
	},
	{
		.compatible = "amlogic, a1-snd-pdm",
		.data		= &a1_pdm_chipinfo,
	},

	{}
};
MODULE_DEVICE_TABLE(of, aml_pdm_device_id);
