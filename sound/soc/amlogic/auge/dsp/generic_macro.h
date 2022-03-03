/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/**
 * define widely used macro here
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#ifndef _GENERIC_MACRO_H_
#define _GENERIC_MACRO_H_

#define SAMPLE_MS 48
#define SAMPLE_BYTES 4
#define SAMPLE_CH 16
#define CHUNK_MS 16
#define CHUNK_BYTES (SAMPLE_MS * SAMPLE_BYTES * SAMPLE_CH * CHUNK_MS)

#ifndef AMX_MIN
#define AMX_MIN(a, b) ({ typeof(a) a_ = (a); typeof(b) b_ = (b);  (a_ < b_) ? a_ : b_; })
#endif

#ifndef AMX_UNUSED
#define AMX_UNUSED(x) ((void)(x))
#endif

#endif
