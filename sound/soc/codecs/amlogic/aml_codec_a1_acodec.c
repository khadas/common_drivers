// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/regmap.h>

#include <linux/amlogic/media/sound/auge_utils.h>

#include "../../../soc/amlogic/auge/iomap.h"
#include "../../../soc/amlogic/auge/regs.h"
#include "aml_codec_a1_acodec.h"

struct a1_acodec_chipinfo {
	int id;
	bool is_bclk_cap_inv;	//default true
	bool is_bclk_o_inv;		//default false
	bool is_lrclk_inv;		//default false
	bool is_dac_phase_differ_exist;
	bool is_adc_phase_differ_exist;
	int mclk_sel;
	bool separate_toacodec_en;
	int ib_con;
};

struct a1_acodec_priv {
	struct snd_soc_component *component;
	struct snd_pcm_hw_params *params;
	struct regmap *regmap;
	struct work_struct work;
	struct a1_acodec_chipinfo *chipinfo;
	int tdmout_index;
	int dat0_ch_sel;

	int tdmin_index;
	int adc_output_sel;
	int dac1_input_sel;
	int dacr_output_inv;
};

static const struct reg_default a1_acodec_init_list[] = {
	{ACODEC_0, 0x3400BFFA},
	{ACODEC_1, 0x50501010},
	{ACODEC_2, 0xFBFB0000},
	{ACODEC_3, 0x00002040},
	{ACODEC_4, 0x00010000},
	{ACODEC_5, 0x0000a50b},/* single end setting default */
};

static struct a1_acodec_chipinfo a1_acodec_cinfo = {
	.id = 0,
	.is_bclk_cap_inv = true,	//default  true
	.is_bclk_o_inv = false,		//default  false
	.is_lrclk_inv = false,

	.is_dac_phase_differ_exist = false,
	.is_adc_phase_differ_exist = true,
	//if is_adc_phase_differ=true,modified tdmin_in_rev_ws,revert ws(lrclk);
	//0 :disable; 1: enable;
	//.mclk_sel = 1,
	.separate_toacodec_en = false,
};

static struct a1_acodec_chipinfo c2_acodec_cinfo = {
	.id = 0,
	.is_bclk_cap_inv = true,	//default  true
	.is_bclk_o_inv = false,		//default  false
	.is_lrclk_inv = false,

	.is_dac_phase_differ_exist = false,
	.is_adc_phase_differ_exist = true,
	//if is_adc_phase_differ=true,modified tdmin_in_rev_ws,revert ws(lrclk);
	//0 :disable; 1: enable;
	//.mclk_sel = 1,
	.separate_toacodec_en = true,
	.ib_con = 2,
};

static int a1_acodec_reg_init(struct snd_soc_component *component)
{
	int i;
	struct a1_acodec_priv *aml_acodec =
		snd_soc_component_get_drvdata(component);

	for (i = 0; i < ARRAY_SIZE(a1_acodec_init_list); i++)
		snd_soc_component_write
			(component,
			 a1_acodec_init_list[i].reg,
			 a1_acodec_init_list[i].def);

	if (!aml_acodec->dacr_output_inv) {
		snd_soc_component_write
			(component, ACODEC_3, 0x00002020);
	}

	if (aml_acodec->chipinfo->ib_con)
		snd_soc_component_update_bits
			(component, ACODEC_4,
			 0x3 << 16,
			 aml_acodec->chipinfo->ib_con << 16);

	return 0;
}

static const DECLARE_TLV_DB_SCALE(pga_in_tlv, -1200, 250, 1);
static const DECLARE_TLV_DB_SCALE(adc_vol_tlv, -29625, 375, 1);
static const DECLARE_TLV_DB_SCALE(dac_vol_tlv, -9435, 37, 1);

static const struct snd_kcontrol_new a1_acodec_snd_controls[] = {
	/*PGA_IN Gain */
	SOC_DOUBLE_TLV
		("PGA IN Gain", ACODEC_1,
		 PGAL_IN_GAIN, PGAR_IN_GAIN,
		 0x1f, 0, pga_in_tlv),

	/*ADC Digital Volume control */
	SOC_DOUBLE_TLV
		("ADC Digital Capture Volume", ACODEC_1,
		 ADCL_VC, ADCR_VC,
		 0x7f, 0, adc_vol_tlv),

	/*DAC Digital Volume control */
	SOC_DOUBLE_TLV
		("DAC Digital Playback Volume",
		 ACODEC_2,
		 DACL_VC, DACR_VC,
		 0xff, 0, dac_vol_tlv),
};

/*line out 1 Left mux */
static const char * const out_lo1l_txt[] = {
	"None", "LO1L_SEL_INL", "LO1L_SEL_DACL", "Reserved", "LO1L_SEL_DACR_INV"
};

static SOC_ENUM_SINGLE_DECL(out_lo1l_enum, ACODEC_3,
			    LO1L_SEL_INL, out_lo1l_txt);

static const struct snd_kcontrol_new lo1l_mux =
SOC_DAPM_ENUM("LO1L_MUX", out_lo1l_enum);

/*line out 1 right mux */
static const char * const out_lo1r_txt[] = {
	"None", "LO1R_SEL_INR", "LO1R_SEL_DACR", "Reserved", "LO1R_SEL_DACL_INV"
};

static SOC_ENUM_SINGLE_DECL(out_lo1r_enum, ACODEC_3,
			    LO1R_SEL_INR, out_lo1r_txt);

static const struct snd_kcontrol_new lo1r_mux =
SOC_DAPM_ENUM("LO1R_MUX", out_lo1r_enum);

static const struct snd_soc_dapm_widget a1_acodec_dapm_widgets[] = {
	/*PGA input */
	SND_SOC_DAPM_PGA("PGAL_IN_EN", SND_SOC_NOPM,
			 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("PGAR_IN_EN", SND_SOC_NOPM,
			 0, 0, NULL, 0),

	/*ADC capture stream */
	SND_SOC_DAPM_ADC("Left ADC", "Capture", SND_SOC_NOPM,
			 0, 0),
	SND_SOC_DAPM_ADC("Right ADC", "Capture", SND_SOC_NOPM,
			 0, 0),

	/*DAC playback stream */
	SND_SOC_DAPM_DAC
		("Left DAC", "Playback",
		 SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_DAC
		("Right DAC", "Playback",
		 SND_SOC_NOPM, 0, 0),

	/*DRV output */
	SND_SOC_DAPM_OUT_DRV
		("LO1L_OUT_EN", SND_SOC_NOPM,
		 LO1L_EN, 0, NULL, 0),
	SND_SOC_DAPM_OUT_DRV
		("LO1R_OUT_EN", SND_SOC_NOPM,
		 LO1R_EN, 0, NULL, 0),

	/*MUX output source select */
	SND_SOC_DAPM_MUX("Lineout 1 left switch", SND_SOC_NOPM,
			 0, 0, &lo1l_mux),
	SND_SOC_DAPM_MUX("Lineout 1 right switch", SND_SOC_NOPM,
			 0, 0, &lo1r_mux),

};

static const struct snd_soc_dapm_route a1_acodec_dapm_routes[] = {
	/* Input path */
	{"Left ADC", NULL, "PGAL_IN_EN"},
	{"Right ADC", NULL, "PGAR_IN_EN"},

	/*Output path*/
	{"Lineout 1 left switch", NULL, "Left DAC"},
	{"Lineout 1 left switch", NULL, "Right DAC"},

	{"Lineout 1 right switch", NULL, "Right DAC"},
	{"Lineout 1 right switch", NULL, "Left DAC"},

	{"LO1L_OUT_EN", NULL, "Lineout 1 left switch"},
	{"LO1R_OUT_EN", NULL, "Lineout 1 right switch"},

	// {"Lineout 1 left", NULL, "LO1L_OUT_EN"},
	// {"Lineout 1 right", NULL, "LO1R_OUT_EN"},
};

static int a1_acodec_dai_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_component *component = dai->component;
	u32 val = snd_soc_component_read(component, ACODEC_0);

	pr_info("%s, format:%x, codec = %p\n", __func__, fmt, component);

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		val |= (0x1 << I2S_MODE);
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		val &= ~(0x1 << I2S_MODE);
		break;
	default:
		return -EINVAL;
	}

	snd_soc_component_write(component, ACODEC_0, val);

	return 0;
}

static int a1_acodec_dai_set_sysclk(struct snd_soc_dai *dai,
				    int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int a1_acodec_dai_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params,
				   struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct a1_acodec_priv *aml_acodec =
		snd_soc_component_get_drvdata(component);

	pr_debug("%s!\n", __func__);

	aml_acodec->params = params;

	return 0;
}

static int a1_acodec_dai_set_bias_level(struct snd_soc_component *component,
					enum snd_soc_bias_level level)
{
	switch (level) {
	case SND_SOC_BIAS_ON:

		break;

	case SND_SOC_BIAS_PREPARE:

		break;

	case SND_SOC_BIAS_STANDBY:
		if (component->dapm.bias_level == SND_SOC_BIAS_OFF)
			snd_soc_component_cache_sync(component);
		break;

	case SND_SOC_BIAS_OFF:
		//snd_soc_component_write(component, ACODEC_0, 0);
		break;

	default:
		break;
	}
	component->dapm.bias_level = level;

	return 0;
}

static int a1_acodec_dai_prepare(struct snd_pcm_substream *substream,
				 struct snd_soc_dai *dai)
{
	return 0;
}

static int a1_acodec_reset(struct snd_soc_component *component)
{
	/* reset register is not audio register,and it is not acodec reset
	 *	struct a1_acodec_priv *a1_acodec =
	 *			snd_soc_component_get_drvdata(component);
	 *	if (a1_acodec)
	 *		auge_acodec_reset();
	 *	usleep_range(950, 1000);
	 */
	return 0;
}

static int a1_acodec_start_up(struct snd_soc_component *component)
{
	snd_soc_component_write(component, ACODEC_0, 0xF000);
	msleep(200);
	snd_soc_component_write(component, ACODEC_0, 0xB000);

	return 0;
}

static int a1_acodec_set_toacodec(struct a1_acodec_priv *aml_acodec);

static void a1_acodec_release_fast_mode_work_func(struct work_struct *p_work)
{
	struct a1_acodec_priv *aml_acodec;
	struct snd_soc_component *component;

	aml_acodec = container_of(p_work, struct a1_acodec_priv, work);
	if (!aml_acodec) {
		pr_err("%s, Get a1_acodec_priv fail\n", __func__);
		return;
	}

	component = aml_acodec->component;
	if (!component) {
		pr_err("%s, Get snd_soc_codec fail\n", __func__);
		return;
	}

	pr_info("%s\n", __func__);
	a1_acodec_set_toacodec(aml_acodec);
	/*reset audio codec register*/
	a1_acodec_reset(component);
	a1_acodec_start_up(component);
	a1_acodec_reg_init(component);

	aml_acodec->component = component;
	a1_acodec_dai_set_bias_level(component, SND_SOC_BIAS_STANDBY);
}

static int a1_acodec_dai_mute_stream(struct snd_soc_dai *dai, int mute, int stream)
{
	struct a1_acodec_priv *aml_acodec =
		snd_soc_component_get_drvdata(dai->component);
	u32 reg_val;
	int ret;

	pr_debug("%s, mute:%d\n", __func__, mute);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		ret = regmap_read
			(aml_acodec->regmap,
			 ACODEC_2, &reg_val);
		if (mute)
			reg_val |= (0x1 << DAC_SOFT_MUTE);
		else
			reg_val &= ~(0x1 << DAC_SOFT_MUTE);

		ret = regmap_write
			(aml_acodec->regmap,
			 ACODEC_2, reg_val);
	}

	return 0;
}

struct snd_soc_dai_ops a1_acodec_dai_ops = {
	.hw_params = a1_acodec_dai_hw_params,
	.prepare = a1_acodec_dai_prepare,
	.set_fmt = a1_acodec_dai_set_fmt,
	.set_sysclk = a1_acodec_dai_set_sysclk,
	.mute_stream = a1_acodec_dai_mute_stream,
};

static int a1_acodec_probe(struct snd_soc_component *component)
{
	struct a1_acodec_priv *aml_acodec =
		snd_soc_component_get_drvdata(component);

	if (!aml_acodec) {
		pr_err("Failed to get a1 acodec priv\n");
		return -EINVAL;
	}
	aml_acodec->component = component;
	INIT_WORK(&aml_acodec->work, a1_acodec_release_fast_mode_work_func);
	schedule_work(&aml_acodec->work);

	pr_info("%s\n", __func__);
	return 0;
}

static void a1_acodec_remove(struct snd_soc_component *component)
{
	struct a1_acodec_priv *aml_acodec =
		snd_soc_component_get_drvdata(component);
	pr_info("%s!\n", __func__);
	cancel_work_sync(&aml_acodec->work);
	a1_acodec_dai_set_bias_level(component, SND_SOC_BIAS_OFF);
}

static int a1_acodec_suspend(struct snd_soc_component *component)
{
	pr_info("%s!\n", __func__);

	//a1_acodec_dai_set_bias_level(component, SND_SOC_BIAS_OFF);
	snd_soc_component_write(component, ACODEC_0, 0x340300c0);
	snd_soc_component_write(component, ACODEC_1, 0x0);
	snd_soc_component_write(component, ACODEC_2, 0x0);
	snd_soc_component_write(component, ACODEC_3, 0x0);
	snd_soc_component_write(component, ACODEC_4, 0x0);
	snd_soc_component_write(component, ACODEC_5, 0x0);

	return 0;
}

static int a1_acodec_resume(struct snd_soc_component *component)
{
	pr_info("%s!\n", __func__);

	a1_acodec_reset(component);
	a1_acodec_start_up(component);
	a1_acodec_reg_init(component);

	a1_acodec_dai_set_bias_level(component, SND_SOC_BIAS_STANDBY);

	return 0;
}

const static struct snd_soc_component_driver soc_codec_dev_a1_acodec = {
	.probe = a1_acodec_probe,
	.remove = a1_acodec_remove,
	.suspend = a1_acodec_suspend,
	.resume = a1_acodec_resume,
	.set_bias_level = a1_acodec_dai_set_bias_level,
	.controls = a1_acodec_snd_controls,
	.num_controls = ARRAY_SIZE(a1_acodec_snd_controls),
	.dapm_widgets = a1_acodec_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(a1_acodec_dapm_widgets),
	.dapm_routes = a1_acodec_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(a1_acodec_dapm_routes),
};

static const struct regmap_config a1_acodec_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = 0x1c,
	.reg_defaults = a1_acodec_init_list,
	.num_reg_defaults = ARRAY_SIZE(a1_acodec_init_list),
	.cache_type = REGCACHE_RBTREE,
};

#define a1_ACODEC_RATES		SNDRV_PCM_RATE_8000_96000
#define a1_ACODEC_FORMATS		(SNDRV_PCM_FMTBIT_S16_LE \
					 | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE \
					 | SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S32_LE)

struct snd_soc_dai_driver aml_a1_acodec_dai = {
	.name = "a1-acodec-hifi",
	.id = 0,
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 8,
		.rates = a1_ACODEC_RATES,
		.formats = a1_ACODEC_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 8,
		.rates = a1_ACODEC_RATES,
		.formats = a1_ACODEC_FORMATS,
	},
	.ops = &a1_acodec_dai_ops,
};

/* TOacodec only support write, no read. */
static int a1_acodec_set_toacodec(struct a1_acodec_priv *aml_acodec)
{
	int dat0_sel, dat1_sel, lrclk_sel, bclk_sel, mclk_sel;
	int tmp;
	unsigned int val = 0x0;

	if (aml_acodec->chipinfo->is_bclk_cap_inv)
		val |= (0x1 << 9);
	if (aml_acodec->chipinfo->is_bclk_o_inv)
		val |= (0x1 << 8);
	if (aml_acodec->chipinfo->is_lrclk_inv)
		val |= (0x1 << 10);

	tmp = (aml_acodec->tdmout_index << 3) + aml_acodec->dat0_ch_sel;
	dat0_sel = tmp << 16;
	dat1_sel = tmp << 20;
	lrclk_sel = (aml_acodec->tdmout_index) << 12;
	bclk_sel = (aml_acodec->tdmout_index) << 4;

	//mclk_sel = aml_acodec->chipinfo->mclk_sel;
	mclk_sel = aml_acodec->tdmout_index;

	val |= dat0_sel | dat1_sel | lrclk_sel | bclk_sel | mclk_sel;

	/* if toacodec_en is separated, need do:
	 * step1: enable/disable mclk
	 * step2: enable/disable bclk
	 * step3: enable/disable dat
	 */
	/* this reg can't be read on a1/c1, so can't use audiobus_update_bits */
	if (aml_acodec->chipinfo->separate_toacodec_en) {
		val |= 0x1 << 29;
		audiobus_write(EE_AUDIO_TOACODEC_CTRL0, val);

		val |= 0x1 << 30;
		audiobus_write(EE_AUDIO_TOACODEC_CTRL0, val);
	}

	val |= 0x1 << 31;
	audiobus_write(EE_AUDIO_TOACODEC_CTRL0, val);

	pr_info("%s, is_bclk_cap_inv %s\n", __func__,
		aml_acodec->chipinfo->is_bclk_cap_inv ? "true" : "false");
	pr_info("%s, is_bclk_o_inv %s\n", __func__,
		aml_acodec->chipinfo->is_bclk_o_inv ? "true" : "false");
	pr_info("%s, is_lrclk_inv %s\n", __func__,
		aml_acodec->chipinfo->is_lrclk_inv ? "true" : "false");
	pr_info("%s read EE_AUDIO_TOACODEC_CTRL0=0x%08x\n", __func__,
		val);

	return 0;
}

static int aml_a1_acodec_probe(struct platform_device *pdev)
{
	struct a1_acodec_priv *aml_acodec;
	struct a1_acodec_chipinfo *p_chipinfo;
	struct resource *res_mem;
	struct device_node *np;
	void __iomem *regs;
	int ret = 0;

	dev_info(&pdev->dev, "%s\n", __func__);

	np = pdev->dev.of_node;

	aml_acodec = devm_kzalloc
		(&pdev->dev,
		 sizeof(struct a1_acodec_priv),
		 GFP_KERNEL);
	if (!aml_acodec)
		return -ENOMEM;

	/* match data */
	p_chipinfo = (struct a1_acodec_chipinfo *)
		of_device_get_match_data(&pdev->dev);
	if (!p_chipinfo)
		dev_warn_once(&pdev->dev, "check whether to update a1_acodec_chipinfo\n");

	aml_acodec->chipinfo = p_chipinfo;

	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res_mem)
		return -ENODEV;

	regs = devm_ioremap_resource(&pdev->dev, res_mem);
	if (IS_ERR(regs))
		return PTR_ERR(regs);

	aml_acodec->regmap = devm_regmap_init_mmio
		(&pdev->dev, regs, &a1_acodec_regmap_config);
	if (IS_ERR(aml_acodec->regmap))
		return PTR_ERR(aml_acodec->regmap);

	of_property_read_u32
		(pdev->dev.of_node,
		 "tdmout_index",
		 &aml_acodec->tdmout_index);
	pr_info("aml_a1_acodec tdmout_index=%d\n",
		aml_acodec->tdmout_index);

	of_property_read_u32
		(pdev->dev.of_node,
		 "dat0_ch_sel",
		 &aml_acodec->dat0_ch_sel);
	pr_info("aml_a1_acodec dat0_ch_sel=%d\n",
		aml_acodec->dat0_ch_sel);

	ret = of_property_read_u32
		(pdev->dev.of_node,
		 "dacr_output_inv",
		 &aml_acodec->dacr_output_inv);
	if (ret < 0)
		aml_acodec->dacr_output_inv = 1;
	pr_info("aml_a1_acodec dacr_output_inv=%d\n",
		aml_acodec->dacr_output_inv);

	of_property_read_u32
		(pdev->dev.of_node,
		 "tdmin_index",
		 &aml_acodec->tdmin_index);
	pr_info("aml_a1_acodec tdmin_index=%d\n",
		aml_acodec->tdmin_index);

	platform_set_drvdata(pdev, aml_acodec);

	ret = devm_snd_soc_register_component
		(&pdev->dev,
		 &soc_codec_dev_a1_acodec,
		 &aml_a1_acodec_dai, 1);

	if (ret)
		pr_info("%s call snd_soc_register_codec error\n", __func__);
	else
		pr_info("%s over\n", __func__);

	return ret;
}

static int aml_a1_acodec_remove(struct platform_device *pdev)
{
	snd_soc_unregister_component(&pdev->dev);

	return 0;
}

static void aml_a1_acodec_shutdown(struct platform_device *pdev)
{
	struct a1_acodec_priv *aml_acodec;
	struct snd_soc_component *component;

	aml_acodec = platform_get_drvdata(pdev);
	component = aml_acodec->component;
	if (component)
		a1_acodec_remove(component);
}

static const struct of_device_id aml_a1_acodec_dt_match[] = {
	{
		.compatible = "amlogic, a1_acodec",
		.data = &a1_acodec_cinfo,
	},
	{
		.compatible = "amlogic, c2_acodec",
		.data = &c2_acodec_cinfo,
	},
	{},
};

static struct platform_driver aml_a1_acodec_platform_driver = {
	.driver = {
		.name = "a1_acodec",
		.owner = THIS_MODULE,
		.of_match_table = aml_a1_acodec_dt_match,
	},
	.probe = aml_a1_acodec_probe,
	.remove = aml_a1_acodec_remove,
	.shutdown = aml_a1_acodec_shutdown,
};

static int __init aml_a1_acodec_modinit(void)
{
	int ret = 0;

	ret = platform_driver_register(&aml_a1_acodec_platform_driver);
	if (ret != 0)
		pr_err("register a1 acodec fail: %d\n", ret);

	return ret;
}

module_init(aml_a1_acodec_modinit);

static void __exit aml_a1_acodec_modexit(void)
{
	platform_driver_unregister(&aml_a1_acodec_platform_driver);
}

module_exit(aml_a1_acodec_modexit);

MODULE_DESCRIPTION("ASoC AML a1 audio codec driver");
MODULE_AUTHOR("AMLogic, Inc.");
MODULE_LICENSE("GPL");
