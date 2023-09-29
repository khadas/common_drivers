// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/clk.h>
#include <linux/of_device.h>
#include <linux/compat.h>
#include <linux/workqueue.h>
#include <linux/mm.h>
#include <linux/of_reserved_mem.h>
#include <linux/cma.h>
#include <linux/dma-mapping.h>
#include <linux/sched/clock.h>
#include <linux/amlogic/pm.h>
#include <linux/amlogic/media/vout/lcd/lcd_vout.h>
#include "ambilight.h"
#include "../lcd_reg.h"

unsigned int amblt_debug_print;

static void vpu_lut_dma_clr_fcnt(struct vpu_lut_dma_wr_s *lut_dma)
{
	lcd_vcbus_write(VPU_DMA_WRMIF_SEL, (0xff0 + lut_dma->wr_sel));
	lcd_vcbus_setb(VPU_DMA_WRMIF0_CTRL + lut_dma->offset, 1, 30, 1);
}

static void set_vpu_lut_dma_mif_wr(struct vpu_lut_dma_wr_s *lut_dma, int flag)
{
	unsigned int ofst = lut_dma->offset;

	if (flag) {
		//Bit 31    ro_lut_wr_hs_r    // unsigned ,    RO , default = 0
		//Bit 30:27 ro_lut_wr_id      // unsigned ,    RO , default = 0
		//Bit 26    pls_hold_clr      // unsigned ,    pls , default = 0
		//Bit 25    ro_hold_timeout   // unsigned ,    RO , default = 0
		//Bit 24:12 ro_wrmif_cnt      // unsigned ,    RO , default = 0
		//Bit 11:4  reg_hold_line     // unsigned ,    RW,  default = 8'hff
		//Bit 3     ro_lut_wr_hs_s    // unsigned ,    RO , default = 0
		//Bit 2     wrmif_hw_reset    // unsigned ,    RW , default = 0
		//Bit 1     lut_wr_cnt_sel    // unsigned ,    RW , default = 0
		//Bit 0     lut_wr_reg_sel    // unsigned ,    RW , default = 0,
				// 0: sel lut0,1,2,...,7 1:sel lut8,9,...,15
		lcd_vcbus_write(VPU_DMA_WRMIF_SEL, (0xff0 + lut_dma->wr_sel));
		lcd_vcbus_write(VPU_DMA_WRMIF0_CTRL + ofst,
					((lut_dma->addr_mode & 0x3) << 28) |
					((lut_dma->rpt_num & 0xff)  << 18) |
					(1 << 16) | //chn_enable
					(lut_dma->stride & 0x1fff));
		lcd_vcbus_write(VPU_DMA_WRMIF0_BADR0 + (ofst << 1), lut_dma->baddr0);
		lcd_vcbus_write(VPU_DMA_WRMIF0_BADR1 + (ofst << 1), lut_dma->baddr1);
	} else {
		lcd_vcbus_write(VPU_DMA_WRMIF_SEL, (0xff0 + lut_dma->wr_sel));
		lcd_vcbus_setb(VPU_DMA_WRMIF0_CTRL + ofst, 0, 16, 1); //chn_disable
	}
}

static void amblt_hw_ctrl(struct amblt_drv_s *amblt_drv)
{
	if (amblt_drv->en) {
		set_vpu_lut_dma_mif_wr(&amblt_drv->lut_dma, 1);

		lcd_vcbus_setb(LCD_OLED_SIZE, amblt_drv->hsize, 0, 13);  //hsize
		lcd_vcbus_setb(LCD_OLED_SIZE, amblt_drv->vsize, 16, 13);  //vsize
		lcd_vcbus_setb(LDC_REG_INPUT_STAT_NUM, amblt_drv->zone_h, 0, 6);  //x num
		lcd_vcbus_setb(LDC_REG_INPUT_STAT_NUM, amblt_drv->zone_v, 8, 5);  //y num
		lcd_vcbus_setb(LDC_REG_INPUT_STAT_NUM, 1, 15, 1);  //reverse vs pol
		lcd_vcbus_setb(LDC_REG_INPUT_STAT_NUM, 1, 21, 1);  //reverse vs pol
		lcd_vcbus_setb(LDC_REG_INPUT_STAT_NUM, 1, 16, 1); //en
		//AMBLTPR("LDC_REG_INPUT_STAT_NUM 0x%04x = 0x%08x\n",
		//	LDC_REG_INPUT_STAT_NUM, lcd_vcbus_read(LDC_REG_INPUT_STAT_NUM));
		amblt_drv->state |= AMBLT_STATE_EN;
		if (amblt_debug_print) {
			AMBLTPR("ambilight enable: zone x:%d, y:%d\n",
				amblt_drv->zone_h, amblt_drv->zone_v);
		}
	} else {
		amblt_drv->state &= ~AMBLT_STATE_EN;
		lcd_vcbus_setb(LDC_REG_INPUT_STAT_NUM, 0, 16, 1);
		set_vpu_lut_dma_mif_wr(&amblt_drv->lut_dma, 0);
		if (amblt_debug_print)
			AMBLTPR("ambilight disabled\n");
	}
}

int amblt_function_enable(struct amblt_drv_s *amblt_drv)
{
	struct amblt_dma_mem_s *dma_mem = &amblt_drv->dma_mem;

	if (dma_mem->flag == 0) {
		AMBLTERR("%s: dma memory failed\n", __func__);
		return -1;
	}
	if (amblt_drv->state & AMBLT_STATE_EN) {
		AMBLTPR("%s: already enabled\n", __func__);
		return 0;
	}

	amblt_drv->lut_dma.rpt_num = (amblt_drv->zone_size + 3) / 4;
	vpu_lut_dma_clr_fcnt(&amblt_drv->lut_dma);

	AMBLTPR("ambilight mem paddr: 0x%x, vaddr: 0x%px, size: 0x%x, zone_h=%d, zone_v=%d\n",
		(unsigned int)dma_mem->paddr, dma_mem->vaddr, dma_mem->size,
		amblt_drv->zone_h, amblt_drv->zone_v);
	memset(dma_mem->vaddr, 0, dma_mem->size);

	amblt_drv->en = 1;

	return 0;
}

int amblt_function_disable(struct amblt_drv_s *amblt_drv)
{
	if ((amblt_drv->state & AMBLT_STATE_EN) == 0) {
		AMBLTPR("%s: already disabled\n", __func__);
		return 0;
	}

	amblt_drv->en = 0;
	AMBLTPR("amblt function disable\n");

	return 0;
}

static void amblt_data_refersh(struct amblt_drv_s *amblt_drv)
{
	struct amblt_data_sum_s *sum, *p;
	int x, y, n;

	if (amblt_drv->dma_mem.flag == 0)
		return;
	if (!amblt_drv->buf)
		return;

	p = (struct amblt_data_sum_s *)amblt_drv->dma_mem.vaddr;
	for (y = 0; y < amblt_drv->zone_v; y++) {
		for (x = 0; x < amblt_drv->zone_h; x++) {
			n = y * amblt_drv->zone_h + x;
			sum = p + n;
			amblt_drv->buf[n].sum_r = sum->sum_r;
			amblt_drv->buf[n].sum_g = sum->sum_g;
			amblt_drv->buf[n].sum_b = sum->sum_b;
			amblt_drv->buf[n].avg_r = sum->sum_r / amblt_drv->zone_pixel;
			amblt_drv->buf[n].avg_g = sum->sum_g / amblt_drv->zone_pixel;
			amblt_drv->buf[n].avg_b = sum->sum_b / amblt_drv->zone_pixel;
			if (x != sum->x || y != sum->y)
				amblt_drv->buf[n].err = 1;
			else
				amblt_drv->buf[n].err = 0;
		}
	}
}

static irqreturn_t amblt_vsync_isr(int irq, void *data)
{
	struct amblt_drv_s *amblt_drv = (struct amblt_drv_s *)data;
	unsigned int state;

	if (!amblt_drv)
		return IRQ_HANDLED;

	state = amblt_drv->state & AMBLT_STATE_EN;
	if (amblt_drv->en != state) {
		amblt_hw_ctrl(amblt_drv);
		return IRQ_HANDLED;
	}

	if (state)
		amblt_data_refersh(amblt_drv);

	return IRQ_HANDLED;
}

static int amblt_vsync_irq_init(struct amblt_drv_s *amblt_drv)
{
	if (!amblt_drv->res_vs_irq)
		return -1;

	if (request_irq(amblt_drv->res_vs_irq->start,
			amblt_vsync_isr, IRQF_SHARED, "amblt_vs", (void *)amblt_drv)) {
		AMBLTERR("can't request amblt_vsync_isr\n");
		return -1;
	}

	return 0;
}

static void amblt_vsync_irq_remove(struct amblt_drv_s *amblt_drv)
{
	if (amblt_drv->res_vs_irq)
		free_irq(amblt_drv->res_vs_irq->start, (void *)amblt_drv);
}

static int amblt_get_config(struct amblt_drv_s *amblt_drv, struct platform_device *pdev)
{
	struct device_node *of_node;
	unsigned int para[2];
	int ret;

	of_node = pdev->dev.of_node;
	ret = of_property_read_u32_array(of_node, "zone_h_v", &para[0], 2);
	if (ret) {
		AMBLTERR("%s: get zone_h_v error\n", __func__);
		return -1;
	}
	amblt_drv->zone_h = para[0];
	amblt_drv->zone_v = para[1];
	amblt_drv->zone_size = para[0] * para[1];
	AMBLTPR("%s: zone h=%d, v=%d, size=%d\n",
		__func__, amblt_drv->zone_h, amblt_drv->zone_v, amblt_drv->zone_size);
	amblt_zone_pixel_init(amblt_drv);

	amblt_drv->res_vs_irq = platform_get_resource_byname(pdev, IORESOURCE_IRQ, "vsync");
	if (!amblt_drv->res_vs_irq) {
		AMBLTERR("%s: can't get vsync irq\n", __func__);
		return -1;
	}

	return 0;
}

void amblt_zone_pixel_init(struct amblt_drv_s *amblt_drv)
{
	struct aml_lcd_drv_s *pdrv = aml_lcd_get_driver(0);
	unsigned int h_active, v_active, h_pixel, v_pixel;

	if (!pdrv)
		return;
	h_active = pdrv->config.basic.h_active;
	v_active = pdrv->config.basic.v_active;

	amblt_drv->hsize = h_active;
	amblt_drv->vsize = v_active;
	h_pixel = h_active / amblt_drv->zone_h;
	v_pixel = v_active / amblt_drv->zone_v;
	amblt_drv->zone_pixel = h_pixel * v_pixel;
	AMBLTPR("%s: %d x %d / %d x %d: %d x %d = %d\n",
		__func__, h_active, v_active, amblt_drv->zone_h, amblt_drv->zone_v,
		h_pixel, v_pixel, amblt_drv->zone_pixel);

	kfree(amblt_drv->buf);
	amblt_drv->buf = kcalloc(amblt_drv->zone_size, sizeof(struct amblt_data_s), GFP_KERNEL);
	if (!amblt_drv->buf)
		AMBLTERR("%s: data buf is NULL\n", __func__);
}

static void amblt_lut_dma_init(struct amblt_drv_s *amblt_drv)
{
	struct vpu_lut_dma_wr_s *lut_dma = &amblt_drv->lut_dma;
	unsigned int rpt_num;

	rpt_num = (amblt_drv->zone_size + 3) / 4;

	lut_dma->dma_wr_id = AMBLT_DMA_ID;
	lut_dma->wr_sel = 1;
	lut_dma->offset = AMBLT_DMA_ID - 8;
	lut_dma->stride = 4;//0x280; //8;    //unit:128bit
	lut_dma->baddr0 = amblt_drv->dma_mem.paddr >> 4;
	lut_dma->baddr1 = amblt_drv->dma_mem.paddr >> 4;
	lut_dma->addr_mode = 1;//0; //1;
	lut_dma->rpt_num = rpt_num;

	lcd_vcbus_setb(VPU_DMA_WRMIF_CTRL, 1, 13, 1);//wr dma enable
}

static int amblt_dma_malloc(struct amblt_drv_s *amblt_drv, struct device *dev)
{
	struct amblt_dma_mem_s *dma_mem = &amblt_drv->dma_mem;
	unsigned int mem_size;

	mem_size = 32 * 20 * 16; //byte
	dma_mem->size = mem_size;
	dma_mem->vaddr = dma_alloc_coherent(dev, mem_size, &dma_mem->paddr, GFP_KERNEL);
	if (!dma_mem->vaddr) {
		AMBLTERR("%s: failed! dma_mem size: 0x%x\n", __func__, mem_size);
		return -1;
	}

	dma_mem->flag = 1;
	AMBLTPR("%s: dma paddr: 0x%lx, vaddr: 0x%px, size: 0x%x, flag: %d\n",
	       __func__, (unsigned long)dma_mem->paddr, dma_mem->vaddr,
	       dma_mem->size, dma_mem->flag);
	return 0;
}

/* ************************************************************* */
/* amblt driver data                                             */
/* ************************************************************* */
static struct amblt_drv_data_s amblt_data_t3x = {
	.chip_type = LCD_CHIP_T3X,
	.chip_name = "t3x",
	.zone_h_max = 32,
	.zone_v_max = 20,
	.zone_size_max = 640,
	.zone_size_min = 4,
};

static const struct of_device_id amblt_dt_match_table[] = {
	{
		.compatible = "amlogic, ambilight-t3x",
		.data = &amblt_data_t3x,
	},
	{}
};

static int amblt_probe(struct platform_device *pdev)
{
	struct amblt_drv_s *amblt_drv;
	const struct of_device_id *match;
	struct amblt_drv_data_s *amblt_data;
	int ret = 0;

	match = of_match_device(amblt_dt_match_table, &pdev->dev);
	if (!match) {
		AMBLTERR("%s: no match table\n", __func__);
		goto amblt_probe_err_0;
	}
	amblt_data = (struct amblt_drv_data_s *)match->data;
	AMBLTPR("driver version: %s(%d-%s)\n",
	      AMBLT_DRV_VER,
	      amblt_data->chip_type,
	      amblt_data->chip_name);

	amblt_drv = kzalloc(sizeof(*amblt_drv), GFP_KERNEL);
	if (!amblt_drv)
		goto amblt_probe_err_0;

	amblt_drv->dev = &pdev->dev;
	amblt_drv->drv_data = amblt_data;
	platform_set_drvdata(pdev, amblt_drv);

	ret = amblt_get_config(amblt_drv, pdev);
	if (ret)
		goto amblt_probe_err_1;

	ret = amblt_dma_malloc(amblt_drv, &pdev->dev);
	if (ret)
		goto amblt_probe_err_1;
	amblt_lut_dma_init(amblt_drv);
	spin_lock_init(&amblt_drv->isr_lock);
	mutex_init(&amblt_drv->power_mutex);

	amblt_debug_file_add(amblt_drv);
	ret = amblt_vsync_irq_init(amblt_drv);
	if (ret)
		goto amblt_probe_err_2;

	AMBLTPR("driver probe ok\n");

	return 0;

amblt_probe_err_2:
	amblt_debug_file_remove(amblt_drv);
amblt_probe_err_1:
	/* free drvdata */
	platform_set_drvdata(pdev, NULL);
	/* free drv */
	kfree(amblt_drv);
amblt_probe_err_0:
	AMBLTERR("driver probe failed\n");
	return -1;
}

static int amblt_remove(struct platform_device *pdev)
{
	struct amblt_drv_s *amblt_drv = platform_get_drvdata(pdev);

	if (!amblt_drv)
		return 0;

	amblt_vsync_irq_remove(amblt_drv);
	amblt_debug_file_remove(amblt_drv);

	/* free drvdata */
	platform_set_drvdata(pdev, NULL);

	kfree(amblt_drv);

	return 0;
}

static int amblt_resume(struct platform_device *pdev)
{
	struct amblt_drv_s *amblt_drv = platform_get_drvdata(pdev);

	if (!amblt_drv)
		return 0;

	mutex_lock(&amblt_drv->power_mutex);
	AMBLTPR("resume done\n");
	mutex_unlock(&amblt_drv->power_mutex);

	return 0;
}

static int amblt_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct amblt_drv_s *amblt_drv = platform_get_drvdata(pdev);

	if (!amblt_drv)
		return 0;

	mutex_lock(&amblt_drv->power_mutex);
	AMBLTPR("suspend done\n");
	mutex_unlock(&amblt_drv->power_mutex);
	return 0;
}

static void amblt_shutdown(struct platform_device *pdev)
{
	struct amblt_drv_s *amblt_drv = platform_get_drvdata(pdev);

	if (!amblt_drv)
		return;

	AMBLTPR("shutdown done\n");
}

static struct platform_driver ambilight_platform_driver = {
	.probe = amblt_probe,
	.remove = amblt_remove,
	.suspend = amblt_suspend,
	.resume = amblt_resume,
	.shutdown = amblt_shutdown,
	.driver = {
		.name = "ambilight",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(amblt_dt_match_table),
#endif
	},
};

int __init ambilight_init(void)
{
	if (platform_driver_register(&ambilight_platform_driver)) {
		AMBLTERR("failed to register ambilight driver module\n");
		return -ENODEV;
	}

	return 0;
}

void __exit ambilight_exit(void)
{
	platform_driver_unregister(&ambilight_platform_driver);
}
