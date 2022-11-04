/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __WIRELESS_MAIN_H_
#define __WIRELESS_MAIN_H_

#if IS_ENABLED(CONFIG_AMLOGIC_BLUETOOTH)
int bt_init(void);
void bt_exit(void);
#else
static inline int bt_init(void)
{
	return 0;
}

static inline void bt_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_WIFI)
int wifi_dt_init(void);
void wifi_dt_exit(void);
#else
static inline int wifi_dt_init(void)
{
	return 0;
}

static inline void wifi_dt_exit(void)
{
}
#endif

#endif /*__WIRELESS_MAIN_H_*/
