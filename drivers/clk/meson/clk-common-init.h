/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _CLK_MODULE_H__
#define _CLK_MODULE_H__

#ifdef MODULE

#ifdef CONFIG_AMLOGIC_MESON_CLK_MEASURE
int clk_measure_init(void);
#else
static inline int clk_measure_init(void)
{
	return 0;
}
#endif

#ifdef CONFIG_AMLOGIC_CLK_DEBUG
int __init clk_debug_init(void);
#else
static inline int clk_debug_init(void)
{
	return 0;
}
#endif

#endif /* end of ifdef MODULE */
#endif /* end of _CLK_MODULE_H__ */
