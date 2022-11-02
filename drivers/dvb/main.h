/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _MAIN_H__
#define _MAIN_H__

#if IS_ENABLED(CONFIG_AMLOGIC_DVB_EXTERN)
int aml_dvb_extern_init(void);
void aml_dvb_extern_exit(void);
#else
static inline int aml_dvb_extern_init(void)
{
	return 0;
}

static inline void aml_dvb_extern_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_AUCPU)
s32 aucpu_init(void);
void aucpu_exit(void);
#else
static inline int aucpu_init(void)
{
	return 0;
}

static inline void aucpu_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_DVB_DSM)
int dsm_init(void);
void dsm_exit(void);
#else
static inline int dsm_init(void)
{
	return 0;
}

static inline void dsm_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_SMARTCARD)
int smc_sc2_mod_init(void);
void smc_sc2_mod_exit(void);
#else
static inline int smc_sc2_mod_init(void)
{
	return 0;
}

static inline void smc_sc2_mod_exit(void)
{
}
#endif

#endif /* _DVB_MAIN_H__ */
