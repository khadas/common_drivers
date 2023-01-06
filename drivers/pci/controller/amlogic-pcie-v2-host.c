// SPDX-License-Identifier: GPL-2.0
/*
 * PCIe host controller driver for Amlogic MESON SoCs
 *
 * Copyright (c) 2018 Amlogic, inc.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/resource.h>
#include <linux/types.h>
#include <linux/phy/phy.h>
#include <linux/module.h>

#include "pcie-designware.h"

#define to_amlogic_pcie(x) dev_get_drvdata((x)->dev)

#define PCIE_CAP_MAX_PAYLOAD_SIZE(x)	((x) << 5)
#define PCIE_CAP_MAX_READ_REQ_SIZE(x)	((x) << 12)

/* PCIe specific config registers */
#define PCIE_CFG0			0x0
#define APP_LTSSM_ENABLE		BIT(7)

#define PCIE_CFG_STATUS12		0x30
#define IS_SMLH_LINK_UP(x)		(((x) >> 6) & 0x1)
#define IS_RDLH_LINK_UP(x)		(((x) >> 16) & 0x1)
#define IS_LTSSM_UP(x)			((((x) >> 10) & 0x1f) == 0x11)

#define PCIE_CFG_STATUS17		0x44
#define PM_CURRENT_STATE(x)		(((x) >> 7) & 0x1)

#define WAIT_LINKUP_TIMEOUT		9000
#define MAX_PAYLOAD_SIZE		256
#define MAX_READ_REQ_SIZE		256
#define PCIE_RESET_DELAY		500
#define PCIE_SHARED_RESET		1
#define PCIE_NORMAL_RESET		0

enum pcie_data_rate {
	PCIE_GEN1,
	PCIE_GEN2,
	PCIE_GEN3,
	PCIE_GEN4
};

enum pcie_phy_type {
	DW_PHY,
	M31_COMBPHY,
};

struct amlogic_pcie {
	struct dw_pcie pci;

	void __iomem *elbi_base;
	void __iomem *cfg_base;
	void __iomem *phy_base;
	void __iomem *reset_base;

	struct clk *pcie_clk;
	struct clk *phy_clk;
	struct clk *refpll_clk;
	struct clk *dev_clk;

	struct reset_control *ctrl_rst;
	struct reset_control *apb_rst;
	struct reset_control *phy_rst;

	u32 ctrl_rst_bit;
	u32 apb_rst_bit;
	u32 phy_rst_bit;

	struct phy *phy;

	int reset_gpio;
	u32 gpio_type;
	u32 phy_type;
};

static inline void amlogic_pcie_disable_clocks(struct amlogic_pcie *aml_pcie)
{
	clk_disable_unprepare(aml_pcie->pcie_clk);
	clk_disable_unprepare(aml_pcie->refpll_clk);
	clk_disable_unprepare(aml_pcie->phy_clk);
	clk_disable_unprepare(aml_pcie->dev_clk);
}

static int amlogic_pcie_enable_clocks(struct amlogic_pcie *aml_pcie)
{
	struct dw_pcie *pci = &aml_pcie->pci;
	struct device *dev = pci->dev;
	int ret = 0;

	ret = clk_prepare_enable(aml_pcie->dev_clk);
	if (ret) {
		dev_err(dev, "unable to enable dev_clk clock\n");
		return ret;
	}

	ret = clk_prepare_enable(aml_pcie->phy_clk);
	if (ret) {
		dev_err(dev, "unable to enable phy_clk clock\n");
		goto err_dev_clk;
	}

	ret = clk_prepare_enable(aml_pcie->refpll_clk);
	if (ret) {
		dev_err(dev, "unable to enable refpll_clk clock\n");
		goto err_phy_clk;
	}

	ret = clk_prepare_enable(aml_pcie->pcie_clk);
	if (ret) {
		dev_err(dev, "unable to enable pcie_clk clock\n");
		goto err_refpll_clk;
	}

	return 0;

err_refpll_clk:
	clk_disable_unprepare(aml_pcie->refpll_clk);
err_phy_clk:
	clk_disable_unprepare(aml_pcie->phy_clk);
err_dev_clk:
	clk_disable_unprepare(aml_pcie->dev_clk);

	return ret;
}

static int amlogic_pcie_get_clocks(struct amlogic_pcie *aml_pcie)
{
	struct dw_pcie *pci = &aml_pcie->pci;
	struct device *dev = pci->dev;

	aml_pcie->phy_clk = devm_clk_get(dev, "pcie_phy");
	if (IS_ERR(aml_pcie->phy_clk))
		return dev_err_probe(dev, PTR_ERR(aml_pcie->phy_clk),
				     "phy_clk clock source missing or invalid\n");

	aml_pcie->pcie_clk = devm_clk_get(dev, "pcie");
	if (IS_ERR(aml_pcie->pcie_clk))
		return dev_err_probe(dev, PTR_ERR(aml_pcie->pcie_clk),
				     "pcie_clk clock source missing or invalid\n");

	aml_pcie->refpll_clk = devm_clk_get(dev, "pcie_refpll");
	if (IS_ERR(aml_pcie->refpll_clk))
		return dev_err_probe(dev, PTR_ERR(aml_pcie->refpll_clk),
				     "refpll_clk clock source missing or invalid\n");

	aml_pcie->dev_clk = devm_clk_get(dev, "pcie_hcsl");
	if (IS_ERR(aml_pcie->dev_clk))
		return dev_err_probe(dev, PTR_ERR(aml_pcie->dev_clk),
				     "dev_clk clock source missing or invalid\n");
	return 0;
}

static int amlogic_pcie_get_mems(struct amlogic_pcie *aml_pcie)
{
	struct dw_pcie *pci = &aml_pcie->pci;
	struct device *dev = pci->dev;
	struct platform_device *pdev = to_platform_device(dev);
	struct resource *res;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "elbi");
	if (res) {
		aml_pcie->elbi_base = devm_ioremap_resource(dev, res);
		if (IS_ERR(aml_pcie->elbi_base)) {
			dev_err(dev, "failed to request elbi_base\n");
			return PTR_ERR(aml_pcie->elbi_base);
		}
		pci->dbi_base = aml_pcie->elbi_base;
	} else {
		dev_err(dev, "failed to get elbi_base resource\n");
		return -ENODEV;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cfg");
	if (res) {
		aml_pcie->cfg_base = devm_ioremap_resource(dev, res);
		if (IS_ERR(aml_pcie->cfg_base)) {
			dev_err(dev, "failed to request cfg_base\n");
			return PTR_ERR(aml_pcie->cfg_base);
		}
	} else {
		dev_err(dev, "failed to get cfg_base resource\n");
		return -ENODEV;
	}

	return 0;
}

static int amlogic_pcie_phy_power_on(struct amlogic_pcie *aml_pcie)
{
	struct device *dev = aml_pcie->pci.dev;
	int ret = 0;
	u32 val;

	if (IS_ERR(aml_pcie->phy))
		goto set_phy_reg;

	ret = phy_init(aml_pcie->phy);
	if (ret)
		return ret;

	ret = phy_power_on(aml_pcie->phy);
	if (ret) {
		phy_exit(aml_pcie->phy);
		return ret;
	}

	return 0;

set_phy_reg:
	switch (aml_pcie->phy_type) {
	case M31_COMBPHY:
		dev_dbg(dev, " pcie init port and M31_COMBPHY\n");
		val = readl(aml_pcie->phy_base);
		val &= ~(BIT(0) | BIT(22) | BIT(25));
		val |= BIT(17);
		writel(val, aml_pcie->phy_base);
		break;
	case DW_PHY:
	default:
		dev_dbg(dev, " pcie init port and DW_PHY\n");
		writel(0x1c, aml_pcie->phy_base);
		break;
	}

	return 0;
}

static void amlogic_pcie_phy_power_off(struct amlogic_pcie *aml_pcie)
{
	struct device *dev = aml_pcie->pci.dev;
	u32 val;

	if (IS_ERR(aml_pcie->phy))
		goto set_phy_reg;

	phy_power_off(aml_pcie->phy);
	phy_exit(aml_pcie->phy);

set_phy_reg:
	switch (aml_pcie->phy_type) {
	case M31_COMBPHY:
		dev_dbg(dev, " pcie deinit port and M31_COMBPHY\n");
		val = readl(aml_pcie->reset_base);
		val &= ~(1 << aml_pcie->phy_rst_bit);
		writel(val, aml_pcie->reset_base);
		break;
	case DW_PHY:
	default:
		dev_dbg(dev, " pcie deinit port and DW_PHY\n");
		writel(0x1d, aml_pcie->phy_base);
		break;
	}
}

static int amlogic_pcie_get_phy(struct amlogic_pcie *aml_pcie)
{
	struct dw_pcie *pci = &aml_pcie->pci;
	struct device *dev = pci->dev;
	struct platform_device *pdev = to_platform_device(dev);
	struct resource *res;

	aml_pcie->phy = devm_phy_get(dev, "pcie-phy");
	if (IS_ERR(aml_pcie->phy)) {
		if (PTR_ERR(aml_pcie->phy) != -EPROBE_DEFER)
			goto get_phy_reg;
	}

get_phy_reg:
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "phy");
	if (res) {
		aml_pcie->phy_base = devm_ioremap(dev, res->start, resource_size(res));
		if (IS_ERR(aml_pcie->phy_base)) {
			dev_err(dev, "failed to request phy_base\n");
			return PTR_ERR(aml_pcie->phy_base);
		}
	} else {
		dev_err(dev, "failed to get phy_base resource\n");
		return -ENODEV;
	}

	if (of_property_read_u32(dev->of_node, "phy-type", &aml_pcie->phy_type))
		aml_pcie->phy_type = DW_PHY;
	dev_dbg(dev, "PCIE phy type is %d\n", aml_pcie->phy_type);

	return 0;
}

static int amlogic_pcie_get_reset(struct amlogic_pcie *aml_pcie)
{
	struct dw_pcie *pci = &aml_pcie->pci;
	struct device *dev = pci->dev;
	struct device_node *node = dev->of_node;
	struct platform_device *pdev = to_platform_device(dev);
	struct resource *res;
	int ret = 0;

	aml_pcie->ctrl_rst = devm_reset_control_get_exclusive(dev, "ctrl_rst");
	if (IS_ERR(aml_pcie->ctrl_rst)) {
		if (PTR_ERR(aml_pcie->ctrl_rst) != -EPROBE_DEFER)
			goto get_rst_reg;
	}

	aml_pcie->apb_rst = devm_reset_control_get_exclusive(dev, "apb_rst");
	if (IS_ERR(aml_pcie->apb_rst)) {
		if (PTR_ERR(aml_pcie->apb_rst) != -EPROBE_DEFER)
			dev_err(dev, "apb_rst property in node\n");
		return PTR_ERR(aml_pcie->apb_rst);
	}

	aml_pcie->phy_rst = devm_reset_control_get_exclusive(dev, "phy_rst");
	if (IS_ERR(aml_pcie->phy_rst)) {
		if (PTR_ERR(aml_pcie->phy_rst) != -EPROBE_DEFER)
			dev_err(dev, "phy_rst property in node\n");
		return PTR_ERR(aml_pcie->phy_rst);
	}

	return 0;

get_rst_reg:
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "reset");
	if (res) {
		aml_pcie->reset_base = devm_ioremap(dev, res->start, resource_size(res));
		if (IS_ERR(aml_pcie->reset_base)) {
			dev_err(dev, "failed to request reset_base\n");
			return PTR_ERR(aml_pcie->reset_base);
		}
	} else {
		dev_err(dev, "failed to get reset_base resource\n");
		return -ENODEV;
	}

	ret = of_property_read_u32(node, "pcie-apb-rst-bit",
				   &aml_pcie->apb_rst_bit);
	if (ret) {
		dev_err(dev, "failed to request apb_rst_bit\n");
		return ret;
	}

	ret = of_property_read_u32(node, "pcie-phy-rst-bit",
				   &aml_pcie->phy_rst_bit);
	if (ret) {
		dev_err(dev, "failed to request phy_rst_bit\n");
		return ret;
	}

	ret = of_property_read_u32(node, "pcie-ctrl-rst-bit",
				   &aml_pcie->ctrl_rst_bit);
	if (ret) {
		dev_err(dev, "failed to request ctrl_rst_bit\n");
		return ret;
	}

	return 0;
}

static int amlogic_pcie_set_reset_gpio(struct amlogic_pcie *aml_pcie)
{
	struct dw_pcie *pci = &aml_pcie->pci;
	struct device *dev = pci->dev;
	int ret;

	/* reset-gpio-type 0:Shared pad(no reset) 1:OD pad 2:Normal pad */
	if (aml_pcie->gpio_type == 0) {
		dev_info(dev, "gpio multiplex, don't reset!\n");
	} else if (aml_pcie->gpio_type == 1) {
		dev_info(dev, "pad gpio\n");
		if (gpio_is_valid(aml_pcie->reset_gpio)) {
			dev_info(dev, "GPIO pad: assert reset\n");
			ret = gpio_direction_output(aml_pcie->reset_gpio, 0);
			if (ret)
				return ret;
			usleep_range(5000, 6000);
			ret = gpio_direction_input(aml_pcie->reset_gpio);
			if (ret)
				return ret;
		}
	} else {
		dev_dbg(dev, "normal gpio\n");
		if (gpio_is_valid(aml_pcie->reset_gpio)) {
			dev_info(dev, "GPIO normal: assert reset\n");
			usleep_range(1000, 2000);
			gpio_set_value_cansleep(aml_pcie->reset_gpio, 0);
			usleep_range(10000, 15000);
			gpio_set_value_cansleep(aml_pcie->reset_gpio, 1);
		}
	}

	return 0;
}

static int amlogic_pcie_get_reset_gpio(struct amlogic_pcie *aml_pcie)
{
	struct dw_pcie *pci = &aml_pcie->pci;
	struct device *dev = pci->dev;
	struct device_node *node = dev->of_node;
	int ret;

	ret = of_property_read_u32(node, "gpio-type",
				   &aml_pcie->gpio_type);
	if (ret) {
		dev_err(dev, "failed to request gpio-type\n");
		return ret;
	}

	aml_pcie->reset_gpio = of_get_named_gpio(node, "reset-gpio", 0);
	if (gpio_is_valid(aml_pcie->reset_gpio)) {
		ret = devm_gpio_request_one(dev, aml_pcie->reset_gpio,
					    GPIOF_OUT_INIT_HIGH,
					    "pcie_perst");
		if (ret) {
			dev_err(dev, "unable to get reset gpio\n");
			return ret;
		}
	} else {
		dev_err(dev, "failed to get reset-gpio\n");
		return -ENODEV;
	}
	return 0;
}

static int amlogic_pcie_assert_reset(struct amlogic_pcie *aml_pcie)
{
	struct dw_pcie *pci = &aml_pcie->pci;
	struct device *dev = pci->dev;
	int ret = 0, val = 0;

	if (aml_pcie->reset_base)
		goto set_rst_reg;

	ret = reset_control_assert(aml_pcie->phy_rst);
	if (ret < 0) {
		dev_err(dev, "assert phy_rst err %d\n", ret);
		return ret;
	}

	ret = reset_control_assert(aml_pcie->apb_rst);
	if (ret < 0) {
		dev_err(dev, "assert apb_rst err %d\n", ret);
		return ret;
	}

	ret = reset_control_assert(aml_pcie->ctrl_rst);
	if (ret < 0) {
		dev_err(dev, "assert ctrl_rst err %d\n", ret);
		return ret;
	}

set_rst_reg:

	val = readl(aml_pcie->reset_base);
	val &= ~((1 << aml_pcie->ctrl_rst_bit) | (1 << aml_pcie->phy_rst_bit) |
		 (1 << aml_pcie->apb_rst_bit));
	writel(val, aml_pcie->reset_base);

	return 0;
}

static int amlogic_pcie_deassert_reset(struct amlogic_pcie *aml_pcie)
{
	struct dw_pcie *pci = &aml_pcie->pci;
	struct device *dev = pci->dev;
	int ret = 0, val = 0;

	if (aml_pcie->reset_base)
		goto set_rst_reg;

	ret = reset_control_deassert(aml_pcie->phy_rst);
	if (ret < 0) {
		dev_err(dev, "deassert phy_rst err %d\n", ret);
		return ret;
	}

	ret = reset_control_deassert(aml_pcie->apb_rst);
	if (ret < 0) {
		dev_err(dev, "deassert apb_rst err %d\n", ret);
		return ret;
	}

	ret = reset_control_deassert(aml_pcie->ctrl_rst);
	if (ret < 0) {
		dev_err(dev, "deassert ctrl_rst err %d\n", ret);
		return ret;
	}

set_rst_reg:

	val = readl(aml_pcie->reset_base);
	val |= (1 << aml_pcie->ctrl_rst_bit) | (1 << aml_pcie->phy_rst_bit) |
		(1 << aml_pcie->apb_rst_bit);
	writel(val, aml_pcie->reset_base);

	return 0;
}

static int amlogic_pcie_parse_dt(struct amlogic_pcie *aml_pcie)
{
	int err;

	err = amlogic_pcie_get_clocks(aml_pcie);
	if (err)
		return err;

	err = amlogic_pcie_get_mems(aml_pcie);
	if (err)
		return err;

	err = amlogic_pcie_get_phy(aml_pcie);
	if (err)
		return err;

	err = amlogic_pcie_get_reset(aml_pcie);
	if (err)
		return err;

	err = amlogic_pcie_get_reset_gpio(aml_pcie);
	if (err)
		return err;

	return 0;
}

static inline u32 amlogic_cfg_readl(struct amlogic_pcie *aml_pcie, u32 reg)
{
	return readl(aml_pcie->cfg_base + reg);
}

static inline void amlogic_cfg_writel(struct amlogic_pcie *aml_pcie, u32 val, u32 reg)
{
	writel(val, aml_pcie->cfg_base + reg);
}

static void amlogic_pcie_ltssm_enable(struct amlogic_pcie *aml_pcie)
{
	u32 val;

	val = amlogic_cfg_readl(aml_pcie, PCIE_CFG0);
	val |= APP_LTSSM_ENABLE;
	amlogic_cfg_writel(aml_pcie, val, PCIE_CFG0);
}

static int amlogic_size_to_payload(struct amlogic_pcie *aml_pcie, int size)
{
	struct device *dev = aml_pcie->pci.dev;

	/*
	 * dwc supports 2^(val+7) payload size, which val is 0~5 default to 1.
	 * So if input size is not 2^order alignment or less than 2^7 or bigger
	 * than 2^12, just set to default size 2^(1+7).
	 */
	if (!is_power_of_2(size) || size < 128 || size > 4096) {
		dev_warn(dev, "payload size %d, set to default 256\n", size);
		return 1;
	}

	return fls(size) - 8;
}

static void amlogic_set_max_payload(struct amlogic_pcie *aml_pcie, int size)
{
	struct dw_pcie *pci = &aml_pcie->pci;
	u32 val;
	u16 offset = dw_pcie_find_capability(pci, PCI_CAP_ID_EXP);
	int max_payload_size = amlogic_size_to_payload(aml_pcie, size);

	val = dw_pcie_readl_dbi(pci, offset + PCI_EXP_DEVCTL);
	val &= ~PCI_EXP_DEVCTL_PAYLOAD;
	dw_pcie_writel_dbi(pci, offset + PCI_EXP_DEVCTL, val);

	val = dw_pcie_readl_dbi(pci, offset + PCI_EXP_DEVCTL);
	val |= PCIE_CAP_MAX_PAYLOAD_SIZE(max_payload_size);
	dw_pcie_writel_dbi(pci, offset + PCI_EXP_DEVCTL, val);
}

static void amlogic_set_max_rd_req_size(struct amlogic_pcie *aml_pcie, int size)
{
	struct dw_pcie *pci = &aml_pcie->pci;
	u32 val;
	u16 offset = dw_pcie_find_capability(pci, PCI_CAP_ID_EXP);
	int max_rd_req_size = amlogic_size_to_payload(aml_pcie, size);

	val = dw_pcie_readl_dbi(pci, offset + PCI_EXP_DEVCTL);
	val &= ~PCI_EXP_DEVCTL_READRQ;
	dw_pcie_writel_dbi(pci, offset + PCI_EXP_DEVCTL, val);

	val = dw_pcie_readl_dbi(pci, offset + PCI_EXP_DEVCTL);
	val |= PCIE_CAP_MAX_READ_REQ_SIZE(max_rd_req_size);
	dw_pcie_writel_dbi(pci, offset + PCI_EXP_DEVCTL, val);
}

static int amlogic_pcie_rd_own_conf(struct pci_bus *bus, u32 devfn,
				  int where, int size, u32 *val)
{
	int ret;

	ret = pci_generic_config_read(bus, devfn, where, size, val);
	if (ret != PCIBIOS_SUCCESSFUL)
		return ret;

	/*
	 * There is a bug in the MESON AXG PCIe controller whereby software
	 * cannot program the PCI_CLASS_DEVICE register, so we must fabricate
	 * the return value in the config accessors.
	 */
	if (where == PCI_CLASS_REVISION && size == 4)
		*val = (PCI_CLASS_BRIDGE_PCI << 16) | (*val & 0xffff);
	else if (where == PCI_CLASS_DEVICE && size == 2)
		*val = PCI_CLASS_BRIDGE_PCI;
	else if (where == PCI_CLASS_DEVICE && size == 1)
		*val = PCI_CLASS_BRIDGE_PCI & 0xff;
	else if (where == PCI_CLASS_DEVICE + 1 && size == 1)
		*val = (PCI_CLASS_BRIDGE_PCI >> 8) & 0xff;

	return PCIBIOS_SUCCESSFUL;
}

static struct pci_ops amlogic_pci_ops = {
	.map_bus = dw_pcie_own_conf_map_bus,
	.read = amlogic_pcie_rd_own_conf,
	.write = pci_generic_config_write,
};

static int amlogic_pcie_link_up(struct dw_pcie *pci)
{
	struct amlogic_pcie *aml_pcie = to_amlogic_pcie(pci);
	struct device *dev = pci->dev;
	u32 speed_okay = 0;
	u32 cnt = 0, val;
	u32 state12, state17, smlh_up = 0, ltssm_up = 0, rdlh_up = 0;

	val = readl(pci->dbi_base + PCIE_PORT_DEBUG1);
	if (((val & PCIE_PORT_DEBUG1_LINK_UP) &&
		(!(val & PCIE_PORT_DEBUG1_LINK_IN_TRAINING))))
		return 1;

	do {
		state12 = amlogic_cfg_readl(aml_pcie, PCIE_CFG_STATUS12);
		state17 = amlogic_cfg_readl(aml_pcie, PCIE_CFG_STATUS17);
		smlh_up = IS_SMLH_LINK_UP(state12);
		rdlh_up = IS_RDLH_LINK_UP(state12);
		ltssm_up = IS_LTSSM_UP(state12) ? 1 : 0;
		dev_dbg(dev, "ltssm_up = 0x%x\n", ((state12 >> 10) & 0x1f));

		if (PM_CURRENT_STATE(state17) < PCIE_GEN3)
			speed_okay = 1;

		if (smlh_up)
			dev_dbg(dev, "smlh_link_up is on\n");
		if (rdlh_up)
			dev_dbg(dev, "rdlh_link_up is on\n");
		if (ltssm_up)
			dev_dbg(dev, "ltssm_up is on\n");
		if (speed_okay)
			dev_dbg(dev, "speed_okay\n");

		if (cnt > WAIT_LINKUP_TIMEOUT)
			goto err_linkup;

		cnt++;

		udelay(20);
	} while (smlh_up == 0 || rdlh_up == 0 || ltssm_up == 0 || speed_okay == 0);

	return 1;

err_linkup:
	dev_dbg(dev, "PHY DEBUG_R0=0x%08x DEBUG_R1=0x%08x\n",
		dw_pcie_readl_dbi(pci, PCIE_PORT_DEBUG0),
		dw_pcie_readl_dbi(pci, PCIE_PORT_DEBUG1));
	return 0;
}

static int amlogic_pcie_start_link(struct dw_pcie *pci)
{
	struct amlogic_pcie *aml_pcie = to_amlogic_pcie(pci);

	amlogic_pcie_ltssm_enable(aml_pcie);

	return 0;
}

static int amlogic_pcie_host_init(struct pcie_port *pp)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct amlogic_pcie *aml_pcie = to_amlogic_pcie(pci);
	int ret;

	pp->bridge->ops = &amlogic_pci_ops;

	ret = amlogic_pcie_assert_reset(aml_pcie);
	if (ret)
		return ret;
	ret = amlogic_pcie_deassert_reset(aml_pcie);
	if (ret)
		return ret;
	ret = amlogic_pcie_phy_power_on(aml_pcie);
	if (ret)
		return ret;
	amlogic_set_max_payload(aml_pcie, MAX_PAYLOAD_SIZE);
	amlogic_set_max_rd_req_size(aml_pcie, MAX_READ_REQ_SIZE);

	amlogic_pcie_set_reset_gpio(aml_pcie);
	return 0;
}

static const struct dw_pcie_host_ops amlogic_pcie_host_ops = {
	.host_init = amlogic_pcie_host_init,
};

static const struct dw_pcie_ops dw_pcie_ops = {
	.start_link = amlogic_pcie_start_link,
	.link_up = amlogic_pcie_link_up,
};

static int amlogic_pcie_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dw_pcie *pci;
	struct amlogic_pcie *aml_pcie;
	u8 offset;
	int ret;
	u32 tmp;

	aml_pcie = devm_kzalloc(dev, sizeof(*aml_pcie), GFP_KERNEL);
	if (!aml_pcie)
		return -ENOMEM;

	pci = &aml_pcie->pci;
	pci->dev = dev;
	pci->ops = &dw_pcie_ops;
	pci->pp.ops = &amlogic_pcie_host_ops;

	ret = amlogic_pcie_parse_dt(aml_pcie);
	if (ret)
		return ret;

	ret = amlogic_pcie_enable_clocks(aml_pcie);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, aml_pcie);

	/* pcie dev memory protect check*/
	if (!of_property_read_bool(dev->of_node, "iommu-map"))
		dev_err(dev, "PCIe device memory protect failed, pls check dts node(iommu-map)\n");

	ret = dw_pcie_host_init(&pci->pp);
	if (ret < 0) {
		dev_err(dev, "Add PCIe port failed, %d\n", ret);
		goto err_disable_clk;
	}

	offset = dw_pcie_find_capability(pci, PCI_CAP_ID_EXP);
	tmp = dw_pcie_readw_dbi(pci, offset + PCI_EXP_LNKSTA);
	dev_info(dev, "Link up, GEN%i,link width is x%d\n", tmp & PCI_EXP_LNKSTA_CLS,
		(tmp & PCI_EXP_LNKSTA_NLW) >> PCI_EXP_LNKSTA_NLW_SHIFT);

	return 0;

err_disable_clk:
	amlogic_pcie_disable_clocks(aml_pcie);
	return ret;
}

#ifdef CONFIG_PM_SLEEP
static int amlogic_pcie_suspend_noirq(struct device *dev)
{
	struct amlogic_pcie *aml_pcie = dev_get_drvdata(dev);

	amlogic_pcie_disable_clocks(aml_pcie);
	amlogic_pcie_phy_power_off(aml_pcie);

	return 0;
}

static int amlogic_pcie_resume_noirq(struct device *dev)
{
	int ret;
	struct amlogic_pcie *aml_pcie = dev_get_drvdata(dev);
	struct pcie_port *pp = &aml_pcie->pci.pp;

	ret = amlogic_pcie_enable_clocks(aml_pcie);
	if (ret)
		return ret;

	ret = amlogic_pcie_host_init(pp);
	if (ret) {
		dev_err(dev, "resume host init failed.\n");
		goto err_resume_host_init;
	}

	dw_pcie_setup_rc(pp);

	ret = dw_pcie_wait_for_link(&aml_pcie->pci);
	if (ret < 0)
		goto err_resume_host_init;

	return 0;

err_resume_host_init:
	amlogic_pcie_disable_clocks(aml_pcie);
	amlogic_pcie_phy_power_off(aml_pcie);
	return ret;
}
#endif

static const struct dev_pm_ops aml_pcie_pm_ops = {
	SET_NOIRQ_SYSTEM_SLEEP_PM_OPS(amlogic_pcie_suspend_noirq,
				      amlogic_pcie_resume_noirq)
};

static int amlogic_pcie_remove(struct platform_device *pdev)
{
	struct amlogic_pcie *aml_pcie = platform_get_drvdata(pdev);
	struct pcie_port *pp = &aml_pcie->pci.pp;

	dw_pcie_host_deinit(pp);
	amlogic_pcie_disable_clocks(aml_pcie);
	amlogic_pcie_assert_reset(aml_pcie);
	amlogic_pcie_phy_power_off(aml_pcie);

	return 0;
}

static void amlogic_pcie_shutdown(struct platform_device *pdev)
{
	struct amlogic_pcie *aml_pcie = platform_get_drvdata(pdev);

	amlogic_pcie_disable_clocks(aml_pcie);
	amlogic_pcie_assert_reset(aml_pcie);
	amlogic_pcie_phy_power_off(aml_pcie);
}

static const struct of_device_id amlogic_pcie_of_match[] = {
	{
		.compatible = "amlogic, amlogic-pcie-v2",
	},
	{
		.compatible = "amlogic,amlogic-pcie-v2",
	},
	{}
};
MODULE_DEVICE_TABLE(of, amlogic_pcie_of_match);

static struct platform_driver amlogic_pcie_driver = {
	.probe = amlogic_pcie_probe,
	.remove = amlogic_pcie_remove,
	.driver = {
		.suppress_bind_attrs = true,
		.name = "amlogic-pcie-v2",
		.of_match_table = amlogic_pcie_of_match,
		.pm = &aml_pcie_pm_ops,
	},
	.shutdown = amlogic_pcie_shutdown,
};

module_platform_driver(amlogic_pcie_driver);

MODULE_AUTHOR("Amlogic Inc.");
MODULE_DESCRIPTION("Amlogic PCIe Controller driver");
MODULE_LICENSE("GPL v2");
