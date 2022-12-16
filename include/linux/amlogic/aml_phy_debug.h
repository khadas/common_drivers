/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _STMMAC_MESON_PHY_DEBUG_H
#define _STMMAC_MESON_PHY_DEBUG_H

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/skbuff.h>
#include <linux/ethtool.h>
#include <linux/if_ether.h>
#include <linux/crc32.h>
#include <linux/mii.h>
#include <linux/if.h>
#include <linux/if_vlan.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/prefetch.h>
#include <linux/pinctrl/consumer.h>
#include <linux/net_tstamp.h>
#include <linux/phylink.h>

//#include "eth_reg.h"
/*add this to stop checking wol,which will reset phy*/
//extern unsigned int enable_wol_check;
//extern unsigned int tx_amp_bl2;
//extern unsigned int enet_type;
//extern void __iomem *ioaddr_dbg;
//#ifdef CONFIG_AMLOGIC_ETH_PRIVE

/**
 * struct phylink - internal data type for phylink
 */
struct phylink {
	/* private: */
	struct net_device *netdev;
	const struct phylink_mac_ops *mac_ops;
	const struct phylink_pcs_ops *pcs_ops;
	struct phylink_config *config;
	struct phylink_pcs *pcs;
	struct device *dev;
	unsigned int old_link_state:1;

	unsigned long phylink_disable_state; /* bitmask of disables */
	struct phy_device *phydev;
	phy_interface_t link_interface;	/* PHY_INTERFACE_xxx */
	u8 cfg_link_an_mode;		/* MLO_AN_xxx */
	u8 cur_link_an_mode;
	u8 link_port;			/* The current non-phy ethtool port */
	__ETHTOOL_DECLARE_LINK_MODE_MASK(supported);

	/* The link configuration settings */
	struct phylink_link_state link_config;

	/* The current settings */
	phy_interface_t cur_interface;

	struct gpio_desc *link_gpio;
	unsigned int link_irq;
	struct timer_list link_poll;
	void (*get_fixed_state)(struct net_device *dev,
				struct phylink_link_state *s);
	/*add for git commit check*/
	struct mutex state_mutex;
	struct phylink_link_state phy_state;
	struct work_struct resolve;

	bool mac_link_dropped;

	struct sfp_bus *sfp_bus;
	bool sfp_may_have_phy;
	__ETHTOOL_DECLARE_LINK_MODE_MASK(sfp_support);
	u8 sfp_port;
};

#ifdef CONFIG_PM_SLEEP
extern unsigned int wol_switch_from_user;
#endif
extern unsigned int tx_amp_bl2;
//#endif
int gmac_create_sysfs(struct phy_device *phydev, void __iomem *ioaddr);
int gmac_remove_sysfs(struct phy_device *phydev);
#endif
