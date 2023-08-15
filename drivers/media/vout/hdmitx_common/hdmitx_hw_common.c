// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_hw_common.h>

int hdmitx_hw_avmute(struct hdmitx_hw_common *tx_hw, int muteflag)
{
	tx_hw->cntlmisc(tx_hw, MISC_AVMUTE_OP, muteflag);

	return 0;
}

int hdmitx_hw_set_phy(struct hdmitx_hw_common *tx_hw, int flag)
{
	int cmd = TMDS_PHY_ENABLE;

	if (flag == 0)
		cmd = TMDS_PHY_DISABLE;
	else
		cmd = TMDS_PHY_ENABLE;
	tx_hw->cntlmisc(tx_hw, MISC_TMDS_PHY_OP, cmd);

	return 0;
}
