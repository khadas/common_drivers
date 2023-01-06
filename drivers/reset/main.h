/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _RESET_MAIN_H__
#define _RESET_MAIN_H__

#if IS_ENABLED(CONFIG_AMLOGIC_RESET_MESON)
int meson_reset_driver_init(void);
void meson_reset_driver_exit(void);
#else
static inline int meson_reset_driver_init(void)
{
	return 0;
}

static inline void meson_reset_driver_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_DOS_RESET_MESON)
int reset_dos(void);
void reset_exit(void);
#else
static inline int reset_dos(void)
{
	return 0;
}

static inline void reset_exit(void)
{
}
#endif

#endif /* _PM_MAIN_H__ */
