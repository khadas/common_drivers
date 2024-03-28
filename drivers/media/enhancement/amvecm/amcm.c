// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * drivers/amlogic/media/enhancement/amvecm/amcm.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
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

/* #include <mach/am_regs.h> */
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/amlogic/media/amvecm/cm.h>
/* #include <linux/amlogic/aml_common.h> */
#include <linux/amlogic/media/vfm/vframe.h>
#include <linux/amlogic/media/amvecm/amvecm.h>
#if CONFIG_AMLOGIC_MEDIA_VIDEO
#include <linux/amlogic/media/video_sink/video.h>
#endif
#include <linux/uaccess.h>
#include "arch/vpp_regs.h"
#include "arch/cm_regs.h"
#include "arch/ve_regs.h"
#include "amcm.h"
#include "amcm_regmap.h"
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
#include <linux/amlogic/media/amdolbyvision/dolby_vision.h>
#endif
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
#include "amcsc.h"
#include "local_contrast.h"
#include "amve_v2.h"
#include "ai_pq/ai_pq.h"
#include "cm2_adj.h"
#endif
#include "amve.h"
#include "reg_helper.h"

#define pr_amcm_dbg(fmt, args...)\
	do {\
		if (debug_amcm)\
			pr_info("AMCM: " fmt, ## args);\
	} while (0)\

static int debug_amcm;
module_param(debug_amcm, int, 0664);
MODULE_PARM_DESC(debug_amcm, "\n debug_amcm\n");

static bool debug_regload;
module_param(debug_regload, bool, 0664);
MODULE_PARM_DESC(debug_regload, "\n debug_regload\n");

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
static int cm_level = 1;/* 0:optimize;1:enhancement */
module_param(cm_level, int, 0664);
MODULE_PARM_DESC(cm_level, "\n select cm lever\n");
#endif

int cm_en;/* 0:disable;1:enable */
module_param(cm_en, int, 0664);
MODULE_PARM_DESC(cm_en, "\n enable or disable cm\n");

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
static unsigned int cm_width_limit = 50;/* vlsi adjust */
module_param(cm_width_limit, uint, 0664);
MODULE_PARM_DESC(cm_width_limit, "\n cm_width_limit\n");
#endif

int pq_reg_wr_rdma;/* 0:disable;1:enable */
module_param(pq_reg_wr_rdma, int, 0664);
MODULE_PARM_DESC(pq_reg_wr_rdma, "\n pq_reg_wr_rdma\n");

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT_C1A
static struct am_regs_s amregs0;
static struct am_regs_s amregs1;
static struct am_regs_s amregs2;
static struct am_regs_s amregs3;
static struct am_regs_s amregs4;
static struct am_regs_s amregs5;
#endif

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
static int cm_force_flag;

static int cm_level_last = 0xff;/* 0:optimize;1:enhancement */
unsigned int cm2_patch_flag;
unsigned int cm_size;
/* cm enable flag internal for cm size issue--0:disable;1:enable */
static bool cm_en_flag;
/* cm disable flag sync with pq-db--1:disable;0:enable */
static bool cm_dis_flag;

#if CONFIG_AMLOGIC_MEDIA_VIDEO
static struct vd_proc_amvecm_info_t *vd_size_info;
#endif

/* extern unsigned int vecm_latch_flag; */
void cm_wr_api(unsigned int addr, unsigned int data,
	unsigned int mask, enum wr_md_e md, int vpp_index)
{
	unsigned int temp;
	int addr_port;
	int data_port;
	struct cm_port_s cm_port;
	int i;
	int slice_max;

	if (chip_type_id == chip_s5 ||
		chip_type_id == chip_t3x) {
		slice_max = get_slice_max();
		cm_port = get_cm_port();
		switch (md) {
		case WR_VCB:
			for (i = 0; i < slice_max; i++) {
				addr_port = cm_port.cm_addr_port[i];
				data_port = cm_port.cm_data_port[i];
				if (mask == 0xffffffff) {
					WRITE_VPP_REG(addr_port, addr);
					WRITE_VPP_REG(data_port, data);
				} else {
					WRITE_VPP_REG(addr_port, addr);
					temp = READ_VPP_REG(data_port);
					WRITE_VPP_REG(addr_port, addr);
					WRITE_VPP_REG(data_port,
						(temp & (~mask)) |
						(data & mask));
				}
			}
			break;
		case WR_DMA:
			for (i = 0; i < slice_max; i++) {
				addr_port = cm_port.cm_addr_port[i];
				data_port = cm_port.cm_data_port[i];
				if (mask == 0xffffffff) {
					VSYNC_WRITE_VPP_REG_EX_VPP_SEL(addr_port, addr,
						0, vpp_index);
					VSYNC_WRITE_VPP_REG_EX_VPP_SEL(data_port, data,
						0, vpp_index);
				} else {
					WRITE_VPP_REG(addr_port, addr);
					temp = READ_VPP_REG(data_port);
					VSYNC_WRITE_VPP_REG_EX_VPP_SEL(addr_port, addr,
						0, vpp_index);
					VSYNC_WRITE_VPP_REG_EX_VPP_SEL(data_port,
						(temp & (~mask)) |
						(data & mask), 0, vpp_index);
				}
			}
			break;
		default:
			break;
		}
	} else {
		addr_port = VPP_CHROMA_ADDR_PORT;
		data_port = VPP_CHROMA_DATA_PORT;

		switch (md) {
		case WR_VCB:
			if (mask == 0xffffffff) {
				WRITE_VPP_REG(addr_port, addr);
				WRITE_VPP_REG(data_port, data);
			} else {
				WRITE_VPP_REG(addr_port, addr);
				temp = READ_VPP_REG(data_port);
				WRITE_VPP_REG(addr_port, addr);
				WRITE_VPP_REG(data_port,
					(temp & (~mask)) |
					(data & mask));
			}
			break;
		case WR_DMA:
			if (mask == 0xffffffff) {
				VSYNC_WRITE_VPP_REG_EX_VPP_SEL(addr_port, addr,
					0, vpp_index);
				VSYNC_WRITE_VPP_REG_EX_VPP_SEL(data_port, data,
					0, vpp_index);
			} else {
				WRITE_VPP_REG(addr_port, addr);
				temp = READ_VPP_REG(data_port);
				VSYNC_WRITE_VPP_REG_EX_VPP_SEL(addr_port, addr,
					0, vpp_index);
				VSYNC_WRITE_VPP_REG_EX_VPP_SEL(data_port,
					(temp & (~mask)) |
					(data & mask), 0, vpp_index);
			}
			break;
		default:
			break;
		}
	}
}
#endif

void am_set_regmap(struct am_regs_s *p, int vpp_index)
{
	unsigned short i;
	unsigned int temp = 0;
	unsigned int mask = 0;
	unsigned int val = 0;
	unsigned int addr = 0;
	unsigned int skip = 0;
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	struct am_reg_s *dejaggy_reg;
	unsigned int addr_tmp = 0;
	unsigned int sr_addr_offset = 0x2500;

	dejaggy_reg = sr0_dej_setting[DEJAGGY_LEVEL - 1].am_reg;
#endif
	for (i = 0; i < p->length; i++) {
		mask = p->am_reg[i].mask;
		val = p->am_reg[i].val;
		addr = p->am_reg[i].addr;
		skip = skip_pq_ctrl_load(&p->am_reg[i]);
		if (skip != 0)
			mask &= ~skip;

		switch (p->am_reg[i].type) {
		case REG_TYPE_PHY:
			break;
		case REG_TYPE_CBUS:
			if (mask == 0xffffffff) {
				/* WRITE_CBUS_REG(p->am_reg[i].addr,*/
				/*  p->am_reg[i].val); */
				aml_write_cbus(addr, val);
			} else {
				/* WRITE_CBUS_REG(p->am_reg[i].addr, */
				/* (READ_CBUS_REG(p->am_reg[i].addr) & */
				/* (~(p->am_reg[i].mask))) | */
				/* (p->am_reg[i].val & p->am_reg[i].mask)); */
				temp = aml_read_cbus(addr);
				aml_write_cbus(addr,
					(temp & (~mask)) |
					(val & mask));
			}
			break;
		case REG_TYPE_APB:
			/* if (p->am_reg[i].mask == 0xffffffff) */
			/* WRITE_APB_REG(p->am_reg[i].addr,*/
			/*  p->am_reg[i].val); */
			/* else */
			/* WRITE_APB_REG(p->am_reg[i].addr, */
			/* (READ_APB_REG(p->am_reg[i].addr) & */
			/* (~(p->am_reg[i].mask))) | */
			/* (p->am_reg[i].val & p->am_reg[i].mask)); */
			break;
		case REG_TYPE_MPEG:
			/* if (p->am_reg[i].mask == 0xffffffff) */
			/* WRITE_MPEG_REG(p->am_reg[i].addr,*/
			/*  p->am_reg[i].val); */
			/* else */
			/* WRITE_MPEG_REG(p->am_reg[i].addr, */
			/* (READ_MPEG_REG(p->am_reg[i].addr) & */
			/* (~(p->am_reg[i].mask))) | */
			/* (p->am_reg[i].val & p->am_reg[i].mask)); */
			break;
		case REG_TYPE_AXI:
			/* if (p->am_reg[i].mask == 0xffffffff) */
			/* WRITE_AXI_REG(p->am_reg[i].addr,*/
			/* p->am_reg[i].val); */
			/* else */
			/* WRITE_AXI_REG(p->am_reg[i].addr, */
			/* (READ_AXI_REG(p->am_reg[i].addr) & */
			/* (~(p->am_reg[i].mask))) | */
			/* (p->am_reg[i].val & p->am_reg[i].mask)); */
			break;
		case REG_TYPE_INDEX_VPPCHROMA:
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
			/*  add for vm2 demo frame size setting */
			if (addr == 0x20f) {
				if ((val & 0xff) != 0) {
					cm2_patch_flag = val;
					val = val & 0xffffff00;
				} else {
					cm2_patch_flag = 0;
				}
			}

			/* add for cm patch size config */
			if (addr == 0x205 ||
				addr == 0x209 ||
				addr == 0x20a) {
				pr_amcm_dbg("[amcm]:%s %s addr:0x%x",
					"REG_TYPE_INDEX_VPPCHROMA",
					__func__, addr);
				break;
			}

			if (!cm_en) {
				if (addr == 0x208) {
					if (get_cpu_type() >=
						MESON_CPU_MAJOR_ID_G12A)
						val = val & 0xfffffffc;
					else
						val = val & 0xfffffff9;
				}
				pr_amcm_dbg("[amcm]:%s %s addr:0x%x",
					"REG_TYPE_INDEX_VPPCHROMA",
					__func__, addr);
			} else if (addr == 0x208) {
				if (get_cpu_type() >=
					MESON_CPU_MAJOR_ID_G12A) {
					val = val & 0xfffffffd;
					if (val & 0x1)
						cm_dis_flag = false;
					else
						cm_dis_flag = true;
				} else {
					val = val & 0xfffffffb;
					if (val & 0x2)
						cm_dis_flag = false;
					else
						cm_dis_flag = true;
				}
			}

			/*if (pq_reg_wr_rdma)*/
			/*	cm_wr_api(addr, val, mask, WR_DMA, vpp_index);*/
			/*else*/
				cm_wr_api(addr, val, mask, WR_VCB, 0);

			default_sat_param(addr, val);
#endif
			break;
		case REG_TYPE_INDEX_GAMMA:
			break;
		case VALUE_TYPE_CONTRAST_BRIGHTNESS:
			break;
		case REG_TYPE_INDEX_VPP_COEF:
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
			//if (((addr & 0xf) == 0) ||
			//    ((addr & 0xf) == 0x8)) {
			if (pq_reg_wr_rdma)
				cm_wr_api(addr, val, 0xffffffff, WR_DMA, vpp_index);
			else
				cm_wr_api(addr, val, 0xffffffff, WR_VCB, 0);
			//} else {
			//	if (pq_reg_wr_rdma)
			//		cm_wr_api(addr, val, 0xffffffff, WR_DMA);
			//	else
			//		cm_wr_api(addr, val, 0xffffffff, WR_VCB);
			//}
			default_sat_param(addr, val);
#endif
			break;
		case REG_TYPE_VCBUS:
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
			if (addr == offset_addr(SRSHARP0_DEJ_ALPHA)) {
				dejaggy_reg[1].val = val & 0xff;
				if (pd_detect_en)
					mask &= ~(0xff);
			}

			if (addr == offset_addr(SRSHARP0_DEJ_CTRL)) {
				dejaggy_reg[0].val = val & 0x1;
				if (pd_detect_en)
					mask &= ~(0x1);
			}
#endif

			if (mask == 0xffffffff) {
				if (pq_reg_wr_rdma)
					VSYNC_WRITE_VPP_REG_EX_VPP_SEL(addr, val, 0, vpp_index);
				else
					WRITE_VPP_REG(p->am_reg[i].addr,
						p->am_reg[i].val);

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
				/*for slice1 sr*/
				if (chip_type_id == chip_t3x &&
					addr >= 0x5000 &&
					addr <= 0x53ff) {
					if (pq_reg_wr_rdma)
						VSYNC_WRITE_VPP_REG_EX_VPP_SEL(addr +
							sr_addr_offset,
							val, 0, vpp_index);
					else
						WRITE_VPP_REG(p->am_reg[i].addr +
							sr_addr_offset,
							p->am_reg[i].val);
				}
#endif
			} else {
				if (addr == VPP_MISC)
					break;

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
				if (addr == offset_addr(SRSHARP1_LC_TOP_CTRL)) {
					if (!lc_en)
						val = val & 0xffffffef;
				}

				if (lc_curve_ctrl_reg_set_flag(addr)) {
					mask |= 0x1;
					val |= 0x1;
				}
#endif

				if (pq_reg_wr_rdma) {
					temp = READ_VPP_REG(addr);
					VSYNC_WRITE_VPP_REG_EX_VPP_SEL(addr,
						(temp & (~mask)) |
						(val & mask), 0, vpp_index);
				} else {
					temp = aml_read_vcbus_s(addr);
					WRITE_VPP_REG(addr,
						(temp & (~mask)) |
						(val & mask));
				}
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
				/*for slice1 sr*/
				if (chip_type_id == chip_t3x &&
					addr >= 0x5000 &&
					addr <= 0x53ff) {
					addr_tmp = addr + sr_addr_offset;
					if (pq_reg_wr_rdma) {
						temp = READ_VPP_REG(addr_tmp);
						VSYNC_WRITE_VPP_REG_EX_VPP_SEL(addr_tmp,
							(temp & (~mask)) |
							(val & mask), 0, vpp_index);
					} else {
						temp = aml_read_vcbus_s(addr_tmp);
						WRITE_VPP_REG(addr_tmp,
							(temp & (~mask)) |
							(val & mask));
					}
				}

				aipq_base_peaking_param(addr, mask, val);
#endif
			}
			break;
		case REG_TYPE_OFFSET_VCBUS:
			if (mask == 0xffffffff) {
				if (pq_reg_wr_rdma)
					VSYNC_WRITE_VPP_REG_EX_VPP_SEL(addr, val, 0, vpp_index);
				else
					WRITE_VPP_REG(addr, val);
			} else {
				if (pq_reg_wr_rdma) {
					temp = READ_VPP_REG(addr);
					VSYNC_WRITE_VPP_REG_EX_VPP_SEL(addr,
						(temp & (~mask)) |
						(val & mask), 0, vpp_index);
				} else {
					temp = READ_VPP_REG(addr);
					WRITE_VPP_REG(addr,
						(temp & (~mask)) |
						(val & mask));
				}
			}
			break;
/* #endif */
		default:
			break;
		}
	}
}

void amcm_disable(enum wr_md_e md, int vpp_index)
{
	int temp;
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	int addr_port;
	int data_port;
	struct cm_port_s cm_port;
	int i;
	int slice_max;

	if (chip_type_id == chip_s7)
		return;

	if (chip_type_id == chip_s5 ||
		chip_type_id == chip_t3x) {
		slice_max = get_slice_max();
		cm_port = get_cm_port();
		switch (md) {
		case WR_VCB:
			for (i = 0; i < slice_max; i++) {
				addr_port = cm_port.cm_addr_port[i];
				data_port = cm_port.cm_data_port[i];

				if (chip_type_id == chip_t3x) {
					WRITE_VPP_REG(addr_port, STA_CFG_REG);
					temp = READ_VPP_REG(data_port);
					temp = (temp & (~0xc0000000)) | (0 << 30);
					WRITE_VPP_REG(addr_port, STA_CFG_REG);
					WRITE_VPP_REG(data_port, temp);
				}

				WRITE_VPP_REG(addr_port, 0x208);
				temp = READ_VPP_REG(data_port);
				WRITE_VPP_REG(addr_port, 0x208);
				WRITE_VPP_REG(data_port,
					temp & 0xfffffffe);
			}
			break;
		case WR_DMA:
			for (i = 0; i < slice_max; i++) {
				addr_port = cm_port.cm_addr_port[i];
				data_port = cm_port.cm_data_port[i];

				if (chip_type_id == chip_t3x) {
					WRITE_VPP_REG(addr_port, STA_CFG_REG);
					temp = READ_VPP_REG(data_port);
					temp = (temp & (~0xc0000000)) | (0 << 30);
					WRITE_VPP_REG(addr_port, STA_CFG_REG);
					WRITE_VPP_REG(data_port, temp);
				}

				WRITE_VPP_REG(addr_port, 0x208);
				temp = READ_VPP_REG(data_port);
				VSYNC_WRITE_VPP_REG_VPP_SEL(addr_port,
					0x208, vpp_index);
				VSYNC_WRITE_VPP_REG_VPP_SEL(data_port,
					temp & 0xfffffffe, vpp_index);
			}
			break;
		default:
			break;
		}
	} else
#endif
	{
		WRITE_VPP_REG(VPP_CHROMA_ADDR_PORT, STA_CFG_REG);
		temp = READ_VPP_REG(VPP_CHROMA_DATA_PORT);
		temp = (temp & (~0xc0000000)) | (0 << 30);
		WRITE_VPP_REG(VPP_CHROMA_ADDR_PORT, STA_CFG_REG);
		WRITE_VPP_REG(VPP_CHROMA_DATA_PORT, temp);

		WRITE_VPP_REG(VPP_CHROMA_ADDR_PORT, 0x208);
		temp = READ_VPP_REG(VPP_CHROMA_DATA_PORT);

		switch (md) {
		case WR_VCB:
			if (get_cpu_type() >= MESON_CPU_MAJOR_ID_G12A) {
				if (temp & 0x1) {
					WRITE_VPP_REG(VPP_CHROMA_ADDR_PORT,
						0x208);
					WRITE_VPP_REG(VPP_CHROMA_DATA_PORT,
						temp & 0xfffffffe);
				}
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
			} else {
				if (temp & 0x2) {
					WRITE_VPP_REG(VPP_CHROMA_ADDR_PORT,
						0x208);
					WRITE_VPP_REG(VPP_CHROMA_DATA_PORT,
						temp & 0xfffffffd);
				}
#endif
			}
			break;
		case WR_DMA:
			if (get_cpu_type() >= MESON_CPU_MAJOR_ID_G12A) {
				if (temp & 0x1) {
					VSYNC_WRITE_VPP_REG_VPP_SEL(VPP_CHROMA_ADDR_PORT,
						0x208, vpp_index);
					VSYNC_WRITE_VPP_REG_VPP_SEL(VPP_CHROMA_DATA_PORT,
						temp & 0xfffffffe, vpp_index);
				}
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
			} else {
				if (temp & 0x2) {
					VSYNC_WRITE_VPP_REG_VPP_SEL(VPP_CHROMA_ADDR_PORT,
						0x208, vpp_index);
					VSYNC_WRITE_VPP_REG_VPP_SEL(VPP_CHROMA_DATA_PORT,
						temp & 0xfffffffd, vpp_index);
				}
#endif
			}
			break;
		default:
			break;
		}
	}

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	cm_en_flag = false;
#endif
}

void amcm_enable(enum wr_md_e md, int vpp_index)
{
	int temp;
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	int addr_port;
	int data_port;
	struct cm_port_s cm_port;
	int i;
	int slice_max;

	if (chip_type_id == chip_s7)
		return;

	if (chip_type_id == chip_s5 ||
		chip_type_id == chip_t3x) {
		slice_max = get_slice_max();
		cm_port = get_cm_port();
		switch (md) {
		case WR_VCB:
			for (i = 0; i < slice_max; i++) {
				addr_port = cm_port.cm_addr_port[i];
				data_port = cm_port.cm_data_port[i];

				WRITE_VPP_REG(addr_port, 0x208);
				temp = READ_VPP_REG(data_port);
				WRITE_VPP_REG(addr_port, 0x208);
				WRITE_VPP_REG(data_port, temp | 0x1);

				if (chip_type_id == chip_t3x) {
					WRITE_VPP_REG(addr_port, STA_CFG_REG);
					temp = READ_VPP_REG(data_port);
					temp = (temp & (~0xc0000000)) | (1 << 30);
					temp = (temp & (~0xff0000)) | (24 << 16);
					WRITE_VPP_REG(addr_port, STA_CFG_REG);
					WRITE_VPP_REG(data_port, temp);

					WRITE_VPP_REG(addr_port, LUMA_ADJ1_REG);
					temp = READ_VPP_REG(data_port);
					temp = (temp & (~(0x1fff0000))) | (0 << 16);
					WRITE_VPP_REG(addr_port, LUMA_ADJ1_REG);
					WRITE_VPP_REG(data_port, temp);

					WRITE_VPP_REG(addr_port, STA_SAT_HIST0_REG);
					WRITE_VPP_REG(data_port, 0 | (1 << 24));

					WRITE_VPP_REG(addr_port, STA_SAT_HIST1_REG);
					WRITE_VPP_REG(data_port, 0);
				}
			}
			break;
		case WR_DMA:
			for (i = 0; i < slice_max; i++) {
				addr_port = cm_port.cm_addr_port[i];
				data_port = cm_port.cm_data_port[i];

				WRITE_VPP_REG(addr_port, 0x208);
				temp = READ_VPP_REG(data_port);
				VSYNC_WRITE_VPP_REG_VPP_SEL(addr_port, 0x208, vpp_index);
				VSYNC_WRITE_VPP_REG_VPP_SEL(data_port, temp | 0x1, vpp_index);

				if (chip_type_id == chip_t3x) {
					WRITE_VPP_REG(addr_port, STA_CFG_REG);
					temp = READ_VPP_REG(data_port);
					temp = (temp & (~0xc0000000)) | (1 << 30);
					temp = (temp & (~0xff0000)) | (24 << 16);
					WRITE_VPP_REG(addr_port, STA_CFG_REG);
					WRITE_VPP_REG(data_port, temp);

					WRITE_VPP_REG(addr_port, LUMA_ADJ1_REG);
					temp = READ_VPP_REG(data_port);
					temp = (temp & (~(0x1fff0000))) | (0 << 16);
					WRITE_VPP_REG(addr_port, LUMA_ADJ1_REG);
					WRITE_VPP_REG(data_port, temp);

					WRITE_VPP_REG(addr_port, STA_SAT_HIST0_REG);
					WRITE_VPP_REG(data_port, 0 | (1 << 24));

					WRITE_VPP_REG(addr_port, STA_SAT_HIST1_REG);
					WRITE_VPP_REG(data_port, 0);
				}
			}
			break;
		default:
			break;
		}
	} else
#endif
	{
		if (!(READ_VPP_REG(VPP_MISC) & (0x1 << 28)))
			WRITE_VPP_REG_BITS(VPP_MISC, 1, 28, 1);

		WRITE_VPP_REG(VPP_CHROMA_ADDR_PORT, 0x208);
		temp = READ_VPP_REG(VPP_CHROMA_DATA_PORT);

		switch (md) {
		case WR_VCB:
			if (get_cpu_type() >= MESON_CPU_MAJOR_ID_G12A) {
				if (!(temp & 0x1)) {
					WRITE_VPP_REG(VPP_CHROMA_ADDR_PORT,
						0x208);
					WRITE_VPP_REG(VPP_CHROMA_DATA_PORT,
						temp | 0x1);
				}
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
			} else {
				if (!(temp & 0x2)) {
					WRITE_VPP_REG(VPP_CHROMA_ADDR_PORT,
						0x208);
					WRITE_VPP_REG(VPP_CHROMA_DATA_PORT,
						temp | 0x2);
				}
#endif
			}
			break;
		case WR_DMA:
			if (get_cpu_type() >= MESON_CPU_MAJOR_ID_G12A) {
				if (!(temp & 0x1)) {
					VSYNC_WRITE_VPP_REG_VPP_SEL(VPP_CHROMA_ADDR_PORT,
						0x208, vpp_index);
					VSYNC_WRITE_VPP_REG_VPP_SEL(VPP_CHROMA_DATA_PORT,
						temp | 0x1, vpp_index);
				}
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
			} else {
				if (!(temp & 0x2)) {
					VSYNC_WRITE_VPP_REG_VPP_SEL(VPP_CHROMA_ADDR_PORT,
						0x208, vpp_index);
					VSYNC_WRITE_VPP_REG_VPP_SEL(VPP_CHROMA_DATA_PORT,
						temp | 0x2, vpp_index);
				}
#endif
			}
			break;
		default:
			break;
		}

		/* enable CM histogram by default, mode 0 */
		WRITE_VPP_REG(VPP_CHROMA_ADDR_PORT, STA_CFG_REG);
		temp = READ_VPP_REG(VPP_CHROMA_DATA_PORT);
		temp = (temp & (~0xc0000000)) | (1 << 30);
		temp = (temp & (~0xff0000)) | (24 << 16);
		WRITE_VPP_REG(VPP_CHROMA_ADDR_PORT, STA_CFG_REG);
		WRITE_VPP_REG(VPP_CHROMA_DATA_PORT, temp);

		WRITE_VPP_REG(VPP_CHROMA_ADDR_PORT, LUMA_ADJ1_REG);
		temp = READ_VPP_REG(VPP_CHROMA_DATA_PORT);
		temp = (temp & (~(0x1fff0000))) | (0 << 16);
		WRITE_VPP_REG(VPP_CHROMA_ADDR_PORT, LUMA_ADJ1_REG);
		WRITE_VPP_REG(VPP_CHROMA_DATA_PORT, temp);

		WRITE_VPP_REG(VPP_CHROMA_ADDR_PORT, STA_SAT_HIST0_REG);
		WRITE_VPP_REG(VPP_CHROMA_DATA_PORT, 0 | (1 << 24));

		WRITE_VPP_REG(VPP_CHROMA_ADDR_PORT, STA_SAT_HIST1_REG);
		WRITE_VPP_REG(VPP_CHROMA_DATA_PORT, 0);
	}

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	cm_en_flag = true;
#endif
}

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
void pd_combing_fix_patch(enum pd_comb_fix_lvl_e level)
{
	int i, dejaggy_reg_count;
	struct am_reg_s *dejaggy_reg;

	/* p212 g12a and so on no related register lead to crash*/
	/* so skip the function */
	if (!(is_meson_tl1_cpu() || is_meson_txlx_cpu() ||
		is_meson_tm2_cpu() || is_meson_txl_cpu() ||
		is_meson_txhd_cpu() ||
		get_cpu_type() == MESON_CPU_MAJOR_ID_T5 ||
		get_cpu_type() == MESON_CPU_MAJOR_ID_T5D ||
		get_cpu_type() == MESON_CPU_MAJOR_ID_T7 ||
		get_cpu_type() == MESON_CPU_MAJOR_ID_T3 ||
		get_cpu_type() == MESON_CPU_MAJOR_ID_T5W ||
		chip_type_id == chip_t5m ||
		chip_type_id == chip_t3x ||
		chip_type_id == chip_txhd2))
		return;

	pr_amcm_dbg("\n[amcm..] pd fix lvl = %d\n", level);

	dejaggy_reg_count = sr0_dej_setting[level].length;
	dejaggy_reg = sr0_dej_setting[level].am_reg;
	for (i = 0; i < dejaggy_reg_count; i++) {
		WRITE_VPP_REG(dejaggy_reg[i].addr,
			      (READ_VPP_REG(dejaggy_reg[i].addr) &
			       (~(dejaggy_reg[i].mask))) |
			      (dejaggy_reg[i].val & dejaggy_reg[i].mask));
	}
}

void amcm_level_sel(unsigned int cm_level, int vpp_index)
{
	int temp;

	if (cm_level == 1)
		am_set_regmap(&cmreg_lever1, vpp_index);
	else if (cm_level == 2)
		am_set_regmap(&cmreg_lever2, vpp_index);
	else if (cm_level == 3)
		am_set_regmap(&cmreg_lever3, vpp_index);
	else if (cm_level == 4)
		am_set_regmap(&cmreg_enhancement, vpp_index);
	else
		am_set_regmap(&cmreg_optimize, vpp_index);

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	if (!is_amdv_enable())
#endif
	{
		if (!(READ_VPP_REG(VPP_MISC) & (0x1 << 28)))
			WRITE_VPP_REG_BITS(VPP_MISC, 1, 28, 1);

		WRITE_VPP_REG(VPP_CHROMA_ADDR_PORT, 0x208);
		temp = READ_VPP_REG(VPP_CHROMA_DATA_PORT);
		if (!(temp & 0x2)) {
			WRITE_VPP_REG(VPP_CHROMA_ADDR_PORT, 0x208);
			WRITE_VPP_REG(VPP_CHROMA_DATA_PORT, temp | 0x2);
		}
	}
}

int size_changed_state(struct vframe_s *vf)
{
	int slice_num = 0;
	int hsize[SLICE_MAX] = {0};
	int vsize[SLICE_MAX] = {0};
	static int pre_slice_num;
	static int pre_hsize[SLICE_MAX];
	static int pre_vsize[SLICE_MAX];
	int changed_flag = 0;
	int i;

#if CONFIG_AMLOGIC_MEDIA_VIDEO
	vd_size_info = get_vd_proc_amvecm_info();
	slice_num = vd_size_info->slice_num;
	if (slice_num > 0) {
		for (i = SLICE0; i < slice_num; i++) {
			hsize[i] = vd_size_info->slice[i].hsize;
			vsize[i] = vd_size_info->slice[i].vsize;
			if (debug_amcm & 2)
				pr_amcm_dbg("\n[amcm..] s[%d]: cm size : hsize = %d, vsize = %d\n",
					i, hsize[i], vsize[i]);
		}
	}
#endif

	if (slice_num != pre_slice_num) {
		changed_flag = 1;
		pre_slice_num = slice_num;
	}

	if (slice_num > 0) {
		for (i = SLICE0; i < slice_num; i++) {
			if (hsize[i] != pre_hsize[i] ||
				vsize[i] != pre_vsize[i]) {
				changed_flag = 1;
				pre_hsize[i] = hsize[i];
				pre_vsize[i] = vsize[i];
			}
		}
	}

	return changed_flag;
}

int cm_force_update_flag(void)
{
	return cm_force_flag;
}

void cm_frame_size_s5(struct vframe_s *vf, int vpp_index)
{
	unsigned int vpp_size, width, height;
	int addr_port;
	int data_port;
	struct cm_port_s cm_port;
	int slice_num;
	static int en_flag = 1;
	int i;
	int changed_flag;
	int slice_max;
	int overlap = 0;

#if CONFIG_AMLOGIC_MEDIA_VIDEO
	vd_size_info = get_vd_proc_amvecm_info();
	slice_num = vd_size_info->slice_num;
#endif

	if (!cm_en)
		return;

	if (!cm_en_flag && !cm_dis_flag)
		amcm_enable(WR_DMA, vpp_index);

	slice_max = get_slice_max();

	cm_port = get_cm_port();

	/*because size from vpp will delay, need reconfig for cm*/
	cm_force_flag = 0;

	if (en_flag && !vf) {
		/*default 4k size*/
		width = 0xf00;
		height = 0x870;
		vpp_size = width | (height << 16);

		for (i = SLICE0; i < slice_max; i++) {
			addr_port = cm_port.cm_addr_port[i];
			data_port = cm_port.cm_data_port[i];

			VSYNC_WRITE_VPP_REG_VPP_SEL(addr_port,
				0x205, vpp_index);
			VSYNC_WRITE_VPP_REG_VPP_SEL(data_port,
				vpp_size, vpp_index);

			if (chip_type_id == chip_s5) {
				VSYNC_WRITE_VPP_REG_VPP_SEL(addr_port,
					0x209, vpp_index);
				VSYNC_WRITE_VPP_REG_VPP_SEL(data_port,
					width << 16, vpp_index);
				VSYNC_WRITE_VPP_REG_VPP_SEL(addr_port,
					0x20a, vpp_index);
				VSYNC_WRITE_VPP_REG_VPP_SEL(data_port,
					height << 16, vpp_index);
			}

			if (chip_type_id == chip_t3x) {
				VSYNC_WRITE_VPP_REG_VPP_SEL(addr_port,
					STA_WIN_XYXY0_REG, vpp_index);
				VSYNC_WRITE_VPP_REG_VPP_SEL(data_port,
					0 | (width << 16), vpp_index);
				VSYNC_WRITE_VPP_REG_VPP_SEL(addr_port,
					STA_WIN_XYXY1_REG, vpp_index);
				VSYNC_WRITE_VPP_REG_VPP_SEL(data_port,
					0 | (height << 16), vpp_index);
			}
		}
		en_flag = 0;
#if CONFIG_AMLOGIC_MEDIA_VIDEO
		memset(vd_size_info, 0, sizeof(struct vd_proc_amvecm_info_t));
#endif
		pr_amcm_dbg("\n[amcm..] s5 set def 4k cm size : %x\n",
			vpp_size);
	} else if (vf) {
		changed_flag = size_changed_state(vf);
		if (slice_num && changed_flag) {
			cm_force_flag = 1;

			for (i = SLICE0; i < slice_num; i++) {
#if CONFIG_AMLOGIC_MEDIA_VIDEO
				if (slice_num == 2)
					overlap = vd_size_info->slice[0].vd1_overlap;
				else
					overlap = 0;

				width = vd_size_info->slice[0].vd1_slice_in_hsize - overlap;
				height = vd_size_info->slice[0].vd1_slice_in_vsize;
#else
				width = 0xf00;
				height = 0x870;
#endif
				vpp_size = width | (height << 16);

				addr_port = cm_port.cm_addr_port[i];
				data_port = cm_port.cm_data_port[i];

				VSYNC_WRITE_VPP_REG_VPP_SEL(addr_port,
					0x205, vpp_index);
				VSYNC_WRITE_VPP_REG_VPP_SEL(data_port,
					vpp_size, vpp_index);

				if (chip_type_id == chip_s5) {
					VSYNC_WRITE_VPP_REG_VPP_SEL(addr_port,
						0x209, vpp_index);
					VSYNC_WRITE_VPP_REG_VPP_SEL(data_port,
						width << 16, vpp_index);
					VSYNC_WRITE_VPP_REG_VPP_SEL(addr_port,
						0x20a, vpp_index);
					VSYNC_WRITE_VPP_REG_VPP_SEL(data_port,
						height << 16, vpp_index);
				}

				if (chip_type_id == chip_t3x) {
					VSYNC_WRITE_VPP_REG_VPP_SEL(addr_port,
						STA_WIN_XYXY0_REG, vpp_index);
					VSYNC_WRITE_VPP_REG_VPP_SEL(data_port,
						0 | (width << 16), vpp_index);
					VSYNC_WRITE_VPP_REG_VPP_SEL(addr_port,
						STA_WIN_XYXY1_REG, vpp_index);
					VSYNC_WRITE_VPP_REG_VPP_SEL(data_port,
						0 | (height << 16), vpp_index);
				}
				pr_amcm_dbg("\n[amcm..] s5: cm size : %x, slice_num = %d, cm_fc_flag = %d\n",
					vpp_size, slice_num, cm_force_flag);
			}
		}
		en_flag = 1;
	}
}

void cm2_frame_size_patch(struct vframe_s *vf,
	unsigned int width, unsigned int height, int vpp_index)
{
	unsigned int vpp_size;
	int addr_port;
	int data_port;

#if CONFIG_AMLOGIC_MEDIA_VIDEO
	if (chip_type_id == chip_s5 ||
		chip_type_id == chip_t3x) {
		cm_frame_size_s5(vf, vpp_index);
		return;
	}
#endif

	addr_port = VPP_CHROMA_ADDR_PORT;
	data_port = VPP_CHROMA_DATA_PORT;

	if (!cm_en)
		return;
	else if (width < cm_width_limit)
		amcm_disable(WR_VCB, 0);/*(WR_DMA);*/
	else if (!cm_en_flag && !cm_dis_flag)
		amcm_enable(WR_DMA, vpp_index);

	vpp_size = width | (height << 16);
	if (cm_size != vpp_size) {
		VSYNC_WRITE_VPP_REG_VPP_SEL(addr_port, 0x205, vpp_index);
		VSYNC_WRITE_VPP_REG_VPP_SEL(data_port, vpp_size, vpp_index);
		//VSYNC_WRITE_VPP_REG(addr_port, 0x209);
		//VSYNC_WRITE_VPP_REG(data_port, width << 16);
		//VSYNC_WRITE_VPP_REG(addr_port, 0x20a);
		///VSYNC_WRITE_VPP_REG(data_port, height << 16);
		/* default set full size for CM histogram */
		VSYNC_WRITE_VPP_REG_VPP_SEL(addr_port, STA_WIN_XYXY0_REG, vpp_index);
		VSYNC_WRITE_VPP_REG_VPP_SEL(data_port, 0 | (width << 16), vpp_index);
		VSYNC_WRITE_VPP_REG_VPP_SEL(addr_port, STA_WIN_XYXY1_REG, vpp_index);
		VSYNC_WRITE_VPP_REG_VPP_SEL(data_port, 0 | (height << 16), vpp_index);
		cm_size =  vpp_size;
		pr_amcm_dbg("\n[amcm..]set cm size from scaler: %x, ",
			vpp_size);
		pr_amcm_dbg("set demo mode  %x\n", cm2_patch_flag);
	}
}

/* set the frame size for cm2 demo*/
void cm2_frame_switch_patch(void)
{
	int addr_port;
	int data_port;
	struct cm_port_s cm_port;

	if (chip_type_id == chip_s7)
		return;

	if (chip_type_id == chip_s5 ||
		chip_type_id == chip_t3x) {
		cm_port = get_cm_port();
		addr_port = cm_port.cm_addr_port[SLICE0];
		data_port = cm_port.cm_data_port[SLICE0];
	} else {
		addr_port = VPP_CHROMA_ADDR_PORT;
		data_port = VPP_CHROMA_DATA_PORT;
	}

	WRITE_VPP_REG(addr_port, 0x20f);
	WRITE_VPP_REG(data_port, cm2_patch_flag);
}
#endif
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT_C1A
void cm_regmap_latch(struct am_regs_s *am_regs, unsigned int reg_map, int vpp_index)
{
	am_set_regmap(am_regs, vpp_index);
	vecm_latch_flag &= ~reg_map;
	pr_amcm_dbg("\n[amcm..] load reg %d table OK!!!\n", reg_map);
}

void cm_latch_process(int vpp_index)
{
	/*if ((vecm_latch_flag & FLAG_REG_MAP0) ||*/
	/*(vecm_latch_flag & FLAG_REG_MAP1) ||*/
	/*(vecm_latch_flag & FLAG_REG_MAP2) ||*/
	/*(vecm_latch_flag & FLAG_REG_MAP3) ||*/
	/*(vecm_latch_flag & FLAG_REG_MAP4) ||*/
	/* (vecm_latch_flag & FLAG_REG_MAP5)){*/
	do {
		if (vecm_latch_flag & FLAG_REG_MAP0)
			cm_regmap_latch(&amregs0, FLAG_REG_MAP0, vpp_index);
		if (vecm_latch_flag & FLAG_REG_MAP1)
			cm_regmap_latch(&amregs1, FLAG_REG_MAP1, vpp_index);
		if (vecm_latch_flag & FLAG_REG_MAP2)
			cm_regmap_latch(&amregs2, FLAG_REG_MAP2, vpp_index);
		if (vecm_latch_flag & FLAG_REG_MAP3)
			cm_regmap_latch(&amregs3, FLAG_REG_MAP3, vpp_index);
		if (vecm_latch_flag & FLAG_REG_MAP4)
			cm_regmap_latch(&amregs4, FLAG_REG_MAP4, vpp_index);
		if (vecm_latch_flag & FLAG_REG_MAP5)
			cm_regmap_latch(&amregs5, FLAG_REG_MAP5, vpp_index);
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
		if ((cm2_patch_flag & 0xff) > 0)
			cm2_frame_switch_patch();
#endif
	} while (0);

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	if (cm_en && cm_level_last != cm_level) {
		cm_level_last = cm_level;
		amcm_enable(WR_DMA, vpp_index);
		pr_amcm_dbg("\n[amcm..] set cm2 load OK!!!\n");
	} else if ((cm_en == 0) && (cm_level_last != 0xff)) {
		cm_level_last = 0xff;
		amcm_disable(WR_DMA, vpp_index);/* CM manage disable */
		pr_amcm_dbg("\n[amcm..] set cm disable!!!\n");
	}
#endif
}

static int amvecm_regmap_info(struct am_regs_s *p)
{
	unsigned short i;

	for (i = 0; i < p->length; i++) {
		switch (p->am_reg[i].type) {
		case REG_TYPE_PHY:
			pr_info("%s:%d bus type: phy...\n", __func__, i);
			break;
		case REG_TYPE_CBUS:
			pr_info("%s:%-3d cbus: 0x%-4x=0x%-8x (%-5u)=(%-10u)",
				__func__, i, p->am_reg[i].addr,
				(p->am_reg[i].val & p->am_reg[i].mask),
				p->am_reg[i].addr,
				(p->am_reg[i].val & p->am_reg[i].mask));
			pr_info(" mask=%-8x(%u)\n",
				p->am_reg[i].mask,
				p->am_reg[i].mask);
			break;
		case REG_TYPE_APB:
			pr_info("%s:%-3d apb: 0x%-4x=0x%-8x (%-5u)=(%-10u)",
				__func__, i, p->am_reg[i].addr,
				(p->am_reg[i].val & p->am_reg[i].mask),
				p->am_reg[i].addr,
				(p->am_reg[i].val & p->am_reg[i].mask));
			pr_info(" mask=%-8x(%u)\n",
				p->am_reg[i].mask,
				p->am_reg[i].mask);
			break;
		case REG_TYPE_MPEG:
			pr_info("%s:%-3d mpeg: 0x%-4x=0x%-8x (%-5u)=(%-10u)",
				__func__, i, p->am_reg[i].addr,
				(p->am_reg[i].val & p->am_reg[i].mask),
				p->am_reg[i].addr,
				(p->am_reg[i].val & p->am_reg[i].mask));
			pr_info(" mask=%-8x(%u)\n",
				p->am_reg[i].mask,
				p->am_reg[i].mask);
			break;
		case REG_TYPE_AXI:
			pr_info("%s:%-3d axi: 0x%-4x=0x%-8x (%-5u)=(%-10u)",
				__func__, i, p->am_reg[i].addr,
				(p->am_reg[i].val & p->am_reg[i].mask),
				p->am_reg[i].addr,
				(p->am_reg[i].val & p->am_reg[i].mask));
			pr_info(" mask=%-8x(%u)\n",
				p->am_reg[i].mask,
				p->am_reg[i].mask);
			break;
		case REG_TYPE_INDEX_VPPCHROMA:
			pr_info("%s:%-3d chroma: 0x%-4x=0x%-8x (%-5u)=(%-10u)",
				__func__, i, p->am_reg[i].addr,
				(p->am_reg[i].val & p->am_reg[i].mask),
				p->am_reg[i].addr,
				(p->am_reg[i].val & p->am_reg[i].mask));
			pr_info(" mask=%-8x(%u)\n",
				p->am_reg[i].mask,
				p->am_reg[i].mask);
			break;
		case REG_TYPE_INDEX_GAMMA:
			pr_info("%s:%-3d bus type: REG_TYPE_INDEX_GAMMA...\n",
				__func__, i);
			break;
		case VALUE_TYPE_CONTRAST_BRIGHTNESS:
			pr_info("%s:%-3d bus type: VALUE_TYPE_CON_BRIT\n",
				__func__, i);
			break;
		case REG_TYPE_INDEX_VPP_COEF:
			pr_info("%s:%-3d vpp coef: 0x%-4x=0x%-8x (%-5u)=(%-10u)",
				__func__, i, p->am_reg[i].addr,
				(p->am_reg[i].val & p->am_reg[i].mask),
				p->am_reg[i].addr,
				(p->am_reg[i].val & p->am_reg[i].mask));
			pr_info(" mask=%-8x(%u)\n",
				p->am_reg[i].mask,
				p->am_reg[i].mask);
			break;
		case REG_TYPE_VCBUS:
			pr_info("%s:%-3d vcbus: 0x%-4x=0x%-8x (%-5u)=(%-10u)",
				__func__, i, p->am_reg[i].addr,
				(p->am_reg[i].val & p->am_reg[i].mask),
				p->am_reg[i].addr,
				(p->am_reg[i].val & p->am_reg[i].mask));
			pr_info(" mask=%-8x(%u)\n",	p->am_reg[i].mask,
				p->am_reg[i].mask);
			break;
		default:
			pr_info("%s:%3d bus type error!!!bustype = 0x%x...\n",
				__func__, i, p->am_reg[i].type);
			break;
		}
	}
	return 0;
}

static long amvecm_regmap_set(struct am_regs_s *regs,
			      struct am_regs_s *arg, unsigned int reg_map)
{
	int ret = 0;

	if (!(memcpy(regs, arg, sizeof(struct am_regs_s)))) {
		pr_amcm_dbg("[amcm..]%s 0x%x: can't get buffer length\n",
			    __func__,
			    reg_map);
		return -EFAULT;
	}
	if (!regs->length || regs->length > am_reg_size) {
		pr_amcm_dbg("[amcm..]%s 0x%x: buf length error! len = 0x%x\n",
			    __func__,
			    reg_map, regs->length);
		return -EINVAL;
	}
	pr_amcm_dbg("\n[amcm..]%s 0x%x reg length=0x%x\n",
		    __func__, reg_map, regs->length);

	if (debug_regload)
		amvecm_regmap_info(regs);

	vecm_latch_flag |= reg_map;

	return ret;
}

int cm_load_reg(struct am_regs_s *arg)
{
	int ret = 0;
	/*force set cm size to 0,enable check vpp size*/
	/*cm_size = 0;*/
	if (!(vecm_latch_flag & FLAG_REG_MAP0))
		ret = amvecm_regmap_set(&amregs0, arg, FLAG_REG_MAP0);
	else if (!(vecm_latch_flag & FLAG_REG_MAP1))
		ret = amvecm_regmap_set(&amregs1, arg, FLAG_REG_MAP1);
	else if (!(vecm_latch_flag & FLAG_REG_MAP2))
		ret = amvecm_regmap_set(&amregs2, arg, FLAG_REG_MAP2);
	else if (!(vecm_latch_flag & FLAG_REG_MAP3))
		ret = amvecm_regmap_set(&amregs3, arg, FLAG_REG_MAP3);
	else if (!(vecm_latch_flag & FLAG_REG_MAP4))
		ret = amvecm_regmap_set(&amregs4, arg, FLAG_REG_MAP4);
	else if (!(vecm_latch_flag & FLAG_REG_MAP5))
		ret = amvecm_regmap_set(&amregs5, arg, FLAG_REG_MAP5);

	return ret;
}
#endif
