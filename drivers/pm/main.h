/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _PM_MAIN_H__
#define _PM_MAIN_H__

#if IS_ENABLED(CONFIG_AMLOGIC_GX_SUSPEND)
int pm_init(void);
void pm_exit(void);
#else
static inline int pm_init(void)
{
	return 0;
}

static inline void pm_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_GX_REBOOT)
int reboot_init(void);
void reboot_exit(void);
#else
static inline int reboot_init(void)
{
	return 0;
}

static inline void reboot_exit(void)
{
}
#endif

#endif /* _PM_MAIN_H__ */
