/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/**
 * amlogic wake up engine woker at kernel side
 *
 * Author: Yang Liu <Yang.Liu@amlogic.com>
 * Version:
 * - 0.1        init
 */

#ifndef _HIFI4RPC_AWE_H_
#define _HIFI4RPC_AWE_H_

/**
 * Simple flow from ALSA/soc to DSP side
 *
 * +------+             +-------+          +-------------+             +-----+
 * | ALSA |             | wwe.c |          | awe wrapper |             | DSP |
 * +------+             +-------+          +-------------+             +-----+
 *   |                      |                     |                       |
 *   |-- aml_wwe_prepare -->| -- awe_open ------->|--- aml_awe_create  -->|
 *   |                      |                     |           &open       |
 *   |                      |                     |                       |
 *   |<- period_elapsed ----| <- notify  ---------|<-- data handler ------|
 *   |                      |   (aml_wwe_notify)  |                       |
 *   |                      |                     |                       |
 *   |-- aml_wwe_pointer -->| -- awe_pointer ---->|                       |
 *   |                      |                     |                       |
 *   |-- aml_wwe_copy_user->| -- awe_copy_user -->|                       |
 *   |                      |                     |                       |
 *   |-- aml_wwe_close ---->| -- awe_close ------>|--- aml_awe_close ---->|
 *   |                      |                     |           &destroy    |
 */
int awe_open(size_t period_size, size_t periods,
	     int (*notify_cb)(void *), void *user);
int awe_close(void);

int awe_pointer(void);
int awe_copy_user(void __user *buf, unsigned long bytes);

#endif
