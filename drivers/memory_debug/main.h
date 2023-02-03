/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _MEMORY_MAIN_H__
#define _MEMORY_MAIN_H__

#if IS_ENABLED(CONFIG_AMLOGIC_FILE_CACHE)
int __init filecache_module_init(void);
void __exit filecache_module_exit(void);
#else
static inline int filecache_module_init(void)
{
	return 0;
}

static inline void filecache_module_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_WATCHPOINT)
int __init aml_watch_pint_init(void);
void __exit aml_watch_point_uninit(void);
#else
static inline int aml_watch_pint_init(void)
{
	return 0;
}

static inline void aml_watch_point_uninit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_WATCHPOINT)
int __init aml_reg_init(void);
void __exit aml_reg_exit(void);
#else
static inline int aml_reg_init(void)
{
	return 0;
}

static inline void aml_reg_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_DDR_TOOL)
int __init ddr_tool_init(void);
void __exit ddr_tool_exit(void);
#else
static inline int ddr_tool_init(void)
{
	return 0;
}

static inline void ddr_tool_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_RAMDUMP)
int __init ramdump_init(void);
void __exit ramdump_uninit(void);
#else
static inline int ramdump_init(void)
{
	return 0;
}

static inline void ramdump_uninit(void)
{
}
#endif

#endif /* _MEMORY_MAIN_H__ */
