/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __GPIO_MAIN_H_
#define __GPIO_MAIN_H_

#if IS_ENABLED(CONFIG_AMLOGIC_MESON_IRQ_GPIO)
int meson_gpio_irq_init(void);
void meson_gpio_irq_exit(void);
#else
static inline void meson_gpio_irq_init(void)
{
	return 0;
}

static inline int meson_gpio_irq_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_GPIOLIB)
int gpiolib_module_init(void);
void gpiolib_module_exit(void);
#else
static inline int gpiolib_module_init(void)
{
	return 0;
}

static inline void gpiolib_module_exit(void)
{
}
#endif

#endif /*__GPIO_MAIN_H_*/
