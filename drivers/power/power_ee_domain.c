// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/bitfield.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/of_device.h>
#include <linux/reset-controller.h>
#include <linux/reset.h>
#include <linux/clk.h>
#include <linux/ctype.h>
#include <dt-bindings/power/amlogic-g12a-power.h>
#include <dt-bindings/power/amlogic-sm1-power.h>
#include <dt-bindings/power/amlogic-tm2-power.h>

/* AO Offsets */

#define AO_RTI_GEN_PWR_SLEEP0		(0x3a << 2)
#define AO_RTI_GEN_PWR_ISO0		(0x3b << 2)

/* HHI Offsets */

#define HHI_MEM_PD_REG0			(0x40 << 2)
#define HHI_VPU_MEM_PD_REG0		(0x41 << 2)
#define HHI_VPU_MEM_PD_REG1		(0x42 << 2)
#define HHI_VPU_MEM_PD_REG3		(0x43 << 2)
#define HHI_VPU_MEM_PD_REG4		(0x44 << 2)
#define HHI_AUDIO_MEM_PD_REG0		(0x45 << 2)
#define HHI_NANOQ_MEM_PD_REG0		(0x46 << 2)
#define HHI_NANOQ_MEM_PD_REG1		(0x47 << 2)
#define HHI_VPU_MEM_PD_REG2		(0x4d << 2)
#define TM2_HHI_VPU_MEM_PD_REG3		(0x4e << 2)
#define TM2_HHI_VPU_MEM_PD_REG4		(0x4c << 2)
#define HHI_DEMOD_MEM_PD_REG		(0x043 << 2)
#define HHI_DSP_MEM_PD_REG0		(0x044 << 2)
#define HHI_MEM_PD_REG1			(0x35 << 2)

#define DOS_MEM_PD_VDEC			(0x0 << 2)
#define DOS_MEM_PD_HCODEC		(0x2 << 2)
#define DOS_MEM_PD_HEVC			(0x3 << 2)
#define DOS_MEM_PD_WAVE420L		(0x9 << 2)

#define DOS_START_ID			7
#define DOS_END_ID			10

struct meson_ee_pwrc;
struct meson_ee_pwrc_domain;

struct meson_ee_pwrc_mem_domain {
	unsigned int reg;
	unsigned int mask;
};

struct meson_ee_pwrc_top_domain {
	unsigned int sleep_reg;
	unsigned int sleep_mask;
	unsigned int iso_reg;
	unsigned int iso_mask;
};

struct meson_ee_pwrc_domain_desc {
	char *name;
	unsigned int reset_names_count;
	unsigned int clk_names_count;
	unsigned int domain_id;
	struct meson_ee_pwrc_top_domain *top_pd;
	unsigned int mem_pd_count;
	struct meson_ee_pwrc_mem_domain *mem_pd;
	unsigned int flags;
	bool (*get_power)(struct meson_ee_pwrc_domain *pwrc_domain);
};

struct meson_ee_pwrc_domain_data {
	unsigned int count;
	struct meson_ee_pwrc_domain_desc *domains;
};

/* TOP Power Domains */

static struct meson_ee_pwrc_top_domain g12a_pwrc_vpu = {
	.sleep_reg = AO_RTI_GEN_PWR_SLEEP0,
	.sleep_mask = BIT(8),
	.iso_reg = AO_RTI_GEN_PWR_SLEEP0,
	.iso_mask = BIT(9),
};

#define SM1_EE_PD(__bit, __set)\
	{\
		.sleep_reg = AO_RTI_GEN_PWR_SLEEP0,\
		.sleep_mask = BIT(__bit),\
		.iso_reg = AO_RTI_GEN_PWR_ISO0,\
		.iso_mask = BIT(__set), \
	}

static struct meson_ee_pwrc_top_domain sm1_pwrc_vpu = SM1_EE_PD(8, 8);
static struct meson_ee_pwrc_top_domain sm1_pwrc_nna = SM1_EE_PD(16, 16);
static struct meson_ee_pwrc_top_domain sm1_pwrc_usb = SM1_EE_PD(17, 17);
static struct meson_ee_pwrc_top_domain sm1_pwrc_pci = SM1_EE_PD(18, 18);
static struct meson_ee_pwrc_top_domain sm1_pwrc_ge2d = SM1_EE_PD(19, 19);
static struct meson_ee_pwrc_top_domain sm1_pwrc_vdec = SM1_EE_PD(1, 1);
static struct meson_ee_pwrc_top_domain sm1_pwrc_hcodec = SM1_EE_PD(0, 0);
static struct meson_ee_pwrc_top_domain sm1_pwrc_hevc = SM1_EE_PD(2, 2);
static struct meson_ee_pwrc_top_domain sm1_pwrc_wave420l = SM1_EE_PD(3, 3);
static struct meson_ee_pwrc_top_domain sm1_pwrc_csi = SM1_EE_PD(6, 6);

#define TM2_EE_PD(__bit, __set)					\
	{							\
		.sleep_reg = AO_RTI_GEN_PWR_SLEEP0,		\
		.sleep_mask = BIT(__bit),			\
		.iso_reg = AO_RTI_GEN_PWR_ISO0,			\
		.iso_mask = BIT(__set),				\
	}

static struct meson_ee_pwrc_top_domain tm2_pwrc_vpu = TM2_EE_PD(8, 8);
static struct meson_ee_pwrc_top_domain tm2_pwrc_nna = TM2_EE_PD(16, 16);
static struct meson_ee_pwrc_top_domain tm2_pwrc_usb = TM2_EE_PD(17, 17);
static struct meson_ee_pwrc_top_domain tm2_pwrc_pciea = TM2_EE_PD(18, 18);
static struct meson_ee_pwrc_top_domain tm2_pwrc_ge2d = TM2_EE_PD(19, 19);
static struct meson_ee_pwrc_top_domain tm2_pwrc_vdec = TM2_EE_PD(1, 1);
static struct meson_ee_pwrc_top_domain tm2_pwrc_hcodec = TM2_EE_PD(0, 0);
static struct meson_ee_pwrc_top_domain tm2_pwrc_hevc = TM2_EE_PD(2, 2);
static struct meson_ee_pwrc_top_domain tm2_pwrc_wave420l = TM2_EE_PD(3, 3);
static struct meson_ee_pwrc_top_domain tm2_pwrc_pcieb = TM2_EE_PD(20, 20);
static struct meson_ee_pwrc_top_domain tm2_pwrc_dspa = TM2_EE_PD(21, 21);
static struct meson_ee_pwrc_top_domain tm2_pwrc_dspb = TM2_EE_PD(22, 22);
static struct meson_ee_pwrc_top_domain tm2_pwrc_demod = TM2_EE_PD(23, 23);
static struct meson_ee_pwrc_top_domain tm2_pwrc_audio = TM2_EE_PD(11, 11);
static struct meson_ee_pwrc_top_domain tm2_pwrc_emmcb = TM2_EE_PD(9, 9);
static struct meson_ee_pwrc_top_domain tm2_pwrc_emmcc = TM2_EE_PD(10, 10);
static struct meson_ee_pwrc_top_domain tm2_pwrc_tvfe = TM2_EE_PD(12, 12);
static struct meson_ee_pwrc_top_domain tm2_pwrc_acodec = TM2_EE_PD(13, 13);
static struct meson_ee_pwrc_top_domain tm2_pwrc_atvdemod = TM2_EE_PD(14, 14);

static struct meson_ee_pwrc_mem_domain g12a_pwrc_mem_vpu[] = {
	{ HHI_VPU_MEM_PD_REG0, GENMASK(1, 0) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(3, 2) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(5, 4) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(7, 6) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(9, 8) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(11, 10) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(13, 12) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(15, 14) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(17, 16) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(19, 18) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(21, 20) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(23, 22) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(25, 24) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(27, 26) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(29, 28) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(31, 30) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(1, 0) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(3, 2) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(5, 4) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(7, 6) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(9, 8) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(11, 10) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(13, 12) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(15, 14) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(17, 16) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(19, 18) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(21, 20) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(23, 22) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(25, 24) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(27, 26) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(29, 28) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(31, 30) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(1, 0) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(3, 2) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(5, 4) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(7, 6) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(9, 8) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(11, 10) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(13, 12) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(15, 14) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(17, 16) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(19, 18) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(21, 20) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(23, 22) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(25, 24) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(27, 26) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(29, 28) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(31, 30) },
	{ HHI_MEM_PD_REG0, BIT(8) },
	{ HHI_MEM_PD_REG0, BIT(9) },
	{ HHI_MEM_PD_REG0, BIT(10) },
	{ HHI_MEM_PD_REG0, BIT(11) },
	{ HHI_MEM_PD_REG0, BIT(12) },
	{ HHI_MEM_PD_REG0, BIT(13) },
	{ HHI_MEM_PD_REG0, BIT(14) },
	{ HHI_MEM_PD_REG0, BIT(15) }
};

static struct meson_ee_pwrc_mem_domain g12a_pwrc_mem_eth[] = {
	{ HHI_MEM_PD_REG0, GENMASK(3, 2) },
};

static struct meson_ee_pwrc_mem_domain sm1_pwrc_mem_vpu[] = {
	{ HHI_VPU_MEM_PD_REG0, GENMASK(1, 0) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(3, 2) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(5, 4) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(7, 6) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(9, 8) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(11, 10) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(13, 12) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(15, 14) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(17, 16) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(19, 18) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(21, 20) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(23, 22) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(25, 24) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(27, 26) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(29, 28) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(31, 30) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(1, 0) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(3, 2) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(5, 4) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(7, 6) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(9, 8) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(11, 10) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(13, 12) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(15, 14) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(17, 16) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(19, 18) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(21, 20) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(23, 22) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(25, 24) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(27, 26) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(29, 28) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(31, 30) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(1, 0) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(3, 2) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(5, 4) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(7, 6) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(9, 8) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(11, 10) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(13, 12) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(15, 14) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(17, 16) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(19, 18) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(21, 20) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(23, 22) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(25, 24) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(27, 26) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(29, 28) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(31, 30) },
	{ HHI_VPU_MEM_PD_REG3, GENMASK(1, 0) },
	{ HHI_VPU_MEM_PD_REG3, GENMASK(3, 2) },
	{ HHI_VPU_MEM_PD_REG3, GENMASK(5, 4) },
	{ HHI_VPU_MEM_PD_REG3, GENMASK(7, 6) },
	{ HHI_VPU_MEM_PD_REG3, GENMASK(9, 8) },
	{ HHI_VPU_MEM_PD_REG3, GENMASK(11, 10) },
	{ HHI_VPU_MEM_PD_REG3, GENMASK(13, 12) },
	{ HHI_VPU_MEM_PD_REG3, GENMASK(15, 14) },
	{ HHI_VPU_MEM_PD_REG3, GENMASK(17, 16) },
	{ HHI_VPU_MEM_PD_REG3, GENMASK(19, 18) },
	{ HHI_VPU_MEM_PD_REG3, GENMASK(21, 20) },
	{ HHI_VPU_MEM_PD_REG3, GENMASK(23, 22) },
	{ HHI_VPU_MEM_PD_REG3, GENMASK(25, 24) },
	{ HHI_VPU_MEM_PD_REG3, GENMASK(27, 26) },
	{ HHI_VPU_MEM_PD_REG3, GENMASK(29, 28) },
	{ HHI_VPU_MEM_PD_REG3, GENMASK(31, 30) },
	{ HHI_VPU_MEM_PD_REG4, GENMASK(1, 0) },
	{ HHI_VPU_MEM_PD_REG4, GENMASK(3, 2) },
	{ HHI_VPU_MEM_PD_REG4, GENMASK(5, 4) },
	{ HHI_VPU_MEM_PD_REG4, GENMASK(7, 6) },
	{ HHI_MEM_PD_REG0, BIT(8) },
	{ HHI_MEM_PD_REG0, BIT(9) },
	{ HHI_MEM_PD_REG0, BIT(10) },
	{ HHI_MEM_PD_REG0, BIT(11) },
	{ HHI_MEM_PD_REG0, BIT(12) },
	{ HHI_MEM_PD_REG0, BIT(13) },
	{ HHI_MEM_PD_REG0, BIT(14) },
	{ HHI_MEM_PD_REG0, BIT(15) }
};

static struct meson_ee_pwrc_mem_domain sm1_pwrc_mem_nna[] = {
	{ HHI_NANOQ_MEM_PD_REG0, 0xffffffff },
	{ HHI_NANOQ_MEM_PD_REG1, 0xffffffff },
};

static struct meson_ee_pwrc_mem_domain sm1_pwrc_mem_csi[] = {
	{ HHI_MEM_PD_REG0, GENMASK(7, 6) },
};

static struct meson_ee_pwrc_mem_domain sm1_pwrc_mem_usb[] = {
	{ HHI_MEM_PD_REG0, GENMASK(31, 30) },
};

static struct meson_ee_pwrc_mem_domain sm1_pwrc_mem_pcie[] = {
	{ HHI_MEM_PD_REG0, GENMASK(29, 26) },
};

static struct meson_ee_pwrc_mem_domain sm1_pwrc_mem_ge2d[] = {
	{ HHI_MEM_PD_REG0, GENMASK(25, 18) },
};

static struct meson_ee_pwrc_mem_domain sm1_pwrc_mem_audio[] = {
	{ HHI_MEM_PD_REG0, GENMASK(5, 4) },
	{ HHI_AUDIO_MEM_PD_REG0, GENMASK(1, 0) },
	{ HHI_AUDIO_MEM_PD_REG0, GENMASK(3, 2) },
	{ HHI_AUDIO_MEM_PD_REG0, GENMASK(5, 4) },
	{ HHI_AUDIO_MEM_PD_REG0, GENMASK(7, 6) },
	{ HHI_AUDIO_MEM_PD_REG0, GENMASK(13, 12) },
	{ HHI_AUDIO_MEM_PD_REG0, GENMASK(15, 14) },
	{ HHI_AUDIO_MEM_PD_REG0, GENMASK(17, 16) },
	{ HHI_AUDIO_MEM_PD_REG0, GENMASK(19, 18) },
	{ HHI_AUDIO_MEM_PD_REG0, GENMASK(21, 20) },
	{ HHI_AUDIO_MEM_PD_REG0, GENMASK(23, 22) },
	{ HHI_AUDIO_MEM_PD_REG0, GENMASK(25, 24) },
	{ HHI_AUDIO_MEM_PD_REG0, GENMASK(27, 26) },
};

static struct meson_ee_pwrc_mem_domain sm1_pwrc_mem_vdec[] = {
	{ DOS_MEM_PD_VDEC, GENMASK(31, 0) },
};

static struct meson_ee_pwrc_mem_domain sm1_pwrc_mem_hcodec[] = {
	{ DOS_MEM_PD_HCODEC, GENMASK(31, 0) },
};

static struct meson_ee_pwrc_mem_domain sm1_pwrc_mem_hevc[] = {
	{ DOS_MEM_PD_HEVC, GENMASK(31, 0) },
};

static struct meson_ee_pwrc_mem_domain sm1_pwrc_mem_wave420l[] = {
	{ DOS_MEM_PD_WAVE420L, GENMASK(31, 0) },
};

static struct meson_ee_pwrc_mem_domain tm2_pwrc_mem_vpu[] = {
	{ HHI_VPU_MEM_PD_REG0, GENMASK(1, 0) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(3, 2) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(5, 4) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(7, 6) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(9, 8) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(11, 10) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(13, 12) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(15, 14) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(17, 16) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(19, 18) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(21, 20) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(23, 22) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(25, 24) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(27, 26) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(29, 28) },
	{ HHI_VPU_MEM_PD_REG0, GENMASK(31, 30) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(1, 0) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(3, 2) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(5, 4) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(7, 6) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(9, 8) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(11, 10) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(13, 12) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(15, 14) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(17, 16) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(19, 18) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(21, 20) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(23, 22) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(25, 24) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(27, 26) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(29, 28) },
	{ HHI_VPU_MEM_PD_REG1, GENMASK(31, 30) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(1, 0) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(3, 2) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(5, 4) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(7, 6) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(9, 8) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(11, 10) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(13, 12) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(15, 14) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(17, 16) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(19, 18) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(21, 20) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(23, 22) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(25, 24) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(27, 26) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(29, 28) },
	{ HHI_VPU_MEM_PD_REG2, GENMASK(31, 30) },
	{ TM2_HHI_VPU_MEM_PD_REG3, GENMASK(1, 0) },
	{ TM2_HHI_VPU_MEM_PD_REG3, GENMASK(3, 2) },
	{ TM2_HHI_VPU_MEM_PD_REG3, GENMASK(5, 4) },
	{ TM2_HHI_VPU_MEM_PD_REG3, GENMASK(7, 6) },
	{ TM2_HHI_VPU_MEM_PD_REG3, GENMASK(9, 8) },
	{ TM2_HHI_VPU_MEM_PD_REG3, GENMASK(11, 10) },
	{ TM2_HHI_VPU_MEM_PD_REG3, GENMASK(13, 12) },
	{ TM2_HHI_VPU_MEM_PD_REG3, GENMASK(15, 14) },
	{ TM2_HHI_VPU_MEM_PD_REG3, GENMASK(17, 16) },
	{ TM2_HHI_VPU_MEM_PD_REG3, GENMASK(19, 18) },
	{ TM2_HHI_VPU_MEM_PD_REG3, GENMASK(21, 20) },
	{ TM2_HHI_VPU_MEM_PD_REG3, GENMASK(23, 22) },
	{ TM2_HHI_VPU_MEM_PD_REG3, GENMASK(25, 24) },
	{ TM2_HHI_VPU_MEM_PD_REG3, GENMASK(27, 26) },
	{ TM2_HHI_VPU_MEM_PD_REG3, GENMASK(29, 28) },
	{ TM2_HHI_VPU_MEM_PD_REG3, GENMASK(31, 30) },
	{ TM2_HHI_VPU_MEM_PD_REG4, GENMASK(1, 0) },
	{ TM2_HHI_VPU_MEM_PD_REG4, GENMASK(3, 2) },
	{ TM2_HHI_VPU_MEM_PD_REG4, GENMASK(5, 4) },
	{ TM2_HHI_VPU_MEM_PD_REG4, GENMASK(7, 6) },
	{ TM2_HHI_VPU_MEM_PD_REG4, GENMASK(9, 8) },
	{ TM2_HHI_VPU_MEM_PD_REG4, GENMASK(11, 10) },
	{ TM2_HHI_VPU_MEM_PD_REG4, GENMASK(13, 12) },
	{ TM2_HHI_VPU_MEM_PD_REG4, GENMASK(15, 14) },
	{ TM2_HHI_VPU_MEM_PD_REG4, GENMASK(17, 16) },
	{ TM2_HHI_VPU_MEM_PD_REG4, GENMASK(19, 18) },
	{ TM2_HHI_VPU_MEM_PD_REG4, GENMASK(21, 20) },
	{ TM2_HHI_VPU_MEM_PD_REG4, GENMASK(23, 22) },
	{ TM2_HHI_VPU_MEM_PD_REG4, GENMASK(25, 24) },
	{ TM2_HHI_VPU_MEM_PD_REG4, GENMASK(27, 26) },
	{ TM2_HHI_VPU_MEM_PD_REG4, GENMASK(29, 28) },
	{ TM2_HHI_VPU_MEM_PD_REG4, GENMASK(31, 30) },
	{ HHI_MEM_PD_REG0, BIT(8) },
	{ HHI_MEM_PD_REG0, BIT(9) },
	{ HHI_MEM_PD_REG0, BIT(10) },
	{ HHI_MEM_PD_REG0, BIT(11) },
	{ HHI_MEM_PD_REG0, BIT(12) },
	{ HHI_MEM_PD_REG0, BIT(13) },
	{ HHI_MEM_PD_REG0, BIT(14) },
	{ HHI_MEM_PD_REG0, BIT(15) }
};

static struct meson_ee_pwrc_mem_domain tm2_pwrc_mem_nna[] = {
	{ HHI_NANOQ_MEM_PD_REG0, GENMASK(31, 0) },
	{ HHI_NANOQ_MEM_PD_REG1, GENMASK(31, 0) },
};

static struct meson_ee_pwrc_mem_domain tm2_pwrc_mem_usb[] = {
	{ HHI_MEM_PD_REG0, GENMASK(31, 30) },
};

static struct meson_ee_pwrc_mem_domain tm2_pwrc_mem_pciea[] = {
	{ HHI_MEM_PD_REG0, GENMASK(29, 26) },
};

static struct meson_ee_pwrc_mem_domain tm2_pwrc_mem_ge2d[] = {
	{ HHI_MEM_PD_REG0, GENMASK(25, 18) },
};

static struct meson_ee_pwrc_mem_domain tm2_pwrc_mem_audio[] = {
	{ HHI_AUDIO_MEM_PD_REG0, GENMASK(31, 16) },
};

static struct meson_ee_pwrc_mem_domain tm2_pwrc_mem_vdec[] = {
	{ DOS_MEM_PD_VDEC, GENMASK(31, 0) },
};

static struct meson_ee_pwrc_mem_domain tm2_pwrc_mem_hcodec[] = {
	{ DOS_MEM_PD_HCODEC, GENMASK(31, 0) },
};

static struct meson_ee_pwrc_mem_domain tm2_pwrc_mem_hevc[] = {
	{ DOS_MEM_PD_HEVC, GENMASK(31, 0) },
};

static struct meson_ee_pwrc_mem_domain tm2_pwrc_mem_wave420l[] = {
	{ DOS_MEM_PD_WAVE420L, GENMASK(31, 0) },
};

static struct meson_ee_pwrc_mem_domain tm2_pwrc_mem_pcieb[] = {
	{ HHI_MEM_PD_REG0, GENMASK(7, 4) },
};

static struct meson_ee_pwrc_mem_domain tm2_pwrc_mem_dspa[] = {
	{ HHI_DSP_MEM_PD_REG0, GENMASK(15, 0) },
};

static struct meson_ee_pwrc_mem_domain tm2_pwrc_mem_dspb[] = {
	{ HHI_DSP_MEM_PD_REG0, GENMASK(31, 16) },
};

static struct meson_ee_pwrc_mem_domain tm2_pwrc_mem_demod[] = {
	{ HHI_DEMOD_MEM_PD_REG, GENMASK(11, 0) },
	{ HHI_DEMOD_MEM_PD_REG, BIT(13) },
};

static struct meson_ee_pwrc_mem_domain tm2_pwrc_mem_emmcb[] = {
	{ HHI_MEM_PD_REG1, GENMASK(11, 10) },
};

static struct meson_ee_pwrc_mem_domain tm2_pwrc_mem_emmcc[] = {
	{ HHI_MEM_PD_REG1, GENMASK(9, 8) },
};

static struct meson_ee_pwrc_mem_domain tm2_pwrc_mem_tvfe[] = {
	{ HHI_MEM_PD_REG1, GENMASK(7, 6) },
};

static struct meson_ee_pwrc_mem_domain tm2_pwrc_mem_acodec[] = {
	{ HHI_MEM_PD_REG1, GENMASK(19, 18) },
};

static struct meson_ee_pwrc_mem_domain tm2_pwrc_mem_atvdemod[] = {
	{ HHI_MEM_PD_REG1, GENMASK(5, 4) },
};

#define VPU_PD(__name, __top_pd, __mem, __get_power, __resets,		\
		__clks, __dom_id, __mem_size)					\
	{								\
		.name = __name,						\
		.reset_names_count = __resets,				\
		.clk_names_count = __clks,				\
		.domain_id = __dom_id,					\
		.top_pd = __top_pd,					\
		.mem_pd_count = ARRAY_SIZE(__mem_size),			\
		.mem_pd = __mem,					\
		.get_power = __get_power,				\
	}

#define TOP_PD(__name, __top_pd, __mem, __get_power, __resets,		\
		__dom_id, __flag, __mem_size)					\
	{								\
		.name = __name,						\
		.reset_names_count = __resets,				\
		.domain_id = __dom_id,					\
		.top_pd = __top_pd,					\
		.mem_pd_count = ARRAY_SIZE(__mem_size),			\
		.mem_pd = __mem,					\
		.flags = __flag,					\
		.get_power = __get_power,				\
	}

#define MEM_PD(__name, __mem, __dom_id, __flag, __mem_size)				\
	TOP_PD(__name, NULL, __mem, NULL, 0, __dom_id, __flag, __mem_size)

static bool pwrc_ee_get_power(struct meson_ee_pwrc_domain *pwrc_domain);

static struct meson_ee_pwrc_domain_desc g12a_pwrc_domains[] = {
	[PWRC_G12A_VPU_ID]  = VPU_PD("VPU", &g12a_pwrc_vpu, g12a_pwrc_mem_vpu,
				     pwrc_ee_get_power, 11, 2,
				     PWRC_G12A_VPU_ID, g12a_pwrc_mem_vpu),
	[PWRC_G12A_ETH_ID] = MEM_PD("ETH", g12a_pwrc_mem_eth, PWRC_G12A_ETH_ID,
					0, g12a_pwrc_mem_eth),
};

static struct meson_ee_pwrc_domain_desc sm1_pwrc_domains[] = {
	[PWRC_SM1_VPU_ID]  = VPU_PD("VPU", &sm1_pwrc_vpu, sm1_pwrc_mem_vpu,
				    pwrc_ee_get_power, 11, 2, PWRC_SM1_VPU_ID, sm1_pwrc_mem_vpu),
	[PWRC_SM1_NNA_ID]  = TOP_PD("NNA", &sm1_pwrc_nna, sm1_pwrc_mem_nna,
				    pwrc_ee_get_power, 3, PWRC_SM1_NNA_ID, 0, sm1_pwrc_mem_nna),
	[PWRC_SM1_USB_ID]  = TOP_PD("USB", &sm1_pwrc_usb, sm1_pwrc_mem_usb,
				    pwrc_ee_get_power, 1, PWRC_SM1_USB_ID,
				    0, sm1_pwrc_mem_usb),
	[PWRC_SM1_PCIE_ID] = TOP_PD("PCIE", &sm1_pwrc_pci, sm1_pwrc_mem_pcie,
				    pwrc_ee_get_power, 3, PWRC_SM1_PCIE_ID,
				    0, sm1_pwrc_mem_pcie),
	[PWRC_SM1_GE2D_ID] = TOP_PD("GE2D", &sm1_pwrc_ge2d, sm1_pwrc_mem_ge2d,
				    pwrc_ee_get_power, 1, PWRC_SM1_GE2D_ID, 0, sm1_pwrc_mem_ge2d),
	[PWRC_SM1_AUDIO_ID] = MEM_PD("AUDIO", sm1_pwrc_mem_audio,
				     PWRC_SM1_AUDIO_ID, 0, sm1_pwrc_mem_audio),
	[PWRC_SM1_ETH_ID] = MEM_PD("ETH", g12a_pwrc_mem_eth, PWRC_SM1_ETH_ID,
					0, g12a_pwrc_mem_eth),
	[PWRC_SM1_VDEC_ID] = TOP_PD("VDEC", &sm1_pwrc_vdec, sm1_pwrc_mem_vdec,
				pwrc_ee_get_power, 13, PWRC_SM1_VDEC_ID, 0, sm1_pwrc_mem_vdec),
	[PWRC_SM1_HCODEC_ID] = TOP_PD("HCODEC", &sm1_pwrc_hcodec,
				      sm1_pwrc_mem_hcodec, pwrc_ee_get_power,
				      16, PWRC_SM1_HCODEC_ID, 0, sm1_pwrc_mem_hcodec),
	[PWRC_SM1_HEVC_ID] = TOP_PD("HEVC", &sm1_pwrc_hevc, sm1_pwrc_mem_hevc,
				pwrc_ee_get_power, 19, PWRC_SM1_HEVC_ID, 0, sm1_pwrc_mem_hevc),
	[PWRC_SM1_WAVE420L_ID] = TOP_PD("WAVE420L", &sm1_pwrc_wave420l,
				sm1_pwrc_mem_wave420l, pwrc_ee_get_power,
				4, PWRC_SM1_WAVE420L_ID, 0, sm1_pwrc_mem_wave420l),
	[PWRC_SM1_CSI_ID] = TOP_PD("csi", &sm1_pwrc_csi, sm1_pwrc_mem_csi,
				pwrc_ee_get_power, 0, PWRC_SM1_CSI_ID, 0, sm1_pwrc_mem_csi),
};

static struct meson_ee_pwrc_domain_desc tm2_pwrc_domains[] = {
	[PWRC_TM2_VPU_ID]  = VPU_PD("VPU", &tm2_pwrc_vpu, tm2_pwrc_mem_vpu,
				    pwrc_ee_get_power, 11, 2, PWRC_TM2_VPU_ID, tm2_pwrc_mem_vpu),
	[PWRC_TM2_NNA_ID]  = TOP_PD("NNA", &tm2_pwrc_nna, tm2_pwrc_mem_nna,
				    pwrc_ee_get_power, 1, PWRC_TM2_NNA_ID, 0, tm2_pwrc_mem_nna),
	[PWRC_TM2_USB_ID]  = TOP_PD("USB", &tm2_pwrc_usb, tm2_pwrc_mem_usb,
				    pwrc_ee_get_power, 1, PWRC_TM2_USB_ID,
				    0, tm2_pwrc_mem_usb),
	[PWRC_TM2_PCIE_ID] = TOP_PD("PCIEA", &tm2_pwrc_pciea, tm2_pwrc_mem_pciea,
				    pwrc_ee_get_power, 3, PWRC_TM2_PCIE_ID,
				    0, tm2_pwrc_mem_pciea),
	[PWRC_TM2_GE2D_ID] = TOP_PD("GE2D", &tm2_pwrc_ge2d, tm2_pwrc_mem_ge2d,
				    pwrc_ee_get_power, 1, PWRC_TM2_GE2D_ID, 0, tm2_pwrc_mem_ge2d),
	[PWRC_TM2_AUDIO_ID] = TOP_PD("AUDIO", &tm2_pwrc_audio, tm2_pwrc_mem_audio,
				     pwrc_ee_get_power, 1, PWRC_TM2_AUDIO_ID, GENPD_FLAG_ALWAYS_ON,
					 tm2_pwrc_mem_audio),
	[PWRC_TM2_ETH_ID] = MEM_PD("ETH", g12a_pwrc_mem_eth, PWRC_TM2_ETH_ID,
					0, g12a_pwrc_mem_eth),
	[PWRC_TM2_VDEC_ID] = TOP_PD("VDEC", &tm2_pwrc_vdec, tm2_pwrc_mem_vdec,
				pwrc_ee_get_power, 0, PWRC_TM2_VDEC_ID, 0, tm2_pwrc_mem_vdec),
	[PWRC_TM2_HCODEC_ID] = TOP_PD("HCODEC", &tm2_pwrc_hcodec,
				      tm2_pwrc_mem_hcodec, pwrc_ee_get_power,
				      0, PWRC_TM2_HCODEC_ID, 0, tm2_pwrc_mem_hcodec),
	[PWRC_TM2_HEVC_ID] = TOP_PD("HEVC", &tm2_pwrc_hevc, tm2_pwrc_mem_hevc,
				pwrc_ee_get_power, 0, PWRC_TM2_HEVC_ID, 0, tm2_pwrc_mem_hevc),
	[PWRC_TM2_WAVE420L_ID] = TOP_PD("WAVE420L", &tm2_pwrc_wave420l,
				tm2_pwrc_mem_wave420l, pwrc_ee_get_power,
				0, PWRC_TM2_WAVE420L_ID, 0, tm2_pwrc_mem_wave420l),
	[PWRC_TM2_PCIE1_ID] = TOP_PD("PCIEB", &tm2_pwrc_pcieb,
				tm2_pwrc_mem_pcieb, pwrc_ee_get_power,
				3, PWRC_TM2_PCIE1_ID, 0, tm2_pwrc_mem_pcieb),
	[PWRC_TM2_DSPA_ID] = TOP_PD("DSPA", &tm2_pwrc_dspa,
				tm2_pwrc_mem_dspa, pwrc_ee_get_power,
				2, PWRC_TM2_DSPA_ID, 0, tm2_pwrc_mem_dspa),
	[PWRC_TM2_DSPB_ID] = TOP_PD("DSPB", &tm2_pwrc_dspb,
				tm2_pwrc_mem_dspb, pwrc_ee_get_power,
				2, PWRC_TM2_DSPB_ID, 0, tm2_pwrc_mem_dspb),
	[PWRC_TM2_DEMOD_ID] = TOP_PD("DEMOD", &tm2_pwrc_demod,
				tm2_pwrc_mem_demod, pwrc_ee_get_power,
				1, PWRC_TM2_DEMOD_ID, 0, tm2_pwrc_mem_demod),
	[PWRC_TM2_EMMCB_ID] = TOP_PD("EMMCB", &tm2_pwrc_emmcb,
				tm2_pwrc_mem_emmcb, pwrc_ee_get_power,
				1, PWRC_TM2_EMMCB_ID, GENPD_FLAG_ALWAYS_ON, tm2_pwrc_mem_emmcb),
	[PWRC_TM2_EMMCC_ID] = TOP_PD("EMMCC", &tm2_pwrc_emmcc,
				tm2_pwrc_mem_emmcc, pwrc_ee_get_power,
				1, PWRC_TM2_EMMCC_ID, GENPD_FLAG_ALWAYS_ON, tm2_pwrc_mem_emmcc),
	[PWRC_TM2_TVFE_ID] = TOP_PD("TVFE", &tm2_pwrc_tvfe,
				tm2_pwrc_mem_tvfe, pwrc_ee_get_power,
				1, PWRC_TM2_TVFE_ID, 0, tm2_pwrc_mem_tvfe),
	[PWRC_TM2_ACODEC_ID] = TOP_PD("ACODEC", &tm2_pwrc_acodec,
				tm2_pwrc_mem_acodec, pwrc_ee_get_power,
				1, PWRC_TM2_ACODEC_ID, 0, tm2_pwrc_mem_acodec),
	[PWRC_TM2_ATVDEMOD_ID] = TOP_PD("ATVDEMOD", &tm2_pwrc_atvdemod,
				tm2_pwrc_mem_atvdemod, pwrc_ee_get_power,
				1, PWRC_TM2_ATVDEMOD_ID, 0, tm2_pwrc_mem_atvdemod),
};

static const struct regmap_config dos_regmap_config = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
};

struct meson_ee_pwrc_domain {
	struct generic_pm_domain base;
	bool enabled;
	struct meson_ee_pwrc *pwrc;
	struct meson_ee_pwrc_domain_desc desc;
	struct clk_bulk_data *clks;
	int num_clks;
	struct reset_control *rstc;
	int num_rstc;
	atomic_t pwron_count;
};

struct meson_ee_pwrc {
	struct regmap *regmap_ao;
	struct regmap *regmap_hhi;
	struct regmap *regmap_dos;
	struct meson_ee_pwrc_domain *domains;
	struct genpd_onecell_data xlate;
};

static bool pwrc_ee_get_power(struct meson_ee_pwrc_domain *pwrc_domain)
{
	u32 reg;

	regmap_read(pwrc_domain->pwrc->regmap_ao,
		    pwrc_domain->desc.top_pd->sleep_reg, &reg);

	return (reg & pwrc_domain->desc.top_pd->sleep_mask);
}

static int meson_ee_pwrc_off(struct generic_pm_domain *domain)
{
	struct meson_ee_pwrc_domain *pwrc_domain =
		container_of(domain, struct meson_ee_pwrc_domain, base);
	int i, ret;

	if (!strcmp(pwrc_domain->desc.name, "USB") ||
	    !strcmp(pwrc_domain->desc.name, "PCIE") ||
	    !strcmp(pwrc_domain->desc.name, "PCIEA") ||
	    !strcmp(pwrc_domain->desc.name, "PCIEB"))
		return 0;

	if (!strcmp(pwrc_domain->desc.name, "DSPA") ||
	    !strcmp(pwrc_domain->desc.name, "DSPB")) {
		ret = reset_control_assert(pwrc_domain->rstc);
		if (ret)
			return ret;
	} else {
		if (atomic_read(&pwrc_domain->pwron_count) == 0) {
			ret = reset_control_deassert(pwrc_domain->rstc);
				if (ret)
					return ret;
		} else {
			atomic_dec(&pwrc_domain->pwron_count);
		}

		ret = reset_control_assert(pwrc_domain->rstc);
		if (ret)
			return ret;
	}

	if (pwrc_domain->desc.top_pd)
		regmap_update_bits(pwrc_domain->pwrc->regmap_ao,
				   pwrc_domain->desc.top_pd->iso_reg,
				   pwrc_domain->desc.top_pd->iso_mask,
				   pwrc_domain->desc.top_pd->iso_mask);
	usleep_range(20, 21);

	if (pwrc_domain->desc.domain_id >= DOS_START_ID &&
	    pwrc_domain->desc.domain_id <= DOS_END_ID) {
		for (i = 0 ; i < pwrc_domain->desc.mem_pd_count ; ++i)
			regmap_update_bits(pwrc_domain->pwrc->regmap_dos,
					   pwrc_domain->desc.mem_pd[i].reg,
					   pwrc_domain->desc.mem_pd[i].mask,
					   pwrc_domain->desc.mem_pd[i].mask);
	} else {
		for (i = 0 ; i < pwrc_domain->desc.mem_pd_count ; ++i)
			regmap_update_bits(pwrc_domain->pwrc->regmap_hhi,
					   pwrc_domain->desc.mem_pd[i].reg,
					   pwrc_domain->desc.mem_pd[i].mask,
					   pwrc_domain->desc.mem_pd[i].mask);
	}
	usleep_range(20, 21);

	if (pwrc_domain->desc.top_pd)
		regmap_update_bits(pwrc_domain->pwrc->regmap_ao,
				   pwrc_domain->desc.top_pd->sleep_reg,
				   pwrc_domain->desc.top_pd->sleep_mask,
				   pwrc_domain->desc.top_pd->sleep_mask);

	if (pwrc_domain->num_clks) {
		msleep(20);
		clk_bulk_disable_unprepare(pwrc_domain->num_clks,
					   pwrc_domain->clks);
	}

	return 0;
}

static int meson_ee_pwrc_on(struct generic_pm_domain *domain)
{
	struct meson_ee_pwrc_domain *pwrc_domain =
		container_of(domain, struct meson_ee_pwrc_domain, base);
	int i, ret;

	if (!strcmp(pwrc_domain->desc.name, "USB") ||
	    !strcmp(pwrc_domain->desc.name, "PCIE") ||
	    !strcmp(pwrc_domain->desc.name, "PCIEA") ||
	    !strcmp(pwrc_domain->desc.name, "PCIEB")) {
		if (!pwrc_domain->desc.get_power(pwrc_domain))
			return 0;
	}

	ret = reset_control_deassert(pwrc_domain->rstc);
	if (ret)
		return ret;

	if (pwrc_domain->desc.top_pd)
		regmap_update_bits(pwrc_domain->pwrc->regmap_ao,
				   pwrc_domain->desc.top_pd->sleep_reg,
				   pwrc_domain->desc.top_pd->sleep_mask, 0);
	usleep_range(20, 21);

	if (pwrc_domain->desc.domain_id >= DOS_START_ID &&
	    pwrc_domain->desc.domain_id <= DOS_END_ID) {
		for (i = 0 ; i < pwrc_domain->desc.mem_pd_count ; ++i)
			regmap_update_bits(pwrc_domain->pwrc->regmap_dos,
					   pwrc_domain->desc.mem_pd[i].reg,
					   pwrc_domain->desc.mem_pd[i].mask, 0);
	} else {
		for (i = 0 ; i < pwrc_domain->desc.mem_pd_count ; ++i)
			regmap_update_bits(pwrc_domain->pwrc->regmap_hhi,
					   pwrc_domain->desc.mem_pd[i].reg,
					   pwrc_domain->desc.mem_pd[i].mask, 0);
	}

	usleep_range(20, 21);

	ret = reset_control_assert(pwrc_domain->rstc);
	if (ret)
		return ret;

	if (pwrc_domain->desc.top_pd)
		regmap_update_bits(pwrc_domain->pwrc->regmap_ao,
				   pwrc_domain->desc.top_pd->iso_reg,
				   pwrc_domain->desc.top_pd->iso_mask, 0);

	ret = reset_control_deassert(pwrc_domain->rstc);
	if (ret)
		return ret;

	atomic_inc(&pwrc_domain->pwron_count);
	return clk_bulk_prepare_enable(pwrc_domain->num_clks,
				       pwrc_domain->clks);
}

static int meson_ee_pwrc_init_domain(struct platform_device *pdev,
				     struct meson_ee_pwrc *pwrc,
				     struct meson_ee_pwrc_domain *dom)
{
	struct device_node *np = NULL;
	int ret;
	char buf[16] = {0};
	int i;

	dom->pwrc = pwrc;
	dom->num_rstc = dom->desc.reset_names_count;
	dom->num_clks = dom->desc.clk_names_count;
	if (dom->num_rstc) {
		for (i = 0; i < strlen(dom->desc.name); i++)
			buf[i] = tolower(*(dom->desc.name + i));

		strcat(buf, ",reset");
		np = of_parse_phandle(pdev->dev.of_node, buf, 0);
		dom->rstc = of_reset_control_array_get(np, true, false, true);
		if (IS_ERR(dom->rstc))
			return PTR_ERR(dom->rstc);
	}

	if (dom->num_clks) {
		int ret = devm_clk_bulk_get_all(&pdev->dev, &dom->clks);

		if (ret < 0)
			return ret;

		if (dom->num_clks != ret) {
			dev_warn(&pdev->dev, "Invalid clocks count %d for domain %s\n",
				 ret, dom->desc.name);
			dom->num_clks = ret;
		}
	}

	dom->base.name = dom->desc.name;
	dom->base.power_on = meson_ee_pwrc_on;
	dom->base.power_off = meson_ee_pwrc_off;
	dom->base.flags = dom->desc.flags;

	if (dom->num_clks && dom->desc.get_power && !dom->desc.get_power(dom)) {
		ret = clk_bulk_prepare_enable(dom->num_clks, dom->clks);
		if (ret)
			return ret;
		ret = pm_genpd_init(&dom->base, NULL,
				    false);
		if (ret)
			return ret;
	} else {
		ret = pm_genpd_init(&dom->base, NULL,
				    (dom->desc.get_power ?
				     dom->desc.get_power(dom) : true));
		if (ret)
			return ret;
	}

	return 0;
}

static int meson_ee_pwrc_probe(struct platform_device *pdev)
{
	const struct meson_ee_pwrc_domain_data *match;
	struct regmap *regmap_ao, *regmap_hhi, *regmap_dos;
	struct meson_ee_pwrc *pwrc;
	struct resource *dos_res;
	void __iomem *base;
	int i, ret;

	match = of_device_get_match_data(&pdev->dev);
	if (!match) {
		dev_err(&pdev->dev, "failed to get match data\n");
		return -ENODEV;
	}

	pwrc = devm_kzalloc(&pdev->dev, sizeof(*pwrc), GFP_KERNEL);
	if (!pwrc)
		return -ENOMEM;

	pwrc->xlate.domains = devm_kcalloc(&pdev->dev, match->count,
					   sizeof(*pwrc->xlate.domains),
					   GFP_KERNEL);
	if (!pwrc->xlate.domains)
		return -ENOMEM;

	pwrc->domains = devm_kcalloc(&pdev->dev, match->count,
				     sizeof(*pwrc->domains), GFP_KERNEL);
	if (!pwrc->domains)
		return -ENOMEM;

	pwrc->xlate.num_domains = match->count;

	dos_res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
					       "dos");
	if (!dos_res)
		return -ENOMEM;

	base = devm_ioremap(&pdev->dev, dos_res->start, resource_size(dos_res));
	if (IS_ERR(base))
		return PTR_ERR(base);

	regmap_dos = devm_regmap_init_mmio(&pdev->dev, base,
					   &dos_regmap_config);
	if (IS_ERR(regmap_dos))
		return PTR_ERR(regmap_dos);

	regmap_hhi = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						     "amlogic,hhi-sysctrl");
	if (IS_ERR(regmap_hhi)) {
		dev_err(&pdev->dev, "failed to get HHI regmap\n");
		return PTR_ERR(regmap_hhi);
	}

	regmap_ao = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						    "amlogic,ao-sysctrl");
	if (IS_ERR(regmap_ao)) {
		dev_err(&pdev->dev, "failed to get AO regmap\n");
		return PTR_ERR(regmap_ao);
	}

	pwrc->regmap_ao = regmap_ao;
	pwrc->regmap_hhi = regmap_hhi;
	pwrc->regmap_dos = regmap_dos;

	platform_set_drvdata(pdev, pwrc);
	for (i = 0 ; i < match->count ; ++i) {
		struct meson_ee_pwrc_domain *dom = &pwrc->domains[i];

		memcpy(&dom->desc, &match->domains[i], sizeof(dom->desc));

		ret = meson_ee_pwrc_init_domain(pdev, pwrc, dom);
		if (ret)
			goto init_fail;

		pwrc->xlate.domains[i] = &dom->base;
	}

	return of_genpd_add_provider_onecell(pdev->dev.of_node, &pwrc->xlate);

init_fail:
	for (i--; i >= 0; i--)
		pm_genpd_remove(&pwrc->domains[i].base);

	devm_kfree(&pdev->dev, pwrc->domains);
	devm_kfree(&pdev->dev, pwrc->xlate.domains);
	devm_kfree(&pdev->dev, pwrc);

	return ret;
}

static struct meson_ee_pwrc_domain_data meson_ee_g12a_pwrc_data = {
	.count = ARRAY_SIZE(g12a_pwrc_domains),
	.domains = g12a_pwrc_domains,
};

static struct meson_ee_pwrc_domain_data meson_ee_sm1_pwrc_data = {
	.count = ARRAY_SIZE(sm1_pwrc_domains),
	.domains = sm1_pwrc_domains,
};

static struct meson_ee_pwrc_domain_data meson_ee_tm2_pwrc_data = {
	.count = ARRAY_SIZE(tm2_pwrc_domains),
	.domains = tm2_pwrc_domains,
};

static const struct of_device_id meson_ee_pwrc_match_table[] = {
	{
		.compatible = "amlogic,g12a-power-domain",
		.data = &meson_ee_g12a_pwrc_data,
	},
	{
		.compatible = "amlogic,sm1-power-domain",
		.data = &meson_ee_sm1_pwrc_data,
	},
	{
		.compatible = "amlogic,tm2-power-domain",
		.data = &meson_ee_tm2_pwrc_data,
	},
	{ /* sentinel */ }
};

static struct platform_driver meson_ee_pwrc_driver = {
	.probe = meson_ee_pwrc_probe,
	.driver = {
		.name		= "meson_ee_pwrc",
		.of_match_table	= meson_ee_pwrc_match_table,
	},
};

int __init power_ee_domain_init(void)
{
	return platform_driver_register(&meson_ee_pwrc_driver);
}

void __exit power_ee_domain_exit(void)
{
	platform_driver_unregister(&meson_ee_pwrc_driver);
}
