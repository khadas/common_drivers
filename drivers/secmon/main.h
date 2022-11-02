/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _SECMON_MAIN_H__
#define _SECMON_MAIN_H__

#if IS_ENABLED(CONFIG_AMLOGIC_SECMON)
int meson_secmon_init(void);
#else
static inline int meson_secmon_init(void)
{
	return 0;
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_DOLBY_FW)
int dolby_fw_init(void);
void dolby_fw_exit(void);
#else
static inline int dolby_fw_init(void)
{
	return 0;
}

static inline void dolby_fw_exit(void)
{
}
#endif

#endif /* _SECMON_MAIN_H__ */
