// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

static int ddr_scrambler_init(void)
{
	struct device_node *np;
	struct resource res;
	void __iomem *vaddr;
	unsigned int val;

	np = of_find_compatible_node(NULL, NULL,
		"amlogic, ddr-scrambler-preserve");
	if (!np)
		return -ENODEV;

	if (of_address_to_resource(np, 0, &res))
		return -ENODEV;

	vaddr = ioremap(res.start, resource_size(&res));
	if (IS_ERR(vaddr))
		return PTR_ERR(vaddr);

	val = readl(vaddr);
	val = val | 0x1;
	writel(val, vaddr);

	iounmap(vaddr);
	pr_warn("%s preserve startup key\n", __func__);

	return 0;
}

static void ddr_scrambler_exit(void)
{
}

module_init(ddr_scrambler_init);
module_exit(ddr_scrambler_exit);
MODULE_LICENSE("GPL");
