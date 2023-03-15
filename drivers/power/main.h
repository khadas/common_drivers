/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _DOMAIN_MAIN_H__
#define _DOMAIN_MAIN_H__

#if IS_ENABLED(CONFIG_AMLOGIC_POWER)
int sec_power_domain_init(void);
void sec_power_domain_exit(void);
#else
static inline int sec_power_domain_init(void)
{
	return 0;
}

static inline void sec_power_domain_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_POWER)
int power_ee_domain_init(void);
void power_ee_domain_exit(void);
#else
static inline int power_ee_domain_init(void)
{
	return 0;
}

static inline void power_ee_domain_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_BATTERY_PMIC6B)
int meson_pmic6b_bat_init(void);
void meson_pmic6b_bat_exit(void);
#else
static inline int meson_pmic6b_bat_init(void)
{
	return 0;
}

static inline void meson_pmic6b_bat_exit(void)
{
}
#endif

#endif /* _DOMAIN_MAIN_H__ */
