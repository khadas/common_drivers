/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _SPI_MODULE_H__
#define _SPI_MODULE_H__

#ifdef MODULE

#ifdef CONFIG_AMLOGIC_SPI_MESON_SPICC
extern struct platform_driver meson_spicc_driver;
#endif

#ifdef CONFIG_AMLOGIC_SPI_MESON_SPIFC
extern struct platform_driver meson_spifc_driver;
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_SPI_MESON_SPIFC_V2)
extern struct platform_driver meson_spifc_v2_driver;
#endif

#ifdef CONFIG_AMLOGIC_SPI_MESON_SPICC_SLAVE
extern struct platform_driver sspicc_driver;
int __init vmem_init(void);
#endif

#endif /* end of ifdef MODULE */
#endif /* end of _SPI_MODULE_H__ */
