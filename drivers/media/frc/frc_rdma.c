// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

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
#include <linux/sched/clock.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/types.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-map-ops.h>
#include <linux/cma.h>
#include <linux/genalloc.h>
#include <linux/amlogic/media/frc/frc_reg.h>

#include "frc_drv.h"
#include "frc_rdma.h"
#include "frc_hw.h"
// #include "frc_regs_table.h"

static u32 rdma_ctrl;
static struct frc_rdma_irq_reg_s irq_status;

static int rdma_cnt;

u32 rdma_trace_num;
static u32 rdma_trace_enable;
u32 rdma_trace_reg[MAX_TRACE_NUM];

struct frc_rdma_s frc_rdma;
struct frc_rdma_info rdma_info[RDMA_CHANNEL]; // 0:man 1:input 2:output 3:dbg 4:read-in

struct reg_test regs_table_test[REG_TEST_NUM] = {
	{0x3210, 0x0}, {0x3211, 0x1}, {0x3212, 0x2}, {0x3213, 0x3},
	{0x3214, 0x4}, {0x3215, 0x5}, {0x3216, 0x6}, {0x3217, 0x7},
	{0x3218, 0x8}, {0x3219, 0x9}, {0x321a, 0xa}, {0x321b, 0xb},
	{0x321c, 0xc}, {0x321d, 0xd}, {0x321e, 0xe}, {0x321f, 0xf},
};

static struct rdma_regadr_s rdma_regadr_t3[RDMA_NUM] = {
	{
		FRC_RDMA_AHB_START_ADDR_MAN,
		FRC_RDMA_AHB_START_ADDR_MAN_MSB,
		FRC_RDMA_AHB_END_ADDR_MAN,
		FRC_RDMA_AHB_END_ADDR_MAN_MSB,
		FRC_RDMA_ACCESS_MAN, 0,
		FRC_RDMA_ACCESS_MAN, 1,
		FRC_RDMA_ACCESS_MAN, 2,
		24, 24
	},
	{
		FRC_RDMA_AHB_START_ADDR_1,
		FRC_RDMA_AHB_START_ADDR_1_MSB,
		FRC_RDMA_AHB_END_ADDR_1,
		FRC_RDMA_AHB_END_ADDR_1_MSB,
		FRC_RDMA_AUTO_SRC1_SEL,  1,
		FRC_RDMA_ACCESS_AUTO,  1,
		FRC_RDMA_ACCESS_AUTO,  5,
		25, 25
	},
	{
		FRC_RDMA_AHB_START_ADDR_2,
		FRC_RDMA_AHB_START_ADDR_2_MSB,
		FRC_RDMA_AHB_END_ADDR_2,
		FRC_RDMA_AHB_END_ADDR_2_MSB,
		FRC_RDMA_AUTO_SRC2_SEL,  0,
		FRC_RDMA_ACCESS_AUTO,  2,
		FRC_RDMA_ACCESS_AUTO,  6,
		26, 26
	},
	{
		FRC_RDMA_AHB_START_ADDR_3,
		FRC_RDMA_AHB_START_ADDR_3_MSB,
		FRC_RDMA_AHB_END_ADDR_3,
		FRC_RDMA_AHB_END_ADDR_3_MSB,
		FRC_RDMA_AUTO_SRC3_SEL,  0,
		FRC_RDMA_ACCESS_AUTO,  3,
		FRC_RDMA_ACCESS_AUTO,  7,
		27, 27
	},
	{
		FRC_RDMA_AHB_START_ADDR_4,
		FRC_RDMA_AHB_START_ADDR_4_MSB,
		FRC_RDMA_AHB_END_ADDR_4,
		FRC_RDMA_AHB_END_ADDR_4_MSB,
		FRC_RDMA_AUTO_SRC4_SEL, 0,
		FRC_RDMA_ACCESS_AUTO2, 0,
		FRC_RDMA_ACCESS_AUTO2, 4,
		28, 28
	},
	{
		FRC_RDMA_AHB_START_ADDR_5,
		FRC_RDMA_AHB_START_ADDR_5_MSB,
		FRC_RDMA_AHB_END_ADDR_5,
		FRC_RDMA_AHB_END_ADDR_5_MSB,
		FRC_RDMA_AUTO_SRC5_SEL, 0,
		FRC_RDMA_ACCESS_AUTO2, 1,
		FRC_RDMA_ACCESS_AUTO2, 5,
		29, 29
	},
	{
		FRC_RDMA_AHB_START_ADDR_6,
		FRC_RDMA_AHB_START_ADDR_6_MSB,
		FRC_RDMA_AHB_END_ADDR_6,
		FRC_RDMA_AHB_END_ADDR_6_MSB,
		FRC_RDMA_AUTO_SRC6_SEL, 0,
		FRC_RDMA_ACCESS_AUTO2, 2,
		FRC_RDMA_ACCESS_AUTO2, 6,
		30, 30
	},
	{
		FRC_RDMA_AHB_START_ADDR_7,
		FRC_RDMA_AHB_START_ADDR_7_MSB,
		FRC_RDMA_AHB_END_ADDR_7,
		FRC_RDMA_AHB_END_ADDR_7_MSB,
		FRC_RDMA_AUTO_SRC7_SEL, 0,
		FRC_RDMA_ACCESS_AUTO2, 3,
		FRC_RDMA_ACCESS_AUTO2, 7,
		31, 31
	}
};

static struct rdma_regadr_s rdma_regadr_t3x[RDMA_NUM] = {
	{
		FRC_RDMA_AHB_START_ADDR_MAN_T3X,
		FRC_RDMA_AHB_START_ADDR_MAN_MSB_T3X,
		FRC_RDMA_AHB_END_ADDR_MAN_T3X,
		FRC_RDMA_AHB_END_ADDR_MAN_MSB_T3X,
		FRC_RDMA_ACCESS_MAN_T3X, 0,
		FRC_RDMA_ACCESS_MAN_T3X, 1,
		FRC_RDMA_ACCESS_MAN_T3X, 2,
		16, 4
	},
	{
		FRC_RDMA_AHB_START_ADDR_1_T3X,
		FRC_RDMA_AHB_START_ADDR_1_MSB_T3X,
		FRC_RDMA_AHB_END_ADDR_1_T3X,
		FRC_RDMA_AHB_END_ADDR_1_MSB_T3X,
		FRC_RDMA_AUTO_SRC1_SEL_T3X,  0,
		FRC_RDMA_ACCESS_AUTO_T3X,  1,
		FRC_RDMA_ACCESS_AUTO_T3X,  5,
		17, 5
	},
	{
		FRC_RDMA_AHB_START_ADDR_2_T3X,
		FRC_RDMA_AHB_START_ADDR_2_MSB_T3X,
		FRC_RDMA_AHB_END_ADDR_2_T3X,
		FRC_RDMA_AHB_END_ADDR_2_MSB_T3X,
		FRC_RDMA_AUTO_SRC2_SEL_T3X,  0,
		FRC_RDMA_ACCESS_AUTO_T3X,  2,
		FRC_RDMA_ACCESS_AUTO_T3X,  6,
		18, 6
	},
	{
		FRC_RDMA_AHB_START_ADDR_3_T3X,
		FRC_RDMA_AHB_START_ADDR_3_MSB_T3X,
		FRC_RDMA_AHB_END_ADDR_3_T3X,
		FRC_RDMA_AHB_END_ADDR_3_MSB_T3X,
		FRC_RDMA_AUTO_SRC3_SEL_T3X,  0,
		FRC_RDMA_ACCESS_AUTO_T3X,  3,
		FRC_RDMA_ACCESS_AUTO_T3X,  7,
		19, 7
	},
	{
		FRC_RDMA_AHB_START_ADDR_4_T3X,
		FRC_RDMA_AHB_START_ADDR_4_MSB_T3X,
		FRC_RDMA_AHB_END_ADDR_4_T3X,
		FRC_RDMA_AHB_END_ADDR_4_MSB_T3X,
		FRC_RDMA_AUTO_SRC4_SEL_T3X, 0,
		FRC_RDMA_ACCESS_AUTO2_T3X, 0,
		FRC_RDMA_ACCESS_AUTO2_T3X, 4,
		20, 8
	},
	{
		FRC_RDMA_AHB_START_ADDR_5,
		FRC_RDMA_AHB_START_ADDR_5_MSB,
		FRC_RDMA_AHB_END_ADDR_5,
		FRC_RDMA_AHB_END_ADDR_5_MSB,
		FRC_RDMA_AUTO_SRC5_SEL, 0,
		FRC_RDMA_ACCESS_AUTO2, 1,
		FRC_RDMA_ACCESS_AUTO2, 5,
		21, 9
	},
	{
		FRC_RDMA_AHB_START_ADDR_6_T3X,
		FRC_RDMA_AHB_START_ADDR_6_MSB_T3X,
		FRC_RDMA_AHB_END_ADDR_6_T3X,
		FRC_RDMA_AHB_END_ADDR_6_MSB_T3X,
		FRC_RDMA_AUTO_SRC6_SEL_T3X, 0,
		FRC_RDMA_ACCESS_AUTO2_T3X, 2,
		FRC_RDMA_ACCESS_AUTO2_T3X, 6,
		22, 10
	},
	{
		FRC_RDMA_AHB_START_ADDR_7_T3X,
		FRC_RDMA_AHB_START_ADDR_7_MSB_T3X,
		FRC_RDMA_AHB_END_ADDR_7_T3X,
		FRC_RDMA_AHB_END_ADDR_7_MSB_T3X,
		FRC_RDMA_AUTO_SRC7_SEL_T3X, 0,
		FRC_RDMA_ACCESS_AUTO2_T3X, 3,
		FRC_RDMA_ACCESS_AUTO2_T3X, 7,
		23, 11
	}
};

struct frc_rdma_s *get_frc_rdma(void)
{
	return &frc_rdma;
}
EXPORT_SYMBOL(get_frc_rdma);

int is_rdma_enable(void)
{
	struct frc_dev_s *devp;
	struct frc_fw_data_s *fw_data;
	struct frc_rdma_s *frc_rdma;

	devp = get_frc_devp();
	frc_rdma = get_frc_rdma();
	fw_data = (struct frc_fw_data_s *)devp->fw_data;

	if (!frc_rdma->buf_alloced || !frc_rdma->rdma_en /*fw_data->frc_top_type.rdma_en*/)
		return 0;
	else
		return 1;
}

void frc_rdma_alloc_buf(struct frc_dev_s *devp)
{
	u8 i;
	enum chip_id chip;
	phys_addr_t dma_handle;
	struct platform_device *pdev;
	struct frc_rdma_s *frc_rdma;
	struct frc_rdma_info *rdma_info;

	chip = get_chip_type();
	frc_rdma = get_frc_rdma();

	if (!devp || !frc_rdma) {
		pr_frc(0, "%s rdma alloc buf failed\n", __func__);
		return;
	}

	pdev = (struct platform_device *)devp->pdev;
	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(64);
	pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

	for (i = 0; i < RDMA_CHANNEL; i++) {
		rdma_info = frc_rdma->rdma_info[i];

		if (!rdma_info->buf_status) {
			// GFP_KERNEL: alloc buf from dts reserved
			// T5M needs to work in reserved memory
			if (chip == ID_T5M)
				rdma_info->rdma_table_addr =
					dma_alloc_coherent(&devp->pdev->dev,
					rdma_info->rdma_table_size, &dma_handle, GFP_KERNEL);
			else
				rdma_info->rdma_table_addr =
					dma_alloc_coherent(&devp->pdev->dev,
					rdma_info->rdma_table_size, &dma_handle, 0);
			rdma_info->rdma_table_phy_addr = dma_handle;
			rdma_info->buf_status = 1;
		}
		if (sizeof(rdma_info->rdma_table_phy_addr) > 4)
			rdma_info->is_64bit_addr = 1; // 64bit addr
		else
			rdma_info->is_64bit_addr = 0;

		pr_frc(2, "%s channel:%d rdma_table_addr: %lx phy:%lx size:%x\n",
			__func__, i, (ulong)rdma_info->rdma_table_addr,
			(ulong)rdma_info->rdma_table_phy_addr,
			rdma_info->rdma_table_size);
	}
	frc_rdma->buf_alloced = 1;
}

void frc_rdma_release_buf(void)
{
	u8 i;
	struct frc_dev_s *devp;
	struct frc_rdma_s *frc_rdma;
	struct frc_rdma_info *rdma_info;

	devp = get_frc_devp();
	frc_rdma = get_frc_rdma();

	if (!devp || !frc_rdma)
		return;

	if (!frc_rdma->buf_alloced) {
		pr_frc(0, "%s buf alread released\n", __func__);
		return;
	}

	for (i = 0; i < RDMA_CHANNEL; i++) {
		rdma_info = frc_rdma->rdma_info[i];

		if (rdma_info->buf_status) {
			dma_free_coherent(&devp->pdev->dev,
			rdma_info->rdma_table_size, rdma_info->rdma_table_addr,
			(dma_addr_t)rdma_info->rdma_table_phy_addr);
			rdma_info->buf_status = 0;
		}
	}
	frc_rdma->buf_alloced = 0;

	pr_frc(0, "%s rdma buf released done\n", __func__);
}

void frc_rdma_read_test_reg(void)
{
	u32 i;

	for (i = 0x0; i < 0x10; i++)
		pr_frc(0, "addr[%x]:%x\n", i + 0x3210, READ_FRC_REG(i + 0x3210));
}

void frc_read_table(struct frc_rdma_s *frc_rdma)
{
	u32 i, j, count;
	struct frc_rdma_info *rdma_info;

	for (i = 0; i < RDMA_CHANNEL; i++) {
		rdma_info = (struct frc_rdma_info *)frc_rdma->rdma_info[i];
		count = rdma_info->rdma_item_count;
		pr_frc(0, "--------------table:%d:[%d]----------------\n", i, count);
		for (j = 0; j < count; j++) {
			pr_frc(0, "addr:0x%04x, value:0x%08x\n",
				rdma_info->rdma_table_addr[j * 2],
				rdma_info->rdma_table_addr[j * 2 + 1]);
		}
		pr_frc(0, "--------------table:%d done----------------\n", i);
	}
}

int frc_check_table(u32 addr)
{
	int i;
	int index = -1;
	struct frc_rdma_info *rdma_info;
	struct frc_rdma_s *frc_rdma = get_frc_rdma();

	rdma_info = (struct frc_rdma_info *)frc_rdma->rdma_info[0];

	if (rdma_info->rdma_item_count == 0)
		return -1;

	pr_frc(20, "rdma_item_count:%d addr:%x\n",
		rdma_info->rdma_item_count, addr);

	for (i = (rdma_info->rdma_item_count - 1) * 2; i >= 0; i -= 2) {
		pr_frc(20, "i:%d, table_addr[%d]=%x, table_value[%d]=%x",
			i, i, rdma_info->rdma_table_addr[i], i + 1,
			rdma_info->rdma_table_addr[i + 1]);
		if (rdma_info->rdma_table_addr[i] == addr) {
			//value = rdma_info->rdma_table_addr[i + 1];
			index = i / 2 + 1;
			break;
		}
	}

	pr_frc(20, "%s index:%d\n", __func__, index);
	return index;
}

void frc_rdma_table_config(u32 addr, u32 val)
{
	u32 i;
	struct frc_rdma_info *rdma_info;
	struct frc_rdma_s *frc_rdma = get_frc_rdma();
	int pr_flag = 11;

	rdma_info = (struct frc_rdma_info *)frc_rdma->rdma_info[0]; // man
	i = rdma_info->rdma_item_count;
	pr_frc(pr_flag, "i:%d, addr:%x, val:%x\n", i, addr, val);

	if ((i + 1) * 8 > rdma_info->rdma_table_size) {
		pr_frc(0, "frc rdma buffer overflow\n");
		return;
	}

	rdma_info->rdma_table_addr[i * 2] = addr & 0xffffffff;
	rdma_info->rdma_table_addr[i * 2 + 1] = val & 0xffffffff;
	pr_frc(pr_flag, "addr:%04x, value:%08x\n",
		rdma_info->rdma_table_addr[i * 2],
		rdma_info->rdma_table_addr[i * 2 + 1]);

	rdma_info->rdma_item_count++;
}

int frc_rdma_rd_reg(u32 addr)
{
	u32 reg_val;
	u32 i, j, tol;
	struct frc_rdma_info *rdma_info;
	struct frc_rdma_s *frc_rdma = get_frc_rdma();
	int pr_flag = 11;
	int match = 0;

	rdma_info = (struct frc_rdma_info *)frc_rdma->rdma_info[4]; // read
	tol = rdma_info->rdma_item_count;

	// if ((tol + 1) * 8 > rdma_info->rdma_table_size) {
	// pr_frc(2, "frc rdma rd buffer overflow\n");
	// return;
	// }

	for (i = 0; i < tol; i++) {
		if (addr == rdma_info->rdma_table_addr[i]) {
			reg_val = rdma_info->rdma_table_addr[tol + i];
			match = 1;
			break;
		}
		pr_frc(7, "table:%d, 0x%x, 0x%x",
			i,
			rdma_info->rdma_table_addr[i * 2],
			rdma_info->rdma_table_addr[i * 2 + 1]);
	}

	if (!match) {
		reg_val = READ_FRC_REG(addr);
		pr_frc(pr_flag, "match error, rdma read error\n");
	} else {
		pr_frc(pr_flag, "match, rdma read success\n");
	}

	pr_frc(pr_flag, "match:%d, addr:%04x, value:%08x\n",
		match,
		rdma_info->rdma_table_addr[i * 2],
		rdma_info->rdma_table_addr[i * 2 + 1]);

	if (rdma_trace_enable) {
		for (j = 0; j < rdma_trace_num; j++) {
			if (addr == rdma_trace_reg[j]) {
				pr_frc(0, "(%s) handle out_isr, %04x=0x%08x (%d)\n",
					__func__, addr, reg_val,
					rdma_info->rdma_item_count);
			}
		}
	}
	return reg_val;
}

void frc_rdma_in_table_config(u32 addr, u32 val)
{
	u32 i, j;
	struct frc_rdma_info *rdma_info;
	struct frc_rdma_s *frc_rdma = get_frc_rdma();
	int pr_flag = 11;

	rdma_info = (struct frc_rdma_info *)frc_rdma->rdma_info[1]; // in
	i = rdma_info->rdma_item_count;
	pr_frc(pr_flag, "i:%d, addr:%x, val:%x\n", i, addr, val);

	if ((i + 1) * 8 > rdma_info->rdma_table_size) {
		pr_frc(2, "frc rdma in buffer overflow\n");
		return;
	}

	rdma_info->rdma_table_addr[i * 2] = addr & 0xffffffff;
	rdma_info->rdma_table_addr[i * 2 + 1] = val & 0xffffffff;

	pr_frc(pr_flag, "addr:%04x, value:%08x\n",
		rdma_info->rdma_table_addr[i * 2],
		rdma_info->rdma_table_addr[i * 2 + 1]);

	if (rdma_trace_enable) {
		for (j = 0; j < rdma_trace_num; j++) {
			if (addr == rdma_trace_reg[j]) {
				pr_frc(0, "(%s) handle in_isr, %04x=0x%08x (%d)\n",
					__func__, addr, val,
					rdma_info->rdma_item_count);
			}
		}
	}
	rdma_info->rdma_item_count++;
}

void frc_rdma_out_table_config(u32 addr, u32 val)
{
	u32 i, j;
	struct frc_rdma_info *rdma_info;
	struct frc_rdma_s *frc_rdma = get_frc_rdma();
	int pr_flag = 11;

	rdma_info = (struct frc_rdma_info *)frc_rdma->rdma_info[2]; // out
	i = rdma_info->rdma_item_count;
	pr_frc(pr_flag, "i:%d, addr:%x, val:%x\n", i, addr, val);

	if ((i + 1) * 8 > rdma_info->rdma_table_size) {
		pr_frc(2, "frc rdma out buffer overflow count:%d, size%x\n",
			i, rdma_info->rdma_table_size);
		return;
	}

	rdma_info->rdma_table_addr[i * 2] = addr & 0xffffffff;
	rdma_info->rdma_table_addr[i * 2 + 1] = val & 0xffffffff;

	pr_frc(pr_flag, "addr:%04x, value:%08x\n",
		rdma_info->rdma_table_addr[i * 2],
		rdma_info->rdma_table_addr[i * 2 + 1]);

	if (rdma_trace_enable) {
		for (j = 0; j < rdma_trace_num; j++) {
			if (addr == rdma_trace_reg[j]) {
				pr_frc(0, "(%s) handle out_isr, %04x=0x%08x (%d)\n",
					__func__, addr, val,
					rdma_info->rdma_item_count);
			}
		}
	}
	rdma_info->rdma_item_count++;
}

int frc_rdma_write_reg(u32 addr, u32 val)
{
	// int ret;
	int pr_flag = 10;
	// u32 write_val;

	// ret = frc_check_table(addr);
	// if (ret == -1)
	// write_val = readl(frc_base + (addr << 2));
	// else
	// write_val = ret;

	pr_frc(pr_flag, "addr:0x%x write_val:0x%x\n", addr, val);
	frc_rdma_table_config(addr, val);

	return 0;
}

int frc_rdma_write_bits(u32 addr, u32 val, u32 start, u32 len)
{
	int ret;
	int pr_flag = 10;
	u32 read_val, write_val, mask;

	// read_val = READ_FRC_REG(addr);
	ret = frc_check_table(addr);
	if (ret == -1)
		read_val = readl(frc_base + (addr << 2));
	else
		read_val = ret;

	mask = (((1L << len) - 1) << start);
	write_val  = read_val & ~mask;
	write_val |= (val << start) & mask;

	pr_frc(pr_flag, "addr:0x%x read_val:0x%x write_val:0x%x\n",
	addr, read_val, write_val);
	frc_rdma_table_config(addr, write_val);

	return 0;
}

int frc_rdma_update_reg_bits(u32 addr, u32 val, u32 mask)
{
	u32 read_val, write_val;
	int pr_flag = 10;

	/*check rdma_table reg value*/
	if (mask == 0xFFFFFFFF) {
		// WRITE_FRC_REG_BY_CPU(addr, val);
		frc_rdma_table_config(addr, val);
	} else {
		// index = frc_check_table(addr);
		// if (index == -1) {
		read_val = readl(frc_base + (addr << 2));
		val &= mask;
		write_val = read_val & ~mask;
		write_val |= val;
		pr_frc(pr_flag, "addr:0x%04x read_val:0x%x write_val:0x%x\n",
			addr, read_val, write_val);
		frc_rdma_table_config(addr, write_val);
	}

	return 0;
}

int FRC_RDMA_VSYNC_WR_REG(u32 addr, u32 val)
{
	int pr_flag = 10;

	if (get_frc_devp()->power_on_flag == 0)
		return 0;

	pr_frc(pr_flag, "man:%s addr:0x%08x, val:0x%x\n",
		__func__, addr, val);

	if (is_rdma_enable()) {
		//frc_rdma_table_config(addr, val);
		frc_rdma_write_reg(addr, val);
	} else {
		writel(val, (frc_base + (addr << 2)));
	}

	return 0;
}
EXPORT_SYMBOL(FRC_RDMA_VSYNC_WR_REG);

int _frc_rdma_wr_reg_in(u32 addr, u32 val)
{
	int pr_flag = 10;

	if (get_frc_devp()->power_on_flag == 0)
		return 0;

	// pr_frc(flag, "in: addr:0x%08x, val:0x%x\n", addr, val);

	if (is_rdma_enable()) {
		frc_rdma_in_table_config(addr, val);
		pr_frc(pr_flag, "rdma on, channel in addr:0x%04x, val:0x%08x\n",
			addr, val);
	} else {
		writel(val, (frc_base + (addr << 2)));
		pr_frc(pr_flag, "rdma off, channel in addr:0x%04x, val:0x%08x\n",
			addr, val);
	}

	return 0;
}

int _frc_rdma_wr_reg_out(u32 addr, u32 val)
{
	int pr_flag = 10;

	if (get_frc_devp()->power_on_flag == 0)
		return 0;

	if (is_rdma_enable()) {
		frc_rdma_out_table_config(addr, val);
		pr_frc(pr_flag, "rdma on, channel out addr:0x%04x, val:0x%08x\n",
			addr, val);
	} else {
		writel(val, (frc_base + (addr << 2)));
		pr_frc(pr_flag, "rdma off, channel out addr:0x%04x, val:0x%08x\n",
			addr, val);
	}

	return 0;
}

int _frc_rdma_rd_reg(u32 addr)
{
	u32 reg_val = readl(frc_base + (addr << 2));
	int pr_flag = 10;

	if (get_frc_devp()->power_on_flag == 0)
		return 0;

	if (is_rdma_enable()) {
		// frc_rdma_out_table_config(addr, val);
		reg_val = frc_rdma_rd_reg(addr);
		pr_frc(pr_flag, "rdma on, channel rd addr:0x%04x, reg_val:0x%08x\n",
			addr, reg_val);
	} else {
		// reg_val = readl(frc_base + (addr << 2));
		pr_frc(pr_flag, "rdma off, channel rd addr:0x%04x, reg_val:0x%08x\n",
			addr, reg_val);
	}

	return reg_val;
}

int FRC_RDMA_VSYNC_WR_BITS(u32 addr, u32 val, u32 start, u32 len)
{
	if (get_frc_devp()->power_on_flag == 0)
		return 0;

	if (is_rdma_enable()) {
		frc_rdma_write_bits(addr, val, start, len);
	} else {
		u32 write_val, mask;
		u32 read_val = readl(frc_base + (addr << 2));
		// write_val = (write_val & ~(((1L << (len)) - 1) << (start))) |
		//     ((u32)(val) << (start));
		mask = (((1L << len) - 1) << start);
		write_val  = read_val & ~mask;
		write_val |= (val << start) & mask;
		writel(write_val, (frc_base + (addr << 2)));
	}

	return 0;
}
EXPORT_SYMBOL(FRC_RDMA_VSYNC_WR_BITS);

int FRC_RDMA_VSYNC_REG_UPDATE(u32 addr, u32 val, u32 mask)
{
	int pr_flag = 10;

	if (get_frc_devp()->power_on_flag == 0)
		return 0;
	pr_frc(pr_flag, "in: addr:0x%08x, val:0x%x, mask:0x%x\n", addr, val, mask);

	if (is_rdma_enable()) {
		frc_rdma_update_reg_bits(addr, val, mask);
	} else {
		if (mask == 0xFFFFFFFF) {
			writel(val, (frc_base + (addr << 2)));
			//frc_rdma_table_config(addr, val);
		} else {
			u32 write_val = readl(frc_base + (addr << 2));

			val &= mask;
			write_val &= ~mask;
			write_val |= val;
			// writel(write_val, (frc_base + (addr << 2)));
			frc_rdma_table_config(addr, write_val);
		}
	}

	return 0;
}
EXPORT_SYMBOL(FRC_RDMA_VSYNC_REG_UPDATE);

void frc_rdma_reg_list(void)
{
	int i, temp;
	struct frc_rdma_info *rdma_info;
	struct frc_rdma_s *frc_rdma = get_frc_rdma();

	rdma_info = (struct frc_rdma_info *)frc_rdma->rdma_info[0];
	temp = rdma_info->rdma_item_count;

	for (i = 0; i < temp; i++) {
		pr_frc(0, "reg list: addr:0x%04x, value:0x%08x\n",
			rdma_info->rdma_table_addr[i * 2],
			rdma_info->rdma_table_addr[i * 2 + 1]);
	}
	pr_frc(0, "---------------------------------------\n");
}

/*
 * copy dbg tool reg val to out-isr write
 */
void frc_rdma_check_tool_reg(struct frc_rdma_s *frc_rdma)
{
	u32 i, j, count;
	struct frc_rdma_info *rdma_info;
	struct frc_rdma_info *rdma_info_dbg;
	int pr_flag = 10;

	rdma_info = (struct frc_rdma_info *)frc_rdma->rdma_info[2]; // out isr write
	rdma_info_dbg = (struct frc_rdma_info *)frc_rdma->rdma_info[3];

	// for debug_tool when rdma on
	if (rdma_info_dbg->rdma_item_count > 0) {
		i = rdma_info->rdma_item_count;
		count = rdma_info_dbg->rdma_item_count;

		for (j = 0; j < count; j++) {
			rdma_info->rdma_table_addr[i * 2] =
				rdma_info_dbg->rdma_table_addr[j * 2];
			rdma_info->rdma_table_addr[i * 2 + 1] =
				rdma_info_dbg->rdma_table_addr[j * 2 + 1];
			pr_frc(pr_flag, "%s addr:0x%04x, value:0x%08x\n",
				__func__,
				rdma_info->rdma_table_addr[i * 2],
				rdma_info->rdma_table_addr[i * 2 + 1]);

			rdma_info->rdma_item_count = ++i;
		}
		rdma_info_dbg->rdma_item_count = 0;
	}
}

void frc_rdma_status(void)
{
	u8 i;
	struct rdma_regadr_s *rdma_regadr;
	struct frc_rdma_info *rdma_info;
	struct frc_rdma_s *frc_rdma = get_frc_rdma();

	pr_frc(0, "rdma version = %s\n", FRC_RDMA_VER);
	pr_frc(0, "rdma used channel = %d\n", RDMA_CHANNEL);
	for (i = 0; i < RDMA_CHANNEL; i++) {
		rdma_info = (struct frc_rdma_info *)frc_rdma->rdma_info[i];
		pr_frc(0, "%s channel:%d rdma_table_addr: %lx phy:%lx size:%x\n",
			__func__, i, (ulong)rdma_info->rdma_table_addr,
			(ulong)rdma_info->rdma_table_phy_addr,
			rdma_info->rdma_table_size);
	}
	pr_frc(0, "rdma in_isr = %08x out_isr = %08x\n",
		READ_FRC_REG(FRC_REG_TOP_RESERVE13),
		READ_FRC_REG(FRC_REG_TOP_RESERVE14));
	pr_frc(0, "rdma trace enable = %d\n", rdma_trace_enable);
	if (rdma_trace_enable && rdma_trace_num) {
		pr_frc(0, "rdma trace num = %d\n", rdma_trace_num);
		for (i = 0; i < rdma_trace_num; i++)
			pr_frc(0, "trace reg addr[%d] = %04x\n", i, rdma_trace_reg[i]);
	}

	pr_frc(0, "--------------- frc rdma HW info ---------------\n");
	if (get_chip_type() >= ID_T3X) {
		// rdma_status = READ_FRC_REG(irq_status.reg);
		for (i = 0; i < RDMA_CHANNEL - 1; i++) {
			rdma_regadr =
			(struct rdma_regadr_s *)frc_rdma->rdma_info[i]->rdma_regadr;
			pr_frc(0, "channel:%d start addr:%08x end:%08x status:%x\n",
				i,
				READ_FRC_REG(rdma_regadr->rdma_ahb_start_addr),
				READ_FRC_REG(rdma_regadr->rdma_ahb_end_addr),
				READ_FRC_REG(rdma_regadr->trigger_mask_reg));
		}
	} else {
		rdma_regadr =
		(struct rdma_regadr_s *)frc_rdma->rdma_info[0]->rdma_regadr;
		pr_frc(0, "man start addr:%08x end:%08x status:%x\n",
			READ_FRC_REG(rdma_regadr->rdma_ahb_start_addr),
			READ_FRC_REG(rdma_regadr->rdma_ahb_end_addr),
			READ_FRC_REG(rdma_regadr->trigger_mask_reg));
	}
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

ssize_t frc_rdma_trace_enable_show(struct class *cla,
	struct class_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%x\n", rdma_trace_enable);
}

ssize_t frc_rdma_trace_enable_stroe(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int ret = 0;

	ret = kstrtoint(buf, 0, &rdma_trace_enable);
	if (ret < 0)
		return -EINVAL;
	return count;
}

ssize_t frc_rdma_trace_reg_show(struct class *cla,
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

ssize_t frc_rdma_trace_reg_stroe(struct class *cla,
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
			pr_frc(0, "trace reg:0x%x\n", rdma_trace_reg[i]);
		}
	}
	return count;
}

/*
 * interrupt call
 */
int frc_rdma_config(int handle, u32 trigger_type)
{
	struct frc_rdma_info *rdma_info;
	struct rdma_regadr_s *rdma_regadr;
	struct frc_rdma_s *frc_rdma = get_frc_rdma();
	int pr_flag = 9;

	if (!is_rdma_enable())
		return 0;

	rdma_info = (struct frc_rdma_info *)frc_rdma->rdma_info[handle];
	rdma_regadr = (struct rdma_regadr_s *)frc_rdma->rdma_info[handle]->rdma_regadr;
	frc_rdma_check_tool_reg(frc_rdma);

	if (handle == 0) { //man
		// manual RDMA
		WRITE_FRC_REG_BY_CPU(rdma_regadr->trigger_mask_reg,
			READ_FRC_REG(rdma_regadr->trigger_mask_reg) & (~1));
		#ifdef CONFIG_ARM64
		WRITE_FRC_REG_BY_CPU(rdma_regadr->rdma_ahb_start_addr,
			rdma_info->rdma_table_phy_addr & 0xffffffff);
		WRITE_FRC_REG_BY_CPU(rdma_regadr->rdma_ahb_start_addr_msb,
			(rdma_info->rdma_table_phy_addr >> 32) & 0xf);
		WRITE_FRC_REG_BY_CPU(rdma_regadr->rdma_ahb_end_addr,
			(rdma_info->rdma_table_phy_addr +
			rdma_info->rdma_item_count * 8 - 1) & 0xffffffff);
		WRITE_FRC_REG_BY_CPU(rdma_regadr->rdma_ahb_end_addr_msb,
			((rdma_info->rdma_table_phy_addr +
			rdma_info->rdma_item_count * 8 - 1) >> 32) & 0xf);
		#else
		WRITE_FRC_REG_BY_CPU(rdma_regadr->rdma_ahb_start_addr,
			rdma_info->rdma_table_phy_addr & 0xffffffff);
		WRITE_FRC_REG_BY_CPU(rdma_regadr->rdma_ahb_end_addr,
			(rdma_info->rdma_table_phy_addr +
			rdma_info->rdma_item_count * 8 - 1) & 0xffffffff);
		#endif

		WRITE_FRC_BITS(rdma_regadr->addr_inc_reg, 0,
			rdma_regadr->addr_inc_reg_bitpos, 1);
		WRITE_FRC_BITS(rdma_regadr->rw_flag_reg, 1,   //1:write
			rdma_regadr->rw_flag_reg_bitpos, 1);

		WRITE_FRC_REG_BY_CPU(rdma_regadr->trigger_mask_reg,
			READ_FRC_REG(rdma_regadr->trigger_mask_reg) | 1);

		pr_frc(pr_flag, "config manual write done\n");
		// rdma_info->rdma_item_count = 0;
	} else {
		//auto RDMA
		WRITE_FRC_BITS(rdma_regadr->trigger_mask_reg, 0,
			rdma_regadr->trigger_mask_reg_bitpos, 1);

		#ifdef CONFIG_ARM64
		WRITE_FRC_REG_BY_CPU(rdma_regadr->rdma_ahb_start_addr,
			rdma_info->rdma_table_phy_addr & 0xffffffff);
		WRITE_FRC_REG_BY_CPU(rdma_regadr->rdma_ahb_start_addr_msb,
			(rdma_info->rdma_table_phy_addr >> 32) & 0xf);
		WRITE_FRC_REG_BY_CPU(rdma_regadr->rdma_ahb_end_addr,
			(rdma_info->rdma_table_phy_addr +
			rdma_info->rdma_item_count * 8 - 1) & 0xffffffff);
		WRITE_FRC_REG_BY_CPU(rdma_regadr->rdma_ahb_end_addr_msb,
			((rdma_info->rdma_table_phy_addr +
			rdma_info->rdma_item_count * 8 - 1) >> 32) & 0xf);
		#else
		WRITE_FRC_REG_BY_CPU(rdma_regadr->rdma_ahb_start_addr,
			rdma_info->rdma_table_phy_addr & 0xffffffff);
		WRITE_FRC_REG_BY_CPU(rdma_regadr->rdma_ahb_end_addr,
			(rdma_info->rdma_table_phy_addr +
			rdma_info->rdma_item_count * 8 - 1) & 0xffffffff);
		#endif

		WRITE_FRC_BITS(rdma_regadr->addr_inc_reg, 0,
			rdma_regadr->addr_inc_reg_bitpos, 1);
		WRITE_FRC_BITS(rdma_regadr->rw_flag_reg, 1,  // 1:write
			rdma_regadr->rw_flag_reg_bitpos, 1);

		WRITE_FRC_BITS(rdma_regadr->trigger_mask_reg, handle,
			rdma_regadr->trigger_mask_reg_bitpos, handle);

		pr_frc(pr_flag, "config auto done handle:%d count:%d\n",
			handle, rdma_info->rdma_item_count);
		rdma_info->rdma_item_count = 0;
	}

	return 0;
}

// static int frc_rdma_isr_count;
irqreturn_t frc_rdma_isr(int irq, void *dev_id)
{
	u32 i, rdma_status;
	struct rdma_regadr_s *info_regadr;
	struct frc_rdma_s *frc_rdma = get_frc_rdma();

	rdma_status = READ_FRC_REG(irq_status.reg);

	// if ((frc_dbg_en & 0x9) && ((frc_rdma_isr_count++ % 30) == 0))
	// pr_frc(0, "%s: %08x\r\n", __func__, rdma_status);
	pr_frc(9, "%s frc_rdma status[0x%x] rdma_cnt:%d\n",
		__func__, rdma_status, rdma_cnt);

	for (i = 0; i < RDMA_CHANNEL; i++) {
		info_regadr = (struct rdma_regadr_s *)frc_rdma->rdma_info[i]->rdma_regadr;
		if (rdma_status & (0x1 << (i + irq_status.start))) {
			WRITE_FRC_REG_BY_CPU(rdma_ctrl,
				0x1 << info_regadr->clear_irq_bitpos);
			pr_frc(9, "channel:%d rdma irq done\n", i);
			rdma_cnt = 0;
			break;
		}
	}
	// rdma_cnt++;
	return IRQ_HANDLED;
}

void frc_rdma_info_init(void)
{
	u8 i;

	for (i = 0; i < RDMA_CHANNEL; i++) {
		rdma_info[i].rdma_table_addr = NULL;
		rdma_info[i].rdma_table_size = FRC_RDMA_SIZE / 8;
		if (i > 2) // dbg, rdma read
			rdma_info[i].rdma_table_size = FRC_RDMA_SIZE / 16;
		rdma_info[i].buf_status = 0;
		rdma_info[i].is_64bit_addr = 0;
		rdma_info[i].rdma_item_count = 0;
		rdma_info[i].rdma_write_count = 0;
		rdma_info[i].rdma_regadr = NULL;
	}
}

void frc_rdma_rd_table_init(struct frc_dev_s *devp)
{
	int i;
	struct frc_fw_data_s *fw_data;
	struct frc_rdma_info *rdma_info;
	struct rdma_regadr_s *rdma_regadr;
	struct frc_rdma_s *frc_rdma = get_frc_rdma();

	if (!is_rdma_enable())
		return;

	fw_data = (struct frc_fw_data_s *)devp->fw_data;
	rdma_info = (struct frc_rdma_info *)frc_rdma->rdma_info[4]; // read
	rdma_regadr = (struct rdma_regadr_s *)rdma_info->rdma_regadr;
	rdma_info->rdma_item_count = 0;

	for (i = 0; i < RD_REG_MAX; i++) {
		if (fw_data->reg_val[i].addr) {
			rdma_info->rdma_table_addr[i] = fw_data->reg_val[i].addr;
			rdma_info->rdma_item_count++;
		}
	}

	WRITE_FRC_BITS(rdma_regadr->trigger_mask_reg, 0,
		rdma_regadr->trigger_mask_reg_bitpos, 1);

	#ifdef CONFIG_ARM64
	WRITE_FRC_REG_BY_CPU(rdma_regadr->rdma_ahb_start_addr,
		rdma_info->rdma_table_phy_addr & 0xffffffff);
	WRITE_FRC_REG_BY_CPU(rdma_regadr->rdma_ahb_start_addr_msb,
		(rdma_info->rdma_table_phy_addr >> 32) & 0xf);
	WRITE_FRC_REG_BY_CPU(rdma_regadr->rdma_ahb_end_addr,
		(rdma_info->rdma_table_phy_addr +
		rdma_info->rdma_item_count * 8 - 1) & 0xffffffff);
	WRITE_FRC_REG_BY_CPU(rdma_regadr->rdma_ahb_end_addr_msb,
		((rdma_info->rdma_table_phy_addr +
		rdma_info->rdma_item_count * 8 - 1) >> 32) & 0xf);
	#else
	WRITE_FRC_REG_BY_CPU(rdma_regadr->rdma_ahb_start_addr,
		rdma_info->rdma_table_phy_addr & 0xffffffff);
	WRITE_FRC_REG_BY_CPU(rdma_regadr->rdma_ahb_end_addr,
		(rdma_info->rdma_table_phy_addr +
		rdma_info->rdma_item_count * 8 - 1) & 0xffffffff);
	#endif

	WRITE_FRC_BITS(rdma_regadr->addr_inc_reg, 0,
		rdma_regadr->addr_inc_reg_bitpos, 1);
	WRITE_FRC_BITS(rdma_regadr->rw_flag_reg, 0,  // 0:read
		rdma_regadr->rw_flag_reg_bitpos, 1);

	WRITE_FRC_BITS(rdma_regadr->trigger_mask_reg, 0x1,
		rdma_regadr->trigger_mask_reg_bitpos, 0x1);  // inp isr
}

int frc_rdma_init(void)
{
	u32 i, data32;
	enum chip_id chip;
	struct frc_dev_s *devp;
	struct frc_fw_data_s *fw_data;
	struct frc_rdma_s *frc_rdma;

	devp = get_frc_devp();
	chip = get_chip_type();
	frc_rdma = get_frc_rdma();
	fw_data = (struct frc_fw_data_s *)devp->fw_data;

	frc_rdma_info_init();
	// init frc rdma
	for (i = 0; i < RDMA_CHANNEL; i++)
		frc_rdma->rdma_info[i] = &rdma_info[i];

	// alloc buf
	frc_rdma_alloc_buf(devp);

	if (frc_rdma->buf_alloced) {
		fw_data->frc_top_type.rdma_en = 0;  // alg init rdma off
		if ((chip == ID_T3 || chip == ID_T5M) && !frc_rdma->rdma_en)
			frc_rdma->rdma_en = 0;              // drv rdma off
		else if (chip == ID_T3X)
			frc_rdma->rdma_en = 1;              // drv rdma en
	} else {
		PR_ERR("alloc frc rdma buffer failed\n");
		return 0;
	}

	if (chip == ID_T3 || chip == ID_T5M) {
		frc_rdma->rdma_info[0]->rdma_regadr = &rdma_regadr_t3[0];  // man
		irq_status.reg = FRC_RDMA_STATUS;
		irq_status.start = 24;
		irq_status.len = 8;
		rdma_ctrl = FRC_RDMA_CTRL;
	} else {
		for (i = 0; i < RDMA_CHANNEL; i++)
			frc_rdma->rdma_info[i]->rdma_regadr = &rdma_regadr_t3x[i];
		irq_status.reg = FRC_RDMA_STATUS1_T3X;
		irq_status.start = 4;
		irq_status.len = 16;
		rdma_ctrl = FRC_RDMA_CTRL_T3X;
	}

	// rdma read init
	frc_rdma_rd_table_init(devp);

	data32  = 0;
	data32 |= 0 << 7; /* write ddr urgent */
	data32 |= 1 << 6; /* read ddr urgent */
	// data32 |= 0 << 4;
	// data32 |= 0 << 2;
	// data32 |= 0 << 1;
	// data32 |= 0 << 0;

	// init clk
	WRITE_FRC_BITS(FRC_RDMA_SYNC_CTRL, 1, 2, 1);
	WRITE_FRC_BITS(FRC_RDMA_SYNC_CTRL, 1, 6, 1);
	// rdma config
	WRITE_FRC_REG_BY_CPU(rdma_ctrl, data32);
	pr_frc(0, "%s frc rdma init done\n", __func__);
	return 1;
}

void frc_rdma_exit(void)
{
	struct frc_rdma_s *frc_rdma = get_frc_rdma();

	// close auto
	WRITE_FRC_BITS(FRC_RDMA_SYNC_CTRL, 0, 2, 1);
	WRITE_FRC_BITS(FRC_RDMA_SYNC_CTRL, 0, 6, 1);
	WRITE_FRC_REG_BY_CPU(FRC_RDMA_AUTO_SRC1_SEL_T3X, 0);
	WRITE_FRC_REG_BY_CPU(FRC_RDMA_AUTO_SRC2_SEL_T3X, 0);
	WRITE_FRC_REG_BY_CPU(rdma_ctrl, 0);
	frc_rdma->rdma_en = 0;
	frc_rdma_release_buf();

	pr_frc(0, "%s frc rdma exit\n", __func__);
}
