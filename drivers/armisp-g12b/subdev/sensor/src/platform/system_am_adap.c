/*
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2018 Amlogic or its affiliates
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#ifndef SENSOR_DEBUG_DRIVER_CONFIG
#define pr_fmt(fmt) "AM_ADAP: " fmt

#include <linux/version.h>
#include "system_am_adap.h"
#include <linux/irqreturn.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/ioport.h>
#include <linux/of_platform.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/device.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <linux/dma-map-ops.h>
#include <linux/cma.h>
#include <linux/kasan.h>
#else
#include <linux/dma-contiguous.h>
#endif
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/of_fdt.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/kfifo.h>
#include <linux/completion.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/freezer.h>
#include <linux/delay.h>
// #include </linux/sched/types.h>
#include "acamera_command_api.h"
#include "system_log.h"

#include <linux/ktime.h>
#include <uapi/linux/sched/types.h>

#define AM_ADAPTER_NAME "amlogic, isp-adapter"
#define BOUNDARY 16

#define CAMERA_NUM 2
#define DDR_BUF_SIZE 6
#define CAMERA_QUEUE_NUM 12

struct am_adap *g_adap = NULL;

struct am_adap_fsm_t
{
	uint8_t cam_en;
	int control_flag;
	int wbuf_index;

	int next_buf_index;
	resource_size_t read_buf;
	resource_size_t buffer_start;
	resource_size_t ddr_buf[DDR_BUF_SIZE];
	struct kfifo adapt_fifo;
	spinlock_t adap_lock;

	struct page *cma_pages;

	struct am_adap_info para;
};

/*we allocate from CMA*/
#define DOL_BUF_SIZE 6
static resource_size_t dol_buf[DOL_BUF_SIZE];

#define DEFAULT_ADAPTER_BUFFER_SIZE 36

static unsigned int frontend1_flag;

struct am_adap_fsm_t adap_fsm[CAMERA_NUM];

uint32_t camera_frame_fifo[CAMERA_QUEUE_NUM];

static int ceil_upper(int val, int mod)
{
	int ret = 0;
	if ((val == 0) || (mod == 0))
	{
		pr_info("input a invalid value.\n");
		return 0;
	}
	else
	{
		if ((val % mod) == 0)
		{
			ret = (val / mod);
		}
		else
		{
			ret = ((val / mod) + 1);
		}
	}
	return ret;
}

static inline void update_wr_reg_bits(unsigned int reg,
									  adap_io_type_t io_type, unsigned int mask,
									  unsigned int val)
{
	unsigned int tmp, orig;
	void __iomem *base = NULL;
	switch (io_type)
	{
	case FRONTEND_IO:
		base = g_adap->base_addr + FRONTEND_BASE;
		break;
	case RD_IO:
		base = g_adap->base_addr + RD_BASE;
		break;
	case PIXEL_IO:
		base = g_adap->base_addr + PIXEL_BASE;
		break;
	case ALIGN_IO:
		base = g_adap->base_addr + ALIGN_BASE;
		break;
	case MISC_IO:
		base = g_adap->base_addr + MISC_BASE;
		break;
	default:
		pr_err("adapter error io type.\n");
		base = NULL;
		break;
	}
	if (base != NULL)
	{
		orig = readl(base + reg);
		tmp = orig & ~mask;
		tmp |= val & mask;
		writel(tmp, base + reg);
	}
}

static inline void adap_wr_reg_bits(unsigned int adr,
									adap_io_type_t io_type, unsigned int val,
									unsigned int start, unsigned int len)
{
	update_wr_reg_bits(adr, io_type,
					   ((1 << len) - 1) << start, val << start);
}

static inline void mipi_adap_reg_wr(int addr, adap_io_type_t io_type, uint32_t val)
{
	void __iomem *base_reg_addr = NULL;
	void __iomem *reg_addr = NULL;
	switch (io_type)
	{
	case FRONTEND_IO:
		base_reg_addr = g_adap->base_addr + FRONTEND_BASE;
		break;
	case RD_IO:
		base_reg_addr = g_adap->base_addr + RD_BASE;
		break;
	case PIXEL_IO:
		base_reg_addr = g_adap->base_addr + PIXEL_BASE;
		break;
	case ALIGN_IO:
		base_reg_addr = g_adap->base_addr + ALIGN_BASE;
		break;
	case MISC_IO:
		base_reg_addr = g_adap->base_addr + MISC_BASE;
		break;
	default:
		pr_err("adapter error io type.\n");
		base_reg_addr = NULL;
		break;
	}
	if (base_reg_addr != NULL)
	{
		reg_addr = base_reg_addr + addr;
		writel(val, reg_addr);
	}
	else
		pr_err("mipi adapter write register failed.\n");
}

static inline void mipi_adap_reg_rd(int addr, adap_io_type_t io_type, uint32_t *val)
{
	void __iomem *base_reg_addr = NULL;
	void __iomem *reg_addr = NULL;
	switch (io_type)
	{
	case FRONTEND_IO:
		base_reg_addr = g_adap->base_addr + FRONTEND_BASE;
		break;
	case RD_IO:
		base_reg_addr = g_adap->base_addr + RD_BASE;
		break;
	case PIXEL_IO:
		base_reg_addr = g_adap->base_addr + PIXEL_BASE;
		break;
	case ALIGN_IO:
		base_reg_addr = g_adap->base_addr + ALIGN_BASE;
		break;
	case MISC_IO:
		base_reg_addr = g_adap->base_addr + MISC_BASE;
		break;
	default:
		pr_err("%s, adapter error io type.\n", __func__);
		base_reg_addr = NULL;
		break;
	}
	if (base_reg_addr != NULL)
	{
		reg_addr = base_reg_addr + addr;
		*val = readl(reg_addr);
	}
	else
		pr_err("mipi adapter read register failed.\n");
}

int am_adap_parse_dt(struct device_node *node)
{
	int rtn = -1;
	int irq = -1;
	int ret = 0;
	struct resource rs;
	struct am_adap *t_adap = NULL;

	if (node == NULL)
	{
		pr_err("%s: Error input param\n", __func__);
		return -1;
	}

	rtn = of_device_is_compatible(node, AM_ADAPTER_NAME);
	if (rtn == 0)
	{
		pr_err("%s: Error match compatible\n", __func__);
		return -1;
	}

	t_adap = kzalloc(sizeof(*t_adap), GFP_KERNEL);
	if (t_adap == NULL)
	{
		pr_err("%s: Failed to alloc adapter\n", __func__);
		return -1;
	}

	t_adap->of_node = node;
	t_adap->f_end_irq = 0;
	t_adap->f_fifo = 0;
	t_adap->f_adap = 0;

	rtn = of_address_to_resource(node, 0, &rs);
	if (rtn != 0)
	{
		pr_err("%s:Error get adap reg resource\n", __func__);
		goto reg_error;
	}

	pr_info("%s: rs idx info: name: %s\n", __func__, rs.name);
	if (strcmp(rs.name, "adapter") == 0)
	{
		t_adap->reg = rs;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
		t_adap->base_addr =
			ioremap(t_adap->reg.start, resource_size(&t_adap->reg));
#else
		t_adap->base_addr =
			ioremap_nocache(t_adap->reg.start, resource_size(&t_adap->reg));
#endif
	}

	irq = irq_of_parse_and_map(node, 0);
	if (irq <= 0)
	{
		pr_err("%s:Error get adap irq\n", __func__);
		goto irq_error;
	}

	t_adap->rd_irq = irq;
	pr_info("%s:rs info: irq: %d\n", __func__, t_adap->rd_irq);

	t_adap->p_dev = of_find_device_by_node(node);
	ret = of_reserved_mem_device_init(&(t_adap->p_dev->dev));
	if (ret != 0)
	{
		pr_err("adapt reserved mem device init failed.\n");
		return ret;
	}

	ret = of_property_read_u32(t_adap->p_dev->dev.of_node, "mem_alloc",
							   &(t_adap->adap_buf_size));
	pr_info("adapter alloc %dM memory\n", t_adap->adap_buf_size);
	if (ret != 0)
	{
		pr_err("failed to get adap-buf-size from dts, use default value\n");
		t_adap->adap_buf_size = DEFAULT_ADAPTER_BUFFER_SIZE;
	}

	t_adap->adap_buf_size = t_adap->adap_buf_size / 2;
	g_adap = t_adap;

	adap_fsm[0].cam_en = CAM_DIS;
	adap_fsm[1].cam_en = CAM_DIS;

	return 0;

irq_error:
	iounmap(t_adap->base_addr);
	t_adap->base_addr = NULL;

reg_error:
	if (t_adap != NULL)
		kfree(t_adap);
	return -1;
}

void am_adap_deinit_parse_dt(void)
{
	struct am_adap *t_adap = NULL;

	t_adap = g_adap;

	if (t_adap == NULL || t_adap->p_dev == NULL ||
		t_adap->base_addr == NULL)
	{
		pr_err("%s: Error input param\n", __func__);
		return;
	}

	iounmap(t_adap->base_addr);
	t_adap->base_addr = NULL;

	kfree(t_adap);
	t_adap = NULL;
	g_adap = NULL;

	pr_info("Success deinit parse adap module\n");
}

void am_adap_set_info(struct am_adap_info *info)
{
	if (info->path > PATH1)
	{
		pr_err("%s: Error input param\n", __func__);
		return;
	}

	memset(&adap_fsm[info->path].para, 0, sizeof(struct am_adap_info));
	memcpy(&adap_fsm[info->path].para, info, sizeof(struct am_adap_info));
}

int am_adap_get_depth(uint8_t channel)
{
	int depth = 0;
	switch (adap_fsm[channel].para.fmt)
	{
	case AM_RAW6:
		depth = 6;
		break;
	case AM_RAW7:
		depth = 7;
		break;
	case AM_RAW8:
		depth = 8;
		break;
	case AM_RAW10:
		depth = 10;
		break;
	case AM_RAW12:
		depth = 12;
		break;
	case AM_RAW14:
		depth = 14;
		break;
	default:
		pr_err("Not supported data format.\n");
		break;
	}
	return depth;
}

int am_disable_irq(uint8_t channel)
{
	// disable irq mask
	if (channel == ADAP0_PATH)
	{
		mipi_adap_reg_wr(CSI2_INTERRUPT_CTRL_STAT, FRONTEND_IO, 0);
		adap_wr_reg_bits(MIPI_ADAPT_IRQ_MASK0, ALIGN_IO, 0, FRONT0_WR_DONE, 1);
		adap_wr_reg_bits(MIPI_ADAPT_IRQ_MASK0, ALIGN_IO, 0, READ0_RD_DONE, 1);
	}
	else
	{
		mipi_adap_reg_wr(CSI2_INTERRUPT_CTRL_STAT + FTE1_OFFSET, FRONTEND_IO, 0);
		adap_wr_reg_bits(MIPI_ADAPT_IRQ_MASK0, ALIGN_IO, 0, FRONT1_WR_DONE, 1);
		adap_wr_reg_bits(MIPI_ADAPT_IRQ_MASK0, ALIGN_IO, 0, READ1_RD_DONE, 1);
	}

	return 0;
}

/*
 *========================AM ADAPTER FRONTEND INTERFACE========================
 */

void am_adap_frontend_start(uint8_t channel)
{
	int width = adap_fsm[channel].para.img.width;
	int depth, val;
	depth = am_adap_get_depth(channel);
	if (!depth)
		pr_err("is not supported data format.");

	val = ceil_upper((width * depth), (8 * 16));
	pr_info("channel %d, frontend : width = %d, val = %d\n", channel, width, val);

	adap_wr_reg_bits(CSI2_DDR_STRIDE_PIX + FTE1_OFFSET * channel, FRONTEND_IO, val, 4, 28);
	adap_wr_reg_bits(CSI2_GEN_CTRL0 + FTE1_OFFSET * channel, FRONTEND_IO, 0x0f, 0, 4);
}

int am_adap_frontend_init(uint8_t channel)
{
	int long_exp_offset = adap_fsm[channel].para.offset.long_offset;
	int short_exp_offset = adap_fsm[channel].para.offset.short_offset;

	mipi_adap_reg_wr(CSI2_CLK_RESET + FTE1_OFFSET * channel, FRONTEND_IO, 0x7); // release from reset
	mipi_adap_reg_wr(CSI2_CLK_RESET + FTE1_OFFSET * channel, FRONTEND_IO, 0x6); // enable frontend module clock and disable auto clock gating
	mipi_adap_reg_wr(CSI2_CLK_RESET + FTE1_OFFSET * channel, FRONTEND_IO, 0x6);
	if (adap_fsm[channel].para.mode == DIR_MODE)
	{
		mipi_adap_reg_wr(CSI2_GEN_CTRL0 + FTE1_OFFSET * channel, FRONTEND_IO, 0x00010000); // bit[0] 1:enable virtual channel 0
		adap_wr_reg_bits(MIPI_ADAPT_AXI_CTRL0, ALIGN_IO, 0x3, 16, 2);
	}
	else if (adap_fsm[channel].para.mode == DDR_MODE)
	{
		mipi_adap_reg_wr(CSI2_GEN_CTRL0 + FTE1_OFFSET * channel, FRONTEND_IO, 0x00010010);
	}
	else if (adap_fsm[channel].para.mode == DCAM_MODE)
	{
		mipi_adap_reg_wr(CSI2_GEN_CTRL0 + FTE1_OFFSET * channel, FRONTEND_IO, 0x00010010);
	}
	else if (adap_fsm[channel].para.mode == DOL_MODE)
	{
		mipi_adap_reg_wr(CSI2_GEN_CTRL0 + FTE1_OFFSET * channel, FRONTEND_IO, 0x000110a0);
	}
	else
	{
		pr_err("%s, Not supported Mode.\n", __func__);
	}

	// applicable only to Raw data, direct MEM path
	// mipi_adap_reg_wr(CSI2_X_START_END_MEM + FTE1_OFFSET * channel, FRONTEND_IO, 0xffff0000);
	// mipi_adap_reg_wr(CSI2_Y_START_END_MEM + FTE1_OFFSET * channel, FRONTEND_IO, 0xffff0000);
	adap_wr_reg_bits(CSI2_X_START_END_ISP + FTE1_OFFSET * channel, FRONTEND_IO, adap_fsm[channel].para.offset.offset_x, 0, 16);
	adap_wr_reg_bits(CSI2_X_START_END_ISP + FTE1_OFFSET * channel, FRONTEND_IO, 0xffff, 16, 16);
	adap_wr_reg_bits(CSI2_X_START_END_MEM + FTE1_OFFSET * channel, FRONTEND_IO, adap_fsm[channel].para.offset.offset_x, 0, 16);
	adap_wr_reg_bits(CSI2_X_START_END_MEM + FTE1_OFFSET * channel, FRONTEND_IO, 0xffff, 16, 16);

	if (adap_fsm[channel].para.mode == DOL_MODE)
	{
		if (adap_fsm[channel].para.type == DOL_VC)
		{ // for ov
			mipi_adap_reg_wr(CSI2_VC_MODE + FTE1_OFFSET * channel, FRONTEND_IO, 0x11220040);
		}
		else if (adap_fsm[channel].para.type == DOL_LINEINFO)
		{																					 // for sony
			mipi_adap_reg_wr(CSI2_VC_MODE + FTE1_OFFSET * channel, FRONTEND_IO, 0x11110052); // ft1 vc_mode

#if PLATFORM_G12B
			if (adap_fsm[channel].para.fmt == AM_RAW10)
			{
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_MASK_L + FTE1_OFFSET * channel, FRONTEND_IO, 0x6f6f6f6f);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_MASK_H + FTE1_OFFSET * channel, FRONTEND_IO, 0xffffffcc);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_TO_VC_L + FTE1_OFFSET * channel, FRONTEND_IO, 0x90909090);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_TO_VC_H + FTE1_OFFSET * channel, FRONTEND_IO, 0x55);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_TO_IGNORE_L + FTE1_OFFSET * channel, FRONTEND_IO, 0x80808080);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_TO_IGNORE_H + FTE1_OFFSET * channel, FRONTEND_IO, 0x0);
			}
			else if (adap_fsm[channel].para.fmt == AM_RAW12)
			{ // to be verified
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_MASK_L + FTE1_OFFSET * channel, FRONTEND_IO, 0xffccdbdb);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_MASK_H + FTE1_OFFSET * channel, FRONTEND_IO, 0xffffffff);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_TO_VC_L + FTE1_OFFSET * channel, FRONTEND_IO, 0x112424);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_TO_VC_H + FTE1_OFFSET * channel, FRONTEND_IO, 0x0);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_TO_IGNORE_L + FTE1_OFFSET * channel, FRONTEND_IO, 0x2020);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_TO_IGNORE_H + FTE1_OFFSET * channel, FRONTEND_IO, 0x0);
			}
			else
			{
				pr_err("raw format to be supported");
			}
#elif PLATFORM_C308X || PLATFORM_C305X
			if (adap_fsm[channel].para.fmt == AM_RAW10)
			{
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_MASK_A_L + FTE1_OFFSET * channel, FRONTEND_IO, 0x6e6e6e6e);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_MASK_A_H + FTE1_OFFSET * channel, FRONTEND_IO, 0xffffff00);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_A_L + FTE1_OFFSET * channel, FRONTEND_IO, 0x90909090);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_A_H + FTE1_OFFSET * channel, FRONTEND_IO, 0x55);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_MASK_B_L + FTE1_OFFSET * channel, FRONTEND_IO, 0x6e6e6e6e);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_MASK_B_H + FTE1_OFFSET * channel, FRONTEND_IO, 0xffffff00);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_B_L + FTE1_OFFSET * channel, FRONTEND_IO, 0x90909090);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_B_H + FTE1_OFFSET * channel, FRONTEND_IO, 0xaa);
			}
			else if (adap_fsm[channel].para.fmt == AM_RAW12)
			{
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_MASK_A_L + FTE1_OFFSET * channel, FRONTEND_IO, 0xff000101);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_MASK_A_H + FTE1_OFFSET * channel, FRONTEND_IO, 0xffffffff);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_A_L + FTE1_OFFSET * channel, FRONTEND_IO, 0x112424);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_A_H + FTE1_OFFSET * channel, FRONTEND_IO, 0x0);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_MASK_B_L + FTE1_OFFSET * channel, FRONTEND_IO, 0xff000101);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_MASK_B_H + FTE1_OFFSET * channel, FRONTEND_IO, 0xffffffff);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_B_L + FTE1_OFFSET * channel, FRONTEND_IO, 0x222424);
				mipi_adap_reg_wr(CSI2_VC_MODE2_MATCH_B_H + FTE1_OFFSET * channel, FRONTEND_IO, 0x0);
			}
			else
			{
				pr_err("raw format to be supported");
			}
#endif
			adap_wr_reg_bits(CSI2_X_START_END_MEM + FTE1_OFFSET * channel, FRONTEND_IO, 0xc, 0, 16);
			adap_wr_reg_bits(CSI2_X_START_END_MEM + FTE1_OFFSET * channel, FRONTEND_IO,
							 0xc + adap_fsm[channel].para.img.width - 1, 16, 16);
			adap_wr_reg_bits(CSI2_Y_START_END_MEM + FTE1_OFFSET * channel, FRONTEND_IO,
							 long_exp_offset, 0, 16);
			adap_wr_reg_bits(CSI2_Y_START_END_MEM + FTE1_OFFSET * channel, FRONTEND_IO,
							 long_exp_offset + adap_fsm[channel].para.img.height - 1, 16, 16);
			// set short exposure offset
			adap_wr_reg_bits(CSI2_X_START_END_ISP + FTE1_OFFSET * channel, FRONTEND_IO, 0xc, 0, 16);
			adap_wr_reg_bits(CSI2_X_START_END_ISP + FTE1_OFFSET * channel, FRONTEND_IO,
							 0xc + adap_fsm[channel].para.img.width - 1, 16, 16);
			adap_wr_reg_bits(CSI2_Y_START_END_ISP + FTE1_OFFSET * channel, FRONTEND_IO,
							 short_exp_offset, 0, 16);
			adap_wr_reg_bits(CSI2_Y_START_END_ISP + FTE1_OFFSET * channel, FRONTEND_IO,
							 short_exp_offset + adap_fsm[channel].para.img.height - 1, 16, 16);
		}
		else
		{
			pr_err("Not support DOL type\n");
		}
	}

	if (adap_fsm[channel].para.mode == DIR_MODE)
	{
		mipi_adap_reg_wr(CSI2_VC_MODE + FTE1_OFFSET * channel, FRONTEND_IO, 0x110000);
	}

	if (adap_fsm[channel].para.mode == DDR_MODE)
	{
		// config ddr_buf[0] address
		adap_wr_reg_bits(CSI2_DDR_START_PIX + FTE1_OFFSET * channel, FRONTEND_IO, adap_fsm[channel].ddr_buf[adap_fsm[channel].wbuf_index], 0, 32);
	}
	else if (adap_fsm[channel].para.mode == DCAM_MODE)
	{
		// config ddr_buf[0] address
		adap_wr_reg_bits(CSI2_DDR_START_PIX + FTE1_OFFSET * channel, FRONTEND_IO, adap_fsm[channel].ddr_buf[adap_fsm[channel].wbuf_index], 0, 32);
	}
	else if (adap_fsm[channel].para.mode == DOL_MODE)
	{
		adap_wr_reg_bits(CSI2_DDR_START_PIX + FTE1_OFFSET * channel, FRONTEND_IO, dol_buf[0], 0, 32);
		adap_wr_reg_bits(CSI2_DDR_START_PIX_ALT + FTE1_OFFSET * channel, FRONTEND_IO, dol_buf[1], 0, 32);
	}

	mipi_adap_reg_wr(CSI2_INTERRUPT_CTRL_STAT + FTE1_OFFSET * channel, FRONTEND_IO, 0x5);

	return 0;
}

/*
 *========================AM ADAPTER READER INTERFACE==========================
 */

void am_adap_reader_start(uint8_t channel)
{
	int height = adap_fsm[channel].para.img.height;
	int width = adap_fsm[channel].para.img.width;
	int val, depth;
	depth = am_adap_get_depth(channel);
	val = ceil_upper((width * depth), (8 * 16));
	pr_info("reader : width = %d, val = %d\n", width, val);

	if (channel == ADAP0_PATH)
	{
		adap_wr_reg_bits(MIPI_ADAPT_DDR_RD0_CNTL1, RD_IO, height, 16, 13);
		adap_wr_reg_bits(MIPI_ADAPT_DDR_RD0_CNTL1, RD_IO, val, 0, 10);
		adap_wr_reg_bits(MIPI_ADAPT_DDR_RD0_CNTL0, RD_IO, 1, 0, 1);

		if (adap_fsm[channel].para.mode == DOL_MODE)
		{
			adap_wr_reg_bits(MIPI_ADAPT_DDR_RD1_CNTL1, RD_IO, height, 16, 13);
			adap_wr_reg_bits(MIPI_ADAPT_DDR_RD1_CNTL1, RD_IO, val, 0, 10);
			adap_wr_reg_bits(MIPI_ADAPT_DDR_RD1_CNTL0, RD_IO, 1, 0, 1);
		}
	}
	else
	{
		adap_wr_reg_bits(MIPI_ADAPT_DDR_RD1_CNTL1, RD_IO, height, 16, 13);
		adap_wr_reg_bits(MIPI_ADAPT_DDR_RD1_CNTL1, RD_IO, val, 0, 10);
		adap_wr_reg_bits(MIPI_ADAPT_DDR_RD1_CNTL0, RD_IO, 1, 0, 1);

		if (adap_fsm[channel].para.mode == DOL_MODE)
		{
			adap_wr_reg_bits(MIPI_ADAPT_DDR_RD0_CNTL1, RD_IO, height, 16, 13);
			adap_wr_reg_bits(MIPI_ADAPT_DDR_RD0_CNTL1, RD_IO, val, 0, 10);
			adap_wr_reg_bits(MIPI_ADAPT_DDR_RD0_CNTL0, RD_IO, 1, 0, 1);
		}
	}
}

int am_adap_reader_init(uint8_t channel)
{
	if (channel == ADAP0_PATH)
	{
		if (adap_fsm[channel].para.mode == DIR_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_DDR_RD0_CNTL1, RD_IO, 0x02d00078);
			mipi_adap_reg_wr(MIPI_ADAPT_DDR_RD0_CNTL0, RD_IO, 0xb5000004);
		}
		else if (adap_fsm[channel].para.mode == DDR_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_DDR_RD0_CNTL1, RD_IO, 0x02d00078);
			adap_wr_reg_bits(MIPI_ADAPT_DDR_RD0_CNTL2, RD_IO, adap_fsm[ADAP0_PATH].ddr_buf[adap_fsm[ADAP0_PATH].wbuf_index], 0, 32); // ddr mode config frame address
			mipi_adap_reg_wr(MIPI_ADAPT_DDR_RD0_CNTL0, RD_IO, 0x70000000);
		}
		else if (adap_fsm[channel].para.mode == DCAM_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_DDR_RD0_CNTL1, RD_IO, 0x02d00078);
			adap_wr_reg_bits(MIPI_ADAPT_DDR_RD0_CNTL2, RD_IO, adap_fsm[ADAP0_PATH].ddr_buf[adap_fsm[ADAP0_PATH].wbuf_index], 0, 32); // ddr mode config frame address
			mipi_adap_reg_wr(MIPI_ADAPT_DDR_RD0_CNTL0, RD_IO, 0x70000000);
		}
		else if (adap_fsm[channel].para.mode == DOL_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_DDR_RD0_CNTL1, RD_IO, 0x04380096);
			adap_wr_reg_bits(MIPI_ADAPT_DDR_RD0_CNTL2, RD_IO, dol_buf[0], 0, 32);
			adap_wr_reg_bits(MIPI_ADAPT_DDR_RD0_CNTL3, RD_IO, dol_buf[1], 0, 32);
			mipi_adap_reg_wr(MIPI_ADAPT_DDR_RD0_CNTL0, RD_IO, 0xb5800000);

			mipi_adap_reg_wr(MIPI_ADAPT_DDR_RD1_CNTL1, RD_IO, 0x04380096);
			adap_wr_reg_bits(MIPI_ADAPT_DDR_RD1_CNTL2, RD_IO, dol_buf[0], 0, 32);
			adap_wr_reg_bits(MIPI_ADAPT_DDR_RD1_CNTL3, RD_IO, dol_buf[1], 0, 32);
			mipi_adap_reg_wr(MIPI_ADAPT_DDR_RD1_CNTL0, RD_IO, 0xf1c10004);
		}
		else
		{
			pr_err("%s, Not supported Mode.\n", __func__);
		}
	}
	else
	{
		if (adap_fsm[channel].para.mode == DIR_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_DDR_RD1_CNTL1, RD_IO, 0x02d00078);
			mipi_adap_reg_wr(MIPI_ADAPT_DDR_RD1_CNTL0, RD_IO, 0xb5000004);
		}
		else if (adap_fsm[channel].para.mode == DDR_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_DDR_RD1_CNTL1, RD_IO, 0x02d00078);
			adap_wr_reg_bits(MIPI_ADAPT_DDR_RD1_CNTL2, RD_IO, adap_fsm[ADAP1_PATH].ddr_buf[adap_fsm[ADAP1_PATH].wbuf_index], 0, 32); // ddr mode config frame address
			mipi_adap_reg_wr(MIPI_ADAPT_DDR_RD1_CNTL0, RD_IO, 0x70000000);
		}
		else if (adap_fsm[channel].para.mode == DCAM_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_DDR_RD1_CNTL1, RD_IO, 0x02d00078);
			adap_wr_reg_bits(MIPI_ADAPT_DDR_RD1_CNTL2, RD_IO, adap_fsm[ADAP1_PATH].ddr_buf[adap_fsm[ADAP1_PATH].wbuf_index], 0, 32); // ddr mode config frame address
			mipi_adap_reg_wr(MIPI_ADAPT_DDR_RD1_CNTL0, RD_IO, 0x70000000);
		}
		else if (adap_fsm[channel].para.mode == DOL_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_DDR_RD0_CNTL1, RD_IO, 0x04380096);
			adap_wr_reg_bits(MIPI_ADAPT_DDR_RD0_CNTL2, RD_IO, dol_buf[0], 0, 32);
			adap_wr_reg_bits(MIPI_ADAPT_DDR_RD0_CNTL3, RD_IO, dol_buf[1], 0, 32);
			mipi_adap_reg_wr(MIPI_ADAPT_DDR_RD0_CNTL0, RD_IO, 0xf1d00008);

			mipi_adap_reg_wr(MIPI_ADAPT_DDR_RD1_CNTL1, RD_IO, 0x04380096);
			adap_wr_reg_bits(MIPI_ADAPT_DDR_RD1_CNTL2, RD_IO, dol_buf[0], 0, 32);
			adap_wr_reg_bits(MIPI_ADAPT_DDR_RD1_CNTL3, RD_IO, dol_buf[1], 0, 32);
			mipi_adap_reg_wr(MIPI_ADAPT_DDR_RD1_CNTL0, RD_IO, 0xb5800000);
		}
		else
		{
			pr_err("%s, Not supported Mode.\n", __func__);
		}
	}

	return 0;
}

/*
 *========================AM ADAPTER PIXEL INTERFACE===========================
 */

void am_adap_pixel_start(uint8_t channel)
{
	int fmt = adap_fsm[channel].para.fmt;
	int width = adap_fsm[channel].para.img.width;

	if (channel == ADAP0_PATH)
	{
		adap_wr_reg_bits(MIPI_ADAPT_PIXEL0_CNTL0, PIXEL_IO, fmt, 13, 3);
		adap_wr_reg_bits(MIPI_ADAPT_PIXEL0_CNTL0, PIXEL_IO, width, 0, 13);
		adap_wr_reg_bits(MIPI_ADAPT_PIXEL0_CNTL1, PIXEL_IO, 1, 31, 1);

		if (adap_fsm[channel].para.mode == DOL_MODE)
		{
			adap_wr_reg_bits(MIPI_ADAPT_PIXEL1_CNTL0, PIXEL_IO, fmt, 13, 3);
			adap_wr_reg_bits(MIPI_ADAPT_PIXEL1_CNTL0, PIXEL_IO, width, 0, 13);
			adap_wr_reg_bits(MIPI_ADAPT_PIXEL1_CNTL1, PIXEL_IO, 1, 31, 1);
		}
	}
	else
	{
		adap_wr_reg_bits(MIPI_ADAPT_PIXEL1_CNTL0, PIXEL_IO, fmt, 13, 3);
		adap_wr_reg_bits(MIPI_ADAPT_PIXEL1_CNTL0, PIXEL_IO, width, 0, 13);
		adap_wr_reg_bits(MIPI_ADAPT_PIXEL1_CNTL1, PIXEL_IO, 1, 31, 1);

		if (adap_fsm[channel].para.mode == DOL_MODE)
		{
			adap_wr_reg_bits(MIPI_ADAPT_PIXEL0_CNTL0, PIXEL_IO, fmt, 13, 3);
			adap_wr_reg_bits(MIPI_ADAPT_PIXEL0_CNTL0, PIXEL_IO, width, 0, 13);
			adap_wr_reg_bits(MIPI_ADAPT_PIXEL0_CNTL1, PIXEL_IO, 1, 31, 1);
		}
	}
}

int am_adap_pixel_init(uint8_t channel)
{
	if (channel == ADAP0_PATH)
	{
		if (adap_fsm[channel].para.mode == DIR_MODE)
		{
			// default width 1280
			mipi_adap_reg_wr(MIPI_ADAPT_PIXEL0_CNTL0, PIXEL_IO, 0x8000a500);
			mipi_adap_reg_wr(MIPI_ADAPT_PIXEL0_CNTL1, PIXEL_IO, 0x00000808);
		}
		else if (adap_fsm[channel].para.mode == DDR_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_PIXEL0_CNTL0, PIXEL_IO, 0x0000a500);
			mipi_adap_reg_wr(MIPI_ADAPT_PIXEL0_CNTL1, PIXEL_IO, 0x00000008);
		}
		else if (adap_fsm[channel].para.mode == DCAM_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_PIXEL0_CNTL0, PIXEL_IO, 0x0000a500);
			mipi_adap_reg_wr(MIPI_ADAPT_PIXEL0_CNTL1, PIXEL_IO, 0x00000008);
		}
		else if (adap_fsm[channel].para.mode == DOL_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_PIXEL0_CNTL0, PIXEL_IO, 0x80008780);
			mipi_adap_reg_wr(MIPI_ADAPT_PIXEL0_CNTL1, PIXEL_IO, 0x00000008);

			mipi_adap_reg_wr(MIPI_ADAPT_PIXEL1_CNTL0, PIXEL_IO, 0x00008780);
			mipi_adap_reg_wr(MIPI_ADAPT_PIXEL1_CNTL1, PIXEL_IO, 0x00000008);
		}
		else
		{
			pr_err("%s, Not supported Mode.\n", __func__);
		}
	}
	else
	{
		if (adap_fsm[channel].para.mode == DIR_MODE)
		{
			// default width 1280
			mipi_adap_reg_wr(MIPI_ADAPT_PIXEL1_CNTL0, PIXEL_IO, 0x8000a500);
			mipi_adap_reg_wr(MIPI_ADAPT_PIXEL1_CNTL1, PIXEL_IO, 0x00000808);
		}
		else if (adap_fsm[channel].para.mode == DDR_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_PIXEL1_CNTL0, PIXEL_IO, 0x0000a500);
			mipi_adap_reg_wr(MIPI_ADAPT_PIXEL1_CNTL1, PIXEL_IO, 0x00000008);
		}
		else if (adap_fsm[channel].para.mode == DCAM_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_PIXEL1_CNTL0, PIXEL_IO, 0x0000a500);
			mipi_adap_reg_wr(MIPI_ADAPT_PIXEL1_CNTL1, PIXEL_IO, 0x00000008);
		}
		else if (adap_fsm[channel].para.mode == DOL_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_PIXEL0_CNTL0, PIXEL_IO, 0x00008780);
			mipi_adap_reg_wr(MIPI_ADAPT_PIXEL0_CNTL1, PIXEL_IO, 0x00000008);

			mipi_adap_reg_wr(MIPI_ADAPT_PIXEL1_CNTL0, PIXEL_IO, 0x80008780);
			mipi_adap_reg_wr(MIPI_ADAPT_PIXEL1_CNTL1, PIXEL_IO, 0x00000008);
		}
		else
		{
			pr_err("%s, Not supported Mode.\n", __func__);
		}
	}

	return 0;
}

/*
 *========================AM ADAPTER ALIGNMENT INTERFACE=======================
 */

void am_adap_alig_start(uint8_t channel)
{
	int width, height, alig_width, alig_height, val;
	width = adap_fsm[channel].para.img.width;
	height = adap_fsm[channel].para.img.height;
	alig_width = width + 6000; // hblank > 32 cycles, H x V x framerate < isp clock

	alig_width = adap_fsm[channel].para.align_width;
	if (channel)
	{
		if (adap_fsm[channel - 1].para.align_width > adap_fsm[channel].para.align_width)
			alig_width = adap_fsm[channel - 1].para.align_width;
	}
	else
	{
		if (adap_fsm[channel + 1].para.align_width > adap_fsm[channel].para.align_width)
			alig_width = adap_fsm[channel + 1].para.align_width;
	}

	alig_height = height + 60; // vblank > 48 lines
	val = width + 35;		   // width < val < alig_width

	if (channel == ADAP0_PATH)
	{
		adap_wr_reg_bits(MIPI_ADAPT_ALIG_CNTL0, ALIGN_IO, alig_width, 0, 13);
		adap_wr_reg_bits(MIPI_ADAPT_ALIG_CNTL0, ALIGN_IO, alig_height, 16, 13);
		adap_wr_reg_bits(MIPI_ADAPT_ALIG_CNTL1, ALIGN_IO, width, 16, 13);
		adap_wr_reg_bits(MIPI_ADAPT_ALIG_CNTL2, ALIGN_IO, height, 16, 13);

		if (adap_fsm[channel].para.mode == DIR_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL6, ALIGN_IO, 0x00fff03D);
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 0x05231020 | 0x200 | (3 << 10));
		}
		else if (adap_fsm[channel].para.mode == DDR_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL6, ALIGN_IO, 0x00ff100D);
			if ((adap_fsm[ADAP0_PATH].cam_en + adap_fsm[ADAP1_PATH].cam_en) == CAM_EN)
				mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 0x05231020 | 0x200 | (3 << 10));
		}
		else if (adap_fsm[channel].para.mode == DCAM_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL6, ALIGN_IO, 0x00ff100D);
			if ((adap_fsm[ADAP0_PATH].cam_en + adap_fsm[ADAP1_PATH].cam_en) == CAM_EN)
				mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 0x05231020 | 0x200 | (3 << 10));
		}

		adap_wr_reg_bits(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, val, 16, 13);
		if ((adap_fsm[ADAP0_PATH].cam_en + adap_fsm[ADAP1_PATH].cam_en) == CAM_EN)
			adap_wr_reg_bits(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 1, 31, 1);
	}
	else
	{
		adap_wr_reg_bits(MIPI_ADAPT_ALIG_CNTL3, ALIGN_IO, alig_width, 0, 13);
		adap_wr_reg_bits(MIPI_ADAPT_ALIG_CNTL3, ALIGN_IO, alig_height, 16, 13);
		adap_wr_reg_bits(MIPI_ADAPT_ALIG_CNTL4, ALIGN_IO, width, 16, 13);
		adap_wr_reg_bits(MIPI_ADAPT_ALIG_CNTL5, ALIGN_IO, height, 16, 13);

		if (adap_fsm[channel].para.mode == DIR_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL6, ALIGN_IO, 0x00fff03D);
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 0x05231020 | 0x300 | (3 << 10));
		}
		else if (adap_fsm[channel].para.mode == DDR_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL6, ALIGN_IO, 0x00ff100D);
			if ((adap_fsm[ADAP0_PATH].cam_en + adap_fsm[ADAP1_PATH].cam_en) == CAM_EN)
				mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 0x05231020 | 0x300 | (3 << 10));
		}
		else if (adap_fsm[channel].para.mode == DCAM_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL6, ALIGN_IO, 0x00ff100D);
			if ((adap_fsm[ADAP0_PATH].cam_en + adap_fsm[ADAP1_PATH].cam_en) == CAM_EN)
				mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 0x05231020 | 0x300 | (3 << 10));
		}

		adap_wr_reg_bits(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, val, 16, 13);
		if ((adap_fsm[ADAP0_PATH].cam_en + adap_fsm[ADAP1_PATH].cam_en) == CAM_EN)
			adap_wr_reg_bits(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 1, 31, 1);
	}
}

int am_adap_alig_init(uint8_t channel)
{
	if (channel == ADAP0_PATH)
	{
		if (adap_fsm[channel].para.mode == DOL_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL0, ALIGN_IO, 0x078a043a); // associate width and height
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL1, ALIGN_IO, 0x07800000); // associate width
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL2, ALIGN_IO, 0x04380000); // associate height
		}
		else
		{
			// default width 1280, height 720
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL0, ALIGN_IO, 0x02f80528); // associate width and height
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL1, ALIGN_IO, 0x05000000); // associate width
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL2, ALIGN_IO, 0x02d00000); // associate height
		}

		if (adap_fsm[channel].para.mode == DIR_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL6, ALIGN_IO, 0x00fff033);
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL7, ALIGN_IO, 0xc350c000);
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 0x05231020 | 0x200 | (3 << 10));
		}
		else if (adap_fsm[channel].para.mode == DDR_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL6, ALIGN_IO, 0x00ff100D);
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL7, ALIGN_IO, 0x0);
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 0x0000020 | 0x200 | (3 << 10));
		}
		else if (adap_fsm[channel].para.mode == DCAM_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL6, ALIGN_IO, 0x00ff100D);
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL7, ALIGN_IO, 0x0);
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 0x0000020 | 0x200 | (3 << 10));
		}
		else if (adap_fsm[channel].para.mode == DOL_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL6, ALIGN_IO, 0x00ff541d);
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL7, ALIGN_IO, 0xffffe000);
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 0x07881020);
		}
		else
		{
			pr_err("Not supported mode.\n");
		}

		if (adap_fsm[channel].para.mode == DDR_MODE)
		{
			adap_wr_reg_bits(MIPI_ADAPT_IRQ_MASK0, ALIGN_IO, 1, READ0_RD_DONE, 1);
			adap_wr_reg_bits(MIPI_ADAPT_IRQ_MASK0, ALIGN_IO, 1, FRONT0_WR_DONE, 1);
		}
		else if (adap_fsm[channel].para.mode == DCAM_MODE)
		{
			adap_wr_reg_bits(MIPI_ADAPT_IRQ_MASK0, ALIGN_IO, 1, READ0_RD_DONE, 1);
			adap_wr_reg_bits(MIPI_ADAPT_IRQ_MASK0, ALIGN_IO, 1, FRONT0_WR_DONE, 1);
		}
	}
	else
	{
		if (adap_fsm[channel].para.mode == DOL_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL3, ALIGN_IO, 0x078a043a); // associate width and height
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL4, ALIGN_IO, 0x07800000); // associate width
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL5, ALIGN_IO, 0x04380000); // associate height
		}
		else
		{
			// default width 1280, height 720
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL3, ALIGN_IO, 0x02f80528); // associate width and height
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL4, ALIGN_IO, 0x05000000); // associate width
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL5, ALIGN_IO, 0x02d00000); // associate height
		}

		if (adap_fsm[channel].para.mode == DIR_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL6, ALIGN_IO, 0x00fff033);
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL7, ALIGN_IO, 0xc350c000);
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 0x05231020 | 0x300 | (3 << 10));
		}
		else if (adap_fsm[channel].para.mode == DDR_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL6, ALIGN_IO, 0x00ff100D);
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL7, ALIGN_IO, 0x0);
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 0x0000020 | 0x300 | (3 << 10));
		}
		else if (adap_fsm[channel].para.mode == DCAM_MODE)
		{
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL6, ALIGN_IO, 0x00ff100D);
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL7, ALIGN_IO, 0x0);
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 0x0000020 | 0x300 | (3 << 10));
		}
		else if (adap_fsm[channel].para.mode == DOL_MODE)
		{
			// mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL6, ALIGN_IO, 0x00fff03D);
			// mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL7, ALIGN_IO, 0xffffe000);
			// mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 0x05231020 | 0x300 | (3<<10));
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL6, ALIGN_IO, 0x00ff542d);
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL7, ALIGN_IO, 0xffffe000);
			mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 0x07881020 | 0x300 | (3 << 10));
		}
		else
		{
			pr_err("Not supported mode.\n");
		}

		if (adap_fsm[channel].para.mode == DDR_MODE)
		{
			adap_wr_reg_bits(MIPI_ADAPT_IRQ_MASK0, ALIGN_IO, 1, FRONT1_WR_DONE, 1);
			adap_wr_reg_bits(MIPI_ADAPT_IRQ_MASK0, ALIGN_IO, 1, READ1_RD_DONE, 1);
		}
		else if (adap_fsm[channel].para.mode == DCAM_MODE)
		{
			adap_wr_reg_bits(MIPI_ADAPT_IRQ_MASK0, ALIGN_IO, 1, FRONT1_WR_DONE, 1);
			adap_wr_reg_bits(MIPI_ADAPT_IRQ_MASK0, ALIGN_IO, 1, READ1_RD_DONE, 1);
		}
	}

	return 0;
}

/*
 *========================AM ADAPTER INTERFACE==========================
 */

static int get_next_wr_buf_index(uint8_t channel)
{
	int index = 0;
	int total_size = DDR_BUF_SIZE;
	adap_fsm[channel].wbuf_index = adap_fsm[channel].wbuf_index + 1;

	index = adap_fsm[channel].wbuf_index % total_size;

	return index;
}

static void get_data_from_frontend(uint8_t fe_chan)
{
	resource_size_t val = 0;
	unsigned long flags;
	spin_lock_irqsave(&adap_fsm[fe_chan].adap_lock, flags);
	if (!kfifo_is_full(&adap_fsm[fe_chan].adapt_fifo))
	{
		kfifo_in(&adap_fsm[fe_chan].adapt_fifo, &adap_fsm[fe_chan].ddr_buf[adap_fsm[fe_chan].next_buf_index], sizeof(resource_size_t));
		if (fe_chan == ADAP0_PATH)
		{
			camera_frame_fifo[g_adap->write_frame_ptr++] = CAM0_ACT;
		}
		else if (fe_chan == ADAP1_PATH)
		{
			camera_frame_fifo[g_adap->write_frame_ptr++] = CAM1_ACT;
		}
		if (g_adap->write_frame_ptr == CAMERA_QUEUE_NUM)
			g_adap->write_frame_ptr = 0;
	}
	else
	{
		spin_unlock_irqrestore(&adap_fsm[fe_chan].adap_lock, flags);
		if (fe_chan == ADAP0_PATH)
		{
			adap_wr_reg_bits(CSI2_DDR_START_PIX, FRONTEND_IO, val, 0, 32);
		}
		else if (fe_chan == ADAP1_PATH)
		{
			adap_wr_reg_bits(CSI2_DDR_START_PIX + FTE1_OFFSET, FRONTEND_IO, val, 0, 32);
		}
		return;
	}
	spin_unlock_irqrestore(&adap_fsm[fe_chan].adap_lock, flags);
	adap_fsm[fe_chan].next_buf_index = get_next_wr_buf_index(fe_chan);
	val = adap_fsm[fe_chan].ddr_buf[adap_fsm[fe_chan].next_buf_index];
	if (fe_chan == ADAP0_PATH)
	{
		adap_wr_reg_bits(CSI2_DDR_START_PIX, FRONTEND_IO, val, 0, 32);
	}
	else if (fe_chan == ADAP1_PATH)
	{
		adap_wr_reg_bits(CSI2_DDR_START_PIX + FTE1_OFFSET, FRONTEND_IO, val, 0, 32);
	}
}

// static int fe0_count = 0;
static irqreturn_t adpapter_isr(int irq, void *para)
{

	uint32_t data = 0;
	mipi_adap_reg_rd(MIPI_ADAPT_IRQ_PENDING0, ALIGN_IO, &data);
	mipi_adap_reg_wr(MIPI_ADAPT_IRQ_PENDING0, ALIGN_IO, data);
	if ((data & (1 << FRONT0_WR_DONE)))
	{
		get_data_from_frontend(ADAP0_PATH);
		// if ((fe0_count++) % 100 == 0)
		// printk("fe0 write \n");
	}

	if (data & (1 << FRONT1_WR_DONE))
		get_data_from_frontend(ADAP1_PATH);

	if ((data & (1 << READ0_RD_DONE)) || (data & (1 << READ1_RD_DONE)))
	{
		adap_fsm[ADAP0_PATH].control_flag = 0;
		adap_fsm[ADAP1_PATH].control_flag = 0;
	}
	return IRQ_HANDLED;
}

static int am_adap_isp_check_status_software(uint8_t adapt_path)
{
	resource_size_t val = 0;
	int check_count = 0;
	int frame_state = 0;
	int kfifo_ret = -1;
	unsigned long flags;
	while (adap_fsm[ADAP0_PATH].control_flag || adap_fsm[ADAP1_PATH].control_flag)
	{
		if (check_count++ > 100)
		{
			pr_err("adapt read%d done timeout %d-%d.\n", adapt_path, adap_fsm[ADAP0_PATH].control_flag, adap_fsm[ADAP1_PATH].control_flag);
			adap_fsm[ADAP0_PATH].control_flag = 0;
			adap_fsm[ADAP1_PATH].control_flag = 0;
			frame_state = -1;
			break;
		}
		else
		{
			mdelay(1);
		}
	}
	spin_lock_irqsave(&adap_fsm[adapt_path].adap_lock, flags);
	kfifo_ret = kfifo_out(&adap_fsm[adapt_path].adapt_fifo, &val, sizeof(val));
	spin_unlock_irqrestore(&adap_fsm[adapt_path].adap_lock, flags);
	adap_fsm[adapt_path].read_buf = val;
	adap_fsm[adapt_path].control_flag = 1;

	if (g_adap->read_frame_ptr > 0)
	{
		adap_wr_reg_bits(MIPI_OTHER_CNTL0, RD_IO, camera_frame_fifo[g_adap->read_frame_ptr - 1], CAM_LAST, 1);
		// pr_info("adapt %d rd last camera frame fifo %d \n", adapt_path, camera_frame_fifo[g_adap->read_frame_ptr - 1]);
	}
	else
	{
		adap_wr_reg_bits(MIPI_OTHER_CNTL0, RD_IO, camera_frame_fifo[CAMERA_QUEUE_NUM - 1], CAM_LAST, 1);
		// pr_info("adapt %d rd last camera frame fifo %d \n", adapt_path, camera_frame_fifo[CAMERA_QUEUE_NUM - 1]);
	}
	adap_wr_reg_bits(MIPI_OTHER_CNTL0, RD_IO, camera_frame_fifo[g_adap->read_frame_ptr], CAM_CURRENT, 1);
	// pr_info("adapt %d rd current camera frame fifo %d \n", adapt_path, camera_frame_fifo[g_adap->read_frame_ptr]);
	adap_wr_reg_bits(MIPI_OTHER_CNTL0, RD_IO, camera_frame_fifo[(g_adap->read_frame_ptr + 1) % CAMERA_QUEUE_NUM], CAM_NEXT, 1);
	// pr_info("adapt %d rd next camera frame fifo %d \n", adapt_path, camera_frame_fifo[(g_adap->read_frame_ptr + 1)%CAMERA_QUEUE_NUM]);
	adap_wr_reg_bits(MIPI_OTHER_CNTL0, RD_IO, camera_frame_fifo[(g_adap->read_frame_ptr + 2) % CAMERA_QUEUE_NUM], CAM_NEXT_NEXT, 1);
	// pr_info("adapt %d rd next next camera frame fifo %d \n", adapt_path, camera_frame_fifo[(g_adap->read_frame_ptr + 2)%CAMERA_QUEUE_NUM]);

	g_adap->read_frame_ptr++;
	g_adap->read_frame_ptr %= CAMERA_QUEUE_NUM;
	return frame_state;
}

static int adap_stream_copy_thread(void *data)
{
	uint8_t frame_num = 0;
	int8_t frame_state = 0;
	int i;

	set_freezable();
	for (;;)
	{
		try_to_freeze();
		if (kthread_should_stop())
			break;

		frame_num = kfifo_len(&adap_fsm[ADAP0_PATH].adapt_fifo) + kfifo_len(&adap_fsm[ADAP1_PATH].adapt_fifo);
		if ((frame_num / 8) < 3)
			continue;
		for (i = 0; i < CAMS_MAX; i++)
		{
			frame_state = 0;
			int64_t frameDuration = 33333333L / 2;
			ktime_t kFrameDuration = ns_to_ktime(frameDuration);
			ktime_t kStartRealTime = ktime_get();
			ktime_t kFrameEndRealTime = ktime_add(kStartRealTime, kFrameDuration);
			if ((kfifo_len(&adap_fsm[i].adapt_fifo) > 0) && camera_frame_fifo[g_adap->read_frame_ptr] == i)
			{
				frame_state = am_adap_isp_check_status_software(i);
				if (i)
				{
					adap_wr_reg_bits(MIPI_ADAPT_DDR_RD1_CNTL2, RD_IO, adap_fsm[ADAP1_PATH].read_buf, 0, 32);
					if (frame_state == 0)
						mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 0x85231020 | 0x300 | (0 << 10));
					else
						mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 0x85231020 | 0x300 | (3 << 10));
					adap_wr_reg_bits(MIPI_ADAPT_DDR_RD1_CNTL0, RD_IO, 1, 31, 1);
				}
				else
				{
					adap_wr_reg_bits(MIPI_ADAPT_DDR_RD0_CNTL2, RD_IO, adap_fsm[ADAP0_PATH].read_buf, 0, 32);
					if (frame_state == 0)
						mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 0x85231020 | 0x200 | (0 << 10));
					else
						mipi_adap_reg_wr(MIPI_ADAPT_ALIG_CNTL8, ALIGN_IO, 0x85231020 | 0x200 | (3 << 10));
					adap_wr_reg_bits(MIPI_ADAPT_DDR_RD0_CNTL0, RD_IO, 1, 31, 1);
				}
				ktime_t kWorkDoneRealTime = ktime_get();
				if ((ktime_to_ms(ktime_sub(kFrameEndRealTime, kWorkDoneRealTime))) > 0)
				{
					mdelay(ktime_to_ms(ktime_sub(kFrameEndRealTime, kWorkDoneRealTime)));
				}
				else
				{
					// pr_info("%s: Adapt 0 time: %lld \n", __func__, ktime_to_ms(ktime_sub(kWorkDoneRealTime0, kFrameEndRealTime0)));
				}
			}
		}
	}
	return 0;
}

int am_adap_alloc_mem(uint8_t channel)
{
	if (adap_fsm[channel].para.mode == DDR_MODE)
	{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
		struct cma *cma_area;
		struct device *dev = &(g_adap->p_dev->dev);
		if (dev && dev->cma_area)
			cma_area = dev->cma_area;
		else
			cma_area = dma_contiguous_default_area;
		adap_fsm[channel].cma_pages = cma_alloc(cma_area, (g_adap->adap_buf_size * SZ_1M) >> PAGE_SHIFT, 0, 0);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
		adap_fsm[channel].cma_pages = dma_alloc_from_contiguous(
			&(g_adap->p_dev->dev),
			(g_adap->adap_buf_size * SZ_1M) >> PAGE_SHIFT, 0, false);
#else
		adap_fsm[channel].cma_pages = dma_alloc_from_contiguous(
			&(g_adap->p_dev->dev),
			(g_adap->adap_buf_size * SZ_1M) >> PAGE_SHIFT, 0);

#endif
		if (adap_fsm[channel].cma_pages)
		{
			adap_fsm[channel].buffer_start = page_to_phys(adap_fsm[channel].cma_pages);
		}
		else
		{
			pr_err("alloc cma pages failed.\n");
			return 0;
		}
	}
	else if (adap_fsm[channel].para.mode == DCAM_MODE)
	{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
		struct cma *cma_area;
		struct device *dev = &(g_adap->p_dev->dev);
		if (dev && dev->cma_area)
			cma_area = dev->cma_area;
		else
			cma_area = dma_contiguous_default_area;
		adap_fsm[channel].cma_pages = cma_alloc(cma_area, (g_adap->adap_buf_size * SZ_1M) >> PAGE_SHIFT, 0, 0);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
		adap_fsm[channel].cma_pages = dma_alloc_from_contiguous(
			&(g_adap->p_dev->dev),
			(g_adap->adap_buf_size * SZ_1M) >> PAGE_SHIFT, 0, false);
#else
		adap_fsm[channel].cma_pages = dma_alloc_from_contiguous(
			&(g_adap->p_dev->dev),
			(g_adap->adap_buf_size * SZ_1M) >> PAGE_SHIFT, 0);

#endif
		if (adap_fsm[channel].cma_pages)
		{
			adap_fsm[channel].buffer_start = page_to_phys(adap_fsm[channel].cma_pages);
		}
		else
		{
			pr_err("alloc cma pages failed.\n");
			return 0;
		}
	}
	else if (adap_fsm[channel].para.mode == DOL_MODE)
	{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
		struct cma *cma_area;
		struct device *dev = &(g_adap->p_dev->dev);
		if (dev && dev->cma_area)
			cma_area = dev->cma_area;
		else
			cma_area = dma_contiguous_default_area;
		adap_fsm[channel].cma_pages = cma_alloc(cma_area, (g_adap->adap_buf_size * SZ_1M) >> PAGE_SHIFT, 0, 0);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
		adap_fsm[channel].cma_pages = dma_alloc_from_contiguous(
			&(g_adap->p_dev->dev),
			(g_adap->adap_buf_size * SZ_1M) >> PAGE_SHIFT, 0, false);
#else
		adap_fsm[channel].cma_pages = dma_alloc_from_contiguous(
			&(g_adap->p_dev->dev),
			(g_adap->adap_buf_size * SZ_1M) >> PAGE_SHIFT, 0);

#endif
		if (adap_fsm[channel].cma_pages)
		{
			adap_fsm[channel].buffer_start = page_to_phys(adap_fsm[channel].cma_pages);
		}
		else
		{
			pr_err("alloc dol cma pages failed.\n");
			return 0;
		}
	}
	return 0;
}

int am_adap_free_mem(uint8_t channel)
{
	if (adap_fsm[channel].para.mode == DDR_MODE)
	{
		if (adap_fsm[channel].cma_pages)
		{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
			struct cma *cma_area;
			struct device *dev = &(g_adap->p_dev->dev);
			if (dev && dev->cma_area)
				cma_area = dev->cma_area;
			else
				cma_area = dma_contiguous_default_area;
			cma_release(cma_area, adap_fsm[channel].cma_pages, (g_adap->adap_buf_size * SZ_1M) >> PAGE_SHIFT);
#else
			dma_release_from_contiguous(
				&(g_adap->p_dev->dev),
				adap_fsm[channel].cma_pages,
				(g_adap->adap_buf_size * SZ_1M) >> PAGE_SHIFT);
#endif
			adap_fsm[channel].cma_pages = NULL;
			adap_fsm[channel].buffer_start = 0;
			pr_info("release alloc CMA buffer. channel:%d\n", channel);
		}
	}
	else if (adap_fsm[channel].para.mode == DCAM_MODE)
	{
		if (adap_fsm[channel].cma_pages)
		{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
			struct cma *cma_area;
			struct device *dev = &(g_adap->p_dev->dev);
			if (dev && dev->cma_area)
				cma_area = dev->cma_area;
			else
				cma_area = dma_contiguous_default_area;
			cma_release(cma_area, adap_fsm[channel].cma_pages, (g_adap->adap_buf_size * SZ_1M) >> PAGE_SHIFT);
#else
			dma_release_from_contiguous(
				&(g_adap->p_dev->dev),
				adap_fsm[channel].cma_pages,
				(g_adap->adap_buf_size * SZ_1M) >> PAGE_SHIFT);
#endif
			adap_fsm[channel].cma_pages = NULL;
			adap_fsm[channel].buffer_start = 0;
			pr_info("release alloc CMA buffer. channel:%d\n", channel);
		}
	}
	else if (adap_fsm[channel].para.mode == DOL_MODE)
	{
		if (adap_fsm[channel].cma_pages)
		{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
			struct cma *cma_area;
			struct device *dev = &(g_adap->p_dev->dev);
			if (dev && dev->cma_area)
				cma_area = dev->cma_area;
			else
				cma_area = dma_contiguous_default_area;
			cma_release(cma_area, adap_fsm[channel].cma_pages, (g_adap->adap_buf_size * SZ_1M) >> PAGE_SHIFT);
#else
			dma_release_from_contiguous(
				&(g_adap->p_dev->dev),
				adap_fsm[channel].cma_pages,
				(g_adap->adap_buf_size * SZ_1M) >> PAGE_SHIFT);
#endif
			adap_fsm[channel].cma_pages = NULL;
			adap_fsm[channel].buffer_start = 0;
			pr_info("release alloc dol CMA buffer. channel:%d\n", channel);
		}
	}
	return 0;
}

int am_adap_init(uint8_t channel)
{
	int ret = 0;
	int depth;
	int i;
	int kfifo_ret = 0;
	resource_size_t temp_buf;
	char *buf = NULL;
	uint32_t stride;
	int buf_cnt;

	adap_fsm[channel].control_flag = 0;
	adap_fsm[channel].wbuf_index = 0;
	adap_fsm[channel].next_buf_index = 0;

	if (adap_fsm[channel].cma_pages)
	{
		am_adap_free_mem(channel);
		adap_fsm[channel].cma_pages = NULL;
	}

	if ((adap_fsm[channel].para.mode == DDR_MODE) ||
		(adap_fsm[channel].para.mode == DCAM_MODE) ||
		(adap_fsm[channel].para.mode == DOL_MODE))
	{
		am_adap_alloc_mem(channel);
		spin_lock_init(&adap_fsm[channel].adap_lock);
		depth = am_adap_get_depth(channel);
		if ((adap_fsm[channel].cma_pages) && (adap_fsm[channel].para.mode == DDR_MODE))
		{
			// note important : ddr_buf[0] and ddr_buf[1] address should alignment 16 byte
			stride = (adap_fsm[channel].para.img.width * depth) / 8;
			stride = ((stride + (BOUNDARY - 1)) & (~(BOUNDARY - 1)));
			adap_fsm[channel].ddr_buf[0] = adap_fsm[channel].buffer_start;
			adap_fsm[channel].ddr_buf[0] = (adap_fsm[channel].ddr_buf[0] + (PAGE_SIZE - 1)) & (~(PAGE_SIZE - 1));
			temp_buf = adap_fsm[channel].ddr_buf[0];
			buf = phys_to_virt(adap_fsm[channel].ddr_buf[0]);
			memset(buf, 0x0, (stride * adap_fsm[channel].para.img.height));
			for (i = 1; i < DDR_BUF_SIZE; i++)
			{
				adap_fsm[channel].ddr_buf[i] = temp_buf + (stride * (adap_fsm[channel].para.img.height + 100));
				adap_fsm[channel].ddr_buf[i] = (adap_fsm[channel].ddr_buf[i] + (PAGE_SIZE - 1)) & (~(PAGE_SIZE - 1));
				temp_buf = adap_fsm[channel].ddr_buf[i];
				buf = phys_to_virt(adap_fsm[channel].ddr_buf[i]);
				memset(buf, 0x0, (stride * adap_fsm[channel].para.img.height));
			}
		}
		else if ((adap_fsm[channel].cma_pages) && (adap_fsm[channel].para.mode == DCAM_MODE))
		{
			// note important : ddr_buf[0] and ddr_buf[1] address should alignment 16 byte
			stride = (adap_fsm[channel].para.img.width * depth) / 8;
			stride = ((stride + (BOUNDARY - 1)) & (~(BOUNDARY - 1)));
			adap_fsm[channel].ddr_buf[0] = adap_fsm[channel].buffer_start;
			adap_fsm[channel].ddr_buf[0] = (adap_fsm[channel].ddr_buf[0] + (PAGE_SIZE - 1)) & (~(PAGE_SIZE - 1));
			temp_buf = adap_fsm[channel].ddr_buf[0];
			buf = phys_to_virt(adap_fsm[channel].ddr_buf[0]);
			memset(buf, 0x0, (stride * adap_fsm[channel].para.img.height));
			for (i = 1; i < DDR_BUF_SIZE; i++)
			{
				adap_fsm[channel].ddr_buf[i] = temp_buf + (stride * (adap_fsm[channel].para.img.height + 100));
				adap_fsm[channel].ddr_buf[i] = (adap_fsm[channel].ddr_buf[i] + (PAGE_SIZE - 1)) & (~(PAGE_SIZE - 1));
				temp_buf = adap_fsm[channel].ddr_buf[i];
				buf = phys_to_virt(adap_fsm[channel].ddr_buf[i]);
				memset(buf, 0x0, (stride * adap_fsm[channel].para.img.height));
			}
		}
		else if ((adap_fsm[channel].cma_pages) && (adap_fsm[channel].para.mode == DOL_MODE))
		{
			dol_buf[0] = adap_fsm[channel].buffer_start;
			dol_buf[0] = (dol_buf[0] + (PAGE_SIZE - 1)) & (~(PAGE_SIZE - 1));
			temp_buf = dol_buf[0];
			if (frontend1_flag)
				buf_cnt = DOL_BUF_SIZE;
			else
				buf_cnt = 2;
			for (i = 1; i < buf_cnt; i++)
			{
				dol_buf[i] = temp_buf + ((adap_fsm[channel].para.img.width) * (adap_fsm[channel].para.img.height) * depth) / 8;
				dol_buf[i] = (dol_buf[i] + (PAGE_SIZE - 1)) & (~(PAGE_SIZE - 1));
				temp_buf = dol_buf[i];
			}
		}
	}

	if (adap_fsm[channel].para.mode == DDR_MODE && g_adap->f_end_irq == 0)
	{
		ret = request_irq(g_adap->rd_irq, &adpapter_isr, IRQF_SHARED | IRQF_TRIGGER_HIGH,
						  "adapter-irq", (void *)g_adap);
		g_adap->f_end_irq = 1;
		pr_info("adapter irq = %d, ret = %d\n", g_adap->rd_irq, ret);
	}
	else if (adap_fsm[channel].para.mode == DCAM_MODE && g_adap->f_end_irq == 0)
	{
		ret = request_irq(g_adap->rd_irq, &adpapter_isr, IRQF_SHARED | IRQF_TRIGGER_HIGH,
						  "adapter-irq", (void *)g_adap);
		g_adap->f_end_irq = 1;
		pr_info("adapter irq = %d, ret = %d\n", g_adap->rd_irq, ret);
	}

	if (adap_fsm[channel].para.mode == DDR_MODE && g_adap->f_fifo == 0)
	{
		kfifo_ret = kfifo_alloc(&adap_fsm[ADAP0_PATH].adapt_fifo, 8 * DDR_BUF_SIZE, GFP_KERNEL);
		kfifo_ret = kfifo_alloc(&adap_fsm[ADAP1_PATH].adapt_fifo, 8 * DDR_BUF_SIZE, GFP_KERNEL);
		if (kfifo_ret)
		{
			pr_info("alloc adapter fifo failed.\n");
			return kfifo_ret;
		}

		init_waitqueue_head(&g_adap->frame_wq);
		static struct sched_param param;
		param.sched_priority = 2; // struct sched_param param = { .sched_priority = 2 };
		g_adap->kadap_stream = kthread_run(adap_stream_copy_thread, NULL, "adap-stream");
		sched_setscheduler(g_adap->kadap_stream, SCHED_IDLE, &param);
		wake_up_process(g_adap->kadap_stream);
		g_adap->f_fifo = 1;
	}
	else if (adap_fsm[channel].para.mode == DCAM_MODE && g_adap->f_fifo == 0)
	{
		kfifo_ret = kfifo_alloc(&adap_fsm[ADAP0_PATH].adapt_fifo, 8 * DDR_BUF_SIZE, GFP_KERNEL);
		kfifo_ret = kfifo_alloc(&adap_fsm[ADAP1_PATH].adapt_fifo, 8 * DDR_BUF_SIZE, GFP_KERNEL);
		if (kfifo_ret)
		{
			pr_info("alloc adapter fifo failed.\n");
			return kfifo_ret;
		}

		init_waitqueue_head(&g_adap->frame_wq);
		static struct sched_param param;
		param.sched_priority = 2; // struct sched_param param = { .sched_priority = 2 };
		g_adap->kadap_stream = kthread_run(adap_stream_copy_thread, NULL, "adap-stream");
		sched_setscheduler(g_adap->kadap_stream, SCHED_IDLE, &param);
		wake_up_process(g_adap->kadap_stream);
		g_adap->f_fifo = 1;
	}

	if (g_adap->f_adap == 0)
	{
		g_adap->frame_state = 0;
		am_adap_reset(channel);
	}
	g_adap->f_adap = 1;

	// default setting : 720p & RAW12
	am_adap_frontend_init(channel);
	am_adap_reader_init(channel);
	am_adap_pixel_init(channel);
	am_adap_alig_init(channel);

	if (adap_fsm[channel].para.mode != DCAM_MODE)
	{
		if (channel == ADAP0_PATH)
		{
			adap_wr_reg_bits(MIPI_OTHER_CNTL0, RD_IO, CAM0_ACT, CAM_LAST, 1);
			adap_wr_reg_bits(MIPI_OTHER_CNTL0, RD_IO, CAM0_ACT, CAM_CURRENT, 1);
			adap_wr_reg_bits(MIPI_OTHER_CNTL0, RD_IO, CAM0_ACT, CAM_NEXT, 1);
			adap_wr_reg_bits(MIPI_OTHER_CNTL0, RD_IO, CAM0_ACT, CAM_NEXT_NEXT, 1);
		}
		else
		{
			adap_wr_reg_bits(MIPI_OTHER_CNTL0, RD_IO, CAM1_ACT, CAM_LAST, 1);
			adap_wr_reg_bits(MIPI_OTHER_CNTL0, RD_IO, CAM1_ACT, CAM_CURRENT, 1);
			adap_wr_reg_bits(MIPI_OTHER_CNTL0, RD_IO, CAM1_ACT, CAM_NEXT, 1);
			adap_wr_reg_bits(MIPI_OTHER_CNTL0, RD_IO, CAM1_ACT, CAM_NEXT_NEXT, 1);
		}

		if (adap_fsm[channel].para.alt0_path == ADAP1_ALT0_PATH)
		{
			adap_wr_reg_bits(MIPI_OTHER_CNTL0, RD_IO, CAM1_ACT, CAM_LAST, 1);
			adap_wr_reg_bits(MIPI_OTHER_CNTL0, RD_IO, CAM1_ACT, CAM_CURRENT, 1);
			adap_wr_reg_bits(MIPI_OTHER_CNTL0, RD_IO, CAM1_ACT, CAM_NEXT, 1);
			adap_wr_reg_bits(MIPI_OTHER_CNTL0, RD_IO, CAM1_ACT, CAM_NEXT_NEXT, 1);
		}
	}

	return 0;
}

int am_adap_start(uint8_t channel, uint8_t dcam)
{
	adap_fsm[channel].cam_en = CAM_EN;

	if (dcam)
	{
		am_adap_alig_start(channel);
		am_adap_pixel_start(channel);
		am_adap_reader_start(channel);
		am_adap_frontend_start(channel);
		if ((adap_fsm[ADAP0_PATH].cam_en + adap_fsm[ADAP1_PATH].cam_en) == CAM_EN)
		{
			memset(camera_frame_fifo, 0, CAMERA_QUEUE_NUM * sizeof(uint32_t));
			g_adap->read_frame_ptr = 0;
			g_adap->write_frame_ptr = 0;
		}
	}
	else
	{
		am_adap_alig_start(channel);
		am_adap_pixel_start(channel);
		am_adap_reader_start(channel);
		am_adap_frontend_start(channel);
		adap_fsm[DUAL_CAM_EN - 1 - channel].cam_en = CAM_DIS;
	}

	return 0;
}

int am_adap_reset(uint8_t channel)
{
	mipi_adap_reg_wr(CSI2_GEN_CTRL0 + FTE1_OFFSET * channel, FRONTEND_IO, 0x00000000);
	adap_wr_reg_bits(MIPI_ADAPT_DDR_RD0_CNTL0 + FTE1_OFFSET * channel, RD_IO, 0, 0, 1);
	adap_wr_reg_bits(MIPI_ADAPT_DDR_RD1_CNTL0 + FTE1_OFFSET * channel, RD_IO, 0, 0, 1);
	system_timer_usleep(1000);

	mipi_adap_reg_wr(CSI2_CLK_RESET + FTE1_OFFSET * channel, FRONTEND_IO, 0x7);
	mipi_adap_reg_wr(CSI2_CLK_RESET + FTE1_OFFSET * channel, FRONTEND_IO, 0x6);
	mipi_adap_reg_wr(CSI2_CLK_RESET + FTE1_OFFSET * channel, FRONTEND_IO, 0x6);
	mipi_adap_reg_wr(CSI2_GEN_CTRL0 + FTE1_OFFSET * channel, FRONTEND_IO, 0x00000000);

	mipi_adap_reg_wr(MIPI_OTHER_CNTL0, ALIGN_IO, 0xf0000000);
	mipi_adap_reg_wr(MIPI_OTHER_CNTL0, ALIGN_IO, 0x00000000);

	return 0;
}

int am_adap_deinit(uint8_t channel)
{
	adap_fsm[channel].cam_en = CAM_DIS;

	if ((adap_fsm[ADAP0_PATH].cam_en + adap_fsm[ADAP1_PATH].cam_en) == CAM_DIS)
	{
		am_adap_reset(ADAP0_PATH);
		am_adap_reset(ADAP1_PATH);
	}

	if (adap_fsm[channel].para.mode == DDR_MODE)
	{
		am_disable_irq(channel);
		// am_disable_irq(ADAP1_PATH);
		am_adap_free_mem(channel);
		if (g_adap->f_fifo && ((adap_fsm[ADAP0_PATH].cam_en + adap_fsm[ADAP1_PATH].cam_en) == CAM_DIS))
		{
			g_adap->f_fifo = 0;
			kfifo_free(&adap_fsm[ADAP0_PATH].adapt_fifo);
			kfifo_free(&adap_fsm[ADAP1_PATH].adapt_fifo);
		}
	}
	else if (adap_fsm[channel].para.mode == DCAM_MODE)
	{
		am_disable_irq(channel);
		// am_disable_irq(ADAP1_PATH);
		am_adap_free_mem(channel);
		if (g_adap->f_fifo && ((adap_fsm[ADAP0_PATH].cam_en + adap_fsm[ADAP1_PATH].cam_en) == CAM_DIS))
		{
			g_adap->f_fifo = 0;
			kfifo_free(&adap_fsm[ADAP0_PATH].adapt_fifo);
			kfifo_free(&adap_fsm[ADAP1_PATH].adapt_fifo);
		}
	}
	else if (adap_fsm[channel].para.mode == DOL_MODE)
	{
		am_adap_free_mem(channel);
	}

	if (g_adap->f_end_irq && ((adap_fsm[ADAP0_PATH].cam_en + adap_fsm[ADAP1_PATH].cam_en) == CAM_DIS))
	{
		g_adap->f_end_irq = 0;
		free_irq(g_adap->rd_irq, (void *)g_adap);
	}

	adap_fsm[channel].control_flag = 0;
	adap_fsm[channel].wbuf_index = 0;
	adap_fsm[channel].next_buf_index = 0;

	if (g_adap->kadap_stream != NULL && ((adap_fsm[ADAP0_PATH].cam_en + adap_fsm[ADAP1_PATH].cam_en) == CAM_DIS))
	{
		kthread_stop(g_adap->kadap_stream);
		g_adap->kadap_stream = NULL;
		g_adap->f_adap = 0;
		g_adap->read_frame_ptr = 0;
		g_adap->write_frame_ptr = 0;
	}

	return 0;
}
#endif
