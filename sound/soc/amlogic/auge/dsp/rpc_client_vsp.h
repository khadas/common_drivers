/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/**
 * voice signal process API
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#ifndef _RPC_CLIENT_VSP_H_
#define _RPC_CLIENT_VSP_H_

/* typedef void *AML_VSP_HANDLE; */
#define AML_VSP_HANDLE			void *

/**
 * Create and initialize an instance context according to vsp id
 *
 * @param[in] vsp id
 *
 * @param[in] static parameter used to initialize instance context
 *
 * @param[in] length of static parameter
 *
 * @return instance handler if successful, otherwise return NULL
 */
AML_VSP_HANDLE aml_vsp_init(const char *vsp_id, void *param, size_t param_size);

/**
 * Destroy and deinitialize instance context.
 *
 * @param[in] instance handler
 *
 * @return
 */
void aml_vsp_deinit(AML_VSP_HANDLE h_vsp);

/**
 * Enable instance, make instance start to work
 * Static parameter can not be configured any more after this call
 *
 * @param[in] instance handler
 *
 * @return error codes
 */
int  aml_vsp_open(AML_VSP_HANDLE h_vsp);

/**
 * Disable instance, make instance stop to work
 * Static parameter can be re-configured after this call
 *
 * @param[in] instance handler
 *
 * @return error codes
 */
int  aml_vsp_close(AML_VSP_HANDLE h_vsp);

/**
 * Configure parameter
 * Static parameter can be configured only after instance stops
 * Dynamic parameter can be configured at run time
 *
 * @param[in] instance handler
 *
 * @param[in] parameter id
 *
 * @param[in] buffer carrying parameter
 *
 * @param[in] length of parameter
 *
 * @return error codes
 */
int  aml_vsp_setparam(AML_VSP_HANDLE h_vsp, int32_t param_id, void *param, size_t param_size);

/**
 * Obtain parameter
 *
 * @param[in] instance handler
 *
 * @param[in] parameter id
 *
 * @param[in] buffer carrying parameter
 *
 * @param[in] length of parameter
 *
 * @return error codes
 */
int  aml_vsp_getparam(AML_VSP_HANDLE h_vsp, int32_t param_id, void *param, size_t param_size);

/**
 * Main data processing entry.
 *
 * @param[in] instance handler
 *
 * @param[in] Buffer carrying input data/meta
 *
 * @param[in] Input buffer size
 *
 * @param[in/out] Buffer carrying output data/meta
 *
 * @param[in/out] Space of buffer before the call
 *                Size of output data/meta after the call
 *
 * @return error codes
 */
int  aml_vsp_process(AML_VSP_HANDLE h_vsp, void *input, size_t input_size,
		     void *output, size_t *output_size);

#endif
