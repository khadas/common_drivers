/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __SECKEY_MAIN_H_
#define __SECKEY_MAIN_H_

#if IS_ENABLED(CONFIG_AMLOGIC_SECKEY_KL)
int aml_seckey_kl_init(void);
void aml_seckey_kl_exit(void);
#else
static inline int aml_seckey_kl_init(void)
{
	return 0;
}

static inline void aml_seckey_kl_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_SECKEY_KT)
int aml_seckey_kt_init(void);
void aml_seckey_kt_exit(void);
int aml_key_driver_init(void);
void aml_key_driver_exit(void);
#else
static inline int aml_seckey_kt_init(void)
{
	return 0;
}

static inline void aml_seckey_kt_exit(void)
{
}

static inline int aml_key_driver_init(void)
{
	return 0;
}

static inline void aml_key_driver_exit(void)
{
}
#endif

#endif /*__SECKEY_MAIN_H_*/
