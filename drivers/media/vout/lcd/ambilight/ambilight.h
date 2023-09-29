/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _AMBILIGHT_H_
#define _AMBILIGHT_H_
#include <linux/amlogic/media/vout/lcd/lcd_vout.h>

//20230613, init version
#define AMBLT_DRV_VER        "20230613"

#define AMBLTPR(fmt, args...)     pr_info("ambilight: " fmt "", ## args)
#define AMBLTERR(fmt, args...)     pr_err("error: ambilight: " fmt "", ## args)

#define AMBLT_CLASS_NAME     "amblt"
#define AMBLT_DEVICE_NAME    "amblt"

enum LUT_DMA_WR_ID_e {
	LC_STTS_0_DMA_ID      = 0,
	LC_STTS_1_DMA_ID      = 1,
	VI_HIST_SPL_0_DMA_ID  = 2,
	VI_HIST_SPL_1_DMA_ID  = 3,
	CM2_HIST_0_DMA_ID     = 4,
	CM2_HIST_1_DMA_ID     = 5,
	VD1_HDR_0_DMA_ID      = 6,
	VD1_HDR_1_DMA_ID      = 7,
	VD2_HDR_DMA_ID        = 8,
	AMBLT_DMA_ID          = 9,
	LUT_WR_MAX_ID         = 10
};

struct vpu_lut_dma_wr_s {
	enum LUT_DMA_WR_ID_e dma_wr_id;
	unsigned int wr_sel;
	unsigned int offset;
	unsigned int stride;
	unsigned int baddr0;
	unsigned int baddr1;
	unsigned int addr_mode;
	unsigned int rpt_num;
};

struct amblt_dma_mem_s {
	unsigned char flag;
	void *vaddr;
	phys_addr_t paddr;
	unsigned int size;
};

struct amblt_data_sum_s {
	unsigned int sum_b;
	unsigned int sum_g;
	unsigned int sum_r;
	unsigned short x;
	unsigned short y;
};

struct amblt_data_s {
	unsigned int sum_r;
	unsigned int sum_g;
	unsigned int sum_b;
	unsigned short avg_r;
	unsigned short avg_g;
	unsigned short avg_b;
	unsigned char  err;
};

struct amblt_drv_data_s {
	enum lcd_chip_e chip_type;
	const char *chip_name;
	unsigned int zone_h_max;
	unsigned int zone_v_max;
	unsigned int zone_size_max;
	unsigned int zone_size_min;
};

#define AMBLT_STATE_EN        BIT(0)

struct amblt_drv_s {
	unsigned int state;
	unsigned int en;
	unsigned int hsize;
	unsigned int vsize;
	unsigned int zone_h;
	unsigned int zone_v;
	unsigned int zone_size;
	unsigned int zone_pixel;
	unsigned int frm_idex;
	unsigned int dbg_level;

	struct amblt_drv_data_s *drv_data;
	struct amblt_dma_mem_s dma_mem;
	struct vpu_lut_dma_wr_s lut_dma;
	struct amblt_data_s *buf;

	dev_t devno;
	struct cdev cdev;
	struct device *dev;
	struct class *clsp;
	struct resource *res_vs_irq;
	spinlock_t isr_lock; //for vsync ir
	struct mutex power_mutex; //for on/off sequence
};

extern unsigned int amblt_debug_print;

void amblt_zone_pixel_init(struct amblt_drv_s *amblt_drv);
int amblt_debug_file_add(struct amblt_drv_s *amblt_drv);
int amblt_debug_file_remove(struct amblt_drv_s *amblt_drv);
int amblt_function_enable(struct amblt_drv_s *amblt_drv);
int amblt_function_disable(struct amblt_drv_s *amblt_drv);

/********************ioctl************************/
#define AMBLT_IOC_TYPE               'B'
#define AMBLT_IOC_EN_CTRL             0x1
#define AMBLT_IOC_GET_ZONE_SIZE       0x2
#define AMBLT_IOC_GET_DATA            0x3

#define AMBLT_IOC_CMD_POWER_CTRL   \
	_IOW(AMBLT_IOC_TYPE, AMBLT_IOC_EN_CTRL, unsigned int)
#define AMBLT_IOC_CMD_GET_SS   \
	_IOR(AMBLT_IOC_TYPE, AMBLT_IOC_GET_ZONE_SIZE, unsigned int)

/********************regs************************/
#define LCD_OLED_SIZE                              0x14ec
#define LDC_REG_INPUT_STAT_NUM                     0x14ef

#define VPU_DMA_WRMIF_CTRL                         0x27d5
#define VPU_DMA_WRMIF0_CTRL                        0x27d6
#define VPU_DMA_WRMIF1_CTRL                        0x27d7
#define VPU_DMA_WRMIF2_CTRL                        0x27d8
#define VPU_DMA_WRMIF3_CTRL                        0x27d9
#define VPU_DMA_WRMIF4_CTRL                        0x27da
#define VPU_DMA_WRMIF0_BADR0                       0x27de
#define VPU_DMA_WRMIF0_BADR1                       0x27df
#define VPU_DMA_WRMIF1_BADR0                       0x27e0
#define VPU_DMA_WRMIF1_BADR1                       0x27e1
#define VPU_DMA_WRMIF2_BADR0                       0x27e2
#define VPU_DMA_WRMIF2_BADR1                       0x27e3
#define VPU_DMA_WRMIF3_BADR0                       0x27e4
#define VPU_DMA_WRMIF3_BADR1                       0x27e5
#define VPU_DMA_WRMIF4_BADR0                       0x27e6
#define VPU_DMA_WRMIF4_BADR1                       0x27e7
#define VPU_DMA_WRMIF_SEL                          0x27ee

#endif
