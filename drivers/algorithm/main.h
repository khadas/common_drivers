/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __ALGORITHM_MAIN_H_
#define __ALGORITHM_MAIN_H_

#if IS_ENABLED(CONFIG_AMLOGIC_DNLP_ALGORITHM)
int aml_dnlp_alg_init(void);
void aml_dnlp_alg_exit(void);
#else
static inline int aml_dnlp_alg_init(void)
{
	return 0;
}

static inline void aml_dnlp_alg_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_AAD_ALGORITHM)
int aad_alg_driver_init(void);
void aad_alg_driver_exit(void);
#else
static inline int aad_alg_driver_init(void)
{
	return 0;
}

static inline void aad_alg_driver_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_CABC_ALGORITHM)
int cabc_alg_driver_init(void);
void cabc_alg_driver_exit(void);
#else
static inline int cabc_alg_driver_init(void)
{
	return 0;
}

static inline void cabc_alg_driver_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_COLOR_TUNE_ALGORITHM)
int color_tune_init(void);
void color_tune_exit(void);
#else
static inline int color_tune_init(void)
{
	return 0;
}

static inline void color_tune_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_HDR10_TMO_ALGORITHM)
int hdr10_tmo_alg_driver_init(void);
void hdr10_tmo_alg_driver_exit(void);
#else
static inline int hdr10_tmo_alg_driver_init(void)
{
	return 0;
}

static inline void hdr10_tmo_alg_driver_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_PREGM_ALGORITHM)
int pregm_alg_driver_init(void);
void pregm_alg_driver_exit(void);
#else
static inline int pregm_alg_driver_init(void)
{
	return 0;
}

static inline void pregm_alg_driver_exit(void)
{
}
#endif

#endif /*_ALGORITHM_MAIN_H__*/
