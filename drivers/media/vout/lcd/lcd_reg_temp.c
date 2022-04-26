// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/amlogic/iomap.h>
#include "lcd_smart_temp.h"
#include "lcd_reg_temp.h"

#define LCD_MAP_PERIPHS     0
#define LCD_MAP_VCBUS       1
#define LCD_MAP_ANA         2
#define LCD_MAP_CLK         3
#define LCD_MAP_DSI_HOST    4
#define LCD_MAP_DSI_PHY     5
#define LCD_MAP_RST         6
#define LCD_MAP_MAX         7

static unsigned int lcd_reg_c3[] = {
	LCD_MAP_ANA,
	LCD_MAP_CLK,
	LCD_MAP_VCBUS,
	LCD_MAP_DSI_HOST,
	LCD_MAP_DSI_PHY,
	LCD_MAP_PERIPHS,
	LCD_MAP_MAX
};

/* for lcd reg access */
static spinlock_t lcd_reg_spinlock;
static struct lcd_reg_map_s *lcd_reg_map;

int lcd_ioremap(struct platform_device *pdev)
{
	struct lcd_reg_map_s *reg_map;
	struct resource *res;
	unsigned int *table;
	int i = 0, ret;

	spin_lock_init(&lcd_reg_spinlock);
	table = lcd_reg_c3;

	lcd_reg_map = kcalloc(LCD_MAP_MAX, sizeof(struct lcd_reg_map_s), GFP_KERNEL);
	if (!lcd_reg_map)
		return -1;

	while (i < LCD_MAP_MAX) {
		if (table[i] == LCD_MAP_MAX)
			break;

		reg_map = &lcd_reg_map[table[i]];
		res = &reg_map->res;
		ret = of_address_to_resource(pdev->dev.of_node, i, res);
		if (ret) {
			LCDERR("%s: get ioresource error\n", __func__);
			goto lcd_ioremap_err;
		}
		//res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		//if (!res) {
		//	LCDERR("%s: get ioresource error\n", __func__);
		//	goto lcd_ioremap_err;
		//}
		reg_map->base_addr = res->start;
		reg_map->size = resource_size(res);
		//reg_map->p = devm_ioremap_resource(&pdev->dev, res);
		reg_map->p = ioremap(reg_map->base_addr, reg_map->size);
		if (!reg_map->p) {
			reg_map->flag = 0;
			LCDERR("%s: reg %d failed: 0x%x 0x%x\n",
			       __func__, i, reg_map->base_addr, reg_map->size);
			goto lcd_ioremap_err;
		}
		reg_map->flag = 1;
		LCDPR("%s: reg %d: 0x%x -> %px, size: 0x%x\n",
			__func__, i, reg_map->base_addr, reg_map->p, reg_map->size);

		i++;
	}

	return 0;

lcd_ioremap_err:
	return -1;
}

static int check_lcd_ioremap(unsigned int n)
{
	if (!lcd_reg_map) {
		LCDERR("%s: reg_map is null\n", __func__);
		return -1;
	}
	if (n >= LCD_MAP_MAX)
		return -1;
	if (lcd_reg_map[n].flag == 0) {
		LCDERR("%s: reg 0x%x mapped error\n",
			__func__, lcd_reg_map[n].base_addr);
		return -1;
	}
	return 0;
}

static inline void __iomem *check_lcd_ana_reg(unsigned int reg)
{
	void __iomem *p;
	int reg_bus;
	unsigned int reg_offset;

	reg_bus = LCD_MAP_ANA;
	if (check_lcd_ioremap(reg_bus))
		return NULL;

	reg_offset = LCD_REG_OFFSET(reg);
	if (reg_offset >= lcd_reg_map[reg_bus].size) {
		LCDERR("invalid lcd_ana reg offset: 0x%04x\n", reg);
		return NULL;
	}
	p = lcd_reg_map[reg_bus].p + reg_offset;
	return p;
}

static inline void __iomem *check_lcd_clk_reg(unsigned int reg)
{
	void __iomem *p;
	int reg_bus;
	unsigned int reg_offset;

	reg_bus = LCD_MAP_CLK;
	if (check_lcd_ioremap(reg_bus))
		return NULL;

	reg_offset = LCD_REG_OFFSET(reg);
	if (reg_offset >= lcd_reg_map[reg_bus].size) {
		LCDERR("invalid lcd_clk reg offset: 0x%04x\n", reg);
		return NULL;
	}
	p = lcd_reg_map[reg_bus].p + reg_offset;
	return p;
}

static inline void __iomem *check_lcd_vcbus_reg(unsigned int reg)
{
	void __iomem *p;
	int reg_bus;
	unsigned int reg_offset;

	reg_bus = LCD_MAP_VCBUS;
	if (check_lcd_ioremap(reg_bus))
		return NULL;

	reg_offset = LCD_REG_OFFSET(reg);
	if (reg_offset >= lcd_reg_map[reg_bus].size) {
		LCDERR("invalid lcd_vcbus reg offset: 0x%04x\n", reg);
		return NULL;
	}
	p = lcd_reg_map[reg_bus].p + reg_offset;
	return p;
}

static inline void __iomem *check_lcd_periphs_reg(unsigned int reg)
{
	void __iomem *p;
	int reg_bus;
	unsigned int reg_offset;

	reg_bus = LCD_MAP_PERIPHS;
	if (check_lcd_ioremap(reg_bus))
		return NULL;

	reg_offset = LCD_REG_OFFSET(reg);

	if (reg_offset >= lcd_reg_map[reg_bus].size) {
		LCDERR("invalid periphs reg offset: 0x%04x\n", reg);
		return NULL;
	}
	p = lcd_reg_map[reg_bus].p + reg_offset;
	return p;
}

static inline void __iomem *check_lcd_dsi_host_reg(unsigned int reg)
{
	void __iomem *p;
	int reg_bus;
	unsigned int reg_offset;

	reg_bus = LCD_MAP_DSI_HOST;
	if (check_lcd_ioremap(reg_bus))
		return NULL;

	reg_offset = LCD_REG_OFFSET(reg);
	if (reg_offset >= lcd_reg_map[reg_bus].size) {
		LCDERR("invalid dsi_host reg offset: 0x%04x\n", reg);
		return NULL;
	}
	p = lcd_reg_map[reg_bus].p + reg_offset;
	return p;
}

static inline void __iomem *check_lcd_dsi_phy_reg(unsigned int reg)
{
	void __iomem *p;
	int reg_bus;
	unsigned int reg_offset;

	reg_bus = LCD_MAP_DSI_PHY;
	if (check_lcd_ioremap(reg_bus))
		return NULL;

	reg_offset = LCD_REG_OFFSET(reg);
	if (reg_offset >= lcd_reg_map[reg_bus].size) {
		LCDERR("invalid dsi_phy reg offset: 0x%04x\n", reg);
		return NULL;
	}
	p = lcd_reg_map[reg_bus].p + reg_offset;
	return p;
}

static inline void __iomem *check_lcd_reset_reg(unsigned int reg)
{
	void __iomem *p;
	int reg_bus;
	unsigned int reg_offset;

	reg_bus = LCD_MAP_RST;
	if (check_lcd_ioremap(reg_bus))
		return NULL;

	reg_offset = LCD_REG_OFFSET(reg);

	if (reg_offset >= lcd_reg_map[reg_bus].size) {
		LCDERR("invalid reset reg offset: 0x%04x\n", reg);
		return NULL;
	}
	p = lcd_reg_map[reg_bus].p + reg_offset;
	return p;
}

/******************************************************/
unsigned int lcd_vcbus_read(unsigned int reg)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_vcbus_reg(reg);
	if (p)
		temp = readl(p);
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
	return temp;
};

void lcd_vcbus_write(unsigned int reg, unsigned int value)
{
	void __iomem *p;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_vcbus_reg(reg);
	if (p)
		writel(value, p);
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
};

void lcd_vcbus_setb(unsigned int reg, unsigned int value,
		    unsigned int start, unsigned int len)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_vcbus_reg(reg);
	if (p) {
		temp = readl(p);
		temp = (temp & (~(((1L << len) - 1) << start))) |
			((value & ((1L << len) - 1)) << start);
		writel(temp, p);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
}

unsigned int lcd_vcbus_getb(unsigned int reg,
			    unsigned int start, unsigned int len)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_vcbus_reg(reg);
	if (p) {
		temp = readl(p);
		temp = (temp >> start) & ((1L << len) - 1);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);

	return temp;
}

void lcd_vcbus_set_mask(unsigned int reg, unsigned int mask)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_vcbus_reg(reg);
	if (p) {
		temp = readl(p);
		temp |= (mask);
		writel(temp, p);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
}

void lcd_vcbus_clr_mask(unsigned int reg, unsigned int mask)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_vcbus_reg(reg);
	if (p) {
		temp = readl(p);
		temp &= ~(mask);
		writel(temp, p);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
}

unsigned int lcd_clk_read(unsigned int reg)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_clk_reg(reg);
	if (p)
		temp = readl(p);
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);

	return temp;
};

void lcd_clk_write(unsigned int reg, unsigned int val)
{
	void __iomem *p;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_clk_reg(reg);
	if (p)
		writel(val, p);
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
};

void lcd_clk_setb(unsigned int reg, unsigned int val,
		  unsigned int start, unsigned int len)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_clk_reg(reg);
	if (p) {
		temp = readl(p);
		temp = (temp & (~(((1L << len) - 1) << start))) |
			((val & ((1L << len) - 1)) << start);
		writel(temp, p);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
}

unsigned int lcd_clk_getb(unsigned int reg,
			  unsigned int start, unsigned int len)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_clk_reg(reg);
	if (p) {
		temp = readl(p);
		temp = (temp >> start) & ((1L << len) - 1);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);

	return temp;
}

void lcd_clk_set_mask(unsigned int reg, unsigned int mask)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_clk_reg(reg);
	if (p) {
		temp = readl(p);
		temp |= (mask);
		writel(temp, p);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
}

void lcd_clk_clr_mask(unsigned int reg, unsigned int mask)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_clk_reg(reg);
	if (p) {
		temp = readl(p);
		temp &= ~(mask);
		writel(temp, p);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
}

unsigned int lcd_ana_read(unsigned int reg)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_ana_reg(reg);
	if (p)
		temp = readl(p);
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);

	return temp;
}

void lcd_ana_write(unsigned int reg, unsigned int val)
{
	void __iomem *p;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_ana_reg(reg);
	if (p)
		writel(val, p);
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
}

void lcd_ana_setb(unsigned int reg, unsigned int val,
		  unsigned int start, unsigned int len)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_ana_reg(reg);
	if (p) {
		temp = readl(p);
		temp = (temp & (~(((1L << len) - 1) << start))) |
			((val & ((1L << len) - 1)) << start);
		writel(temp, p);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
}

unsigned int lcd_ana_getb(unsigned int reg,
			  unsigned int start, unsigned int len)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_ana_reg(reg);
	if (p) {
		temp = readl(p);
		temp = (temp >> start) & ((1L << len) - 1);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);

	return temp;
}

unsigned int lcd_cbus_read(unsigned int reg)
{
	unsigned int temp;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	temp = aml_read_cbus(reg);
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);

	return temp;
};

void lcd_cbus_write(unsigned int reg, unsigned int val)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	aml_write_cbus(reg, val);
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
};

void lcd_cbus_setb(unsigned int reg, unsigned int val,
		   unsigned int start, unsigned int len)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	aml_write_cbus(reg, ((aml_read_cbus(reg) &
		       ~(((1L << (len)) - 1) << (start))) |
		       (((val) & ((1L << (len)) - 1)) << (start))));
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
}

unsigned int lcd_periphs_read(unsigned int reg)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_periphs_reg(reg);
	if (p)
		temp = readl(p);
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);

	return temp;
};

void lcd_periphs_write(unsigned int reg, unsigned int val)
{
	void __iomem *p;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_periphs_reg(reg);
	if (p)
		writel(val, p);
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
};

unsigned int dsi_host_read(unsigned int reg)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_dsi_host_reg(reg);
	if (p)
		temp = readl(p);
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);

	return temp;
};

void dsi_host_write(unsigned int reg, unsigned int val)
{
	void __iomem *p;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_dsi_host_reg(reg);
	if (p)
		writel(val, p);
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
};

void dsi_host_setb(unsigned int reg, unsigned int value,
		   unsigned int start, unsigned int len)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_dsi_host_reg(reg);
	if (p) {
		temp = readl(p);
		temp = (temp & (~(((1L << len) - 1) << start))) |
			((value & ((1L << len) - 1)) << start);
		writel(temp, p);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
}

unsigned int dsi_host_getb(unsigned int reg,
			   unsigned int start, unsigned int len)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_dsi_host_reg(reg);
	if (p) {
		temp = readl(p);
		temp = (temp >> start) & ((1L << len) - 1);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);

	return temp;
}

void dsi_host_set_mask(unsigned int reg, unsigned int mask)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_dsi_host_reg(reg);
	if (p) {
		temp = readl(p);
		temp |= (mask);
		writel(temp, p);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
}

void dsi_host_clr_mask(unsigned int reg, unsigned int mask)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_dsi_host_reg(reg);
	if (p) {
		temp = readl(p);
		temp &= ~(mask);
		writel(temp, p);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
}

unsigned int dsi_phy_read(unsigned int reg)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_dsi_phy_reg(reg);
	if (p)
		temp = readl(p);
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);

	return temp;
};

void dsi_phy_write(unsigned int reg, unsigned int val)
{
	void __iomem *p;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_dsi_phy_reg(reg);
	if (p)
		writel(val, p);
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
};

void dsi_phy_setb(unsigned int reg, unsigned int value,
		  unsigned int start, unsigned int len)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_dsi_phy_reg(reg);
	if (p) {
		temp = readl(p);
		temp = (temp & (~(((1L << len) - 1) << start))) |
			((value & ((1L << len) - 1)) << start);
		writel(temp, p);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
}

unsigned int dsi_phy_getb(unsigned int reg,
			  unsigned int start, unsigned int len)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_dsi_phy_reg(reg);
	if (p) {
		temp = readl(p);
		temp = (temp >> start) & ((1L << len) - 1);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);

	return temp;
}

void dsi_phy_set_mask(unsigned int reg, unsigned int mask)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_dsi_phy_reg(reg);
	if (p) {
		temp = readl(p);
		temp |= (mask);
		writel(temp, p);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
}

void dsi_phy_clr_mask(unsigned int reg, unsigned int mask)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_dsi_phy_reg(reg);
	if (p) {
		temp = readl(p);
		temp &= ~(mask);
		writel(temp, p);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
}

unsigned int lcd_reset_read(unsigned int reg)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_reset_reg(reg);
	if (p)
		temp = readl(p);
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);

	return temp;
};

void lcd_reset_write(unsigned int reg, unsigned int val)
{
	void __iomem *p;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_reset_reg(reg);
	if (p)
		writel(val, p);
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
};

void lcd_reset_setb(unsigned int reg, unsigned int value,
		    unsigned int start, unsigned int len)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_reset_reg(reg);
	if (p) {
		temp = readl(p);
		temp = (temp & (~(((1L << len) - 1) << start))) |
			((value & ((1L << len) - 1)) << start);
		writel(temp, p);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
}

unsigned int lcd_reset_getb(unsigned int reg,
			    unsigned int start, unsigned int len)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_reset_reg(reg);
	if (p) {
		temp = readl(p);
		temp = (temp >> start) & ((1L << len) - 1);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);

	return temp;
}

void lcd_reset_set_mask(unsigned int reg, unsigned int mask)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_reset_reg(reg);
	if (p) {
		temp = readl(p);
		temp |= (mask);
		writel(temp, p);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
}

void lcd_reset_clr_mask(unsigned int reg, unsigned int mask)
{
	void __iomem *p;
	unsigned int temp = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&lcd_reg_spinlock, flags);
	p = check_lcd_reset_reg(reg);
	if (p) {
		temp = readl(p);
		temp &= ~(mask);
		writel(temp, p);
	}
	spin_unlock_irqrestore(&lcd_reg_spinlock, flags);
}
