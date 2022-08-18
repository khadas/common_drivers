/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef AMLOGIC_DMABUF_HEAP_H
#define AMLOGIC_DMABUF_HEAP_H

#define DMABUF_FLAG_EXTEND_CACHED                  BIT(29)
#define DMABUF_FLAG_EXTEND_MESON_HEAP              BIT(30)
#define DMABUF_FLAG_EXTEND_PROTECTED               BIT(31)

#define CODECMM_HEAP_NAME			"heap-codecmm"
#define CODECMM_SECURE_HEAP_NAME		"heap-secure-codecmm"
#define CODECMM_CACHED_HEAP_NAME		"heap-cached-codecmm"

#define SYSTEM_SECURE_UNCACHE_HEAP_NAME		"system-secure-uncached"

#endif /**/

