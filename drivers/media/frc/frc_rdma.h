/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#include <linux/amlogic/media/frc/frc_reg.h>

#define FRC_RDMA_VER			"2024-0110 frc rdma version 1.1.1.011024_beta"

#define RDMA_NUM        8
#define RDMA_CHANNEL    4
#define RDMA_NUM_T3X    16
#define REG_TEST_NUM    16

#define MAX_TRACE_NUM   16

#ifndef PAGE_SIZE
# define PAGE_SIZE 4096
#endif

struct rdma_op_s {
	void (*irq_cb)(void *arg);
	void *arg;
};

/*RDMA total memory size is 1M*/
#define FRC_RDMA_SIZE (256 * (PAGE_SIZE))

struct reg_test {
	u32 addr;
	u32 value;
};

struct frc_rdma_irq_reg_s {
	u32 reg;
	u32 start;
	u32 len;
};

struct frc_rdma_info {
	ulong rdma_table_phy_addr;
	u32 *rdma_table_addr;
	int rdma_table_size;
	u8 buf_status;
	u8 is_64bit_addr;
	int rdma_item_count;
	int rdma_write_count;
	struct rdma_regadr_s *rdma_regadr;
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

struct frc_rdma_s {
	u8 rdma_en;
	u8 rdma_alg_en;
	u8 buf_alloced;
	u8 reserved;

	struct frc_rdma_info *rdma_info_dbg;
	struct frc_rdma_info *rdma_info[RDMA_CHANNEL];
};

extern u32 rdma_trace_num;
extern u32 rdma_trace_reg[MAX_TRACE_NUM];

void frc_rdma_alloc_buf(struct frc_dev_s *devp);
void frc_rdma_release_buf(void);
void frc_read_table(struct frc_rdma_s *frc_rdma);
void frc_rdma_status(void);
void frc_rdma_table_config(u32 addr, u32 val);
int frc_rdma_config(int handle, u32 trigger_type);
int frc_rdma_init(void);
void frc_rdma_exit(void);
struct frc_rdma_s *get_frc_rdma(void);
int frc_auto_test(int val, int val2);
int is_rdma_enable(void);
void frc_rdma_reg_list(void);

ssize_t frc_rdma_trace_enable_show(struct class *cla,
	struct class_attribute *attr, char *buf);
ssize_t frc_rdma_trace_enable_stroe(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count);
ssize_t frc_rdma_trace_reg_show(struct class *cla,
	struct class_attribute *attr, char *buf);
ssize_t frc_rdma_trace_reg_stroe(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count);

irqreturn_t frc_rdma_isr(int irq, void *dev_id);
int FRC_RDMA_VSYNC_WR_REG(u32 addr, u32 val);
int FRC_RDMA_VSYNC_WR_BITS(u32 addr, u32 val, u32 start, u32 len);
int FRC_RDMA_VSYNC_REG_UPDATE(u32 addr, u32 val, u32 mask);
int _frc_rdma_wr_reg_in(u32 addr, u32 val);
int _frc_rdma_wr_reg_out(u32 addr, u32 val);
int _frc_rdma_rd_reg_man(u32 addr);

