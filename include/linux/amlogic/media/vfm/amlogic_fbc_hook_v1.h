/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * drivers/amlogic/media/frame_provider/decoder/utils/amlogic_fbc_hook.h
 *
 * Copyright (C) 2016 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef AMLGIC_FBC_V1_HEADER___
#define AMLGIC_FBC_V1_HEADER___

struct fbc_decoder_param {
	unsigned long compHeadAddr;
	u32 compWidth;
	u32 compHeight;
	u32 bitdepth;
};

int AMLOGIC_FBC_vframe_decoder_v1(void *dstyuv[4], struct fbc_decoder_param *param,
				  int out_format, int flags);
int AMLOGIC_FBC_vframe_encoder_v1(void *srcyuv[4], void *dst_header,
				  void *dst_body, int in_format, int flags);

typedef int (*AMLOGIC_FBC_vframe_decoder_fun_t)(void **, struct fbc_decoder_param *,
						int, int);
typedef int (*AMLOGIC_FBC_vframe_encoder_fun_t)(void **, void *, void *,
						int, int);
int register_amlogic_afbc_dec_fun_v1(AMLOGIC_FBC_vframe_decoder_fun_t fn);
int register_amlogic_afbc_enc_fun_v1(AMLOGIC_FBC_vframe_encoder_fun_t fn);
int unregister_amlogic_afbc_dec_fun_v1(void);
int unregister_amlogic_afbc_enc_fun_v1(void);
#endif
