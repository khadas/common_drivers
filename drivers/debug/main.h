/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __DEBUG_MAIN_H_
#define __DEBUG_MAIN_H_

int aml_debug_init(void);
int debug_lockup_init(void);
#ifdef CONFIG_AMLOGIC_DEBUG_MHZ
int cpu_mhz_init(void);
#else
int cpu_mhz_init(void)  { return 0; }
#endif
int aml_sched_init(void);
int aml_kprobes_init(void);

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_ATRACE)
int meson_atrace_init(void);
#else
static inline int meson_atrace_init(void)
{
	return 0;
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_FILE)
int debug_file_init(void);
void debug_file_exit(void);
#else
static inline int debug_file_init(void)
{
	return 0;
}

static inline void debug_file_exit(void)
{
}
#endif

#if defined(CONFIG_AMLOGIC_GKI_CONFIG) && defined(MODULE)
int gki_config_init(void);
#else
static inline int gki_config_init(void)
{
	return 0;
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_HLD)
int aml_hld_init(void);
#else
static inline int aml_hld_init(void)
{
	return 0;
}
#endif

#if IS_MODULE(CONFIG_AMLOGIC_DEBUG_PRINTK)
int printk_vendor_hook_init(void);
void printk_vendor_hook_exit(void);
#endif

#endif /*_DEBUG_MAIN_H__*/
