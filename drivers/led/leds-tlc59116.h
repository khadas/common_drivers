/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _LEDS_TLC59116_H_
#define _LEDS_TLC59116_H_

#define REGISTER_OFFSET 2

struct meson_tlc59116_io {
	unsigned int r_io;
	unsigned int g_io;
	unsigned int b_io;
};

struct meson_tlc59116_colors {
	unsigned int red;
	unsigned int green;
	unsigned int blue;
};

struct meson_tlc59116 {
	struct device *dev;
	struct i2c_client *i2c;
	struct led_classdev cdev;
	struct meson_tlc59116_io *io;
	struct meson_tlc59116_colors *colors;
	unsigned int *colors_buf;
	int ignore_led_suspend;
	int reset_gpio;
	int led_counts;
};

enum led_num {
	LED_NUM0 = 0,
	LED_NUM1,
	LED_NUM2,
	LED_NUM3,
};
#endif

