/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __INPUT_MAIN_H_
#define __INPUT_MAIN_H_

#if IS_ENABLED(CONFIG_AMLOGIC_GPIO_KEY)
int meson_gpio_kp_init(void);
void meson_gpio_kp_exit(void);
#else
static inline int meson_gpio_kp_init(void)
{
	return 0;
}

static inline void meson_gpio_kp_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_ADC_KEYPADS)
int meson_adc_kp_init(void);
void meson_adc_kp_exit(void);
#else
static inline int meson_adc_kp_init(void)
{
	return 0;
}

static inline void meson_adc_kp_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_MESON_IR)
int meson_ir_driver_init(void);
void meson_ir_driver_exit(void);
#else
static inline int meson_ir_driver_init(void)
{
	return 0;
}

static inline void meson_ir_driver_exit(void)
{
}
#endif

#endif /*_INPUT_MAIN_H__*/
