/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef VIDEO_REG_COMMON_HEADER_HH
#define VIDEO_REG_COMMON_HEADER_HH

struct vpu_venc_regs_s {
	u32 vpu_enci_stat;
	u32 vpu_encp_stat;
	u32 vpu_encl_stat;
	u32 enci_vavon_bline;
	u32 encp_vavon_bline;
	u32 encl_vavon_bline;
};

#endif
