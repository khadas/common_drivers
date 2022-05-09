// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifdef MODULE

#include <linux/platform_device.h>
#include <linux/module.h>
#include "module-init.h"

static int __init spi_module_init(void)
{
	int ret = 0;

#ifdef CONFIG_AMLOGIC_SPI_MESON_SPICC
	ret = platform_driver_register(&meson_spicc_driver);
	if (ret)
		pr_err("insmod meson_spicc_driver fail\n");
#endif

#ifdef CONFIG_AMLOGIC_SPI_MESON_SPIFC
	ret = platform_driver_register(&meson_spifc_driver);
	if (ret)
		pr_err("insmod meson_spifc_driver fail\n");
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_SPI_MESON_SPIFC_V2)
	ret = platform_driver_register(&meson_spifc_v2_driver);
	if (ret)
		pr_err("insmod meson_spifc_v2_driver fail\n");
#endif

#ifdef CONFIG_AMLOGIC_SPI_MESON_SPICC_SLAVE
	ret = platform_driver_register(&sspicc_driver);
	if (ret)
		pr_err("insmod sspicc_driver fail\n");
	vmem_init();
#endif

	return ret;
}

static void __exit spi_module_exit(void)
{
}

module_init(spi_module_init);
module_exit(spi_module_exit);

MODULE_LICENSE("GPL v2");
#endif
