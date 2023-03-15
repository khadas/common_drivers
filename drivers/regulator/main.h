/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __REGULATOR_MAIN_H__
#define __REGULATOR_MAIN_H__

#if IS_ENABLED(CONFIG_AMLOGIC_REGULATOR_PMIC6B)
int meson_pmic6b_regulator_init(void);
void meson_pmic6b_regulator_exit(void);
#else
static inline int meson_pmic6b_regulator_init(void)
{
	return 0;
}

static inline void meson_pmic6b_regulator_exit(void)
{
}
#endif

#endif /*__REGULATOR_MAIN_H__*/
