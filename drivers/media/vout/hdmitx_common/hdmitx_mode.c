// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/math64.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_mode.h>

/**
 * sync function drm_mode_vrefresh()
 */
int hdmi_timing_vrefresh(const struct hdmi_timing *t)
{
	unsigned int num, den;

	if (t->h_total == 0 || t->v_total == 0)
		return 0;

	num = t->pixel_freq;
	den = t->h_total * t->v_total;

	/*interlace mode*/
	if (t->pi_mode == 0)
		num *= 2;

	return DIV_ROUND_CLOSEST_ULL(mul_u32_u32(num, 1000), den);
}

bool hdmitx_mode_have_alternate_clock(const struct hdmi_timing *t)
{
	/*to be confirm if VESA can support frac rate.*/
	if (t->vic == HDMI_0_UNKNOWN || t->vic >= HDMI_CEA_VIC_END)
		return false;

	if (hdmi_timing_vrefresh(t) % 6 != 0)
		return false;

	return true;
}

int hdmitx_mode_update_timing(struct hdmi_timing *t,
	bool to_frac_mode)
{
	unsigned int alternate_clock = 0;
	bool frac_timing = t->v_freq % 1000 == 0 ? false : true;

	if (!hdmitx_mode_have_alternate_clock(t))
		return -EINVAL;

	if (!frac_timing && to_frac_mode)
		alternate_clock = DIV_ROUND_CLOSEST_ULL(mul_u32_u32(t->pixel_freq, 1000), 1001);
	else if (frac_timing && !to_frac_mode)
		alternate_clock = DIV_ROUND_CLOSEST_ULL(mul_u32_u32(t->pixel_freq, 1001), 1000);

	if (alternate_clock) {
		t->pixel_freq = alternate_clock;
		/*update vsync/hsync*/
		t->v_freq = DIV_ROUND_CLOSEST_ULL(mul_u32_u32(t->pixel_freq, 1000000),
						mul_u32_u32(t->h_total, t->v_total));
		t->h_freq = DIV_ROUND_CLOSEST_ULL(mul_u32_u32(t->pixel_freq, 1000), t->h_total);

		pr_info("Timing %s update frac_mode(%d):\n",
			t->name, to_frac_mode);
		pr_info("\tPixel_freq(%d), h_freq (%d), v_freq(%d).\n",
			t->pixel_freq, t->h_freq, t->v_freq);
	}

	return alternate_clock;
}
