/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __AML_PDM_H__
#define __AML_PDM_H__

#include <linux/clk.h>
#include <linux/pinctrl/consumer.h>

#define DRV_NAME "snd_pdm"

#define DEFAULT_FS_RATIO		256

#define PDM_CHANNELS_MIN		1
/* 8ch pdm in, 8 ch tdmin_lb */
#define PDM_CHANNELS_LB_MAX		(PDM_CHANNELS_MAX + 8)

#define PDM_RATES			(SNDRV_PCM_RATE_96000 |\
					 SNDRV_PCM_RATE_64000 |\
					 SNDRV_PCM_RATE_48000 |\
					 SNDRV_PCM_RATE_32000 |\
					 SNDRV_PCM_RATE_16000 |\
					 SNDRV_PCM_RATE_8000)

#define PDM_FORMATS			(SNDRV_PCM_FMTBIT_S16_LE |\
					 SNDRV_PCM_FMTBIT_S24_LE |\
					 SNDRV_PCM_FMTBIT_S32_LE)

enum {
	PDM_RUN_MUTE_VAL = 0,
	PDM_RUN_MUTE_CHMASK,

	PDM_RUN_MAX,
};

struct pdm_chipinfo {
	/* pdm supports mute function */
	bool mute_fn;
	/* truncate invalid data when filter init */
	bool truncate_data;
	/* train */
	bool train;
	/* vad top */
	bool vad_top;
};

struct aml_pdm {
	struct device *dev;
	struct aml_audio_controller *actrl;
	struct pinctrl *pdm_pins;
	struct clk *clk_gate;
	/* sel: fclk_div3(666M) */
	struct clk *sysclk_srcpll;
	/* consider same source with tdm, 48k(24576000) */
	struct clk *dclk_srcpll;
	struct clk *clk_pdm_sysclk;
	struct clk *clk_pdm_dclk;
	struct toddr *tddr;

	struct pdm_chipinfo *chipinfo;
	struct snd_kcontrol *controls[PDM_RUN_MAX];

	/* sample rate */
	int rate;
	/*
	 * filter mode:0~4,
	 * from mode 0 to 4, the performance is from high to low,
	 * the group delay (latency) is from high to low.
	 */
	int filter_mode;
	/* dclk index */
	int dclk_idx;
	/* PCM or Raw Data */
	int bypass;

	/* lane mask in, each lane carries two channels */
	int lane_mask_in;

	unsigned int syssrc_clk_rate;
	unsigned int sram[2];
	unsigned int aud_sram;
	/* PDM clk on/off, only clk on, pdm registers can be accessed */
	unsigned int clk_on;

	/* train */
	unsigned int train_en;

	/* external setting: low power mode, for dclk_sycpll to 24m */
	unsigned int is_low_power;
	/* internal status: low power mode or not */
	unsigned int force_lowpower;
	/* dts config: keep high power or not, when suspend */
	unsigned int high_power;
};

#endif /*__AML_PDM_H__*/
