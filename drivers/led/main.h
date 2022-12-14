/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __LED_MAIN_H_
#define __LED_MAIN_H_

#if IS_ENABLED(CONFIG_AMLOGIC_LEDS_STATE)
int led_state_init(void);
void led_state_exit(void);
#else
static inline void led_state_init(void)
{
	return 0;
}

static inline int led_state_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_LEDS_AW9523B)
int led_aw9523_init(void);
void led_aw9523_exit(void);
#else
static inline int led_aw9523_init(void)
{
	return 0;
}

static inline void led_aw9523_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_LEDS_TLC59116)
int led_tlc59116_init(void);
void led_tlc59116_exit(void);
#else
static inline int led_tlc59116_init(void)
{
	return 0;
}

static inline void led_tlc59116_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_LEDS_DCON)
void led_unipolar_ctrl_exit(void);
int led_unipolar_ctrl_init(void);
#else
static inline int led_unipolar_ctrl_init(void)
{
	return 0;
}

static inline void led_unipolar_ctrl_exit(void)
{
}
#endif

#endif /*__LED_MAIN_H_*/
