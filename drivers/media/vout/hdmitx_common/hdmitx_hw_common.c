// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_hw_common.h>

int hdmitx_hw_cntl_config(struct hdmitx_hw_common *tx_hw,
	u32 cmd, u32 arg)
{
	return tx_hw->cntlconfig(tx_hw, cmd, arg);
}

int hdmitx_hw_cntl_misc(struct hdmitx_hw_common *tx_hw,
	u32 cmd, u32 arg)
{
	return tx_hw->cntlmisc(tx_hw, cmd, arg);
}

int hdmitx_hw_cntl_ddc(struct hdmitx_hw_common *tx_hw,
	unsigned int cmd, unsigned long arg)
{
	return tx_hw->cntlddc(tx_hw, cmd, arg);
}

int hdmitx_hw_cntl(struct hdmitx_hw_common *tx_hw,
	unsigned int cmd, unsigned long arg)
{
	return tx_hw->cntl(tx_hw, cmd, arg);
}

int hdmitx_hw_get_state(struct hdmitx_hw_common *tx_hw,
	u32 cmd, u32 arg)
{
	return tx_hw->getstate(tx_hw, cmd, arg);
}

int hdmitx_hw_validate_mode(struct hdmitx_hw_common *tx_hw, u32 vic)
{
	return tx_hw->validatemode(tx_hw, vic);
}

/* calculate clk_ratio/tmds_scramble/frl/dsc */
int hdmitx_hw_calc_format_para(struct hdmitx_hw_common *tx_hw,
	struct hdmi_format_para *para)
{
	int ret = -1;

	if (tx_hw->calc_format_para)
		ret = tx_hw->calc_format_para(tx_hw, para);
	return ret;
}

int hdmitx_hw_set_packet(struct hdmitx_hw_common *tx_hw,
	int type, unsigned char *DB, unsigned char *HB)
{
	if (tx_hw->setpacket)
		tx_hw->setpacket(type, DB, HB);
	return 0;
}

int hdmitx_hw_disable_packet(struct hdmitx_hw_common *tx_hw,
	int type)
{
	if (tx_hw->disablepacket)
		tx_hw->disablepacket(type);
	return 0;
}

int hdmitx_hw_set_phy(struct hdmitx_hw_common *tx_hw, int flag)
{
	int cmd = TMDS_PHY_ENABLE;

	if (flag == 0)
		cmd = TMDS_PHY_DISABLE;
	else
		cmd = TMDS_PHY_ENABLE;
	return hdmitx_hw_cntl_misc(tx_hw, MISC_TMDS_PHY_OP, cmd);
}
EXPORT_SYMBOL(hdmitx_hw_set_phy);

enum hdmi_tf_type hdmitx_hw_get_hdr_st(struct hdmitx_hw_common *tx_hw)
{
	return hdmitx_hw_get_state(tx_hw, STAT_TX_HDR, 0);
}

enum hdmi_tf_type hdmitx_hw_get_dv_st(struct hdmitx_hw_common *tx_hw)
{
	return hdmitx_hw_get_state(tx_hw, STAT_TX_DV, 0);
}

enum hdmi_tf_type hdmitx_hw_get_hdr10p_st(struct hdmitx_hw_common *tx_hw)
{
	return hdmitx_hw_get_state(tx_hw, STAT_TX_HDR10P, 0);
}

