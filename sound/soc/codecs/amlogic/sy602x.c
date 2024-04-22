// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/*
 * ASoC Driver for SY602X
 *
 * Author:    Xiaotian Su <xiaotian.su@silergycorp.com>
 *        Copyright 2022
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

//updated by Xiaotian Su on 20220210
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/of_device.h>
#include <linux/spi/spi.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include "sy602x.h"

#define DAP_RAM_ITEM_SIZE           (1 + 20) // address(1 byte) + data(20 byte)
#define DAP_RAM_ITEM_ALL           (BIT(DAP_RAM_ITEM_NUM) - 1)
static struct i2c_client *i2c;

struct sy602x_reg_switch {
	uint reg;
	uint mask;
	uint on_value;
	uint off_value;
};

static const struct sy602x_reg sy602x_reg_init_reset[] = {
	{.reg = SY602X_DC_SOFT_RESET, .val = 0x01			},	// Reset
};

struct sy602x_priv {
	struct regmap *regmap;
	int mclk_div;
	struct snd_soc_component *component;
	unsigned int format;
	u8  dap_ram[DAP_RAM_ITEM_NUM][DAP_RAM_ITEM_SIZE];
	u32 dap_ram_update;
	struct mutex dap_ram_lock; /* device lock */
};

static struct sy602x_priv *sy602x_priv_data;

static void sy602x_write_dap_ram_commands(const struct sy602x_dap_ram_command *cmds,
	uint cnt)
{
	uint i;

	for (i = 0 ; i < cnt ; ++i) {
		if (cmds[i].item  < DAP_RAM_ITEM_NUM) {
			memcpy(sy602x_priv_data->dap_ram[cmds[i].item], cmds[i].data,
				sy602x_reg_size[cmds[i].data[0]] + 1);
			sy602x_priv_data->dap_ram_update |= ((u32)1 << cmds[i].item);
		}
	}
}

static int sy602x_write_reg_list(struct snd_soc_component *component,
	const struct sy602x_reg *reg_list, uint reg_cnt)
{
	int ret, i;

	if (!reg_list || reg_cnt == 0)
		return 0;

	for (i = 0; i < reg_cnt; ++i) {
		ret = snd_soc_component_write(component, reg_list[i].reg, reg_list[i].val);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int sy602x_dap_ram_update(struct snd_soc_component *component)
{
	struct sy602x_priv *sy602x;
	int ret, i, cnt;
	u32 update;

	sy602x = snd_soc_component_get_drvdata(component);

	update = sy602x->dap_ram_update;
	if (!update)
		return 0;

	ret = snd_soc_component_update_bits(component, SY602X_SYS_CTL_REG1,
		SY602X_I2C_ACCESS_RAM_MASK, SY602X_I2C_ACCESS_RAM_ENABLE);
	if (ret < 0)
		return ret;

	i = 0;
	while (update) {
		if (update & 0x01) {
			cnt = sy602x_reg_size[sy602x->dap_ram[i][0]] + 1;
			ret = i2c_master_send(i2c, &sy602x->dap_ram[i][0], cnt);
			if (ret < 0)
				return ret;
		}
		update >>= 1;
		++i;
	}

	ret = snd_soc_component_update_bits(component, SY602X_SYS_CTL_REG1,
		SY602X_I2C_ACCESS_RAM_MASK, SY602X_I2C_ACCESS_RAM_DISABLE);

	sy602x->dap_ram_update = 0;

	return ret;
}

static int sy602x_reg_switch_info(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;

	return 0;
}

static int sy602x_reg_switch_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sy602x_reg_switch *reg_switch = (struct sy602x_reg_switch *)kcontrol->private_value;

	int val;

	val = snd_soc_component_read(component, reg_switch->reg);

	ucontrol->value.integer.value[0] =
		((val & reg_switch->mask) == reg_switch->off_value) ? 0 : 1;

	return 0;
}

static int sy602x_reg_switch_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sy602x_reg_switch *reg_switch = (struct sy602x_reg_switch *)kcontrol->private_value;

	int ret;

	ret = snd_soc_component_update_bits(component, reg_switch->reg, reg_switch->mask,
			(ucontrol->value.integer.value[0]) ?
			reg_switch->on_value : reg_switch->off_value);

	return ret;
}

#define SY602X_REG_SWITCH(xname, xreg, xmask, xon_value, xoff_value) \
{ \
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE, \
	.info = sy602x_reg_switch_info, \
	.get = sy602x_reg_switch_get, .put = sy602x_reg_switch_put, \
	.private_value = (unsigned long)&(struct sy602x_reg_switch) \
	{.reg = xreg, .mask = xmask, .on_value = xon_value, .off_value = xoff_value } \
}

/*
 *    _   _    ___   _      ___         _           _
 *   /_\ | |  / __| /_\    / __|___ _ _| |_ _ _ ___| |___
 *  / _ \| |__\__ \/ _ \  | (__/ _ \ ' \  _| '_/ _ \ (_-<
 * /_/ \_\____|___/_/ \_\  \___\___/_||_\__|_| \___/_/__/
 *
 */

static const DECLARE_TLV_DB_SCALE(sy602x_vol_tlv_m, -12750, 50, 1);
static const DECLARE_TLV_DB_SCALE(sy602x_vol_tlv,          -7950, 50, 1);

static const struct snd_kcontrol_new sy602x_snd_controls[] = {
	SOC_SINGLE_TLV("Master Playback Volume",
		SY602X_MAS_VOL, 0, 255, 0, sy602x_vol_tlv_m),
	SOC_SINGLE_TLV("Ch1 Playback Volume",
		SY602X_CH1_VOL, 0, 255, 0, sy602x_vol_tlv),
	SOC_SINGLE_TLV("Ch2 Playback Volume",
		SY602X_CH2_VOL, 0, 255, 0, sy602x_vol_tlv),
	// switch
	SOC_SINGLE("Master Playback Switch", SY602X_SOFT_MUTE,
		SY602X_SOFT_MUTE_MASTER_SHIFT, 1, 1),
	SOC_SINGLE("Ch1 Playback Switch", SY602X_SOFT_MUTE,
		SY602X_SOFT_MUTE_CH1_SHIFT, 1, 1),
	SOC_SINGLE("Ch2 Playback Switch", SY602X_SOFT_MUTE,
		SY602X_SOFT_MUTE_CH2_SHIFT, 1, 1),
	SOC_SINGLE("EQ Switch", SY602X_SYS_CTL_REG2,
		SY602X_EQ_ENABLE_SHIFT, 1, 0),
	SY602X_REG_SWITCH("DRC Switch", SY602X_DRC_CTRL,
		SY602X_DRC_EN_MASK, SY602X_DRC_ENABLE, SY602X_DRC_DISABLE),
};

/*
 *  __  __         _    _            ___      _
 * |  \/  |__ _ __| |_ (_)_ _  ___  |   \ _ _(_)_ _____ _ _
 * | |\/| / _` / _| ' \| | ' \/ -_) | |) | '_| \ V / -_) '_|
 * |_|  |_\__,_\__|_||_|_|_||_\___| |___/|_| |_|\_/\___|_|
 *
 */

static int sy602x_set_dai_fmt(struct snd_soc_dai *dai, unsigned int format)
{
	struct sy602x_priv *sy602x;
	struct snd_soc_component *component = dai->component;

	sy602x = snd_soc_component_get_drvdata(component);

	sy602x->format = format;

	return 0;
}

static int sy602x_hw_params(struct snd_pcm_substream *substream,
			struct snd_pcm_hw_params *params,
			struct snd_soc_dai *dai)
{
	u32  val = 0x00;
	bool err =  false;

	struct sy602x_priv *sy602x;
	struct snd_soc_component *component = dai->component;

	sy602x = snd_soc_component_get_drvdata(component);
	sy602x_priv_data->component = component;

	switch (sy602x->format & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		val |= 0x00;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		val |= 0x20;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		val |= 0x40;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		val |= 0x60;
		break;
	default:
		err = true;
		break;
	}

	switch (sy602x->format & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		val |= 0x00;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		val |= 0x04;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		val |= 0x08;
		break;
	default:
		err = true;
		break;
	}

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		val |= 0x03;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		val |= 0x01;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		val   |= 0x00;
		break;
	default:
		err = true;
		break;
	}

	if (!err)
		val |= 0x10;

	snd_soc_component_write(component, SY602X_I2S_CONTROL, val);

	return 0;
}

static int sy602x_mute_stream(struct snd_soc_dai *dai, int mute, int stream)
{
	unsigned int val;

	struct sy602x_priv *sy602x;
	struct snd_soc_component *component = dai->component;

	sy602x = snd_soc_component_get_drvdata(component);

	val = (mute) ? SY602X_SOFT_MUTE_ALL : 0;

	return (snd_soc_component_update_bits(component,
			SY602X_SOFT_MUTE, SY602X_SOFT_MUTE_ALL, val));
}

static const struct snd_soc_dai_ops sy602x_dai_ops = {
	.set_fmt		= sy602x_set_dai_fmt,
	.hw_params		= sy602x_hw_params,
	.mute_stream	= sy602x_mute_stream,
};

static struct snd_soc_dai_driver sy602x_dai = {
	.name		= "sy602x-hifi",
	.playback	= {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates			= (SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100
				| SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000),
		.formats		= (SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE),
	},
	.ops	= &sy602x_dai_ops,
};

/*
 *   ___         _          ___      _
 *  / __|___  __| |___ __  |   \ _ _(_)_ _____ _ _
 * | (__/ _ \/ _` / -_) _| | |) | '_| \ V / -_) '_|
 *  \___\___/\__,_\___\__| |___/|_| |_|\_/\___|_|
 *
 */

static void sy602x_remove(struct snd_soc_component *component)
{
	struct sy602x_priv *sy602x;

	sy602x = snd_soc_component_get_drvdata(component);
}

static int sy602x_probe(struct snd_soc_component *component)
{
	struct sy602x_priv *sy602x;
	int ret;

	i2c = container_of(component->dev, struct i2c_client, dev);

	sy602x = snd_soc_component_get_drvdata(component);

	ret = sy602x_write_reg_list(component, sy602x_reg_init_reset,
			ARRAY_SIZE(sy602x_reg_init_reset));
	if (ret < 0)
		return ret;
	mdelay(5);

	ret = sy602x_write_reg_list(component, sy602x_reg_init_mute,
		ARRAY_SIZE(sy602x_reg_init_mute));
	if (ret < 0)
		return ret;
	mdelay(10);

	ret = sy602x_write_reg_list(component, sy602x_reg_init_seq_0,
		ARRAY_SIZE(sy602x_reg_init_seq_0));
	if (ret < 0)
		return ret;
	sy602x->dap_ram_update = 0;
	sy602x_write_dap_ram_commands(sy602x_dap_ram_init,
		ARRAY_SIZE(sy602x_dap_ram_init));
	sy602x_write_dap_ram_commands(sy602x_dap_ram_eq_1,
		ARRAY_SIZE(sy602x_dap_ram_eq_1));
	ret = sy602x_dap_ram_update(component);
	if (ret < 0)
		return ret;

	ret = sy602x_write_reg_list(component, sy602x_reg_eq_1,
			ARRAY_SIZE(sy602x_reg_eq_1));
	if (ret < 0)
		return ret;
	ret = sy602x_write_reg_list(component, sy602x_reg_speq_ctrl,
			ARRAY_SIZE(sy602x_reg_speq_ctrl));
	if (ret < 0)
		return ret;
	ret = sy602x_write_reg_list(component, sy602x_reg_init_seq_1,
			ARRAY_SIZE(sy602x_reg_init_seq_1));

	return ret;
}

static const struct snd_soc_component_driver soc_codec_dev_sy602x = {
	.probe = sy602x_probe,
	.remove = sy602x_remove,
	.controls = sy602x_snd_controls,
	.num_controls = ARRAY_SIZE(sy602x_snd_controls),
};

/*
 *   ___ ___ ___   ___      _
 *  |_ _|_  ) __| |   \ _ _(_)_ _____ _ _
 *   | | / / (__  | |) | '_| \ V / -_) '_|
 *  |___/___\___| |___/|_| |_|\_/\___|_|
 *
 */

static const struct reg_default sy602x_reg_defaults[] = {
	{ 0x07, 0x00 },  // R7  - VOL_MASTER    -   mute
	{ 0x08, 0x9F },  // R8  - VOL_CH1       -   0dB
	{ 0x09, 0x9F },  // R9  - VOL_CH2       -   0dB
};

static bool sy602x_reg_volatile(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case SY602X_CLK_CTL_REG:
	case SY602X_DEV_ID_REG:
	case SY602X_ERR_STATUS_REG:
	case SY602X_DC_SOFT_RESET:
		return true;
	default:
		return false;
	}
}

static const struct of_device_id sy602x_of_match[] = {
	{ .compatible = "silergy,sy602x", },
	{ }
};
MODULE_DEVICE_TABLE(of, sy602x_of_match);

static int sy602x_reg_read(void *context, unsigned int reg,
	unsigned int *value)
{
	int ret;
	uint size, i;
	u8 send_buf[4] = {0};
	u8 recv_buf[4] = {0};
	struct i2c_client *client = context;
	struct i2c_msg msgs[2];
	struct device *dev = &client->dev;

	if (reg > SY602X_MAX_REGISTER)
		return -EINVAL;

	size = sy602x_reg_size[reg];
	if (size == 0)
		return -EINVAL;
	if (size > 4)
		size = 4;

	send_buf[0]   = (u8)reg;

	msgs[0].addr  = client->addr;
	msgs[0].len   = 1;
	msgs[0].buf   = send_buf;
	msgs[0].flags = 0;

	msgs[1].addr  = client->addr;
	msgs[1].len   = size;
	msgs[1].buf   = recv_buf;
	msgs[1].flags = I2C_M_RD;

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0) {
		dev_err(dev, "Failed to register codec: %d\n", ret);
		return ret;
	} else if (ret != ARRAY_SIZE(msgs)) {
		dev_err(dev, "Failed to register codec: %d\n", ret);
		return -EIO;
	}

	*value = 0;
	for (i = 1 ; i <= size ; ++i)
		*value |= ((uint)recv_buf[i - 1] << ((size - i) << 3));

#ifdef SY602X_DBG_REG_DUMP
	dev_dbg(dev, "I2C read address %d => %08x\n", size, *value);
#endif

	return 0;
}

static int sy602x_reg_write(void *context, unsigned int reg,
			      unsigned int value)
{
	struct i2c_client *client = context;
	uint size, i;
	u8   buf[5];
	int  ret;
	struct device *dev = &client->dev;

	if (reg > SY602X_MAX_REGISTER)
		return -EINVAL;

	size = sy602x_reg_size[reg];
	if (size == 0)
		return -EINVAL;
	if (size > 4)
		size = 4;

#ifdef SY602X_DBG_REG_DUMP
	dev_dbg(dev, "I2C write address %d <= %08x\n", size, value);
#endif

	buf[0] = (u8)reg;
	for (i = 1 ; i <= size ; ++i)
		buf[i] = (u8)(value >> ((size - i) << 3));

	ret = i2c_master_send(client, buf, size + 1);
	if (ret == size + 1) {
		ret =  0;
	} else if (ret < 0) {
		dev_err(dev, "I2C write address failed\n");
	} else {
		dev_err(dev, "I2C write failed\n");
		ret =  -EIO;
	}

	return ret;
}

static struct regmap_config sy602x_regmap_config = {
	.reg_bits = 8,
	.val_bits = 32,

	.max_register = SY602X_MAX_REGISTER,
	.volatile_reg = sy602x_reg_volatile,

	.cache_type = REGCACHE_RBTREE,
	.reg_defaults = sy602x_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(sy602x_reg_defaults),

	.reg_read  = sy602x_reg_read,
	.reg_write = sy602x_reg_write,
};

static int sy602x_i2c_probe(struct i2c_client *i2c,
			const struct i2c_device_id *id)
{
	int ret;

	sy602x_priv_data = devm_kzalloc(&i2c->dev, sizeof(struct sy602x_priv), GFP_KERNEL);
	if (!sy602x_priv_data)
		return -ENOMEM;

	sy602x_priv_data->regmap = devm_regmap_init_i2c(i2c,
									&sy602x_regmap_config);

	if (IS_ERR(sy602x_priv_data->regmap)) {
		ret = PTR_ERR(sy602x_priv_data->regmap);
		return ret;
	}

	mutex_init(&sy602x_priv_data->dap_ram_lock);
	i2c_set_clientdata(i2c, sy602x_priv_data);

	ret = snd_soc_register_component(&i2c->dev,
				&soc_codec_dev_sy602x, &sy602x_dai, 1);

	return ret;
}

static int sy602x_i2c_remove(struct i2c_client *i2c)
{
	snd_soc_unregister_component(&i2c->dev);
	i2c_set_clientdata(i2c, NULL);

	kfree(sy602x_priv_data);

	return 0;
}

static const struct i2c_device_id sy602x_i2c_id[] = {
	{ "sy602x", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, sy602x_i2c_id);

static struct i2c_driver sy602x_i2c_driver = {
	.driver = {
		.name = "sy602x",
		.owner = THIS_MODULE,
		.of_match_table = sy602x_of_match,
	},
	.probe = sy602x_i2c_probe,
	.remove = sy602x_i2c_remove,
	.id_table = sy602x_i2c_id
};

static int __init sy602x_modinit(void)
{
	int ret = 0;

	ret = i2c_add_driver(&sy602x_i2c_driver);
	if (ret) {
		pr_err("%s Failed to register sy602x I2C driver: %d\n", __func__,
				ret);
	}

	return ret;
}
module_init(sy602x_modinit);

static void __exit sy602x_exit(void)
{
	i2c_del_driver(&sy602x_i2c_driver);
}
module_exit(sy602x_exit);

MODULE_AUTHOR("Xiaotian Su <xiaotian.su@silergycorp.com>");
MODULE_DESCRIPTION("ASoC driver for SY602X");
MODULE_LICENSE("GPL v2");
