// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#define pr_fmt(fmt) "rdma: " fmt

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>

#include <linux/string.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/ctype.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/clk.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/reset.h>

#include <linux/amlogic/media/registers/cpu_version.h>
#include <linux/amlogic/media/utils/vdec_reg.h>
#include <linux/amlogic/media/rdma/rdma_mgr.h>
#include <linux/amlogic/media/utils/am_com.h>
#include <linux/amlogic/media/vout/vinfo.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#ifdef CONFIG_AMLOGIC_VPU
#include <linux/amlogic/media/vpu/vpu.h>
#endif
#include "rdma.h"

#define DRIVER_NAME "amlogic-rdma"
#define MODULE_NAME "amlogic-rdma"
#define DEVICE_NAME "rdma"
#define CLASS_NAME  "rdma-class"

#define pr_dbg(fmt, args...)   pr_info("RDMA: " fmt, ## args)
#define pr_error(fmt, args...) pr_err("RDMA: " fmt, ## args)

#define rdma_io_read(addr)      readl(addr)
#define rdma_io_write(addr, val) writel((val), addr)

/* #define SKIP_OSD_CHANNEL */
#define RDMA_NUM        16
int has_multi_vpp;
int rdma_mgr_irq_request;
int rdma_reset_trigger_flag[RDMA_NUM];

struct reset_control *rdma_rst;
static int debug_flag;
/* burst size 0=16; 1=24; 2=32; 3=48.*/
static int ctrl_ahb_rd_burst_size = 3;
static int ctrl_ahb_wr_burst_size = 3;
static int rdma_watchdog = 20;
static int reset_count;
static int rdma_watchdog_count[RDMA_NUM];
static int rdma_force_reset = -1;

#define RDMA_TABLE_SIZE (8 * (PAGE_SIZE))
#define MAX_TRACE_NUM  16
#define RDMA_MGR_CLASS_NAME  "rdma_mgr"
static int rdma_trace_num;
static int rdma_trace_enable;
static int rdma_trace_channel;
static u32 rdma_trace_reg[MAX_TRACE_NUM];
static int rdma_table_size = RDMA_TABLE_SIZE;

static int g_vsync_rdma_item_count;
static int g_vsync_rdma_item_count_max;
static int enable[RDMA_NUM];
int rdma_configured[RDMA_NUM];
ulong rdma_config_us[RDMA_NUM];
int enc_num_configed[RDMA_NUM] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

MODULE_PARM_DESC(g_vsync_rdma_item_count, "\n g_vsync_rdma_item_count\n");
module_param(g_vsync_rdma_item_count, uint, 0664);

MODULE_PARM_DESC(g_vsync_rdma_item_count_max, "\n g_vsync_rdma_item_count_max\n");
module_param(g_vsync_rdma_item_count_max, uint, 0664);

struct rdma_irq_reg_s {
	u32 reg;
	u32 start;
	u32 len;
};

struct rdma_regadr_s {
	u32 rdma_ahb_start_addr;
	u32 rdma_ahb_start_addr_msb;
	u32 rdma_ahb_end_addr;
	u32 rdma_ahb_end_addr_msb;
	u32 trigger_mask_reg;
	u32 trigger_mask_reg_bitpos;
	u32 addr_inc_reg;
	u32 addr_inc_reg_bitpos;
	u32 rw_flag_reg;
	u32 rw_flag_reg_bitpos;
	u32 clear_irq_bitpos;
	u32 irq_status_bitpos;
};

struct rdma_instance_s {
	int not_process;
	struct rdma_regadr_s *rdma_regadr;
	struct rdma_op_s *op;
	void *op_arg;
	int rdma_table_size;
	u32 *reg_buf;
	dma_addr_t dma_handle;
	u32 *rdma_table_addr;
	u32 *rdma_table_mirror;
	ulong rdma_table_phy_addr;
	int rdma_item_count;
	int rdma_write_count;
	unsigned char keep_buf;
	unsigned char used;
	int prev_trigger_type;
	int prev_read_count;
	int lock_flag;
	int irq_count;
	int rdma_config_count;
	int rdma_empty_config_count;
};

#define MAX_CONFLICT 32
struct rdma_conflict_regs_s {
	u32 adr[MAX_CONFLICT];
	u32 val[MAX_CONFLICT];
};

struct rdma_device_info {
	const char *device_name;
	struct platform_device *rdma_dev;
	struct rdma_instance_s rdma_ins[RDMA_NUM];
	struct rdma_conflict_regs_s rdma_reg;
};

static struct rdma_device_data_s rdma_meson_dev;
static struct rdma_irq_reg_s irq_status;
static int rdma_ctrl;
static int rdma_access_man;
int rdma_status_reg;
static int support_64bit_addr;
static DEFINE_SPINLOCK(rdma_lock);

static struct rdma_device_info rdma_info;
#ifdef CONFIG_AMLOGIC_VPU
static struct vpu_dev_s *rdma_vpu_dev;
#endif
static struct rdma_regadr_s rdma_regadr[] = {
	{RDMA_AHB_START_ADDR_MAN,
		0,
		RDMA_AHB_END_ADDR_MAN,
		0,
		0, 0,
		RDMA_ACCESS_MAN, 1,
		RDMA_ACCESS_MAN, 2,
		24, 24
	},
	{RDMA_AHB_START_ADDR_1,
		0,
		RDMA_AHB_END_ADDR_1,
		0,
		RDMA_ACCESS_AUTO,  8,
		RDMA_ACCESS_AUTO,  1,
		RDMA_ACCESS_AUTO,  5,
		25, 25
	},
	{RDMA_AHB_START_ADDR_2,
		0,
		RDMA_AHB_END_ADDR_2,
		0,
		RDMA_ACCESS_AUTO,  16,
		RDMA_ACCESS_AUTO,  2,
		RDMA_ACCESS_AUTO,  6,
		26, 26
	},
	{RDMA_AHB_START_ADDR_3,
		0,
		RDMA_AHB_END_ADDR_3,
		0,
		RDMA_ACCESS_AUTO,  24,
		RDMA_ACCESS_AUTO,  3,
		RDMA_ACCESS_AUTO,  7,
		27, 27
	},
	{RDMA_AHB_START_ADDR_4,
		0,
		RDMA_AHB_END_ADDR_4,
		0,
		RDMA_ACCESS_AUTO3, 0,
		RDMA_ACCESS_AUTO2, 0,
		RDMA_ACCESS_AUTO2, 4,
		28, 28
	},
	{RDMA_AHB_START_ADDR_5,
		0,
		RDMA_AHB_END_ADDR_5,
		0,
		RDMA_ACCESS_AUTO3, 8,
		RDMA_ACCESS_AUTO2, 1,
		RDMA_ACCESS_AUTO2, 5,
		29, 29
	},
	{RDMA_AHB_START_ADDR_6,
		0,
		RDMA_AHB_END_ADDR_6,
		0,
		RDMA_ACCESS_AUTO3, 16,
		RDMA_ACCESS_AUTO2, 2,
		RDMA_ACCESS_AUTO2, 6,
		30, 30
	},
	{RDMA_AHB_START_ADDR_7,
		0,
		RDMA_AHB_END_ADDR_7,
		0,
		RDMA_ACCESS_AUTO3, 24,
		RDMA_ACCESS_AUTO2, 3,
		RDMA_ACCESS_AUTO2, 7,
		31, 31
	}
};

static struct rdma_regadr_s rdma_regadr_tl1[] = {
	{RDMA_AHB_START_ADDR_MAN,
		0,
		RDMA_AHB_END_ADDR_MAN,
		0,
		0, 0,
		RDMA_ACCESS_MAN, 1,
		RDMA_ACCESS_MAN, 2,
		24, 24
	},
	{RDMA_AHB_START_ADDR_1,
		0,
		RDMA_AHB_END_ADDR_1,
		0,
		RDMA_AUTO_SRC1_SEL,  0,
		RDMA_ACCESS_AUTO,  1,
		RDMA_ACCESS_AUTO,  5,
		25, 25
	},
	{RDMA_AHB_START_ADDR_2,
		0,
		RDMA_AHB_END_ADDR_2,
		0,
		RDMA_AUTO_SRC2_SEL,  0,
		RDMA_ACCESS_AUTO,  2,
		RDMA_ACCESS_AUTO,  6,
		26, 26
	},
	{RDMA_AHB_START_ADDR_3,
		0,
		RDMA_AHB_END_ADDR_3,
		0,
		RDMA_AUTO_SRC3_SEL,  0,
		RDMA_ACCESS_AUTO,  3,
		RDMA_ACCESS_AUTO,  7,
		27, 27
	},
	{RDMA_AHB_START_ADDR_4,
		0,
		RDMA_AHB_END_ADDR_4,
		0,
		RDMA_AUTO_SRC4_SEL, 0,
		RDMA_ACCESS_AUTO2, 0,
		RDMA_ACCESS_AUTO2, 4,
		28, 28
	},
	{RDMA_AHB_START_ADDR_5,
		0,
		RDMA_AHB_END_ADDR_5,
		0,
		RDMA_AUTO_SRC5_SEL, 0,
		RDMA_ACCESS_AUTO2, 1,
		RDMA_ACCESS_AUTO2, 5,
		29, 29
	},
	{RDMA_AHB_START_ADDR_6,
		0,
		RDMA_AHB_END_ADDR_6,
		0,
		RDMA_AUTO_SRC6_SEL, 0,
		RDMA_ACCESS_AUTO2, 2,
		RDMA_ACCESS_AUTO2, 6,
		30, 30
	},
	{RDMA_AHB_START_ADDR_7,
		0,
		RDMA_AHB_END_ADDR_7,
		0,
		RDMA_AUTO_SRC7_SEL, 0,
		RDMA_ACCESS_AUTO2, 3,
		RDMA_ACCESS_AUTO2, 7,
		31, 31
	}
};

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
static struct rdma_regadr_s rdma_regadr_t7[] = {
	{RDMA_AHB_START_ADDR_MAN,
		RDMA_AHB_START_ADDR_MAN_MSB,
		RDMA_AHB_END_ADDR_MAN,
		RDMA_AHB_END_ADDR_MAN_MSB,
		0, 0,
		RDMA_ACCESS_MAN, 1,
		RDMA_ACCESS_MAN, 2,
		24, 24
	},
	{RDMA_AHB_START_ADDR_1,
		RDMA_AHB_START_ADDR_1_MSB,
		RDMA_AHB_END_ADDR_1,
		RDMA_AHB_END_ADDR_1_MSB,
		RDMA_AUTO_SRC1_SEL,  0,
		RDMA_ACCESS_AUTO,  1,
		RDMA_ACCESS_AUTO,  5,
		25, 25
	},
	{RDMA_AHB_START_ADDR_2,
		RDMA_AHB_START_ADDR_2_MSB,
		RDMA_AHB_END_ADDR_2,
		RDMA_AHB_END_ADDR_2_MSB,
		RDMA_AUTO_SRC2_SEL,  0,
		RDMA_ACCESS_AUTO,  2,
		RDMA_ACCESS_AUTO,  6,
		26, 26
	},
	{RDMA_AHB_START_ADDR_3,
		RDMA_AHB_START_ADDR_3_MSB,
		RDMA_AHB_END_ADDR_3,
		RDMA_AHB_END_ADDR_3_MSB,
		RDMA_AUTO_SRC3_SEL,  0,
		RDMA_ACCESS_AUTO,  3,
		RDMA_ACCESS_AUTO,  7,
		27, 27
	},
	{RDMA_AHB_START_ADDR_4,
		RDMA_AHB_START_ADDR_4_MSB,
		RDMA_AHB_END_ADDR_4,
		RDMA_AHB_END_ADDR_4_MSB,
		RDMA_AUTO_SRC4_SEL, 0,
		RDMA_ACCESS_AUTO2, 0,
		RDMA_ACCESS_AUTO2, 4,
		28, 28
	},
	{RDMA_AHB_START_ADDR_5,
		RDMA_AHB_START_ADDR_5_MSB,
		RDMA_AHB_END_ADDR_5,
		RDMA_AHB_END_ADDR_5_MSB,
		RDMA_AUTO_SRC5_SEL, 0,
		RDMA_ACCESS_AUTO2, 1,
		RDMA_ACCESS_AUTO2, 5,
		29, 29
	},
	{RDMA_AHB_START_ADDR_6,
		RDMA_AHB_START_ADDR_6_MSB,
		RDMA_AHB_END_ADDR_6,
		RDMA_AHB_END_ADDR_6_MSB,
		RDMA_AUTO_SRC6_SEL, 0,
		RDMA_ACCESS_AUTO2, 2,
		RDMA_ACCESS_AUTO2, 6,
		30, 30
	},
	{RDMA_AHB_START_ADDR_7,
		RDMA_AHB_START_ADDR_7_MSB,
		RDMA_AHB_END_ADDR_7,
		RDMA_AHB_END_ADDR_7_MSB,
		RDMA_AUTO_SRC7_SEL, 0,
		RDMA_ACCESS_AUTO2, 3,
		RDMA_ACCESS_AUTO2, 7,
		31, 31
	}
};

static struct rdma_regadr_s rdma_regadr_t3x[] = {
	{T3X_RDMA_AHB_START_ADDR_MAN,
		T3X_RDMA_AHB_START_ADDR_MAN_MSB,
		T3X_RDMA_AHB_END_ADDR_MAN,
		T3X_RDMA_AHB_END_ADDR_MAN_MSB,
		0, 0,
		T3X_RDMA_ACCESS_MAN, 1,
		T3X_RDMA_ACCESS_MAN, 2,
		16, 4
	},
	{T3X_RDMA_AHB_START_ADDR_1,
		T3X_RDMA_AHB_START_ADDR_1_MSB,
		T3X_RDMA_AHB_END_ADDR_1,
		T3X_RDMA_AHB_END_ADDR_1_MSB,
		T3X_RDMA_AUTO_SRC1_SEL,  0,
		T3X_RDMA_ACCESS_AUTO,  1,
		T3X_RDMA_ACCESS_AUTO,  5,
		17, 5
	},
	{T3X_RDMA_AHB_START_ADDR_2,
		T3X_RDMA_AHB_START_ADDR_2_MSB,
		T3X_RDMA_AHB_END_ADDR_2,
		T3X_RDMA_AHB_END_ADDR_2_MSB,
		T3X_RDMA_AUTO_SRC2_SEL,  0,
		T3X_RDMA_ACCESS_AUTO,  2,
		T3X_RDMA_ACCESS_AUTO,  6,
		18, 6
	},
	{T3X_RDMA_AHB_START_ADDR_3,
		T3X_RDMA_AHB_START_ADDR_3_MSB,
		T3X_RDMA_AHB_END_ADDR_3,
		T3X_RDMA_AHB_END_ADDR_3_MSB,
		T3X_RDMA_AUTO_SRC3_SEL,  0,
		T3X_RDMA_ACCESS_AUTO,  3,
		T3X_RDMA_ACCESS_AUTO,  7,
		19, 7
	},
	{T3X_RDMA_AHB_START_ADDR_4,
		T3X_RDMA_AHB_START_ADDR_4_MSB,
		T3X_RDMA_AHB_END_ADDR_4,
		T3X_RDMA_AHB_END_ADDR_4_MSB,
		T3X_RDMA_AUTO_SRC4_SEL, 0,
		T3X_RDMA_ACCESS_AUTO2, 0,
		T3X_RDMA_ACCESS_AUTO2, 4,
		20, 8
	},
	{T3X_RDMA_AHB_START_ADDR_5,
		T3X_RDMA_AHB_START_ADDR_5_MSB,
		T3X_RDMA_AHB_END_ADDR_5,
		T3X_RDMA_AHB_END_ADDR_5_MSB,
		T3X_RDMA_AUTO_SRC5_SEL, 0,
		T3X_RDMA_ACCESS_AUTO2, 1,
		T3X_RDMA_ACCESS_AUTO2, 5,
		21, 9
	},
	{T3X_RDMA_AHB_START_ADDR_6,
		T3X_RDMA_AHB_START_ADDR_6_MSB,
		T3X_RDMA_AHB_END_ADDR_6,
		T3X_RDMA_AHB_END_ADDR_6_MSB,
		T3X_RDMA_AUTO_SRC6_SEL, 0,
		T3X_RDMA_ACCESS_AUTO2, 2,
		T3X_RDMA_ACCESS_AUTO2, 6,
		22, 10
	},
	{T3X_RDMA_AHB_START_ADDR_7,
		T3X_RDMA_AHB_START_ADDR_7_MSB,
		T3X_RDMA_AHB_END_ADDR_7,
		T3X_RDMA_AHB_END_ADDR_7_MSB,
		T3X_RDMA_AUTO_SRC7_SEL, 0,
		T3X_RDMA_ACCESS_AUTO2, 3,
		T3X_RDMA_ACCESS_AUTO2, 7,
		23, 11
	},
	{T3X_RDMA_AHB_START_ADDR_8,
		T3X_RDMA_AHB_START_ADDR_8_MSB,
		T3X_RDMA_AHB_END_ADDR_8,
		T3X_RDMA_AHB_END_ADDR_8_MSB,
		T3X_RDMA_AUTO_SRC8_SEL, 0,
		T3X_RDMA_ACCESS_AUTO3, 0,
		T3X_RDMA_ACCESS_AUTO3, 8,
		24, 12
	},
	{T3X_RDMA_AHB_START_ADDR_9,
		T3X_RDMA_AHB_START_ADDR_9_MSB,
		T3X_RDMA_AHB_END_ADDR_9,
		T3X_RDMA_AHB_END_ADDR_9_MSB,
		T3X_RDMA_AUTO_SRC9_SEL,  0,
		T3X_RDMA_ACCESS_AUTO3,  1,
		T3X_RDMA_ACCESS_AUTO3,  9,
		25, 13
	},
	{T3X_RDMA_AHB_START_ADDR_10,
		T3X_RDMA_AHB_START_ADDR_10_MSB,
		T3X_RDMA_AHB_END_ADDR_10,
		T3X_RDMA_AHB_END_ADDR_10_MSB,
		T3X_RDMA_AUTO_SRC10_SEL,  0,
		T3X_RDMA_ACCESS_AUTO3,  2,
		T3X_RDMA_ACCESS_AUTO3,  10,
		26, 14
	},
	{T3X_RDMA_AHB_START_ADDR_11,
		T3X_RDMA_AHB_START_ADDR_11_MSB,
		T3X_RDMA_AHB_END_ADDR_11,
		T3X_RDMA_AHB_END_ADDR_11_MSB,
		T3X_RDMA_AUTO_SRC11_SEL,  0,
		T3X_RDMA_ACCESS_AUTO3,  3,
		T3X_RDMA_ACCESS_AUTO3,  11,
		27, 15
	},
	{T3X_RDMA_AHB_START_ADDR_12,
		T3X_RDMA_AHB_START_ADDR_12_MSB,
		T3X_RDMA_AHB_END_ADDR_12,
		T3X_RDMA_AHB_END_ADDR_12_MSB,
		T3X_RDMA_AUTO_SRC12_SEL, 0,
		T3X_RDMA_ACCESS_AUTO3, 4,
		T3X_RDMA_ACCESS_AUTO3, 12,
		28, 16
	},
	{T3X_RDMA_AHB_START_ADDR_13,
		T3X_RDMA_AHB_START_ADDR_13_MSB,
		T3X_RDMA_AHB_END_ADDR_13,
		T3X_RDMA_AHB_END_ADDR_13_MSB,
		T3X_RDMA_AUTO_SRC13_SEL, 0,
		T3X_RDMA_ACCESS_AUTO3, 5,
		T3X_RDMA_ACCESS_AUTO3, 13,
		29, 17
	},
	{T3X_RDMA_AHB_START_ADDR_14,
		T3X_RDMA_AHB_START_ADDR_14_MSB,
		T3X_RDMA_AHB_END_ADDR_14,
		T3X_RDMA_AHB_END_ADDR_14_MSB,
		T3X_RDMA_AUTO_SRC14_SEL, 0,
		T3X_RDMA_ACCESS_AUTO3, 6,
		T3X_RDMA_ACCESS_AUTO3, 14,
		30, 18
	},
	{T3X_RDMA_AHB_START_ADDR_15,
		T3X_RDMA_AHB_START_ADDR_15_MSB,
		T3X_RDMA_AHB_END_ADDR_15,
		T3X_RDMA_AHB_END_ADDR_15_MSB,
		T3X_RDMA_AUTO_SRC15_SEL, 0,
		T3X_RDMA_ACCESS_AUTO3, 7,
		T3X_RDMA_ACCESS_AUTO3, 15,
		31, 19
	}
};
#endif

int rdma_register(struct rdma_op_s *rdma_op, void *op_arg, int table_size)
{
	int i;
	unsigned long flags;
	struct rdma_device_info *info = &rdma_info;
	dma_addr_t dma_handle;

	spin_lock_irqsave(&rdma_lock, flags);
	for (i = 1; i < rdma_meson_dev.channel_num; i++) {
		/* 0 is reserved for RDMA MANUAL */
		if (!info->rdma_ins[i].op &&
		    info->rdma_ins[i].used == 0) {
			info->rdma_ins[i].op = rdma_op;
			info->rdma_ins[i].irq_count = 0;
			info->rdma_ins[i].rdma_config_count = 0;
			info->rdma_ins[i].rdma_empty_config_count = 0;
			break;
		}
	}
	spin_unlock_irqrestore(&rdma_lock, flags);
	if (i < rdma_meson_dev.channel_num) {
		info->rdma_ins[i].not_process = 0;
		info->rdma_ins[i].op_arg = op_arg;
		info->rdma_ins[i].rdma_item_count = 0;
		info->rdma_ins[i].rdma_write_count = 0;
		info->rdma_ins[i].prev_read_count = 0;
		if (info->rdma_ins[i].rdma_table_size == 0) {
			info->rdma_ins[i].rdma_table_addr =
				dma_alloc_coherent
				(&info->rdma_dev->dev, table_size,
				&dma_handle, GFP_KERNEL);
			info->rdma_ins[i].rdma_table_phy_addr =
				(ulong)(dma_handle);
			info->rdma_ins[i].reg_buf =
				kmalloc(table_size, GFP_KERNEL);
			info->rdma_ins[i].rdma_table_mirror =
				kmalloc(table_size, GFP_KERNEL);
			pr_debug("%s, rdma_table_addr %lx phy: %lx reg_buf %lx\n",
				__func__,
				(unsigned long)
				info->rdma_ins[i].rdma_table_addr,
				info->rdma_ins[i].rdma_table_phy_addr,
				(unsigned long)info->rdma_ins[i].reg_buf);
			info->rdma_ins[i].rdma_table_size = table_size;
		}

		if (!info->rdma_ins[i].rdma_table_addr ||
		    !info->rdma_ins[i].reg_buf ||
		    !info->rdma_ins[i].rdma_table_mirror) {
			if (!info->rdma_ins[i].keep_buf) {
				kfree(info->rdma_ins[i].reg_buf);
				info->rdma_ins[i].reg_buf = NULL;
				kfree(info->rdma_ins[i].rdma_table_mirror);
				info->rdma_ins[i].rdma_table_mirror = NULL;
			}
			if (info->rdma_ins[i].rdma_table_addr) {
				dma_free_coherent
				(&info->rdma_dev->dev,
				 table_size,
				 info->rdma_ins[i].rdma_table_addr,
				 (dma_addr_t)
				 info->rdma_ins[i].rdma_table_phy_addr);
				 info->rdma_ins[i].rdma_table_addr = NULL;
			}
			info->rdma_ins[i].rdma_table_size  = 0;
			info->rdma_ins[i].op = NULL;
			i = -1;
			pr_info("%s: memory allocate fail\n",
				__func__);
		} else {
			pr_debug("%s success, handle %d table_size %d\n",
				__func__, i, table_size);
		}
		return i;
	}

	return -1;
}
EXPORT_SYMBOL(rdma_register);

void rdma_unregister(int i)
{
	unsigned long flags;
	struct rdma_device_info *info = &rdma_info;

	pr_info("%s(%d)\r\n", __func__, i);
	if (i > 0 && i < rdma_meson_dev.channel_num && info->rdma_ins[i].op) {
		/*rdma_clear(i);*/
		info->rdma_ins[i].op_arg = NULL;
		if (!info->rdma_ins[i].keep_buf) {
			kfree(info->rdma_ins[i].reg_buf);
			info->rdma_ins[i].reg_buf = NULL;
			kfree(info->rdma_ins[i].rdma_table_mirror);
			info->rdma_ins[i].rdma_table_mirror = NULL;
		}
		if (info->rdma_ins[i].rdma_table_addr) {
			dma_free_coherent
			(&info->rdma_dev->dev,
			 info->rdma_ins[i].rdma_table_size,
			 info->rdma_ins[i].rdma_table_addr,
			 (dma_addr_t)
			 info->rdma_ins[i].rdma_table_phy_addr);
			 info->rdma_ins[i].rdma_table_addr = NULL;
		}
		info->rdma_ins[i].rdma_table_size = 0;
		spin_lock_irqsave(&rdma_lock, flags);
		info->rdma_ins[i].op = NULL;
		spin_unlock_irqrestore(&rdma_lock, flags);
	}
}
EXPORT_SYMBOL(rdma_unregister);
static void rdma_reset(unsigned char external_reset)
{
	if (debug_flag & 4)
		pr_info("%s(%d)\n",
			__func__, external_reset);

	if (external_reset) {
		if (rdma_meson_dev.cpu_type >= CPU_SC2)
			reset_control_reset(rdma_rst);
		else
			WRITE_MPEG_REG
				(RESET4_REGISTER,
				(1 << 5));
	} else {
		WRITE_VCBUS_REG(rdma_ctrl, (0x1 << 1));
		WRITE_VCBUS_REG(rdma_ctrl, (0x1 << 1));
		WRITE_VCBUS_REG
		(rdma_ctrl,
		 (ctrl_ahb_wr_burst_size << 4) |
		 (ctrl_ahb_rd_burst_size << 2) |
		 (0x0 << 1));
	}
	reset_count++;
}

unsigned int rdma_hw_done_bit(void)
{
	u32 rdma_status, done_bit;

	rdma_status = READ_VCBUS_REG(irq_status.reg);
	done_bit = rdma_status >> irq_status.start;

	return done_bit;
}

static int rdma_isr_count;
irqreturn_t rdma_mgr_isr(int irq, void *dev_id)
{
	struct rdma_device_info *info = &rdma_info;
	int retry_count = 0;
	u32 rdma_status, mask;
	int i;
	u32 read_val;

	if (debug_flag & 0x10)
		return IRQ_HANDLED;
	rdma_isr_count++;
QUERY:
	retry_count++;
	rdma_status = READ_VCBUS_REG(irq_status.reg);
	if ((debug_flag & 4) && ((rdma_isr_count % 30) == 0))
		pr_info("%s: %x\r\n", __func__, rdma_status);
	for (i = 0; i < rdma_meson_dev.channel_num; i++) {
		struct rdma_instance_s *ins = &info->rdma_ins[i];

		if (ins->not_process)
			continue;
		if (!(rdma_status & (1 << (i + irq_status.start))))
			continue;
		/*bypass osd rdma done case */
#ifdef SKIP_OSD_CHANNEL
		if (i == 3)
			continue;
#endif
		if (rdma_status & (1 << ins->rdma_regadr->irq_status_bitpos)) {
			if (debug_flag & 2)
				pr_info("%s: process %d\r\n", __func__, i);

			if (ins->op && ins->op->irq_cb) {
				ins->op->irq_cb(ins->op->arg);
				ins->irq_count++;
			}

			WRITE_VCBUS_REG
				(rdma_ctrl,
				(1 << ins->rdma_regadr->clear_irq_bitpos));
		}
	}
	rdma_status = READ_VCBUS_REG(irq_status.reg);
#ifdef SKIP_OSD_CHANNEL
	if ((rdma_status & 0xf7000000) && retry_count < 100)
		goto QUERY;
#else
	mask = ((1L << irq_status.len) - 1) << irq_status.start;
	if ((rdma_status & mask) && retry_count < 100) {
		if (retry_count >= 3)
			pr_info("rdma_status=0x%x, retry_count=%d\n",
				rdma_status, retry_count);
		goto QUERY;
	}
#endif
	for (i = 0; i < MAX_CONFLICT; i++) {
		if (info->rdma_reg.adr[i]) {
			read_val = READ_VCBUS_REG(info->rdma_reg.adr[i]);
			if (read_val == info->rdma_reg.val[i])
				info->rdma_reg.adr[i] = 0;
		}
	}
	return IRQ_HANDLED;
}

void rdma_stop(int handle)
{
	struct rdma_device_info *info = &rdma_info;
	struct rdma_instance_s *ins = &info->rdma_ins[handle];

	WRITE_VCBUS_REG_BITS(ins->rdma_regadr->trigger_mask_reg,
			     0, ins->rdma_regadr->trigger_mask_reg_bitpos,
			     rdma_meson_dev.trigger_mask_len);
}

/*
 *	trigger_type:
 *		0, stop,
 *		0x1~0xff, interrupt input trigger mode
 *		0x100, RDMA_TRIGGER_MANUAL
 *		> 0x100, debug mode
 *
 *	return:
 *		-1, fail
 *		0, rdma table is empty, will not have rdma irq
 *		1, success
 */

int rdma_config(int handle, u32 trigger_type)
{
	int ret = 0;
	unsigned long flags;
	struct rdma_device_info *info = &rdma_info;
	struct rdma_instance_s *ins = &info->rdma_ins[handle];
	bool auto_start = false;
	bool rdma_read = false;
	int buffer_lock = 0;
	int trigger_type_backup = trigger_type, rdma_type;
	bool configured = 0;

	if (handle <= 0 || handle >= rdma_meson_dev.channel_num) {
		pr_info
		("%s error, (handle == %d) not allowed\n",
		 __func__, handle);
		return -1;
	}

	if (!enable[handle])
		return 0;

	spin_lock_irqsave(&rdma_lock, flags);
	if (!ins->op) {
		spin_unlock_irqrestore(&rdma_lock, flags);

		pr_info("%s: handle (%d) not register\n",
			__func__, handle);
		return -1;
	}

	if (!(trigger_type & RDMA_TRIGGER_OMIT_LOCK))
		buffer_lock = ins->lock_flag;

	if (trigger_type & RDMA_READ_MASK)
		rdma_read = true;

	trigger_type &= ~RDMA_READ_MASK;

	if (trigger_type & RDMA_AUTO_START_MASK)
		auto_start = true;

	trigger_type &= ~RDMA_AUTO_START_MASK;
	trigger_type &= ~RDMA_TRIGGER_OMIT_LOCK;
	if (auto_start) {
		WRITE_VCBUS_REG_BITS
			(ins->rdma_regadr->trigger_mask_reg,
			 0,
			 ins->rdma_regadr->trigger_mask_reg_bitpos,
			 rdma_meson_dev.trigger_mask_len);

		WRITE_VCBUS_REG_BITS
			(ins->rdma_regadr->addr_inc_reg,
			 0,
			 ins->rdma_regadr->addr_inc_reg_bitpos,
			 1);
		WRITE_VCBUS_REG_BITS
			(ins->rdma_regadr->rw_flag_reg,
			 rdma_read ? 0 : 1,
			 ins->rdma_regadr->rw_flag_reg_bitpos,
			 1);
		WRITE_VCBUS_REG_BITS
			(ins->rdma_regadr->trigger_mask_reg,
			 trigger_type,
			 ins->rdma_regadr->trigger_mask_reg_bitpos,
			 rdma_meson_dev.trigger_mask_len);
		ret = 1;
		ins->rdma_write_count = 0;
	} else if (ins->rdma_item_count <= 0 ||
		trigger_type == 0 ||
		buffer_lock == 1) {
		if (trigger_type == RDMA_TRIGGER_MANUAL)
			WRITE_VCBUS_REG
			(rdma_access_man,
			 READ_VCBUS_REG(rdma_access_man) & (~1));
		if (debug_flag & 2) {
			pr_info("%s: handle=%d trigger_type %d : %d buffer_lock:%d\r\n",
				__func__,
				handle,
				trigger_type_backup,
				ins->rdma_item_count,
				buffer_lock);
		}
		WRITE_VCBUS_REG_BITS
		(ins->rdma_regadr->trigger_mask_reg,
		 0, ins->rdma_regadr->trigger_mask_reg_bitpos,
		 rdma_meson_dev.trigger_mask_len);
		ins->rdma_write_count = 0;
		ins->prev_read_count = 0;
		ins->rdma_empty_config_count++;
		ret = 0;
	} else {
		if (use_rdma_done_detect &&
		    handle == get_rdma_handle(PRE_VSYNC_RDMA)) {
			rdma_done_detect_cnt++;
			rdma_write_reg(handle, rdma_done_detect_reg,
				       rdma_done_detect_cnt);
		}

		memcpy(ins->rdma_table_addr, ins->reg_buf,
		       ins->rdma_item_count *
		       (rdma_read ? 1 : 2) * sizeof(u32));
		/* cp to mirror buf */
		memcpy(ins->rdma_table_mirror, ins->reg_buf,
		       ins->rdma_item_count *
		       (rdma_read ? 1 : 2) * sizeof(u32));
		ins->prev_read_count = ins->rdma_item_count;

		if (trigger_type > 0 &&
		    trigger_type <= BIT(rdma_meson_dev.trigger_mask_len)) {
			ins->rdma_write_count = ins->rdma_item_count;
			ins->prev_trigger_type = trigger_type;
			if (trigger_type == RDMA_TRIGGER_MANUAL) {
				/*manual RDMA */
				struct rdma_instance_s *man_ins =
					&info->rdma_ins[0];

				WRITE_VCBUS_REG
				(rdma_access_man,
				 READ_VCBUS_REG(rdma_access_man) & (~1));
				if (support_64bit_addr) {
					#ifdef CONFIG_ARM64
					WRITE_VCBUS_REG
					(man_ins->rdma_regadr->rdma_ahb_start_addr,
					 ins->rdma_table_phy_addr & 0xffffffff);
					WRITE_VCBUS_REG
					(man_ins->rdma_regadr->rdma_ahb_start_addr_msb,
					(ins->rdma_table_phy_addr >> 32) & 0xffffffff);
					WRITE_VCBUS_REG
					(man_ins->rdma_regadr->rdma_ahb_end_addr,
					(ins->rdma_table_phy_addr + ins->rdma_item_count * 8 - 1)
					& 0xffffffff);
					WRITE_VCBUS_REG
					(man_ins->rdma_regadr->rdma_ahb_end_addr_msb,
					((ins->rdma_table_phy_addr
					+ ins->rdma_item_count * 8 - 1) >> 32) & 0xffffffff);
					#else
					WRITE_VCBUS_REG
					(man_ins->rdma_regadr->rdma_ahb_start_addr,
					 ins->rdma_table_phy_addr & 0xffffffff);
					WRITE_VCBUS_REG
					(man_ins->rdma_regadr->rdma_ahb_start_addr_msb,
					0);
					WRITE_VCBUS_REG
					(man_ins->rdma_regadr->rdma_ahb_end_addr,
					(ins->rdma_table_phy_addr +
					ins->rdma_item_count * 8 - 1)
					& 0xffffffff);
					WRITE_VCBUS_REG
					(man_ins->rdma_regadr->rdma_ahb_end_addr_msb,
					0);
					#endif
				} else {
					WRITE_VCBUS_REG
					(man_ins->rdma_regadr->rdma_ahb_start_addr,
					 ins->rdma_table_phy_addr & 0xffffffff);
					WRITE_VCBUS_REG
					(man_ins->rdma_regadr->rdma_ahb_end_addr,
					(ins->rdma_table_phy_addr & 0xffffffff)
					+ ins->rdma_item_count * 8 - 1);
				}

				WRITE_VCBUS_REG_BITS
				(man_ins->rdma_regadr->addr_inc_reg,
				 0,
				 man_ins->rdma_regadr->addr_inc_reg_bitpos,
				 1);
				WRITE_VCBUS_REG_BITS
				(man_ins->rdma_regadr->rw_flag_reg,
				 rdma_read ? 0 : 1,
				 man_ins->rdma_regadr->rw_flag_reg_bitpos,
				 1);
				/* Manual-start RDMA*/
				WRITE_VCBUS_REG
				(rdma_access_man,
				 READ_VCBUS_REG(rdma_access_man) | 1);

				if (debug_flag & 2)
					pr_info("%s: manual config %d:\r\n",
						__func__, ins->rdma_item_count);
			} else {
				/* interrupt input trigger RDMA */
				if (debug_flag & 2 ||
					(rdma_trace_enable &&
					rdma_trace_channel == handle))
					pr_info("%s: handle=%d case 3 : %d:\r\n",
					__func__, handle, ins->rdma_item_count);
				WRITE_VCBUS_REG_BITS
				(ins->rdma_regadr->trigger_mask_reg,
				 0,
				 ins->rdma_regadr->trigger_mask_reg_bitpos,
				 rdma_meson_dev.trigger_mask_len);
				if (support_64bit_addr) {
					#ifdef CONFIG_ARM64
					WRITE_VCBUS_REG
					(ins->rdma_regadr->rdma_ahb_start_addr,
					ins->rdma_table_phy_addr & 0xffffffff);
					WRITE_VCBUS_REG
					(ins->rdma_regadr->rdma_ahb_start_addr_msb,
					(ins->rdma_table_phy_addr >> 32) & 0xffffffff);
					WRITE_VCBUS_REG
					(ins->rdma_regadr->rdma_ahb_end_addr,
					 (ins->rdma_table_phy_addr +
					 ins->rdma_item_count * 8 - 1) & 0xffffffff);
					WRITE_VCBUS_REG
					(ins->rdma_regadr->rdma_ahb_end_addr_msb,
					 ((ins->rdma_table_phy_addr +
					 ins->rdma_item_count * 8 - 1) >> 32) & 0xffffffff);
					#else
					WRITE_VCBUS_REG
					(ins->rdma_regadr->rdma_ahb_start_addr,
					ins->rdma_table_phy_addr & 0xffffffff);
					WRITE_VCBUS_REG
					(ins->rdma_regadr->rdma_ahb_start_addr_msb,
					0);
					WRITE_VCBUS_REG
					(ins->rdma_regadr->rdma_ahb_end_addr,
					(ins->rdma_table_phy_addr +
					ins->rdma_item_count * 8 - 1) &
					0xffffffff);
					WRITE_VCBUS_REG
					(ins->rdma_regadr->rdma_ahb_end_addr_msb,
					0);
					#endif
				} else {
					WRITE_VCBUS_REG
					(ins->rdma_regadr->rdma_ahb_start_addr,
					ins->rdma_table_phy_addr & 0xffffffff);
					WRITE_VCBUS_REG
					(ins->rdma_regadr->rdma_ahb_end_addr,
					 (ins->rdma_table_phy_addr & 0xffffffff) +
					 ins->rdma_item_count * 8 - 1);
				}
				WRITE_VCBUS_REG_BITS
				(ins->rdma_regadr->addr_inc_reg,
				 0,
				 ins->rdma_regadr->addr_inc_reg_bitpos,
				 1);
				WRITE_VCBUS_REG_BITS
				(ins->rdma_regadr->rw_flag_reg,
				 rdma_read ? 0 : 1,
				 ins->rdma_regadr->rw_flag_reg_bitpos,
				 1);
				WRITE_VCBUS_REG_BITS
				(ins->rdma_regadr->trigger_mask_reg,
				 trigger_type,
				 ins->rdma_regadr->trigger_mask_reg_bitpos,
				 rdma_meson_dev.trigger_mask_len);
			}
		} else if (trigger_type == 0x101) {	/* debug mode */
			int i;

			for (i = 0; i < ins->rdma_item_count; i++) {
				WRITE_VCBUS_REG
					(ins->rdma_table_addr[i << 1],
					 ins->rdma_table_addr[(i << 1) + 1]);
				if (debug_flag & 1)
					pr_info("WR(%x)<=%x\n",
						ins->rdma_table_addr[i << 1],
						ins->rdma_table_addr
						[(i << 1) + 1]);
			}
			ins->rdma_write_count = 0;
		} else if (trigger_type == 0x102) { /* debug mode */
			int i;

			for (i = 0; i < ins->rdma_item_count; i++) {
				WRITE_VCBUS_REG(ins->reg_buf[i << 1],
						ins->reg_buf[(i << 1) + 1]);
				if (debug_flag & 1)
					pr_info("WR(%x)<=%x\n",
						ins->reg_buf[i << 1],
						ins->reg_buf[(i << 1) + 1]);
			}
			ins->rdma_write_count = 0;
		}
		ret = 1;
	}

	rdma_type = get_rdma_type(handle);
	if (rdma_type >= 0) {
		configured = ret ? 1 : 0;
		if (configured) {
			struct timeval t;

			do_gettimeofday(&t);
			rdma_config_us[rdma_type] =
				t.tv_sec * 1000000 + t.tv_usec;
			enc_num_configed[rdma_type] = get_cur_enc_num();
		} else {
			enc_num_configed[rdma_type] = 0xff;
		}
		rdma_configured[rdma_type] = configured;
	}
	/* don't reset rdma_item_count for read function */
	if (handle != get_rdma_handle(VSYNC_RDMA_READ) &&
		!buffer_lock) {
		if (handle == get_rdma_handle(VSYNC_RDMA)) {
			g_vsync_rdma_item_count = ins->rdma_item_count;
			if (g_vsync_rdma_item_count >= g_vsync_rdma_item_count_max)
				g_vsync_rdma_item_count_max = g_vsync_rdma_item_count;
		}
		ins->rdma_item_count = 0;
	}
	spin_unlock_irqrestore(&rdma_lock, flags);

	if (debug_flag & 2 ||
		(rdma_trace_enable &&
		rdma_trace_channel == handle))
		pr_info("%s: (%d 0x%x) rdma_item_count=%d, ret %d\r\n",
			__func__, handle, trigger_type_backup, ins->rdma_item_count, ret);
	ins->rdma_config_count++;
	return ret;
}
EXPORT_SYMBOL(rdma_config);

int rdma_clear(int handle)
{
	int ret = 0;
	unsigned long flags;
	struct rdma_device_info *info = &rdma_info;
	struct rdma_instance_s *ins = &info->rdma_ins[handle];

	spin_lock_irqsave(&rdma_lock, flags);
	if (handle <= 0 ||
	    handle >= rdma_meson_dev.channel_num ||
	    !ins->op) {
		spin_unlock_irqrestore(&rdma_lock, flags);
		pr_info("%s error, handle (%d) not register\n",
			__func__, handle);
		return -1;
	}
	WRITE_VCBUS_REG_BITS(ins->rdma_regadr->trigger_mask_reg,
			     0, ins->rdma_regadr->trigger_mask_reg_bitpos,
			     rdma_meson_dev.trigger_mask_len);
	if (debug_flag & 2 ||
		(rdma_trace_enable &&
		rdma_trace_channel == handle))
		pr_info("%s: handle=%d : write count %d item count %d\n",
			__func__, handle,
			ins->rdma_write_count, ins->rdma_item_count);
	ins->rdma_write_count = 0;
	ins->rdma_item_count = 0;
	spin_unlock_irqrestore(&rdma_lock, flags);
	return ret;
}
EXPORT_SYMBOL(rdma_clear);

u32 rdma_read_reg(int handle, u32 adr)
{
	int i, j = 0;
	u32 *write_table;
	int match = 0;
	int match_oth = 0;
	int read_from = 0;
	struct rdma_device_info *info = &rdma_info;
	struct rdma_instance_s *ins = &info->rdma_ins[handle];
	u32 read_val = READ_VCBUS_REG(adr);

	for (i = 0; i < MAX_CONFLICT; i++) {
		if (info->rdma_reg.adr[i] == adr) {
			read_val = info->rdma_reg.val[i];
			match_oth = 1;
			read_from = 3;
			break;
		}
	}
	if (!match_oth) {
		for (i = (ins->rdma_item_count - 1); i >= 0; i--) {
			if (ins->reg_buf[i << 1] == adr) {
				read_val = ins->reg_buf[(i << 1) + 1];
				match = 1;
				read_from = 1;
				break;
			}
		}
	}
	/* changed to read from rdma_table_adr to mirror for optimize */
	if (!match) {
		write_table = ins->rdma_table_mirror;
		for (i = (ins->rdma_write_count - 1);
			i >= 0; i--) {
			if (write_table[i << 1] == adr) {
				read_val =
					write_table[(i << 1) + 1];
				read_from = 2;
				break;
			}
		}
	}
	if (rdma_trace_enable) {
		for (j = 0; j < rdma_trace_num; j++) {
			if (adr == rdma_trace_reg[j]) {
				if (read_from == 3)
					pr_info("(%s) handle %d, %04x=0x%08x from conflict table(%d), cur_val:0x%x\n",
						__func__,
						handle, adr,
						read_val,
						ins->rdma_write_count,
						READ_VCBUS_REG(adr));
				else if (read_from == 2)
					pr_info("(%s) handle %d, %04x=0x%08x from write table(%d), cur_val:0x%x\n",
						__func__,
						handle, adr,
						read_val,
						ins->rdma_write_count,
						READ_VCBUS_REG(adr));
				else if (read_from == 1)
					pr_info("(%s) handle %d, %04x=0x%08x from item table(%d), cur_val:0x%x\n",
						__func__,
						handle, adr,
						read_val,
						ins->rdma_item_count,
						READ_VCBUS_REG(adr));
				else
					pr_info("(%s) handle %d, %04x=0x%08x from real reg, cur_val:0x%x\n",
						__func__,
						handle, adr,
						read_val,
						READ_VCBUS_REG(adr));
			}
		}
	}
	return read_val;
}
EXPORT_SYMBOL(rdma_read_reg);

int rdma_buffer_lock(int handle)
{
	int ret = 0;
	unsigned long flags;
	struct rdma_device_info *info = &rdma_info;
	struct rdma_instance_s *ins = &info->rdma_ins[handle];

	spin_lock_irqsave(&rdma_lock, flags);
	if (handle <= 0 ||
		handle >= rdma_meson_dev.channel_num ||
		!ins->op) {
		spin_unlock_irqrestore(&rdma_lock, flags);
		pr_info("%s error, handle (%d) not register\n",
			__func__, handle);
		return -1;
	}
	ins->lock_flag = 1;
	spin_unlock_irqrestore(&rdma_lock, flags);
	return ret;
}
EXPORT_SYMBOL(rdma_buffer_lock);

int rdma_buffer_unlock(int handle)
{
	int ret = 0;
	unsigned long flags;
	struct rdma_device_info *info = &rdma_info;
	struct rdma_instance_s *ins = &info->rdma_ins[handle];

	spin_lock_irqsave(&rdma_lock, flags);
	if (handle <= 0 ||
		handle >= rdma_meson_dev.channel_num ||
		!ins->op) {
		spin_unlock_irqrestore(&rdma_lock, flags);
		pr_info("%s error, handle (%d) not register\n",
			__func__, handle);
		return -1;
	}
	ins->lock_flag = 0;
	spin_unlock_irqrestore(&rdma_lock, flags);
	return ret;
}
EXPORT_SYMBOL(rdma_buffer_unlock);

int rdma_watchdog_setting(int flag, int handle)
{
	int ret = 0;

	if (handle < 0 || handle >= RDMA_NUM) {
		pr_info("%s, flag:%d handle:%d out of range[0, %d)\n",
			__func__, flag, handle, RDMA_NUM);
		return 0;
	}

	if (flag == 0)
		rdma_watchdog_count[handle] = 0;
	else
		rdma_watchdog_count[handle]++;

	if (debug_flag & 8) {
		rdma_force_reset = 1;
		debug_flag = 0;
	}
	if (debug_flag & 0x40)
		pr_info("%s, flag:%d handle:%d rdma_watchdog_count:%d\n",
			__func__, flag, handle, rdma_watchdog_count[handle]);

	if ((rdma_watchdog > 0 &&
	     rdma_watchdog_count[handle] > rdma_watchdog) ||
	     rdma_force_reset > 0) {
		pr_info("%s rdma reset: %d, force flag:%d handle:%d\n",
			__func__,
			rdma_watchdog_count[handle],
			rdma_force_reset,
			handle);
		rdma_watchdog_count[handle] = 0;
		rdma_force_reset = 0;
		rdma_reset(1);
		rdma_reset_trigger_flag[handle] = 1;
		ret = 1;
	}
	return ret;
}
EXPORT_SYMBOL(rdma_watchdog_setting);

static bool rdma_check_conflict(int handle, u32 adr, u32 *read_val)
{
	struct rdma_instance_s *oth_ins;
	int i, j, k, n;

	for (i = 0; i < MAX_CONFLICT; i++) {
		if (rdma_info.rdma_reg.adr[i] == adr) {
			if (read_val)
				*read_val = rdma_info.rdma_reg.val[i];
			return true;
		}
	}

	for (i = 0; i < rdma_meson_dev.channel_num; i++) {
		oth_ins = &rdma_info.rdma_ins[i];
		if (!oth_ins->rdma_table_size ||
		    !oth_ins->rdma_item_count ||
		    i == handle)
			continue;
		for (j = 0; j < oth_ins->rdma_item_count; j++) {
			if (oth_ins->reg_buf[j << 1] != adr)
				continue;
			for (n = 0; n < rdma_trace_num; n++) {
				if (adr == rdma_trace_reg[n] ||
				    (debug_flag & 0x20))
					pr_info("(%s) handle %d, conflict write %04x=0x%08x (oth handle %d), cur_val:0x%x\n",
						__func__,
						handle, adr,
						oth_ins->reg_buf[(j << 1) + 1],
						i,
						READ_VCBUS_REG(adr));
			}
			for (k = 0; k < MAX_CONFLICT; k++) {
				if (!rdma_info.rdma_reg.adr[i]) {
					rdma_info.rdma_reg.adr[k] = adr;
					rdma_info.rdma_reg.val[k] =
						oth_ins->reg_buf[(j << 1) + 1];
					if (read_val)
						*read_val = oth_ins->reg_buf
							[(j << 1) + 1];
					return true;
				}
			}
		}
	}
	return false;
}

static void rdma_update_conflict(u32 adr, u32 val)
{
	int i;

	for (i = 0; i < MAX_CONFLICT; i++) {
		if (rdma_info.rdma_reg.adr[i] == adr) {
			rdma_info.rdma_reg.val[i] = val;
			if (debug_flag & 0x20)
				pr_info("(%s) %04x=0x%08x\n",
					__func__,
					adr, val);
			break;
		}
	}
}

int rdma_write_reg(int handle, u32 adr, u32 val)
{
	struct rdma_device_info *info = &rdma_info;
	struct rdma_instance_s *ins = &info->rdma_ins[handle];
	int j = 0;

	if (ins->rdma_table_size == 0)
		return -1;
#ifdef CONFIG_AMLOGIC_BL_LDIM
	if (!is_video_process_in_thread() &&
		(((smp_processor_id() == 0) &&
		!is_in_vsync_isr() &&
		!is_in_pre_vsync_isr() &&
		!is_in_vsync_isr_viu2() &&
		!is_in_vsync_isr_viu3() &&
		!is_in_ldim_vsync_isr()) || (smp_processor_id() != 0)) &&
		get_rdma_handle(VSYNC_RDMA) == handle) {
#else
	if (!is_video_process_in_thread() &&
		(((smp_processor_id() == 0) &&
		!is_in_vsync_isr() &&
		!is_in_pre_vsync_isr() &&
		!is_in_vsync_isr_viu2() &&
		!is_in_vsync_isr_viu3()) || (smp_processor_id() != 0)) &&
		get_rdma_handle(VSYNC_RDMA) == handle) {
#endif
		dump_stack();
		pr_info("rdma_write(%d) %d(%x)<=%x\n",
			handle, ins->rdma_item_count, adr, val);
	}

	if (adr == 0) {
		pr_info("rdma_write(%d) write zero addr = %x, count:%d\n",
			handle, val, ins->rdma_item_count);
		dump_stack();
	}
	if (debug_flag & 1 ||
		(rdma_trace_enable &&
		rdma_trace_channel == handle))
		pr_info("rdma_write(%d) %d(%x)<=%x\n",
			handle, ins->rdma_item_count, adr, val);
	if (rdma_check_conflict(handle, adr, NULL))
		rdma_update_conflict(adr, val);

	if (((ins->rdma_item_count << 1) + 1) <
		(ins->rdma_table_size / sizeof(u32))) {
		ins->reg_buf[ins->rdma_item_count << 1] = adr;
		ins->reg_buf[(ins->rdma_item_count << 1) + 1] = val;
		ins->rdma_item_count++;
	} else {
		int i;

			pr_info("%s(%d, %x, %x ,%d) buf overflow, ins->rdma_item_count=%d\n",
				__func__, rdma_watchdog_count[handle],
				handle, adr, val,
				ins->rdma_item_count);
		for (i = 0; i < ins->rdma_item_count; i++)
			WRITE_VCBUS_REG(ins->reg_buf[i << 1],
					ins->reg_buf[(i << 1) + 1]);
		ins->rdma_item_count = 0;
		ins->rdma_write_count = 0;
		ins->reg_buf[ins->rdma_item_count << 1] = adr;
		ins->reg_buf[(ins->rdma_item_count << 1) + 1] = val;
		ins->rdma_item_count++;
	}
	if (rdma_trace_enable) {
		for (j = 0; j < rdma_trace_num; j++) {
			if (adr == rdma_trace_reg[j]) {
				pr_info("(%s) handle %d, %04x=0x%08x (%d), cur_val:0x%x\n",
					__func__,
					handle, adr,
					val,
					ins->rdma_item_count,
					READ_VCBUS_REG(adr));
			}
		}
	}
	return 0;
}
EXPORT_SYMBOL(rdma_write_reg);

int rdma_write_reg_bits(int handle, u32 adr, u32 val, u32 start, u32 len)
{
	int i, j = 0;
	u32 *write_table;
	int match = 0;
	int match_oth = 0;
	int read_from = 0;
	struct rdma_device_info *info = &rdma_info;
	struct rdma_instance_s *ins = &info->rdma_ins[handle];
	u32 read_val = READ_VCBUS_REG(adr);
	u32 oth_val = 0;
	u32 write_val;

	if (ins->rdma_table_size == 0)
		return -1;

	if (rdma_check_conflict(handle, adr, &oth_val)) {
		match_oth = 1;
		read_val = oth_val;
		read_from = 3;
	}

	for (i = (ins->rdma_item_count - 1); i >= 0; i--) {
		if (ins->reg_buf[i << 1] == adr) {
			match = 1;
			if (!match_oth) {
				read_val = ins->reg_buf[(i << 1) + 1];
				read_from = 1;
			}
			break;
		}
	}
	/* changed to read from rdma_table_adr to mirror for optimize */
	if (!match) {
		write_table = ins->rdma_table_mirror;
		for (i = (ins->rdma_write_count - 1);
			i >= 0; i--) {
			if (write_table[i << 1] == adr) {
				if (!match_oth) {
					read_val =
						write_table[(i << 1) + 1];
					read_from = 2;
				}
				break;
			}
		}
	}
	write_val = (read_val & ~(((1L << (len)) - 1) << (start))) |
			((unsigned int)(val) << (start));
	if (match_oth)
		rdma_update_conflict(adr, write_val);
	for (j = 0; j < rdma_trace_num; j++) {
		if (adr == rdma_trace_reg[j]) {
			if (read_from == 3)
				pr_info("(%s) handle %d, %04x=0x%08x->0x%08x from conflict table(%d %d %d), cur_val:0x%x\n",
					__func__,
					handle, adr,
					read_val,
					write_val,
					ins->rdma_write_count,
					match,
					match ? i : ins->rdma_write_count,
					READ_VCBUS_REG(adr));
			else if (read_from == 2)
				pr_info("(%s) handle %d, %04x=0x%08x->0x%08x from write table(%d %d %d), cur_val:0x%x\n",
					__func__,
					handle, adr,
					read_val,
					write_val,
					ins->rdma_write_count,
					match,
					match ? i : ins->rdma_write_count,
					READ_VCBUS_REG(adr));
			else if (read_from == 1)
				pr_info("(%s) handle %d, %04x=0x%08x->0x%08x from item table(%d %d %d), cur_val:0x%x\n",
					__func__,
					handle, adr,
					read_val,
					write_val,
					ins->rdma_item_count,
					match,
					match ? i : ins->rdma_item_count,
					READ_VCBUS_REG(adr));
			else
				pr_info("(%s) handle %d, %04x=0x%08x->0x%08x from real reg, cur_val:0x%x\n",
					__func__,
					handle, adr,
					read_val,
					write_val,
					READ_VCBUS_REG(adr));
		}
	}
	if (match) {
		ins->reg_buf[(i << 1) + 1] = write_val;
		return 0;
	}
	if (debug_flag & 1 ||
		(rdma_trace_enable &&
		rdma_trace_channel == handle))
		pr_info("rdma_write(%d) %d(%x)<=%x\n",
			handle, ins->rdma_item_count, adr, val);

	rdma_write_reg(handle, adr, write_val);
	return 0;
}
EXPORT_SYMBOL(rdma_write_reg_bits);

s32 rdma_add_read_reg(int handle, u32 adr)
{
	struct rdma_device_info *info = &rdma_info;
	struct rdma_instance_s *ins = NULL;

	if (handle > 0 && handle < rdma_meson_dev.channel_num) {
		ins = &info->rdma_ins[handle];
		if (((ins->rdma_item_count + 1) << 1) <
			(ins->rdma_table_size / sizeof(u32))) {
			ins->reg_buf[ins->rdma_item_count] = adr;
			ins->rdma_item_count++;
			return (ins->rdma_item_count - 1);
		}
		pr_info("%s: out of bound\n", __func__);
		return -1;
	}
	pr_info("%s: handle is error\n", __func__);
	return -1;
}
EXPORT_SYMBOL(rdma_add_read_reg);

u32 *rdma_get_read_back_addr(int handle)
{
	struct rdma_device_info *info = &rdma_info;
	struct rdma_instance_s *ins = NULL;
	u32 *table;

	if (handle > 0 && handle < rdma_meson_dev.channel_num) {
		ins = &info->rdma_ins[handle];
		table = ins->rdma_table_addr;
		if (debug_flag & 2)
			pr_info("%s, handle: %d, pre_count: %d\n",
				__func__, handle, ins->prev_read_count);

		return (table + ins->prev_read_count);
	}

	return NULL;
}
EXPORT_SYMBOL(rdma_get_read_back_addr);

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
static struct rdma_device_data_s rdma_meson = {
	.cpu_type = CPU_NORMAL,
	.rdma_ver = RDMA_VER_1,
	.trigger_mask_len = 8,
	.channel_num = 8
};

static struct rdma_device_data_s rdma_g12b_reva = {
	.cpu_type = CPU_G12B_REVA,
	.rdma_ver = RDMA_VER_1,
	.trigger_mask_len = 8,
	.channel_num = 8
};

static struct rdma_device_data_s rdma_g12b_revb = {
	.cpu_type = CPU_G12B_REVB,
	.rdma_ver = RDMA_VER_1,
	.trigger_mask_len = 8,
	.channel_num = 8
};

static struct rdma_device_data_s rdma_tl1 = {
	.cpu_type = CPU_TL1,
	.rdma_ver = RDMA_VER_2,
	.trigger_mask_len = 16,
	.channel_num = 8
};
#endif

static struct rdma_device_data_s rdma_sc2 = {
	.cpu_type = CPU_SC2,
	.rdma_ver = RDMA_VER_2,
	.trigger_mask_len = 16,
	.channel_num = 8
};

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
static struct rdma_device_data_s rdma_t7 = {
	.cpu_type = CPU_T7,
	.rdma_ver = RDMA_VER_3,
	.trigger_mask_len = 24,
	.channel_num = 8
};

static struct rdma_device_data_s rdma_t3 = {
	.cpu_type = CPU_T7,
	.rdma_ver = RDMA_VER_4,
	.trigger_mask_len = 25,
	.channel_num = 8
};

static struct rdma_device_data_s rdma_t3x = {
	.cpu_type = CPU_T7,
	.rdma_ver = RDMA_VER_5,
	.trigger_mask_len = 25,
	.channel_num = 16
};
#endif

static const struct of_device_id rdma_dt_match[] = {
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	{
		.compatible = "amlogic, meson, rdma",
		.data = &rdma_meson,
	},
	{
		.compatible = "amlogic, meson-g12b-reva, rdma",
		.data = &rdma_g12b_reva,
	},
	{
		.compatible = "amlogic, meson-g12b-revb, rdma",
		.data = &rdma_g12b_revb,
	},
	{
		.compatible = "amlogic, meson-tl1, rdma",
		.data = &rdma_tl1,
	},
#endif
	{
		.compatible = "amlogic, meson-sc2, rdma",
		.data = &rdma_sc2,
	},
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	{
		.compatible = "amlogic, meson-t7, rdma",
		.data = &rdma_t7,
	},
	{
		.compatible = "amlogic, meson-t3, rdma",
		.data = &rdma_t3,
	},
	{
		.compatible = "amlogic, meson-t3x, rdma",
		.data = &rdma_t3x,
	},
#endif
	{},
};

u32 is_meson_g12b_revb(void)
{
	if (rdma_meson_dev.cpu_type == CPU_G12B_REVB)
		return 1;
	else
		return 0;
}

void vpp1_vsync_rdma_register(void)
{
	int handle;

	handle = rdma_register(get_rdma_ops(VSYNC_RDMA_VPP1),
			       NULL, rdma_table_size);
	set_rdma_handle(VSYNC_RDMA_VPP1, handle);
}
EXPORT_SYMBOL(vpp1_vsync_rdma_register);

void vpp2_vsync_rdma_register(void)
{
	int handle;

	handle = rdma_register(get_rdma_ops(VSYNC_RDMA_VPP2),
			       NULL, rdma_table_size);
	set_rdma_handle(VSYNC_RDMA_VPP2, handle);
}
EXPORT_SYMBOL(vpp2_vsync_rdma_register);

void pre_vsync_rdma_register(void)
{
	int handle;

	handle = rdma_register(get_rdma_ops(PRE_VSYNC_RDMA),
			       NULL, rdma_table_size);
	set_rdma_handle(PRE_VSYNC_RDMA, handle);
}
EXPORT_SYMBOL(pre_vsync_rdma_register);

void ex_vsync_rdma_register(void)
{
	int handle;

	handle = rdma_register(get_rdma_ops(EX_VSYNC_RDMA),
			       NULL, rdma_table_size);
	set_rdma_handle(EX_VSYNC_RDMA, handle);
}
EXPORT_SYMBOL(ex_vsync_rdma_register);

int rdma_start_addr(int handle)
{
	struct rdma_regadr_s *rdma_regadr;

	if (handle >= rdma_meson_dev.channel_num) {
		handle = 0;
		pr_err("%s, wrong handle:%d\n", __func__, handle);
	}
	rdma_regadr = rdma_info.rdma_ins[handle].rdma_regadr;

	return rdma_regadr->rdma_ahb_start_addr;
}
EXPORT_SYMBOL(rdma_start_addr);

int rdma_start_addr_msb(int handle)
{
	struct rdma_regadr_s *rdma_regadr;

	if (handle >= rdma_meson_dev.channel_num) {
		handle = 0;
		pr_err("%s, wrong handle:%d\n", __func__, handle);
	}
	rdma_regadr = rdma_info.rdma_ins[handle].rdma_regadr;

	return rdma_regadr->rdma_ahb_start_addr_msb;
}
EXPORT_SYMBOL(rdma_start_addr_msb);

int rdma_end_addr(int handle)
{
	struct rdma_regadr_s *rdma_regadr;

	if (handle >= rdma_meson_dev.channel_num) {
		handle = 0;
		pr_err("%s, wrong handle:%d\n", __func__, handle);
	}
	rdma_regadr = rdma_info.rdma_ins[handle].rdma_regadr;

	return rdma_regadr->rdma_ahb_end_addr;
}
EXPORT_SYMBOL(rdma_end_addr);

int rdma_end_addr_msb(int handle)
{
	struct rdma_regadr_s *rdma_regadr;

	if (handle >= rdma_meson_dev.channel_num) {
		handle = 0;
		pr_err("%s, wrong handle:%d\n", __func__, handle);
	}
	rdma_regadr = rdma_info.rdma_ins[handle].rdma_regadr;

	return rdma_regadr->rdma_ahb_end_addr_msb;
}
EXPORT_SYMBOL(rdma_end_addr_msb);

static ssize_t show_debug_flag(struct class *class,
			       struct class_attribute *attr,
			       char *buf)
{
	return snprintf(buf, 40, "%d\n", debug_flag);
}

static ssize_t store_debug_flag(struct class *class,
				struct class_attribute *attr,
				const char *buf, size_t count)
{
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	pr_info("debug_flag: %d->%d\n", debug_flag, res);
	debug_flag = res;

	return count;
}

static ssize_t show_rdma_watchdog(struct class *class,
				  struct class_attribute *attr,
				  char *buf)
{
	return snprintf(buf, 40, "%d\n", rdma_watchdog);
}

static ssize_t store_rdma_watchdog(struct class *class,
				   struct class_attribute *attr,
				   const char *buf, size_t count)
{
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	pr_info("rdma_watchdog: %d->%d\n", rdma_watchdog, res);
	rdma_watchdog = res;

	return count;
}

static ssize_t show_reset_count(struct class *class,
				struct class_attribute *attr,
				char *buf)
{
	return snprintf(buf, 40, "%d\n", reset_count);
}

static ssize_t store_reset_count(struct class *class,
				 struct class_attribute *attr,
				 const char *buf, size_t count)
{
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	pr_info("reset_count: %d->%d\n", reset_count, res);
	reset_count = res;

	return count;
}

static ssize_t show_ctrl_ahb_rd_burst_size(struct class *class,
					   struct class_attribute *attr,
					   char *buf)
{
	return snprintf(buf, 40, "%d\n", ctrl_ahb_rd_burst_size);
}

static ssize_t store_ctrl_ahb_rd_burst_size(struct class *class,
					    struct class_attribute *attr,
					    const char *buf, size_t count)
{
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	pr_info("ctrl_ahb_rd_burst_size: %d->%d\n",
		ctrl_ahb_rd_burst_size, res);
	ctrl_ahb_rd_burst_size = res;

	return count;
}

static ssize_t show_ctrl_ahb_wr_burst_size(struct class *class,
					   struct class_attribute *attr,
					   char *buf)
{
	return snprintf(buf, 40, "%d\n", ctrl_ahb_wr_burst_size);
}

static ssize_t store_ctrl_ahb_wr_burst_size(struct class *class,
					    struct class_attribute *attr,
					    const char *buf, size_t count)
{
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	pr_info("ctrl_ahb_wr_burst_size: %d->%d\n",
		ctrl_ahb_wr_burst_size, res);
	ctrl_ahb_wr_burst_size = res;

	return count;
}

static int parse_para(const char *para, int para_num, int *result)
{
	char *token = NULL;
	char *params, *params_base;
	int *out = result;
	int len = 0, count = 0;
	int res = 0;
	int ret = 0;

	if (!para)
		return 0;

	params = kstrdup(para, GFP_KERNEL);
	params_base = params;
	token = params;
	if (!token)
		return 0;
	len = strlen(token);
	do {
		token = strsep(&params, " ");
		while (token && (isspace(*token) ||
				 !isgraph(*token)) && len) {
			token++;
			len--;
		}
		if (len == 0 || !token)
			break;
		ret = kstrtoint(token, 0, &res);
		if (ret < 0)
			break;
		len = strlen(token);
		*out++ = res;
		count++;
	} while ((token) && (count < para_num) && (len > 0));

	kfree(params_base);
	return count;
}

static ssize_t rdma_mgr_trace_enable_show(struct class *cla,
					  struct class_attribute *attr,
					  char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%x\n", rdma_trace_enable);
}

static ssize_t rdma_mgr_trace_enable_stroe(struct class *cla,
					   struct class_attribute *attr,
					   const char *buf, size_t count)
{
	int ret = 0;

	ret = kstrtoint(buf, 0, &rdma_trace_enable);
	if (ret < 0)
		return -EINVAL;
	return count;
}

static ssize_t rdma_mgr_trace_reg_show(struct class *cla,
				       struct class_attribute *attr, char *buf)
{
	int i;
	char reg_info[16];
	char *trace_info = NULL;

	trace_info = kmalloc(rdma_trace_num * 16 + 1, GFP_KERNEL);
	if (!trace_info)
		return 0;
	for (i = 0; i < rdma_trace_num; i++) {
		sprintf(reg_info, "0x%x", rdma_trace_reg[i]);
		strcat(trace_info, reg_info);
		strcat(trace_info, " ");
	}
	i = snprintf(buf, PAGE_SIZE, "%s\n", trace_info);
	kfree(trace_info);
	trace_info = NULL;
	return i;
}

static ssize_t rdma_mgr_trace_reg_stroe(struct class *cla,
					struct class_attribute *attr,
					const char *buf, size_t count)
{
	int parsed[MAX_TRACE_NUM];
	int i = 0, num = 0;

	for (i	= 0; i < MAX_TRACE_NUM; i++)
		rdma_trace_reg[i] = 0;
	num = parse_para(buf, MAX_TRACE_NUM, parsed);
	if (num <= MAX_TRACE_NUM) {
		rdma_trace_num = num;
		for (i  = 0; i < num; i++) {
			rdma_trace_reg[i] = parsed[i];
			pr_info("trace reg:0x%x\n", rdma_trace_reg[i]);
		}
	}
	return count;
}

static ssize_t show_ex_vsync_rdma(struct class *class,
			       struct class_attribute *attr,
			       char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n",
			get_ex_vsync_rdma_enable());
}

static ssize_t store_ex_vsync_rdma(struct class *class,
				struct class_attribute *attr,
				const char *buf, size_t count)
{
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	pr_info("ex_vsync_rdma_enable: %d->%d\n", get_ex_vsync_rdma_enable(), res);
	set_ex_vsync_rdma_enable(res);

	return count;
}

static ssize_t rdma_irq_count_show(struct class *cla,
				       struct class_attribute *attr, char *buf)
{
	int i;
	char str_info[1024];
	char chan_info[128];
	struct rdma_device_info *info = &rdma_info;

	sprintf(str_info, "rdma_isr_count:%d\n", rdma_isr_count);
	for (i = 1; i < rdma_meson_dev.channel_num; i++) {
		if (info->rdma_ins[i].rdma_table_size) {
			sprintf(chan_info,
			"rdma handle=%d, cb irq_count=%d, empty_config=%d, rdma_config_count=%d",
				i, info->rdma_ins[i].irq_count,
				info->rdma_ins[i].rdma_empty_config_count,
				info->rdma_ins[i].rdma_config_count);
			strcat(str_info, chan_info);
			strcat(str_info, "\n");
		}
	}

	i = snprintf(buf, PAGE_SIZE, "%s\n", str_info);
	return i;
}

static ssize_t rdma_mgr_trace_channel_show(struct class *cla,
					  struct class_attribute *attr,
					  char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%x\n", rdma_trace_channel);
}

static ssize_t rdma_mgr_trace_channel_stroe(struct class *cla,
					   struct class_attribute *attr,
					   const char *buf, size_t count)
{
	int ret = 0;

	ret = kstrtoint(buf, 0, &rdma_trace_channel);
	if (ret < 0)
		return -EINVAL;
	return count;
}

static ssize_t store_rdma_reset(struct class *class,
				 struct class_attribute *attr,
				 const char *buf, size_t count)
{
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	rdma_reset(res);

	return count;
}

static ssize_t show_enable(struct class *class,
			   struct class_attribute *attr,
			   char *buf)
{
	int i;
	int enable_flag = 0;

	for (i = 0; i < RDMA_NUM; i++)
		if (enable[i])
			enable_flag |= (1 << i);

	return snprintf(buf, PAGE_SIZE, "enable_flag: 0x%x\n",
			enable_flag);
}

static ssize_t store_enable(struct class *class,
			    struct class_attribute *attr,
			    const char *buf, size_t count)
{
	int i = 0;
	int enable_flag = 0, ret;

	ret = kstrtoint(buf, 0, &enable_flag);
	for (i = 0; i < RDMA_NUM; i++) {
		enable[i] = (enable_flag >> i) & 1;
		pr_info("enable[%d]=%d\n", i, enable[i]);
	}
	return count;
}

static struct class_attribute rdma_mgr_attrs[] = {
	__ATTR(debug_flag, 0664,
	       show_debug_flag, store_debug_flag),
	__ATTR(rdma_watchdog, 0664,
	       show_rdma_watchdog, store_rdma_watchdog),
	__ATTR(reset_count, 0664,
	       show_reset_count, store_reset_count),
	__ATTR(ctrl_ahb_rd_burst_size, 0664,
	       show_ctrl_ahb_rd_burst_size, store_ctrl_ahb_rd_burst_size),
	__ATTR(ctrl_ahb_wr_burst_size, 0664,
	       show_ctrl_ahb_wr_burst_size, store_ctrl_ahb_wr_burst_size),
	__ATTR(trace_enable, 0664,
	       rdma_mgr_trace_enable_show, rdma_mgr_trace_enable_stroe),
	__ATTR(trace_reg, 0664,
	       rdma_mgr_trace_reg_show, rdma_mgr_trace_reg_stroe),
	__ATTR(ex_vsync_rdma, 0664,
	       show_ex_vsync_rdma, store_ex_vsync_rdma),
	__ATTR(irq_count_stat, 0664,
		   rdma_irq_count_show, NULL),
	__ATTR(trace_channel, 0664,
	       rdma_mgr_trace_channel_show, rdma_mgr_trace_channel_stroe),
	__ATTR(reset, 0664,
	       NULL, store_rdma_reset),
	__ATTR(enable, 0664,
	       show_enable, store_enable),
};

static struct class *rdma_mgr_class;
static int create_rdma_mgr_class(void)
{
	int i;

	rdma_mgr_class = class_create(THIS_MODULE, RDMA_MGR_CLASS_NAME);
	if (IS_ERR_OR_NULL(rdma_mgr_class)) {
		pr_err("create rdma_mgr_class failed\n");
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(rdma_mgr_attrs); i++) {
		if (class_create_file(rdma_mgr_class,
				      &rdma_mgr_attrs[i])) {
			pr_err("create rdma mgr attribute %s failed\n",
			       rdma_mgr_attrs[i].attr.name);
		}
	}
	return 0;
}

static int remove_rdma_mgr_class(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(rdma_mgr_attrs); i++)
		class_remove_file(rdma_mgr_class, &rdma_mgr_attrs[i]);

	class_destroy(rdma_mgr_class);
	rdma_mgr_class = NULL;
	return 0;
}

/* static int __devinit rdma_probe(struct platform_device *pdev) */
static int __init rdma_probe(struct platform_device *pdev)
{
	int i;
	u32 data32;
	int int_rdma;
	int handle;
	const void *prop;
	struct rdma_device_info *info = &rdma_info;
	int line_n_rdma_en = 0;

	int_rdma = platform_get_irq_byname(pdev, "rdma");
	if (int_rdma == -ENXIO) {
		dev_err(&pdev->dev, "cannot get rdma irq resource\n");
		return -ENODEV;
	}

	if (pdev->dev.of_node) {
		const struct of_device_id *match;
		struct rdma_device_data_s *rdma_meson;
		struct device_node	*of_node = pdev->dev.of_node;

		match = of_match_node(rdma_dt_match, of_node);
		if (match) {
			rdma_meson = (struct rdma_device_data_s *)match->data;
			if (rdma_meson) {
				memcpy(&rdma_meson_dev, rdma_meson,
				       sizeof(struct rdma_device_data_s));
			} else {
				pr_err("%s data NOT match\n", __func__);
				return -ENODEV;
			}
		} else {
			pr_err("%s NOT match\n", __func__);
			return -ENODEV;
		}
	} else {
		pr_err("dev %s NOT found\n", __func__);
		return -ENODEV;
	}
	/* get rdma_table_size resource  */
	rdma_table_size = RDMA_TABLE_SIZE;
	if (cpu_after_eq(MESON_CPU_MAJOR_ID_G12A)) {
		rdma_table_size = 16 * PAGE_SIZE;
		prop = of_get_property(pdev->dev.of_node, "rdma_table_page_count", NULL);
		if (prop)
			rdma_table_size = of_read_ulong(prop, 1) * PAGE_SIZE;

		prop = of_get_property(pdev->dev.of_node, "line_n_rdma", NULL);
		if (prop) {
			line_n_rdma_en = of_read_ulong(prop, 1);
			pr_info("line_n_rdma_en = %d\n", line_n_rdma_en);
		} else {
			pr_info("line_n_rdma_en = %d\n", line_n_rdma_en);
		}
	}
	pr_info("%s,cpu_type:%d, ver:%d, len:%d,rdma_table_size:%d\n", __func__,
		rdma_meson_dev.cpu_type,
		rdma_meson_dev.rdma_ver, rdma_meson_dev.trigger_mask_len, rdma_table_size);
#ifdef CONFIG_AMLOGIC_VPU
	rdma_vpu_dev = vpu_dev_register(VPU_RDMA, "rdma");
	vpu_dev_mem_power_on(rdma_vpu_dev);

	WRITE_VCBUS_REG(VPU_VDISP_ASYNC_HOLD_CTRL, 0x18101810);
	WRITE_VCBUS_REG(VPU_VPUARB2_ASYNC_HOLD_CTRL, 0x18101810);
#endif
	rdma_mgr_irq_request = 0;

	memset((void *)&info->rdma_reg, 0, sizeof(struct rdma_conflict_regs_s));
	for (i = 0; i < rdma_meson_dev.channel_num; i++) {
		info->rdma_ins[i].rdma_table_size = 0;
		if (rdma_meson_dev.rdma_ver == RDMA_VER_1) {
			info->rdma_ins[i].rdma_regadr = &rdma_regadr[i];
		} else if (rdma_meson_dev.rdma_ver == RDMA_VER_2) {
			info->rdma_ins[i].rdma_regadr = &rdma_regadr_tl1[i];
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
		} else if (rdma_meson_dev.rdma_ver == RDMA_VER_3) {
			info->rdma_ins[i].rdma_regadr = &rdma_regadr_t7[i];
			support_64bit_addr = 1;
			has_multi_vpp = 1;
		} else if (rdma_meson_dev.rdma_ver == RDMA_VER_4) {
			info->rdma_ins[i].rdma_regadr = &rdma_regadr_t7[i];
			support_64bit_addr = 1;
			has_multi_vpp = 1;
		} else if (rdma_meson_dev.rdma_ver == RDMA_VER_5) {
			info->rdma_ins[i].rdma_regadr = &rdma_regadr_t3x[i];
			support_64bit_addr = 1;
			has_multi_vpp = 1;
#endif
		} else {
			info->rdma_ins[i].rdma_regadr = &rdma_regadr[i];
		}
		info->rdma_ins[i].keep_buf = 1;
		/*do not change it in normal case */
		info->rdma_ins[i].used = 0;
		info->rdma_ins[i].prev_trigger_type = 0;
		info->rdma_ins[i].rdma_write_count = 0;
		info->rdma_ins[i].lock_flag = 0;
		enable[i] = 1;
	}

	if (rdma_meson_dev.rdma_ver == RDMA_VER_5) {
		irq_status.reg = T3X_RDMA_STATUS1;
		irq_status.start = 4;
		irq_status.len = 16;
		rdma_ctrl = T3X_RDMA_CTRL;
		rdma_access_man = T3X_RDMA_ACCESS_MAN;
		rdma_status_reg = T3X_RDMA_STATUS;
	} else {
		irq_status.reg = RDMA_STATUS;
		irq_status.start = 24;
		irq_status.len = 8;
		rdma_ctrl = RDMA_CTRL;
		rdma_access_man = RDMA_ACCESS_MAN;
		rdma_status_reg = RDMA_STATUS;
	}

	if (rdma_meson_dev.cpu_type >= CPU_SC2) {
		rdma_rst = devm_reset_control_get(&pdev->dev, "rdma");
		if (IS_ERR(rdma_rst)) {
			pr_err("failed to get reset: %ld\n", PTR_ERR(rdma_rst));
			return PTR_ERR(rdma_rst);
		}
		reset_control_reset(rdma_rst);
	} else {
		WRITE_MPEG_REG(RESET4_REGISTER, (1 << 5));
	}

#ifdef SKIP_OSD_CHANNEL
	info->rdma_ins[3].used = 1; /* OSD driver uses this channel */
#endif

	if (request_irq(int_rdma, &rdma_mgr_isr,
			IRQF_SHARED, "rdma", (void *)"rdma")) {
		dev_err(&pdev->dev, "can't request irq for rdma\n");
		return -ENODEV;
	}

	rdma_mgr_irq_request = 1;
	data32  = 0;
	data32 |= 1 << 7; /* write ddr urgent */
	data32 |= 1 << 6; /* read ddr urgent */
	data32 |= ctrl_ahb_wr_burst_size << 4;
	data32 |= ctrl_ahb_rd_burst_size << 2;
	data32 |= 0 << 1;
	data32 |= 0 << 0;
	WRITE_VCBUS_REG(rdma_ctrl, data32);

	info->rdma_dev = pdev;
	if (is_meson_t5m_cpu()) {
		/* rdma alloc in 1G */
		info->rdma_dev->dev.coherent_dma_mask = DMA_BIT_MASK(30);
		info->rdma_dev->dev.dma_mask = &info->rdma_dev->dev.coherent_dma_mask;
	}

	handle = rdma_register(get_rdma_ops(VSYNC_RDMA),
			       NULL, rdma_table_size);
	set_rdma_handle(VSYNC_RDMA, handle);
	if (!has_multi_vpp) {
		handle = rdma_register(get_rdma_ops(VSYNC_RDMA_READ),
				       NULL, rdma_table_size);
		set_rdma_handle(VSYNC_RDMA_READ, handle);
	}
	/* T7 lack rdma channel. So do not alloc EX_VSYNC_RDMA channel for 3 vppout */
	if (rdma_meson_dev.rdma_ver != RDMA_VER_3) {
		handle = rdma_register(get_rdma_ops(EX_VSYNC_RDMA),
			NULL, RDMA_TABLE_SIZE);
		set_rdma_handle(EX_VSYNC_RDMA, handle);
	}

	if (line_n_rdma_en) {
		handle = rdma_register(get_rdma_ops(LINE_N_INT_RDMA),
			NULL, 2 * rdma_table_size);
		set_rdma_handle(LINE_N_INT_RDMA, handle);
	}

	create_rdma_mgr_class();

	rdma_init();
	return 0;
}

#ifdef CONFIG_PM
static int rdma_suspend(struct platform_device *dev, pm_message_t state)
{
	set_rdma_channel_enable(0);
	return 0;
}

static int rdma_resume(struct platform_device *dev)
{
	set_rdma_channel_enable(1);
	return 0;
}
#endif

/* static int __devexit rdma_remove(struct platform_device *pdev) */
static int rdma_remove(struct platform_device *pdev)
{
	pr_error("RDMA driver removed.\n");
	remove_rdma_mgr_class();
	rdma_exit();
#ifdef CONFIG_AMLOGIC_VPU
	vpu_dev_mem_power_down(rdma_vpu_dev);
	vpu_dev_unregister(rdma_vpu_dev);
#endif
	return 0;
}

static struct platform_driver rdma_driver = {
	.remove = rdma_remove,
#ifdef CONFIG_PM
	.suspend  = rdma_suspend,
	.resume    = rdma_resume,
#endif
	.driver = {
		.name = "amlogic-rdma",
		.of_match_table = rdma_dt_match,
	},
};

int __init amrdma_init(void)
{
	return platform_driver_probe(&rdma_driver, rdma_probe);
}

void __exit amrdma_exit(void)
{
	platform_driver_unregister(&rdma_driver);
}

//MODULE_DESCRIPTION("AMLOGIC RDMA management driver");
//MODULE_LICENSE("GPL");
//MODULE_AUTHOR("Rain Zhang <rain.zhang@amlogic.com>");
