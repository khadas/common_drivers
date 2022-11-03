/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __RTC_MAIN_H_
#define __RTC_MAIN_H_

#if IS_ENABLED(CONFIG_AMLOGIC_RTC_DRV_MESON_VRTC)
int vrtc_init(void);
void vrtc_exit(void);
#else
static inline void vrtc_init(void)
{
	return 0;
}

static inline int vrtc_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_MESON_RTC)
int rtc_init(void);
void rtc_exit(void);
#else
static inline int rtc_init(void)
{
	return 0;
}

static inline void rtc_exit(void)
{
}
#endif

#endif /*_RTC_MAIN_H__*/
