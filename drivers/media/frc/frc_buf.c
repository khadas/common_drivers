// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_irq.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/ctype.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/highmem.h>
#include <linux/dma-mapping.h>
#include <linux/dma-map-ops.h>
#include <linux/page-flags.h>
#include <linux/amlogic/media/frc/frc_reg.h>
#include <linux/amlogic/media/frc/frc_common.h>
#include <linux/sched/clock.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/amlogic/media/codec_mm/codec_mm.h>

#include "frc_drv.h"
#include "frc_buf.h"
#include "frc_rdma.h"
#include "frc_hw.h"

void frc_dump_mm_data(void *addr, u32 size)
{
	u32 *c = addr;
	ulong start;
	ulong end;

	start = (ulong)addr;
	end = start + size;

	//pr_info("addr:0x%lx size:0x%x\n", (ulong)addr);
	for (start = (ulong)addr; start < end; start += 64) {
		pr_info("\tvaddr(0x%lx): %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x\n",
		(ulong)c, c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7],
		c[8], c[9], c[10], c[11], c[12], c[13], c[14], c[15]);
		c += 16;
	}
}

/*
 * frc map p to v memory address
 */
u8 *frc_buf_vmap(ulong addr, u32 size)
{
	u8 *vaddr = NULL;
	struct page **pages = NULL;
	u32 i, npages, offset = 0;
	ulong phys, page_start;
	pgprot_t pgprot = PAGE_KERNEL;

	if (!PageHighMem(phys_to_page(addr))) {
		vaddr = phys_to_virt(addr);
		//pr_frc(1, "low mem map to 0x%lx\n", (ulong)vaddr);
		return vaddr;
	}

	offset = offset_in_page(addr);
	page_start = addr - offset;
	npages = DIV_ROUND_UP(size + offset, PAGE_SIZE);

	pages = kmalloc_array(npages, sizeof(struct page *), GFP_KERNEL);
	if (!pages)
		return NULL;

	for (i = 0; i < npages; i++) {
		phys = page_start + i * PAGE_SIZE;
		pages[i] = pfn_to_page(phys >> PAGE_SHIFT);
	}

	vaddr = vmap(pages, npages, VM_MAP, pgprot);
	if (!vaddr) {
		pr_frc(0, "the phy(%lx) vmaped fail, size: %d\n",
		       (ulong)page_start, npages << PAGE_SHIFT);
		kfree(pages);
		return NULL;
	}

	kfree(pages);

	pr_frc(1, "[MEM-MAP] %s, pa(%lx) to va(%lx), size: %d\n",
	       __func__, (ulong)page_start, (ulong)vaddr, npages << PAGE_SHIFT);

	return vaddr + offset;
}

void frc_buf_unmap(u32 *vaddr)
{
	void *addr = (void *)(PAGE_MASK & (ulong)vaddr);

	if (is_vmalloc_or_module_addr(vaddr)) {
		pr_frc(0, "unmap v: %px\n", addr);
		vunmap(addr);
	}
}

void frc_buf_dma_flush(struct frc_dev_s *devp, void *vaddr, ulong phy_addr, int size,
					enum dma_data_direction dir)
{
	//ulong phy_addr;

	//if (is_vmalloc_or_module_addr(vaddr)) {
	//	phy_addr = page_to_phys(vmalloc_to_page(vaddr)) + offset_in_page(vaddr);
	//	if (phy_addr && PageHighMem(phys_to_page(phy_addr))) {
	//		pr_frc(0, "flush v: %lx, p: %lx\n", vaddr, phy_addr);
	//		dma_sync_single_for_device(&devp->pdev->dev, phy_addr, size, dir);
	//	}
	//	return;
	//}

	dma_sync_single_for_device(&devp->pdev->dev, phy_addr, size, DMA_FROM_DEVICE);
}

/*
 * frc dump all memory address
 */
void frc_buf_dump_memory_addr_info(struct frc_dev_s *devp)
{
	u32 i;
	ulong base;
	enum chip_id chip;

	chip = get_chip_type();

	base = devp->buf.cma_mem_paddr_start;
	/*info buffer*/
	pr_frc(0, "lossy_mc_y_info_buf_paddr:0x%lx size:0x%x\n",
		base + devp->buf.lossy_mc_y_info_buf_paddr,
		devp->buf.lossy_mc_y_info_buf_size);
	pr_frc(0, "lossy_mc_c_info_buf_paddr:0x%lx size:0x%x\n",
		base + devp->buf.lossy_mc_c_info_buf_paddr,
		devp->buf.lossy_mc_c_info_buf_size);
//	pr_frc(0, "lossy_mc_v_info_buf_paddr:0x%lx size:0x%x\n",
//		base + devp->buf.lossy_mc_v_info_buf_paddr,
//		devp->buf.lossy_mc_v_info_buf_size);
	pr_frc(0, "lossy_me_x_info_buf_paddr:0x%lx size:0x%x\n",
		base + devp->buf.lossy_me_x_info_buf_paddr,
		devp->buf.lossy_me_x_info_buf_size);
	/*t3x mcdw info buffer*/
	if (chip == ID_T3X) {
		pr_frc(0, "lossy_mcdw_y_info_buf_paddr:0x%lx size:0x%x\n",
			base + devp->buf.lossy_mcdw_y_info_buf_paddr,
			devp->buf.lossy_mcdw_y_info_buf_size);
		pr_frc(0, "lossy_mcdw_c_info_buf_paddr:0x%lx size:0x%x\n",
			base + devp->buf.lossy_mcdw_c_info_buf_paddr,
			devp->buf.lossy_mcdw_c_info_buf_size);
	}

	/*lossy data buffer*/
	for (i = 0; i < FRC_TOTAL_BUF_NUM; i++)
		pr_frc(0, "lossy_mc_y_data_buf_paddr[%d]:0x%lx size:0x%x\n", i,
		       base + devp->buf.lossy_mc_y_data_buf_paddr[i],
		       devp->buf.lossy_mc_y_data_buf_size[i]);

	for (i = 0; i < FRC_TOTAL_BUF_NUM; i++)
		pr_frc(0, "lossy_mc_c_data_buf_paddr[%d]:0x%lx size:0x%x\n", i,
		       base + devp->buf.lossy_mc_c_data_buf_paddr[i],
		       devp->buf.lossy_mc_c_data_buf_size[i]);

//	for (i = 0; i < FRC_TOTAL_BUF_NUM; i++)
//		pr_frc(0, "lossy_mc_v_data_buf_paddr[%d]:0x%lx size:0x%x\n", i,
//		       base + devp->buf.lossy_mc_v_data_buf_paddr[i],
//		       devp->buf.lossy_mc_v_data_buf_size[i]);

	for (i = 0; i < FRC_TOTAL_BUF_NUM; i++)
		pr_frc(0, "lossy_me_data_buf_paddr[%d]:0x%lx size:0x%x\n", i,
		       base + devp->buf.lossy_me_data_buf_paddr[i],
		       devp->buf.lossy_me_data_buf_size[i]);

	/*lossy mcdw data buffer*/
	if (chip == ID_T3X) {
		for (i = 0; i < FRC_TOTAL_BUF_NUM; i++)
			pr_frc(0, "lossy_mcdw_y_data_buf_paddr[%d]:0x%lx size:0x%x\n", i,
				base + devp->buf.lossy_mcdw_y_data_buf_paddr[i],
				devp->buf.lossy_mcdw_y_data_buf_size[i]);

		for (i = 0; i < FRC_TOTAL_BUF_NUM; i++)
			pr_frc(0, "lossy_mcdw_c_data_buf_paddr[%d]:0x%lx size:0x%x\n", i,
				base + devp->buf.lossy_mcdw_c_data_buf_paddr[i],
				devp->buf.lossy_mcdw_c_data_buf_size[i]);
	}

	/*link buffer*/
	for (i = 0; i < FRC_TOTAL_BUF_NUM; i++)
		pr_frc(0, "lossy_mc_y_link_buf_paddr[%d]:0x%lx size:0x%x\n", i,
		       base + devp->buf.lossy_mc_y_link_buf_paddr[i],
		       devp->buf.lossy_mc_y_link_buf_size[i]);

	for (i = 0; i < FRC_TOTAL_BUF_NUM; i++)
		pr_frc(0, "lossy_mc_c_link_buf_paddr[%d]:0x%lx size:0x%x\n", i,
		       base + devp->buf.lossy_mc_c_link_buf_paddr[i],
		       devp->buf.lossy_mc_c_link_buf_size[i]);

//	for (i = 0; i < FRC_TOTAL_BUF_NUM; i++)
//		pr_frc(0, "lossy_mc_v_link_buf_paddr[%d]:0x%lx size:0x%x\n", i,
//		       base + devp->buf.lossy_mc_v_link_buf_paddr[i],
//		       devp->buf.lossy_mc_v_link_buf_size[i]);

	for (i = 0; i < FRC_TOTAL_BUF_NUM; i++)
		pr_frc(0, "lossy_me_link_buf_paddr[%d]:0x%lx size:0x%x\n", i,
		       base + devp->buf.lossy_me_link_buf_paddr[i],
		       devp->buf.lossy_me_link_buf_size[i]);
	/*t3x mcdw link buffer*/
	if (chip == ID_T3X) {
		for (i = 0; i < FRC_TOTAL_BUF_NUM; i++)
			pr_frc(0, "lossy_mcdw_y_link_buf_paddr[%d]:0x%lx size:0x%x\n", i,
				base + devp->buf.lossy_mcdw_y_link_buf_paddr[i],
				devp->buf.lossy_mcdw_y_link_buf_size[i]);

		for (i = 0; i < FRC_TOTAL_BUF_NUM; i++)
			pr_frc(0, "lossy_mcdw_c_link_buf_paddr[%d]:0x%lx size:0x%x\n", i,
				base + devp->buf.lossy_mcdw_c_link_buf_paddr[i],
				devp->buf.lossy_mcdw_c_link_buf_size[i]);
	}

	/*norm buffer*/
	if (chip == ID_T3) {
		for (i = 0; i < FRC_TOTAL_BUF_NUM; i++)
			pr_frc(0, "norm_hme_data_buf_paddr[%d]:0x%lx size:0x%x\n", i,
				base + devp->buf.norm_hme_data_buf_paddr[i],
				devp->buf.norm_hme_data_buf_size[i]);
	}

	for (i = 0; i < FRC_MEMV_BUF_NUM; i++)
		pr_frc(0, "norm_memv_buf_paddr[%d]:0x%lx size:0x%x\n", i,
		       base + devp->buf.norm_memv_buf_paddr[i],
		       devp->buf.norm_memv_buf_size[i]);

	if (chip == ID_T3) {
		for (i = 0; i < FRC_MEMV2_BUF_NUM; i++)
			pr_frc(0, "norm_hmemv_buf_paddr[%d]:0x%lx size:0x%x\n", i,
				base + devp->buf.norm_hmemv_buf_paddr[i],
				devp->buf.norm_hmemv_buf_size[i]);
	}

	for (i = 0; i < FRC_MEVP_BUF_NUM; i++)
		pr_frc(0, "norm_mevp_out_buf_paddr[%d]:0x%lx size:0x%x\n", i,
		       base + devp->buf.norm_mevp_out_buf_paddr[i],
		       devp->buf.norm_mevp_out_buf_size[i]);

	for (i = 0; i < FRC_TOTAL_BUF_NUM; i++)
		pr_frc(0, "norm_iplogo_buf_paddr[%d]:0x%lx size:0x%x\n", i,
		       base + devp->buf.norm_iplogo_buf_paddr[i],
		       devp->buf.norm_iplogo_buf_size[i]);

	pr_frc(0, "norm_logo_irr_buf_paddr:0x%lx size:0x%x\n",
		base + devp->buf.norm_logo_irr_buf_paddr, devp->buf.norm_logo_irr_buf_size);
	pr_frc(0, "norm_logo_scc_buf_paddr:0x%lx size:0x%x\n",
		base + devp->buf.norm_logo_scc_buf_paddr, devp->buf.norm_logo_scc_buf_size);

	for (i = 0; i < FRC_TOTAL_BUF_NUM; i++)
		pr_frc(0, "norm_melogo_buf_paddr[%d]:0x%lx size:0x%x\n", i,
		       base + devp->buf.norm_melogo_buf_paddr[i],
		       devp->buf.norm_melogo_buf_size[i]);

	pr_frc(0, "total_size:0x%x\n", devp->buf.total_size);
	pr_frc(0, "real_total_size:0x%x\n", devp->buf.real_total_size);
	pr_frc(0, "cma_mem_alloced:0x%x\n", devp->buf.cma_mem_alloced);
	pr_frc(0, "cma_mem_paddr_start:0x%lx\n", (ulong)devp->buf.cma_mem_paddr_start);
}

void frc_buf_dump_memory_size_info(struct frc_dev_s *devp)
{
	u32 log = 0;
	u32 temp;
	u8 frm_buf_num, logo_buf_num;
	enum chip_id chip;

	chip = get_chip_type();

	pr_frc(log, "in hv (%d, %d) align (%d, %d)",
			devp->buf.in_hsize, devp->buf.in_vsize,
			devp->buf.in_align_hsize, devp->buf.in_align_vsize);
	pr_frc(log, "me hv (%d, %d)", devp->buf.me_hsize, devp->buf.me_vsize);
	pr_frc(log, "logo hv (%d, %d)", devp->buf.logo_hsize, devp->buf.logo_vsize);
	pr_frc(log, "hme hv (%d, %d)", devp->buf.hme_hsize, devp->buf.hme_vsize);
	pr_frc(log, "me blk hv (%d, %d)", devp->buf.me_blk_hsize,
			devp->buf.me_blk_vsize);
	pr_frc(log, "hme blk (%d, %d)", devp->buf.hme_blk_hsize,
			devp->buf.hme_blk_vsize);

	if (devp->clk_state == FRC_CLOCK_OFF) {
		frm_buf_num = devp->buf.frm_buf_num;
		logo_buf_num = devp->buf.logo_buf_num;
	} else {
		temp = READ_FRC_REG(FRC_FRAME_BUFFER_NUM);
		frm_buf_num = temp & 0x1F;
		logo_buf_num = (temp >> 8) & 0x1F;
		if (frm_buf_num != devp->buf.frm_buf_num ||
			logo_buf_num != devp->buf.logo_buf_num)
			pr_frc(log, "buf num is not match (frm:(%d,%d) logo:(%d,%d)",
						frm_buf_num, logo_buf_num,
						devp->buf.frm_buf_num,
						devp->buf.logo_buf_num);
		devp->buf.frm_buf_num = frm_buf_num;
		devp->buf.logo_buf_num = logo_buf_num;
	}
	pr_frc(log, "buf num (frm:%d, logo:%d)", frm_buf_num, logo_buf_num);

	/*mc info buffer*/
	pr_frc(log, "lossy_mc_info_buf_size=%d, line buf:%d all(y+c):%d\n",
	       devp->buf.lossy_mc_y_info_buf_size, LOSSY_MC_INFO_LINE_SIZE,
	       devp->buf.lossy_mc_y_info_buf_size * 2);
	/*me info buffer*/
	pr_frc(log, "lossy_me_info_buf_size=%d, line buf:%d all:%d\n",
	       devp->buf.lossy_me_x_info_buf_size, LOSSY_MC_INFO_LINE_SIZE,
	       devp->buf.lossy_me_x_info_buf_size * 1);
	/*mcdw info buffer*/
	if (chip == ID_T3X) {
		pr_frc(log, "lossy_mcdw_info_buf_size=%d, line buf:%d all(y+c):%d\n",
			devp->buf.lossy_mcdw_y_info_buf_size, LOSSY_MC_INFO_LINE_SIZE,
			devp->buf.lossy_mcdw_y_info_buf_size * 2);
	}

	/*lossy mc data buffer*/
	pr_frc(log, "lossy_mc_y_data_buf_size=%d, all:%d\n",
	       devp->buf.lossy_mc_y_data_buf_size[0],
	       devp->buf.lossy_mc_y_data_buf_size[0] * frm_buf_num);
	pr_frc(log, "lossy_mc_c_data_buf_size=%d, all:%d\n",
	       devp->buf.lossy_mc_c_data_buf_size[0],
	       devp->buf.lossy_mc_c_data_buf_size[0] * frm_buf_num);

	/*lossy me data buffer*/
	pr_frc(log, "lossy_me_data_buf_size=%d, all:%d\n",
	       devp->buf.lossy_me_data_buf_size[0],
	       devp->buf.lossy_me_data_buf_size[0] * frm_buf_num);

	/*lossy mcdw data buffer*/
	if (chip == ID_T3X) {
		pr_frc(log, "lossy_mcdw_y_data_buf_size=%d, all:%d\n",
			devp->buf.lossy_mcdw_y_data_buf_size[0],
			devp->buf.lossy_mcdw_y_data_buf_size[0] * frm_buf_num);
		pr_frc(log, "lossy_mcdw_c_data_buf_size=%d, all:%d\n",
			devp->buf.lossy_mcdw_c_data_buf_size[0],
			devp->buf.lossy_mcdw_c_data_buf_size[0] * frm_buf_num);
	}

	/*mc y link buffer*/
	pr_frc(log, "lossy_mc_y_link_buf_size=%d, all:%d\n",
	       devp->buf.lossy_mc_y_link_buf_size[0],
	       devp->buf.lossy_mc_y_link_buf_size[0] * frm_buf_num);

	/*mc c link buffer*/
	pr_frc(log, "lossy_mc_c_link_buf_size=%d, all:%d\n",
	       devp->buf.lossy_mc_c_link_buf_size[0],
	       devp->buf.lossy_mc_c_link_buf_size[0] * frm_buf_num);
	/*me link buffer*/
	pr_frc(log, "lossy_me_link_buf_size=%d , all:%d\n",
	       devp->buf.lossy_me_link_buf_size[0],
	       devp->buf.lossy_me_link_buf_size[0] * frm_buf_num);

	/*mcdw link buffer*/
	if (chip == ID_T3X) {
		pr_frc(log, "lossy_mcdw_y_link_buf_size=%d, all:%d\n",
			devp->buf.lossy_mcdw_y_link_buf_size[0],
			devp->buf.lossy_mcdw_y_link_buf_size[0] * frm_buf_num);
		pr_frc(log, "lossy_mcdw_c_link_buf_size=%d, all:%d\n",
			devp->buf.lossy_mcdw_c_link_buf_size[0],
			devp->buf.lossy_mcdw_c_link_buf_size[0] * frm_buf_num);
	}

	/*norm hme data buffer*/
	if (chip == ID_T3) {
		pr_frc(log, "norm_hme_data_buffer=%d , all:%d\n",
			devp->buf.norm_hme_data_buf_size[0],
			devp->buf.norm_hme_data_buf_size[0] * frm_buf_num);
	}

	/*norm memv buffer*/
	pr_frc(log, "norm_memv_buf_size=%d , all:%d\n",
	       devp->buf.norm_memv_buf_size[0],
	       devp->buf.norm_memv_buf_size[0] * FRC_MEMV_BUF_NUM);

	/*norm hmemv buffer*/
	if (chip == ID_T3) {
		pr_frc(log, "norm_hmemv_buf_size=%d , all:%d\n",
			devp->buf.norm_hmemv_buf_size[0],
			devp->buf.norm_hmemv_buf_size[0] * FRC_MEMV2_BUF_NUM);
	}

	/*norm mevp buffer*/
	pr_frc(log, "norm_mevp_out_buf_size=%d , all:%d\n",
	       devp->buf.norm_mevp_out_buf_size[0],
	       devp->buf.norm_mevp_out_buf_size[0] * FRC_MEVP_BUF_NUM);

	/*norm iplogo buffer*/
	pr_frc(log, "norm_iplogo_buf_size=%d , all:%d\n",
	       devp->buf.norm_iplogo_buf_size[0],
	       devp->buf.norm_iplogo_buf_size[0] * logo_buf_num);

	/*norm logo irr buffer*/
	pr_frc(log, "norm_logo_irr_buf_size=%d\n",
	       devp->buf.norm_logo_irr_buf_size);

	/*norm logo scc buffer*/
	pr_frc(log, "norm_logo_scc_buf_size=%d\n",
	       devp->buf.norm_logo_scc_buf_size);

	/*norm iplogo buffer*/
	pr_frc(log, "norm_melogo_buf_size=%d , all:%d\n",
	       devp->buf.norm_melogo_buf_size[0],
	       devp->buf.norm_melogo_buf_size[0] * logo_buf_num);

	pr_frc(0, "total_size=%d\n", devp->buf.total_size);
}

void frc_buf_dump_link_tab(struct frc_dev_s *devp, u32 mode)
{
	u32 *vaddr;
	u32 *vaddr_start;
	phys_addr_t cma_addr = 0;
	u32 map_size = 0;
	u32 i;

	pr_frc(0, "%s md:%d\n", __func__, mode);
	if (mode == FRC_BUF_MC_Y_IDX) {
		for (i = 0; i < FRC_TOTAL_BUF_NUM; i++) {
			cma_addr =
			(devp->buf.cma_mem_paddr_start + devp->buf.lossy_mc_y_link_buf_paddr[i]);
			map_size = roundup(devp->buf.lossy_mc_y_link_buf_size[i], ALIGN_64);
			if (map_size == 0)
				break;
			pr_frc(0, "dump buf %d:0x%lx size:0x%x\n", i, (ulong)cma_addr, map_size);
			vaddr = (u32 *)frc_buf_vmap(cma_addr, map_size);
			vaddr_start = vaddr;
			frc_dump_mm_data(vaddr, map_size);
			frc_buf_unmap(vaddr_start);
		}
	} else if (mode == FRC_BUF_MC_C_IDX) {
		for (i = 0; i < FRC_TOTAL_BUF_NUM; i++) {
			cma_addr =
			devp->buf.cma_mem_paddr_start + devp->buf.lossy_mc_c_link_buf_paddr[i];
			map_size = roundup(devp->buf.lossy_mc_c_link_buf_size[i], ALIGN_64);
			if (map_size == 0)
				break;
			pr_frc(0, "dump :0x%lx size:0x%x\n", (ulong)cma_addr, map_size);
			vaddr = (u32 *)frc_buf_vmap(cma_addr, map_size);
			vaddr_start = vaddr;
			frc_dump_mm_data(vaddr, map_size);
			frc_buf_unmap(vaddr_start);
		}
	} else if (mode == FRC_BUF_MC_V_IDX) {
		for (i = 0; i < FRC_TOTAL_BUF_NUM; i++) {
			cma_addr =
			devp->buf.cma_mem_paddr_start + devp->buf.lossy_mc_v_link_buf_paddr[i];
			map_size = roundup(devp->buf.lossy_mc_v_link_buf_size[i], ALIGN_64);
			if (map_size == 0)
				break;
			pr_frc(0, "dump :0x%lx size:0x%x\n", (ulong)cma_addr, map_size);
			vaddr = (u32 *)frc_buf_vmap(cma_addr, map_size);
			vaddr_start = vaddr;
			frc_dump_mm_data(vaddr, map_size);
			frc_buf_unmap(vaddr_start);
		}
	} else if (mode == FRC_BUF_ME_IDX) {
		for (i = 0; i < FRC_TOTAL_BUF_NUM; i++) {
			cma_addr =
			devp->buf.cma_mem_paddr_start + devp->buf.lossy_me_link_buf_paddr[i];
			map_size = roundup(devp->buf.lossy_me_link_buf_size[i], ALIGN_64);
			if (map_size == 0)
				break;
			pr_frc(0, "dump :0x%lx size:0x%x\n", (ulong)cma_addr, map_size);
			vaddr = (u32 *)frc_buf_vmap(cma_addr, map_size);
			vaddr_start = vaddr;
			frc_dump_mm_data(vaddr, map_size);
			frc_buf_unmap(vaddr_start);
		}
	} else if (mode == FRC_BUF_MCDW_Y_IDX) {
		for (i = 0; i < FRC_TOTAL_BUF_NUM; i++) {
			cma_addr =
			(devp->buf.cma_mem_paddr_start + devp->buf.lossy_mcdw_y_link_buf_paddr[i]);
			map_size = roundup(devp->buf.lossy_mcdw_y_link_buf_size[i], ALIGN_64);
			if (map_size == 0)
				break;
			pr_frc(0, "dump buf %d:0x%lx size:0x%x\n", i, (ulong)cma_addr, map_size);
			vaddr = (u32 *)frc_buf_vmap(cma_addr, map_size);
			vaddr_start = vaddr;
			frc_dump_mm_data(vaddr, map_size);
			frc_buf_unmap(vaddr_start);
		}
	} else if (mode == FRC_BUF_MCDW_C_IDX) {
		for (i = 0; i < FRC_TOTAL_BUF_NUM; i++) {
			cma_addr =
			devp->buf.cma_mem_paddr_start + devp->buf.lossy_mcdw_c_link_buf_paddr[i];
			map_size = roundup(devp->buf.lossy_mcdw_c_link_buf_size[i], ALIGN_64);
			if (map_size == 0)
				break;
			pr_frc(0, "dump :0x%lx size:0x%x\n", (ulong)cma_addr, map_size);
			vaddr = (u32 *)frc_buf_vmap(cma_addr, map_size);
			vaddr_start = vaddr;
			frc_dump_mm_data(vaddr, map_size);
			frc_buf_unmap(vaddr_start);
		}
	}
}

void frc_dump_buf_data(struct frc_dev_s *devp, u32 cma_addr, u32 size)
{
#ifdef DEMOD_KERNEL_WR
	struct file *filp = NULL;
	loff_t pos = 0;
	void *buf = NULL;
	unsigned int len = size;
	mm_segment_t old_fs = get_fs();
	char *path = "/data/frc.bin";

	pr_debug("%s paddr:0x%x, size:0x%x\n", __func__, cma_addr, size);
	if ((size > 1024 * 1024 * 20) || size == 0)
		return;

	set_fs(KERNEL_DS);
	filp = filp_open(path, O_RDWR | O_CREAT, 0666);

	if (IS_ERR_OR_NULL(filp)) {
		pr_info("create %s error or filp is NULL\n", path);
		set_fs(old_fs);
		return;
	}

	if (!devp->buf.cma_mem_alloced) {
		pr_frc(0, "%s:no cma alloc mem\n", __func__);
		set_fs(old_fs);
		return;
	}

	buf = (u32 *)frc_buf_vmap(cma_addr, len);
	if (!buf) {
		pr_info("vdin_vmap error\n");
		goto exit;
	}
	frc_buf_dma_flush(devp, buf, cma_addr, len, DMA_FROM_DEVICE);
	//write
	vfs_write(filp, buf, len, &pos);

	frc_buf_unmap(buf);
exit:
	vfs_fsync(filp, 0);
	filp_close(filp, NULL);
	set_fs(old_fs);
#endif
}

void frc_dump_buf_reg(void)
{
	u32 i = 0;

	pr_frc(0, "%s (0x%x) val:0x%x\n", "FRC_REG_MC_YINFO_BADDR", FRC_REG_MC_YINFO_BADDR,
	       READ_FRC_REG(FRC_REG_MC_YINFO_BADDR));
	pr_frc(0, "%s (0x%x) val:0x%x\n", "FRC_REG_MC_CINFO_BADDR", FRC_REG_MC_CINFO_BADDR,
	       READ_FRC_REG(FRC_REG_MC_CINFO_BADDR));
	pr_frc(0, "%s (0x%x) val:0x%x\n", "FRC_REG_MC_VINFO_BADDR", FRC_REG_MC_VINFO_BADDR,
	       READ_FRC_REG(FRC_REG_MC_VINFO_BADDR));
	pr_frc(0, "%s (0x%x) val:0x%x\n", "FRC_REG_ME_XINFO_BADDR", FRC_REG_ME_XINFO_BADDR,
	       READ_FRC_REG(FRC_REG_ME_XINFO_BADDR));
	pr_frc(0, "%s (0x%x) val:0x%x\n", "FRC_REG_MCDW_YINFO_BADDR", FRC_REG_MCDW_YINFO_BADDR,
	       READ_FRC_REG(FRC_REG_MCDW_YINFO_BADDR));
	pr_frc(0, "%s (0x%x) val:0x%x\n", "FRC_REG_MCDW_CINFO_BADDR", FRC_REG_MCDW_CINFO_BADDR,
	       READ_FRC_REG(FRC_REG_MCDW_CINFO_BADDR));

	for (i = FRC_REG_MC_YBUF_ADDRX_0; i <= FRC_REG_MC_YBUF_ADDRX_15; i++)
		pr_frc(0, "%s%d (0x%x) val:0x%x\n",
		       "FRC_REG_MC_YBUF_ADDRX_", i - FRC_REG_MC_YBUF_ADDRX_0, i,
		       READ_FRC_REG(i));

	for (i = FRC_REG_MC_CBUF_ADDRX_0; i <= FRC_REG_MC_CBUF_ADDRX_15; i++)
		pr_frc(0, "%s%d (0x%x) val:0x%x\n",
		       "FRC_REG_MC_CBUF_ADDRX_", i - FRC_REG_MC_CBUF_ADDRX_0, i,
		       READ_FRC_REG(i));

	for (i = FRC_REG_MC_VBUF_ADDRX_0; i <= FRC_REG_MC_VBUF_ADDRX_15; i++)
		pr_frc(0, "%s%d (0x%x) val:0x%x\n",
		       "FRC_REG_MC_VBUF_ADDRX_", i - FRC_REG_MC_VBUF_ADDRX_0, i,
		       READ_FRC_REG(i));

	for (i = FRC_REG_MCDW_YBUF_ADDRX_0; i <= FRC_REG_MCDW_YBUF_ADDRX_15; i++)
		pr_frc(0, "%s%d (0x%x) val:0x%x\n",
		       "FRC_REG_MCDW_YBUF_ADDRX_", i - FRC_REG_MCDW_YBUF_ADDRX_0, i,
		       READ_FRC_REG(i));

	for (i = FRC_REG_MCDW_CBUF_ADDRX_0; i <= FRC_REG_MCDW_CBUF_ADDRX_15; i++)
		pr_frc(0, "%s%d (0x%x) val:0x%x\n",
		       "FRC_REG_MCDW_CBUF_ADDRX_", i - FRC_REG_MCDW_CBUF_ADDRX_0, i,
		       READ_FRC_REG(i));

	/*lossy me link buffer*/
	for (i = FRC_REG_ME_BUF_ADDRX_0; i <= FRC_REG_ME_BUF_ADDRX_15; i++)
		pr_frc(0, "%s%d (0x%x) val:0x%x\n",
		       "FRC_REG_ME_BUF_ADDRX_", i - FRC_REG_ME_BUF_ADDRX_0, i,
		       READ_FRC_REG(i));

	/*norm hme data buffer*/
	for (i = FRC_REG_HME_BUF_ADDRX_0; i <= FRC_REG_HME_BUF_ADDRX_15; i++)
		pr_frc(0, "%s%d (0x%x) val:0x%x\n",
		       "FRC_REG_HME_BUF_ADDRX_", i - FRC_REG_HME_BUF_ADDRX_0, i,
		       READ_FRC_REG(i));

	/*norm memv buffer*/
	for (i = FRC_REG_ME_NC_UNI_MV_ADDRX_0; i <= FRC_REG_ME_PC_PHS_MV_ADDR; i++)
		pr_frc(0, "%s%d (0x%x) val:0x%x\n",
		       "FRC_REG_ME_NC_UNI_MV_ADDRX_", i - FRC_REG_ME_NC_UNI_MV_ADDRX_0, i,
		       READ_FRC_REG(i));

	/*norm hmemv buffer*/
	for (i = FRC_REG_HME_NC_UNI_MV_ADDRX_0; i <= FRC_REG_VP_PF_UNI_MV_ADDR; i++)
		pr_frc(0, "%s%d (0x%x) val:0x%x\n",
		       "FRC_REG_HME_NC_UNI_MV_ADDRX_", i - FRC_REG_HME_NC_UNI_MV_ADDRX_0, i,
		       READ_FRC_REG(i));

	/*norm mevp buffer*/
	for (i = FRC_REG_VP_MC_MV_ADDRX_0; i <= FRC_REG_VP_MC_MV_ADDRX_1; i++)
		pr_frc(0, "%s%d (0x%x) val:0x%x\n",
		       "FRC_REG_VP_MC_MV_ADDRX_", i - FRC_REG_VP_MC_MV_ADDRX_0, i,
		       READ_FRC_REG(i));

	/*norm iplogo buffer*/
	for (i = FRC_REG_IP_LOGO_ADDRX_0; i <= FRC_REG_IP_LOGO_ADDRX_15; i++)
		pr_frc(0, "%s%d (0x%x) val:0x%x\n",
		       "FRC_REG_IP_LOGO_ADDRX_", i - FRC_REG_IP_LOGO_ADDRX_0, i,
		       READ_FRC_REG(i));

	pr_frc(0, "%s (0x%x) val:0x%x\n",
	       "FRC_REG_LOGO_IIR_BUF_ADDR", FRC_REG_LOGO_IIR_BUF_ADDR,
	       READ_FRC_REG(FRC_REG_LOGO_IIR_BUF_ADDR));
	pr_frc(0, "%s (0x%x) val:0x%x\n",
	       "FRC_REG_LOGO_SCC_BUF_ADDR", FRC_REG_LOGO_SCC_BUF_ADDR,
	       READ_FRC_REG(FRC_REG_LOGO_SCC_BUF_ADDR));

	/*norm iplogo buffer*/
	for (i = FRC_REG_ME_LOGO_ADDRX_0; i <= FRC_REG_ME_LOGO_ADDRX_15; i++)
		pr_frc(0, "%s%d (0x%x) val:0x%x\n",
		       "FRC_REG_ME_LOGO_ADDRX_", i - FRC_REG_ME_LOGO_ADDRX_0, i,
		       READ_FRC_REG(i));
}

/*
 * alloc resolve memory
 */
int frc_buf_alloc(struct frc_dev_s *devp)
{
	int ret;
	u32 frc_buf_size;
	struct device_node *np = devp->pdev->dev.of_node;

	ret = of_reserved_mem_device_init_by_idx(&devp->pdev->dev, np, 0);
	if (ret) {
		pr_frc(0, "%s cma resource undefined!\n", __func__);
		devp->buf.cma_mem_size = 0;
		return -1;
	}

	devp->buf.cma_mem_size = dma_get_cma_size_int_byte(&devp->pdev->dev);
	frc_buf_size = devp->buf.cma_mem_size - FRC_RDMA_SIZE; // reserved 1M for RDMA
	devp->buf.cma_mem_paddr_pages = dma_alloc_from_contiguous(&devp->pdev->dev,
		frc_buf_size >> PAGE_SHIFT, 0, 0);
	if (!devp->buf.cma_mem_paddr_pages) {
		devp->buf.cma_mem_size = 0;
		pr_frc(0, "cma_alloc buffer fail\n");
		return -1;
	}
	devp->buf.cma_mem_paddr_start = page_to_phys(devp->buf.cma_mem_paddr_pages);
	devp->buf.cma_mem_alloced = 1;
	if (devp->buf.cma_mem_paddr_start > 0x300000000) {
		WRITE_FRC_REG(FRC_AXI_ADDR_EXT_CTRL, 0x3333);
		pr_frc(0, "frc run on 16G-dram board\n");
	} else if (devp->buf.cma_mem_paddr_start > 0x200000000) {
		WRITE_FRC_REG(FRC_AXI_ADDR_EXT_CTRL, 0x2222);
		pr_frc(0, "frc run on 12G-dram board\n");
	} else if (devp->buf.cma_mem_paddr_start > 0x100000000) {
		WRITE_FRC_REG(FRC_AXI_ADDR_EXT_CTRL, 0x1111);
		pr_frc(0, "frc run on 8G-dram board\n");
	}

	pr_frc(0, "cma paddr_start=0x%lx size:0x%x\n",
		(ulong)devp->buf.cma_mem_paddr_start, frc_buf_size);

	return 0;
}

int frc_buf_release(struct frc_dev_s *devp)
{
	u32 frc_buf_size;

	frc_buf_size = devp->buf.cma_mem_size - FRC_RDMA_SIZE; // reserved 1M for RDMA

	if (devp->buf.cma_mem_size && devp->buf.cma_mem_paddr_pages) {
		dma_release_from_contiguous(&devp->pdev->dev,
			devp->buf.cma_mem_paddr_pages, frc_buf_size >> PAGE_SHIFT);
		devp->buf.cma_mem_paddr_pages = NULL;
		devp->buf.cma_mem_paddr_start = 0;
		devp->buf.cma_mem_alloced = 0;
		// devp->buf.cma_buf_alloc = 0;
		pr_frc(0, "%s buffer1 released\n", __func__);
		frc_rdma_release_buf();
	} else {
		pr_frc(0, "%s no buffer exist\n", __func__);
	}

	return 0;
}

/*
 * calculate buffer depend on input source
 */
int frc_buf_calculate(struct frc_dev_s *devp)
{
	u32 i;
	u32 align_hsize, align_vsize;
	u32 temp, temp2, temp3;
	int log = 2;
	u32 ratio;
	u8  info_factor;
	u32 mcdw_h_ratio, mcdw_v_ratio;
	u8  frm_buf_num;
	u8  logo_buf_num;
	enum chip_id chip;

	if (!devp)
		return -1;

	chip = get_chip_type();

	temp = READ_FRC_REG(FRC_FRAME_BUFFER_NUM);
	frm_buf_num = temp & 0x1F;
	logo_buf_num = (temp >> 8) & 0x1F;
	if (frm_buf_num == 0 || logo_buf_num == 0) {
		frm_buf_num = FRC_TOTAL_BUF_NUM;
		logo_buf_num = FRC_TOTAL_BUF_NUM;
		WRITE_FRC_REG_BY_CPU(FRC_FRAME_BUFFER_NUM,
			frm_buf_num << 8 | frm_buf_num);
		pr_frc(log, "buf num reg read 0  retore default");
	}
	devp->buf.frm_buf_num = frm_buf_num;
	devp->buf.logo_buf_num = logo_buf_num;
	pr_frc(log, "buf num (frm:%d, logo:%d)", frm_buf_num, logo_buf_num);
	if (devp->buf.memc_comprate == 0)
		devp->buf.memc_comprate = FRC_COMPRESS_RATE;
	if (devp->buf.total_size == 0) {
		if (chip == ID_T3) {
			devp->buf.me_comprate = FRC_COMPRESS_RATE_ME_T3;
			devp->buf.mc_y_comprate = FRC_COMPRESS_RATE_MC_Y;
			devp->buf.mc_c_comprate = FRC_COMPRESS_RATE_MC_C;
			devp->buf.addr_shft_bits = DDR_SHFT_0_BITS;
			devp->buf.info_factor = FRC_INFO_BUF_FACTOR_T3;

		} else if (chip == ID_T5M) {
			devp->buf.me_comprate = FRC_COMPRESS_RATE_ME_T5M;
			devp->buf.mc_y_comprate = FRC_COMPRESS_RATE_MC_Y;
			devp->buf.mc_c_comprate = FRC_COMPRESS_RATE_MC_C;
			devp->buf.addr_shft_bits = DDR_SHFT_0_BITS;
			devp->buf.info_factor = FRC_INFO_BUF_FACTOR_T5M;

		} else if (chip == ID_T3X) {
			devp->buf.me_comprate = FRC_COMPRESS_RATE_ME_T3;
			devp->buf.mc_y_comprate = FRC_COMPRESS_RATE_MC_Y_T3X;
			devp->buf.mc_c_comprate = FRC_COMPRESS_RATE_MC_C_T3X;
			devp->buf.addr_shft_bits = DDR_SHFT_4_BITS;
			devp->buf.info_factor = FRC_INFO_BUF_FACTOR_T3X;
		} else {
			devp->buf.me_comprate = FRC_COMPRESS_RATE_ME_T3;
			devp->buf.mc_y_comprate = FRC_COMPRESS_RATE_MC_Y;
			devp->buf.mc_c_comprate = FRC_COMPRESS_RATE_MC_C;
			devp->buf.addr_shft_bits = DDR_SHFT_0_BITS;
			devp->buf.info_factor = FRC_INFO_BUF_FACTOR_T3;
		}
		if (devp->buf.rate_margin == 0)
			devp->buf.rate_margin = FRC_COMPRESS_RATE_MARGIN;
		devp->buf.mcdw_c_comprate = FRC_COMPRESS_RATE_MCDW_C;
		devp->buf.mcdw_y_comprate = FRC_COMPRESS_RATE_MCDW_Y;
		devp->buf.mcdw_size_rate = FRC_MCDW_H_SIZE_RATE << 4;
		devp->buf.mcdw_size_rate += FRC_MCDW_V_SIZE_RATE;
	}

	pr_frc(log, "buf addr shfit :%d", devp->buf.addr_shft_bits);

	/*size initial, alloc max support size accordint to vout*/
	devp->buf.in_hsize = devp->out_sts.vout_width;
	devp->buf.in_vsize = devp->out_sts.vout_height;

	/*align size*/
	devp->buf.in_align_hsize = roundup(devp->buf.in_hsize, FRC_HVSIZE_ALIGN_SIZE);
	devp->buf.in_align_vsize = roundup(devp->buf.in_vsize, FRC_HVSIZE_ALIGN_SIZE);
	align_hsize = devp->buf.in_align_hsize;
	align_vsize = devp->buf.in_align_vsize;
	mcdw_h_ratio = (devp->buf.mcdw_size_rate >> 4) & 0xF;
	mcdw_v_ratio =	devp->buf.mcdw_size_rate & 0xF;
	info_factor = devp->buf.info_factor;

	if (devp->out_sts.vout_width > 1920 && devp->out_sts.vout_height > 1080) {
		devp->buf.me_hsize =
			roundup(devp->buf.in_align_hsize, FRC_ME_SD_RATE_4K) / FRC_ME_SD_RATE_4K;
		devp->buf.me_vsize =
			roundup(devp->buf.in_align_vsize, FRC_ME_SD_RATE_4K) / FRC_ME_SD_RATE_4K;
		ratio = 32;
	} else {
		devp->buf.me_hsize =
			roundup(devp->buf.in_align_hsize, FRC_ME_SD_RATE_HD) / FRC_ME_SD_RATE_HD;
		devp->buf.me_vsize =
			roundup(devp->buf.in_align_vsize, FRC_ME_SD_RATE_HD) / FRC_ME_SD_RATE_HD;
		ratio = 16;
	}

	devp->buf.logo_hsize =
		roundup(devp->buf.me_hsize, FRC_LOGO_SD_RATE) / FRC_LOGO_SD_RATE;
	devp->buf.logo_vsize =
		roundup(devp->buf.me_vsize, FRC_LOGO_SD_RATE) / FRC_LOGO_SD_RATE;

	devp->buf.hme_hsize =
		roundup(devp->buf.me_hsize, FRC_HME_SD_RATE) / FRC_HME_SD_RATE;
	devp->buf.hme_vsize =
		roundup(devp->buf.me_vsize, FRC_HME_SD_RATE) / FRC_HME_SD_RATE;

	devp->buf.me_blk_hsize =
		roundup(devp->buf.me_hsize, 4) / 4;
	devp->buf.me_blk_vsize =
		roundup(devp->buf.me_vsize, 4) / 4;

	devp->buf.hme_blk_hsize =
		roundup(devp->buf.hme_hsize, 4) / 4;
	devp->buf.hme_blk_vsize =
		roundup(devp->buf.hme_vsize, 4) / 4;

	pr_frc(log, "in hv (%d, %d) align (%d, %d)",
	       devp->buf.in_hsize, devp->buf.in_vsize,
	       devp->buf.in_align_hsize, devp->buf.in_align_vsize);
	pr_frc(log, "me hv (%d, %d)", devp->buf.me_hsize, devp->buf.me_vsize);
	pr_frc(log, "logo hv (%d, %d)", devp->buf.logo_hsize, devp->buf.logo_vsize);
	pr_frc(log, "hme hv (%d, %d)", devp->buf.hme_hsize, devp->buf.hme_vsize);
	pr_frc(log, "me blk hv (%d, %d)", devp->buf.me_blk_hsize,
	       devp->buf.me_blk_vsize);
	pr_frc(log, "hme blk (%d, %d)", devp->buf.hme_blk_hsize,
	       devp->buf.hme_blk_vsize);

	/* ------------ cal buffer start -----------------*/
	pr_frc(0, "dc_rate:(me:%d,mc_y:%d,mc_c:%d,mcdw_y:%d,mcdw_c:%d)\n",
		devp->buf.me_comprate, devp->buf.mc_y_comprate,
		devp->buf.mc_c_comprate, devp->buf.mcdw_y_comprate,
		devp->buf.mcdw_c_comprate);
	pr_frc(0, "mcdw_rate(h:%d,v:%d) mcdw_size(h:%d,v:%d)\n",
		mcdw_h_ratio, mcdw_v_ratio, align_hsize / mcdw_h_ratio,
		align_vsize / mcdw_v_ratio);
	devp->buf.total_size = 0;
	/*mc y/c/v info buffer, address 64 bytes align*/
	devp->buf.lossy_mc_y_info_buf_size =
		LOSSY_MC_INFO_LINE_SIZE * FRC_SLICER_NUM * info_factor;
	devp->buf.lossy_mc_c_info_buf_size =
		LOSSY_MC_INFO_LINE_SIZE * FRC_SLICER_NUM * info_factor;
	devp->buf.lossy_mc_v_info_buf_size = 0;
	devp->buf.total_size += devp->buf.lossy_mc_y_info_buf_size;
	devp->buf.total_size += devp->buf.lossy_mc_c_info_buf_size;
	devp->buf.total_size += devp->buf.lossy_mc_v_info_buf_size;
	pr_frc(log, "lossy_mc_info_buf_size=%d, line buf:%d  all:%d\n",
	       devp->buf.lossy_mc_y_info_buf_size, LOSSY_MC_INFO_LINE_SIZE,
	       devp->buf.lossy_mc_y_info_buf_size * 2);

	/*me info buffer*/
	devp->buf.lossy_me_x_info_buf_size =
		LOSSY_MC_INFO_LINE_SIZE * FRC_SLICER_NUM * info_factor;
	devp->buf.total_size += devp->buf.lossy_me_x_info_buf_size;

	pr_frc(log, "lossy_me_info_buf_size=%d, line buf:%d all:%d\n",
			devp->buf.lossy_me_x_info_buf_size, LOSSY_MC_INFO_LINE_SIZE,
			devp->buf.lossy_me_x_info_buf_size * 1);

	// only t3x mc y/c info buffer, address 64 bytes align
	if (chip == ID_T3X) {
		devp->buf.lossy_mcdw_y_info_buf_size =
			LOSSY_MC_INFO_LINE_SIZE * FRC_SLICER_NUM * info_factor;
		devp->buf.lossy_mcdw_c_info_buf_size =
			LOSSY_MC_INFO_LINE_SIZE * FRC_SLICER_NUM * info_factor;
		devp->buf.total_size += devp->buf.lossy_mcdw_y_info_buf_size;
		devp->buf.total_size += devp->buf.lossy_mcdw_c_info_buf_size;
		pr_frc(log, "lossy_mcdw_info_buf_size=%d, line buf:%d  all:%d\n",
		devp->buf.lossy_mcdw_y_info_buf_size, LOSSY_MC_INFO_LINE_SIZE,
			devp->buf.lossy_mcdw_y_info_buf_size * 2);
	} else {
		devp->buf.lossy_mcdw_y_info_buf_size = 0;
		devp->buf.lossy_mcdw_c_info_buf_size = 0;
		devp->buf.total_size += devp->buf.lossy_mcdw_y_info_buf_size;
		devp->buf.total_size += devp->buf.lossy_mcdw_c_info_buf_size;
	}

	temp = (align_hsize * FRC_MC_BITS_NUM + 511) / 8;
	temp2 = temp * align_vsize * devp->buf.mc_y_comprate / 100;
	temp3 = temp * align_vsize * devp->buf.mc_c_comprate / 100;
	/*lossy mc data buffer*/
	for (i = 0; i < frm_buf_num; i++) {
		devp->buf.lossy_mc_y_data_buf_size[i] = ALIGN_4K * ratio +
			temp2 * (devp->buf.rate_margin + 100) / 100;
		devp->buf.lossy_mc_c_data_buf_size[i] = ALIGN_4K * ratio +
			temp3 * (devp->buf.rate_margin + 100) / 100;
		devp->buf.lossy_mc_v_data_buf_size[i] = 0;//ALIGN_4K * 4 +
			//(temp * align_vsize * FRC_COMPRESS_RATE) / 100;
		devp->buf.total_size += devp->buf.lossy_mc_y_data_buf_size[i];
		devp->buf.total_size += devp->buf.lossy_mc_c_data_buf_size[i];
		devp->buf.total_size += devp->buf.lossy_mc_v_data_buf_size[i];
	}
	for (i = frm_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
		devp->buf.lossy_mc_y_data_buf_size[i] = 0;
		devp->buf.lossy_mc_c_data_buf_size[i] = 0;
		devp->buf.lossy_mc_v_data_buf_size[i] = 0;
		devp->buf.total_size += devp->buf.lossy_mc_y_data_buf_size[i];
		devp->buf.total_size += devp->buf.lossy_mc_c_data_buf_size[i];
		devp->buf.total_size += devp->buf.lossy_mc_v_data_buf_size[i];
	}
	pr_frc(log, "lossy_mc_y_data_buf_size=%d, line buf:%d all:%d\n",
		devp->buf.lossy_mc_y_data_buf_size[0], temp,
		devp->buf.lossy_mc_y_data_buf_size[0] * frm_buf_num);
	pr_frc(log, "lossy_mc_c_data_buf_size=%d, line buf:%d all:%d\n",
		devp->buf.lossy_mc_c_data_buf_size[0], temp,
		devp->buf.lossy_mc_c_data_buf_size[0] * frm_buf_num);

	temp = (devp->buf.me_hsize * FRC_ME_BITS_NUM + 511) / 8;
	temp2 = (temp * devp->buf.me_vsize * devp->buf.me_comprate) / 100;
	if (devp->buf.me_comprate < 100)
		temp3 = temp2 * (devp->buf.rate_margin + 100) / 100;
	else
		temp3 = temp2;
	/*lossy me data buffer*/
	for (i = 0; i < frm_buf_num; i++) {
		devp->buf.lossy_me_data_buf_size[i] = ALIGN_4K * ratio + temp3;
		devp->buf.total_size += devp->buf.lossy_me_data_buf_size[i];
	}
	for (i = frm_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
		devp->buf.lossy_me_data_buf_size[i] = 0;
		devp->buf.total_size += devp->buf.lossy_me_data_buf_size[i];
	}
	pr_frc(log, "lossy_me_data_buf_size=%d, line buf:%d all:%d\n",
	       devp->buf.lossy_me_data_buf_size[0], temp,
	       devp->buf.lossy_me_data_buf_size[0] * frm_buf_num);

	/*t3x lossy mcdw data buffer*/
	if (chip == ID_T3X) {
		temp = (align_hsize / mcdw_h_ratio * FRC_MC_BITS_NUM + 511) / 8;
		temp2 = (temp * align_vsize / mcdw_v_ratio * devp->buf.mcdw_y_comprate) / 100;
		temp3 = (temp * align_vsize / mcdw_v_ratio * devp->buf.mcdw_c_comprate) / 100;
		for (i = 0; i < frm_buf_num; i++) {
			devp->buf.lossy_mcdw_y_data_buf_size[i] = ALIGN_4K * ratio +
				temp2 * (devp->buf.rate_margin + 100) / 100;
			devp->buf.lossy_mcdw_c_data_buf_size[i] = ALIGN_4K * ratio +
				temp3 * (devp->buf.rate_margin + 100) / 100;
			devp->buf.total_size += devp->buf.lossy_mcdw_y_data_buf_size[i];
			devp->buf.total_size += devp->buf.lossy_mcdw_c_data_buf_size[i];
		}
		for (i = frm_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
			devp->buf.lossy_mcdw_y_data_buf_size[i] = 0;
			devp->buf.lossy_mcdw_c_data_buf_size[i] = 0;
			devp->buf.total_size += devp->buf.lossy_mcdw_y_data_buf_size[i];
			devp->buf.total_size += devp->buf.lossy_mcdw_c_data_buf_size[i];
		}
		pr_frc(log, "lossy_mcdw_y_data_buf_size=%d, line buf:%d all:%d\n",
			devp->buf.lossy_mcdw_y_data_buf_size[0], temp,
			devp->buf.lossy_mcdw_y_data_buf_size[0] * frm_buf_num);
		pr_frc(log, "lossy_mcdw_c_data_buf_size=%d, line buf:%d all:%d\n",
			devp->buf.lossy_mcdw_c_data_buf_size[0], temp,
			devp->buf.lossy_mcdw_c_data_buf_size[0] * frm_buf_num);
	} else {  // t3/t5m
		for (i = 0; i < FRC_TOTAL_BUF_NUM; i++) {
			devp->buf.lossy_mcdw_y_data_buf_size[i] = 0;
			devp->buf.lossy_mcdw_c_data_buf_size[i] = 0;
			devp->buf.total_size += devp->buf.lossy_mcdw_y_data_buf_size[i];
			devp->buf.total_size += devp->buf.lossy_mcdw_c_data_buf_size[i];
		}
	}

	/*mc y link buffer*/
	temp = (roundup(devp->buf.lossy_mc_y_data_buf_size[0], ALIGN_4K) / ALIGN_4K) * 4;
	for (i = 0; i < frm_buf_num; i++) {
		devp->buf.lossy_mc_y_link_buf_size[i] = temp;
		devp->buf.total_size += devp->buf.lossy_mc_y_link_buf_size[i];
	}
	pr_frc(log, "lossy_mc_y_link_buf_size=%d, line buf:%d all:%d\n",
		devp->buf.lossy_mc_y_link_buf_size[0], temp,
		devp->buf.lossy_mc_y_link_buf_size[0] * frm_buf_num);

	/*mc c link buffer*/
	temp = (roundup(devp->buf.lossy_mc_c_data_buf_size[0], ALIGN_4K) / ALIGN_4K) * 4;
	for (i = 0; i < frm_buf_num; i++) {
		devp->buf.lossy_mc_c_link_buf_size[i] = temp;
		devp->buf.lossy_mc_v_link_buf_size[i] = 0;
		devp->buf.total_size += devp->buf.lossy_mc_c_link_buf_size[i];
		devp->buf.total_size += devp->buf.lossy_mc_v_link_buf_size[i];
	}
	pr_frc(log, "lossy_mc_c_link_buf_size=%d, line buf:%d all:%d\n",
		devp->buf.lossy_mc_c_link_buf_size[0], temp,
		devp->buf.lossy_mc_c_link_buf_size[0] * frm_buf_num);
	for (i = frm_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
		devp->buf.lossy_mc_y_link_buf_size[i] = 0;
		devp->buf.lossy_mc_c_link_buf_size[i] = 0;
		devp->buf.lossy_mc_v_link_buf_size[i] = 0;
		devp->buf.total_size += devp->buf.lossy_mc_y_link_buf_size[i];
		devp->buf.total_size += devp->buf.lossy_mc_c_link_buf_size[i];
		devp->buf.total_size += devp->buf.lossy_mc_v_link_buf_size[i];
	}

	/*me link buffer*/
	temp = (roundup(devp->buf.lossy_me_data_buf_size[0], ALIGN_4K) / ALIGN_4K) * 4;
	for (i = 0; i < frm_buf_num; i++) {
		devp->buf.lossy_me_link_buf_size[i] = temp;
		devp->buf.total_size += devp->buf.lossy_me_link_buf_size[i];
	}
	for (i = frm_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
		devp->buf.lossy_me_link_buf_size[i] = 0;
		devp->buf.total_size += devp->buf.lossy_me_link_buf_size[i];
	}
	pr_frc(log, "lossy_me_link_buf_size=%d ,line buf:%d all:%d\n",
	       devp->buf.lossy_me_link_buf_size[0], temp,
	       devp->buf.lossy_me_link_buf_size[0] * frm_buf_num);

	/*t3x mcdw link buffer*/
	if (chip == ID_T3X) {
		temp = (roundup(devp->buf.lossy_mcdw_y_data_buf_size[0], ALIGN_4K) / ALIGN_4K) * 4;
		for (i = 0; i < frm_buf_num; i++) {
			devp->buf.lossy_mcdw_y_link_buf_size[i] = temp * 1;
			devp->buf.total_size += devp->buf.lossy_mcdw_y_link_buf_size[i];
		}
		pr_frc(log, "lossy_mcdw_y_link_buf_size=%d ,line buf:%d all:%d\n",
			devp->buf.lossy_mcdw_y_link_buf_size[0], temp,
			devp->buf.lossy_mcdw_y_link_buf_size[0] * frm_buf_num);
		for (i = 0; i < frm_buf_num; i++) {
			devp->buf.lossy_mcdw_c_link_buf_size[i] = temp * 1;
			devp->buf.total_size += devp->buf.lossy_mcdw_c_link_buf_size[i];
		}
		pr_frc(log, "lossy_mcdw_c_link_buf_size=%d ,line buf:%d all:%d\n",
			devp->buf.lossy_mcdw_c_link_buf_size[0], temp,
			devp->buf.lossy_mcdw_c_link_buf_size[0] * frm_buf_num);
		for (i = frm_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
			devp->buf.lossy_mcdw_y_link_buf_size[i] = 0;
			devp->buf.lossy_mcdw_c_link_buf_size[i] = 0;
			devp->buf.total_size += 0;
		}
	} else { // t3/t5m
		for (i = 0; i < FRC_TOTAL_BUF_NUM; i++) {
			devp->buf.lossy_mcdw_y_link_buf_size[i] = 0;
			devp->buf.lossy_mcdw_c_link_buf_size[i] = 0;
			devp->buf.total_size += devp->buf.lossy_mcdw_y_link_buf_size[i];
			devp->buf.total_size += devp->buf.lossy_mcdw_c_link_buf_size[i];
		}
	}

	/*t3 norm hme data buffer*/
	if (chip == ID_T3) {
		temp = (devp->buf.hme_hsize * FRC_ME_BITS_NUM + 511) / 8;
		for (i = 0; i < frm_buf_num; i++) {
			devp->buf.norm_hme_data_buf_size[i] = temp * devp->buf.hme_vsize;
			devp->buf.total_size += devp->buf.norm_hme_data_buf_size[i];
		}
		for (i = frm_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
			devp->buf.norm_hme_data_buf_size[i] = 0;
			devp->buf.total_size += devp->buf.norm_hme_data_buf_size[i];
		}
		pr_frc(log, "norm_hme_data_buffer=%d ,line buf:%d all:%d\n",
			devp->buf.norm_hme_data_buf_size[0], temp,
			devp->buf.norm_hme_data_buf_size[0] * frm_buf_num);
	} else { // t3x t5m
		for (i = 0; i < FRC_TOTAL_BUF_NUM; i++) {
			devp->buf.norm_hme_data_buf_size[i] = 0;
			devp->buf.total_size += devp->buf.norm_hme_data_buf_size[i];
		}
	}

	/*norm memv buffer*/
	temp = (devp->buf.me_blk_hsize * 64 + 511) / 8;
	for (i = 0; i < FRC_MEMV_BUF_NUM; i++) {
		devp->buf.norm_memv_buf_size[i] = temp * devp->buf.me_blk_vsize;
		devp->buf.total_size += devp->buf.norm_memv_buf_size[i];
	}
	pr_frc(log, "norm_memv_buf_size=%d ,line buf:%d all:%d\n",
		devp->buf.norm_memv_buf_size[0], temp,
		devp->buf.norm_memv_buf_size[0] * FRC_MEMV_BUF_NUM);

	/*t3 norm hmemv buffer*/
	if (chip == ID_T3) {
		temp = (devp->buf.hme_blk_hsize * 64 + 511) / 8;
		for (i = 0; i < FRC_MEMV2_BUF_NUM; i++) {
			devp->buf.norm_hmemv_buf_size[i] = temp * devp->buf.hme_blk_vsize;
			devp->buf.total_size += devp->buf.norm_hmemv_buf_size[i];
		}
		pr_frc(log, "norm_hmemv_buf_size=%d ,line buf:%d all:%d\n",
			devp->buf.norm_hmemv_buf_size[0], temp,
			devp->buf.norm_hmemv_buf_size[0] * FRC_MEMV2_BUF_NUM);
	} else { // t3x t5m
		for (i = 0; i < FRC_MEMV2_BUF_NUM; i++) {
			devp->buf.norm_hmemv_buf_size[i] = 0;
			devp->buf.total_size += devp->buf.norm_hmemv_buf_size[i];
		}
	}

	/*norm mevp buffer*/
	temp = (devp->buf.me_blk_hsize * 64 + 511) / 8;
	for (i = 0; i < FRC_MEVP_BUF_NUM; i++) {
		devp->buf.norm_mevp_out_buf_size[i] = temp * devp->buf.me_blk_vsize;
		devp->buf.total_size += devp->buf.norm_mevp_out_buf_size[i];
	}
	pr_frc(log, "norm_mevp_out_buf_size=%d ,line buf:%d all:%d\n",
	       devp->buf.norm_mevp_out_buf_size[0], temp,
	       devp->buf.norm_mevp_out_buf_size[0] * FRC_MEVP_BUF_NUM);

	/*norm iplogo buffer*/
	temp = (devp->buf.logo_hsize * 1 + 511) / 8;
	for (i = 0; i < logo_buf_num; i++) {
		devp->buf.norm_iplogo_buf_size[i] = temp * devp->buf.logo_vsize;
		devp->buf.total_size += devp->buf.norm_iplogo_buf_size[i];
	}
	for (i = logo_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
		devp->buf.norm_iplogo_buf_size[i] = 0;
		devp->buf.total_size += devp->buf.norm_iplogo_buf_size[i];
	}
	pr_frc(log, "norm_iplogo_buf_size=%d ,line buf:%d all:%d\n",
		devp->buf.norm_iplogo_buf_size[0], temp,
		devp->buf.norm_iplogo_buf_size[0] * logo_buf_num);

	/*norm logo irr buffer*/
	temp = (devp->buf.logo_hsize * 6 + 511) / 8;
	devp->buf.norm_logo_irr_buf_size = temp * devp->buf.logo_vsize;
	devp->buf.total_size += devp->buf.norm_logo_irr_buf_size;
	pr_frc(log, "norm_logo_irr_buf_size=%d ,line buf:%d\n",
		devp->buf.norm_logo_irr_buf_size, temp);

	/*norm logo scc buffer*/
	temp = (devp->buf.logo_hsize * 5 + 511) / 8;
	devp->buf.norm_logo_scc_buf_size = temp * devp->buf.logo_vsize;
	devp->buf.total_size += devp->buf.norm_logo_scc_buf_size;
	pr_frc(log, "norm_logo_scc_buf_size=%d ,line buf:%d\n",
		devp->buf.norm_logo_scc_buf_size, temp);

	/*norm melogo buffer*/
	temp = (devp->buf.me_blk_hsize + 1 + 511) / 8;
	for (i = 0; i < logo_buf_num; i++) {
		devp->buf.norm_melogo_buf_size[i] = temp * devp->buf.me_blk_vsize;
		devp->buf.total_size += devp->buf.norm_melogo_buf_size[i];
	}
	for (i = logo_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
		devp->buf.norm_melogo_buf_size[i] = 0;
		devp->buf.total_size += devp->buf.norm_melogo_buf_size[i];
	}
	pr_frc(log, "norm_melogo_buf_size=%d ,line buf:%d all:%d\n",
		devp->buf.norm_melogo_buf_size[0], temp,
		devp->buf.norm_melogo_buf_size[0] * logo_buf_num);

	pr_frc(0, "total_size=%d\n", devp->buf.total_size);

	return 0;
}

int frc_buf_distribute(struct frc_dev_s *devp)
{
	u32 i;
	u32 real_onebuf_size;
	ulong paddr = 0, base;
	int log = 2;
	enum chip_id chip;
	u32 temp;
	u8  frm_buf_num;
	u8  logo_buf_num;

	if (!devp)
		return -1;

	chip = get_chip_type();
	temp = READ_FRC_REG(FRC_FRAME_BUFFER_NUM);
	frm_buf_num = temp & 0x1F;
	logo_buf_num = (temp >> 8) & 0x1F;
	if (frm_buf_num == 0 || logo_buf_num == 0) {
		pr_frc(log, "buf num reg read 0");
		return -1;
	}
	/*----------------- buffer alloc------------------*/
	base = devp->buf.cma_mem_paddr_start;
	/*mc y/c/v me info buffer, address 64 bytes align*/
	devp->buf.lossy_mc_y_info_buf_paddr = paddr;
	pr_frc(log, "lossy_mc_y_info_buf_paddr:0x%lx", paddr);
	paddr += roundup(devp->buf.lossy_mc_y_info_buf_size, ALIGN_4K);
	devp->buf.lossy_mc_c_info_buf_paddr = paddr;
	pr_frc(log, "lossy_mc_c_info_buf_paddr:0x%lx", paddr);
	paddr += roundup(devp->buf.lossy_mc_c_info_buf_size, ALIGN_4K);
	devp->buf.lossy_mc_v_info_buf_paddr = 0;
	pr_frc(log, "lossy_mc_v_info_buf_paddr:0x%lx", paddr);
	paddr += roundup(devp->buf.lossy_mc_v_info_buf_size, ALIGN_4K);
	devp->buf.lossy_me_x_info_buf_paddr = paddr;
	pr_frc(log, "lossy_me_x_info_buf_paddr:0x%lx", paddr);
	paddr += roundup(devp->buf.lossy_me_x_info_buf_size, ALIGN_4K);
	/* t3x */
	if (chip == ID_T3X) {
		devp->buf.lossy_mcdw_y_info_buf_paddr = paddr;
		pr_frc(log, "lossy_mcdw_y_info_buf_paddr:0x%lx", paddr);
		paddr += roundup(devp->buf.lossy_mcdw_y_info_buf_size, ALIGN_4K);
		devp->buf.lossy_mcdw_c_info_buf_paddr = paddr;
		pr_frc(log, "lossy_mcdw_c_info_buf_paddr:0x%lx", paddr);
		paddr += roundup(devp->buf.lossy_mcdw_c_info_buf_size, ALIGN_4K);
	}

	/*lossy lossy_mc_y data buffer*/
	paddr = roundup(paddr, ALIGN_4K * 16);/*secure size need 64K align*/
	real_onebuf_size = roundup(devp->buf.lossy_mc_y_data_buf_size[0], ALIGN_4K);
	for (i = 0; i < frm_buf_num; i++) {
		devp->buf.lossy_mc_y_data_buf_paddr[i] = paddr;
		pr_frc(log, "lossy_mc_y_data_buf_paddr[%d]:0x%lx\n", i, paddr);
		paddr += real_onebuf_size;
	}
	for (i = frm_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
		devp->buf.lossy_mc_y_data_buf_paddr[i] = 0;
		pr_frc(log, "lossy_mc_y_data_buf_paddr[%d]:0x%lx\n", i, 0L);
	}

	/*lossy lossy_mc_c data buffer*/
	real_onebuf_size = roundup(devp->buf.lossy_mc_c_data_buf_size[0], ALIGN_4K);
	for (i = 0; i < frm_buf_num; i++) {
		devp->buf.lossy_mc_c_data_buf_paddr[i] = paddr;
		pr_frc(log, "lossy_mc_c_data_buf_paddr[%d]:0x%lx\n", i, paddr);
		paddr += real_onebuf_size;
	}
	for (i = frm_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
		devp->buf.lossy_mc_c_data_buf_paddr[i] = 0;
		pr_frc(log, "lossy_mc_c_data_buf_paddr[%d]:0x%lx\n", i, 0L);
	}
	/*lossy lossy_mc_v data buffer*/
	//real_onebuf_size = roundup(devp->buf.lossy_mc_v_data_buf_size[0], ALIGN_4K);
	//for (i = 0; i < FRC_TOTAL_BUF_NUM; i++) {
	//	devp->buf.lossy_mc_v_data_buf_paddr[i] = 0;
	//	pr_frc(log, "lossy_mc_v_data_buf_paddr[%d]:0x%lx\n", i, 0);
	//	// paddr += real_onebuf_size;
	//}
	/*lossy lossy_me data buffer*/
	real_onebuf_size = roundup(devp->buf.lossy_me_data_buf_size[0], ALIGN_4K);
	for (i = 0; i < frm_buf_num; i++) {
		devp->buf.lossy_me_data_buf_paddr[i] = paddr;
		pr_frc(log, "lossy_me_data_buf_paddr[%d]:0x%lx\n", i, paddr);
		paddr += real_onebuf_size;
	}
	for (i = frm_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
		devp->buf.lossy_me_data_buf_paddr[i] = 0;
		pr_frc(log, "lossy_me_data_buf_paddr[%d]:0x%lx\n", i, 0L);
	}

	/*t3x lossy lossy_mcdw data buffer*/
	if (chip == ID_T3X) {
		/*t3x lossy lossy_mcdw_c data buffer*/
		real_onebuf_size = roundup(devp->buf.lossy_mcdw_y_data_buf_size[0], ALIGN_4K);
		for (i = 0; i < frm_buf_num; i++) {
			devp->buf.lossy_mcdw_y_data_buf_paddr[i] = paddr;
			pr_frc(log, "lossy_mcdw_y_data_buf_paddr[%d]:0x%lx\n", i, paddr);
			paddr += real_onebuf_size;
		}
		for (i = frm_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
			devp->buf.lossy_mcdw_y_data_buf_paddr[i] = 0;
			pr_frc(log, "lossy_mcdw_y_data_buf_paddr[%d]:0x%lx\n", i, 0L);
		}
		/*t3x lossy lossy_mcdw_c data buffer*/
		real_onebuf_size = roundup(devp->buf.lossy_mcdw_c_data_buf_size[0], ALIGN_4K);
		for (i = 0; i < frm_buf_num; i++) {
			devp->buf.lossy_mcdw_c_data_buf_paddr[i] = paddr;
			pr_frc(log, "lossy_mcdw_c_data_buf_paddr[%d]:0x%lx\n", i, paddr);
			paddr += real_onebuf_size;
		}
		for (i = frm_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
			devp->buf.lossy_mcdw_y_data_buf_paddr[i] = 0;
			pr_frc(log, "lossy_mcdw_c_data_buf_paddr[%d]:0x%lx\n", i, 0L);
		}
	}

	paddr = roundup(paddr, ALIGN_4K * 16);/*secure size need 64K align*/
	/*link buffer*/
	real_onebuf_size = roundup(devp->buf.lossy_mc_y_link_buf_size[0], ALIGN_4K);
	for (i = 0; i < frm_buf_num; i++) {
		devp->buf.lossy_mc_y_link_buf_paddr[i] = paddr;
		pr_frc(log, "lossy_mc_y_link_buf_paddr[%d]:0x%lx\n", i, paddr);
		paddr += real_onebuf_size;
	}
	for (i = frm_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
		devp->buf.lossy_mc_y_link_buf_paddr[i] = 0;
		pr_frc(log, "lossy_mc_y_link_buf_paddr[%d]:0x%lx\n", i, 0L);
	}

	real_onebuf_size = roundup(devp->buf.lossy_mc_c_link_buf_size[0], ALIGN_4K);
	for (i = 0; i < frm_buf_num; i++) {
		devp->buf.lossy_mc_c_link_buf_paddr[i] = paddr;
		pr_frc(log, "lossy_mc_c_link_buf_paddr[%d]:0x%lx\n", i, paddr);
		paddr += real_onebuf_size;
	}
	for (i = frm_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
		devp->buf.lossy_mc_c_link_buf_paddr[i] = 0;
		pr_frc(log, "lossy_mc_c_link_buf_paddr[%d]:0x%lx\n", i, 0L);
	}
	//real_onebuf_size = roundup(devp->buf.lossy_mc_v_link_buf_size[0], ALIGN_4K);
	//for (i = 0; i < FRC_TOTAL_BUF_NUM; i++) {
	//	devp->buf.lossy_mc_v_link_buf_paddr[i] = 0;
	//	pr_frc(log, "lossy_mc_v_link_buf_paddr[%d]:0x%lx\n", i, 0);
	//	// paddr += real_onebuf_size;
	//}

	real_onebuf_size = roundup(devp->buf.lossy_me_link_buf_size[0], ALIGN_4K);
	for (i = 0; i < frm_buf_num; i++) {
		devp->buf.lossy_me_link_buf_paddr[i] = paddr;
		pr_frc(log, "lossy_me_link_buf_paddr[%d]:0x%lx\n", i, paddr);
		paddr += real_onebuf_size;
	}
	for (i = frm_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
		devp->buf.lossy_me_link_buf_paddr[i] = 0;
		pr_frc(log, "lossy_me_link_buf_paddr[%d]:0x%lx\n", i, 0L);
	}

	/*t3x link buffer*/
	if (chip == ID_T3X) {
		real_onebuf_size = roundup(devp->buf.lossy_mcdw_y_link_buf_size[0], ALIGN_4K);
		for (i = 0; i < frm_buf_num; i++) {
			devp->buf.lossy_mcdw_y_link_buf_paddr[i] = paddr;
			pr_frc(log, "lossy_mcdw_y_link_buf_paddr[%d]:0x%lx\n", i, paddr);
			paddr += real_onebuf_size;
		}
		for (i = frm_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
			devp->buf.lossy_mcdw_y_link_buf_paddr[i] = 0;
			pr_frc(log, "lossy_mcdw_y_link_buf_paddr[%d]:0x%lx\n", i, 0L);
		}
		real_onebuf_size = roundup(devp->buf.lossy_mcdw_c_link_buf_size[0], ALIGN_4K);
		for (i = 0; i < frm_buf_num; i++) {
			devp->buf.lossy_mcdw_c_link_buf_paddr[i] = paddr;
			pr_frc(log, "lossy_mcdw_c_link_buf_paddr[%d]:0x%lx\n", i, paddr);
			paddr += real_onebuf_size;
		}
		for (i = frm_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
			devp->buf.lossy_mcdw_c_link_buf_paddr[i] = 0;
			pr_frc(log, "lossy_mcdw_c_link_buf_paddr[%d]:0x%lx\n", i, 0L);
		}
	}
	/*norm buffer*/
	paddr = roundup(paddr, ALIGN_4K * 16);/*secure size need 64K align*/

	/*t3 hme buffer*/
	if (chip == ID_T3) {
		real_onebuf_size = roundup(devp->buf.norm_hme_data_buf_size[0], ALIGN_4K);
		for (i = 0; i < frm_buf_num; i++) {
			devp->buf.norm_hme_data_buf_paddr[i] = paddr;
			pr_frc(log, "norm_hme_data_buf_paddr[%d]:0x%lx\n", i, paddr);
			paddr += real_onebuf_size;
		}
		for (i = frm_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
			devp->buf.norm_hme_data_buf_paddr[i] = 0;
			pr_frc(log, "norm_hme_data_buf_paddr[%d]:0x%lx\n", i, 0L);
		}
	}

	real_onebuf_size = roundup(devp->buf.norm_memv_buf_size[0], ALIGN_4K);
	for (i = 0; i < FRC_MEMV_BUF_NUM; i++) {
		devp->buf.norm_memv_buf_paddr[i] = paddr;
		pr_frc(log, "norm_memv_buf_paddr[%d]:0x%lx\n", i, paddr);
		paddr += real_onebuf_size;
	}

	/*t3 hmemv buffer*/
	if (chip == ID_T3) {
		real_onebuf_size = roundup(devp->buf.norm_hmemv_buf_size[0], ALIGN_4K);
		for (i = 0; i < FRC_MEMV2_BUF_NUM; i++) {
			devp->buf.norm_hmemv_buf_paddr[i] = paddr;
			pr_frc(log, "norm_hmemv_buf_paddr[%d]:0x%lx\n", i, paddr);
			paddr += real_onebuf_size;
		}
	}

	real_onebuf_size = roundup(devp->buf.norm_mevp_out_buf_size[0], ALIGN_4K);
	for (i = 0; i < FRC_MEVP_BUF_NUM; i++) {
		devp->buf.norm_mevp_out_buf_paddr[i] = paddr;
		pr_frc(log, "norm_mevp_out_buf_paddr[%d]:0x%lx\n", i, paddr);
		paddr += real_onebuf_size;
	}

	/*ip logo*/
	paddr = roundup(paddr, ALIGN_4K);
	real_onebuf_size = roundup(devp->buf.norm_iplogo_buf_size[0], ALIGN_4K);
	for (i = 0; i < logo_buf_num; i++) {
		devp->buf.norm_iplogo_buf_paddr[i] = paddr;
		pr_frc(log, "norm_iplogo_buf_paddr[%d]:0x%lx\n", i, paddr);
		paddr += real_onebuf_size;
	}
	for (i = logo_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
		devp->buf.norm_iplogo_buf_paddr[i] = 0;
		pr_frc(log, "norm_iplogo_buf_paddr[%d]:0x%lx\n", i, 0L);
	}

	/*logo irr*/
	paddr = roundup(paddr, ALIGN_4K);
	real_onebuf_size = roundup(devp->buf.norm_logo_irr_buf_size, ALIGN_4K);
	devp->buf.norm_logo_irr_buf_paddr = paddr;
	pr_frc(log, "norm_logo_irr_buf_paddr:0x%lx\n", paddr);
	paddr += real_onebuf_size;

	/*logo ssc*/
	paddr = roundup(paddr, ALIGN_4K);
	real_onebuf_size = roundup(devp->buf.norm_logo_scc_buf_size, ALIGN_4K);
	devp->buf.norm_logo_scc_buf_paddr = paddr;
	pr_frc(log, "norm_logo_scc_buf_paddr:0x%lx\n", paddr);
	paddr += real_onebuf_size;

	/*melogo*/
	paddr = roundup(paddr, ALIGN_4K);
	real_onebuf_size = roundup(devp->buf.norm_melogo_buf_size[0], ALIGN_4K);
	for (i = 0; i < logo_buf_num; i++) {
		devp->buf.norm_melogo_buf_paddr[i] = paddr;
		pr_frc(log, "norm_melogo_buf_paddr[%d]:0x%lx\n", i, paddr);
		paddr += real_onebuf_size;
	}
	for (i = logo_buf_num; i < FRC_TOTAL_BUF_NUM; i++) {
		devp->buf.norm_melogo_buf_paddr[i] = 0;
		pr_frc(log, "norm_melogo_buf_paddr[%d]:0x%lx\n", i, 0L);
	}

	paddr = roundup(paddr, ALIGN_4K * 16);

	devp->buf.real_total_size = paddr;

	if (chip == ID_T3) {
		devp->buf.link_tab_size = devp->buf.norm_hme_data_buf_paddr[0] -
				devp->buf.lossy_mc_y_link_buf_paddr[0];
	} else {
		devp->buf.link_tab_size = devp->buf.norm_memv_buf_paddr[0] -
				devp->buf.lossy_mc_y_link_buf_paddr[0];
	}

	/*data  secure buffer */
	devp->buf.secure_start = devp->buf.cma_mem_paddr_start +
					devp->buf.lossy_mc_y_data_buf_paddr[0];
	devp->buf.secure_size = devp->buf.lossy_mc_y_link_buf_paddr[0] -
				devp->buf.lossy_mc_y_data_buf_paddr[0];

	if (devp->buf.link_tab_size <= 0) {
		pr_frc(0, "buf err: link buffer size %d\n", devp->buf.link_tab_size);
		return -1;
	}
	if (devp->buf.secure_size <= 0) {
		pr_frc(0, "buf err: secure buffer size %d\n", devp->buf.secure_size);
		return -1;
	}

	if (devp->buf.real_total_size > devp->buf.cma_mem_size) {
		pr_frc(0, "buf err: need %d, cur size:%d\n", (u32)paddr,
					devp->buf.cma_mem_size);
		return -1;
	}
	pr_frc(0, "%s base:0x%lx  real_total_size:0x%x(%d)\n",
			__func__, base, (u32)paddr, (u32)paddr);
	return 0;
}

/*
 * config data link buffer
 */
int frc_buf_mapping_tab_init(struct frc_dev_s *devp)
{
	u32 i, j; //  k = 0;
	phys_addr_t cma_paddr = 0;
	dma_addr_t paddr;
	u8 *cma_vaddr = 0;
	ulong vmap_offset_start = 0, vmap_offset_end;
	u32 *linktab_vaddr = NULL;
	u8 *p = NULL;
	ulong data_buf_addr;
	s32 link_tab_all_size, data_buf_size;
	u32 log = 2;
	u32 shft_bits;
	//u32 *init_start_addr;
	enum chip_id chip;
	u32 temp;
	u8  frm_buf_num;
	u8  logo_buf_num;

	if (!devp)
		return -1;
	chip = get_chip_type();
	shft_bits = devp->buf.addr_shft_bits;
	pr_frc(log, "%s get ddr shift bits:0x%x\n", __func__, shft_bits);

	temp = READ_FRC_REG(FRC_FRAME_BUFFER_NUM);
	frm_buf_num = temp & 0x1F;
	logo_buf_num = (temp >> 8) & 0x1F;
	if (frm_buf_num == 0 || logo_buf_num == 0) {
		pr_frc(log, "buf num reg read 0");
		return -1;
	}
	cma_paddr = devp->buf.cma_mem_paddr_start;
	link_tab_all_size = devp->buf.link_tab_size;
	pr_frc(log, "paddr start:0x%lx, link start=0x%lx - 0x%lx, size:0x%x\n",
			(ulong)devp->buf.cma_mem_paddr_start,
			(ulong)devp->buf.lossy_mc_y_link_buf_paddr[0],
			(ulong)(chip <= ID_T3 ? devp->buf.norm_hme_data_buf_paddr[0] :
			devp->buf.norm_memv_buf_paddr[0]),
			link_tab_all_size);
	if (link_tab_all_size <= 0) {
		pr_frc(0, "link buf err\n");
		return -1;
	}
	vmap_offset_start = devp->buf.lossy_mc_y_link_buf_paddr[0];
	vmap_offset_end = chip <= ID_T3 ? devp->buf.norm_hme_data_buf_paddr[0] :
						devp->buf.norm_memv_buf_paddr[0];
	cma_vaddr = frc_buf_vmap(cma_paddr, vmap_offset_end);
	pr_frc(0, "map: paddr=0x%lx, vaddr=0x%lx, link tab size=0x%x (%d)\n",
		(ulong)cma_paddr, (ulong)cma_vaddr, link_tab_all_size, link_tab_all_size);

	//init_start_addr = (u32 *)(cma_vaddr + vmap_offset_start);
	//for (i = 0; i < link_tab_all_size; i++) {
	//	*init_start_addr = cma_paddr + devp->buf.lossy_mc_y_data_buf_paddr[0];
	//	init_start_addr++;
	//}
	memset(cma_vaddr + vmap_offset_start, 0, link_tab_all_size);

	/*split data buffer and fill to link mc buffer: mc y*/
	data_buf_size = roundup(devp->buf.lossy_mc_y_data_buf_size[0], ALIGN_4K);
	if (data_buf_size > 0) {
		pr_frc(log, "lossy_mc_y_data_buf_size:0x%x (%d)\n", data_buf_size, data_buf_size);
		for (i = 0; i < frm_buf_num; i++) {
			p = cma_vaddr + (devp->buf.lossy_mc_y_link_buf_paddr[i]);
			paddr = cma_paddr + devp->buf.lossy_mc_y_link_buf_paddr[i];
			linktab_vaddr = (u32 *)p;
			data_buf_addr =
			(cma_paddr + devp->buf.lossy_mc_y_data_buf_paddr[i]) & 0xfffffffff;
			// k = 0;
			for (j = 0; j < data_buf_size; j += ALIGN_4K) {
				*linktab_vaddr = (data_buf_addr + j) >> shft_bits;
				linktab_vaddr++;
				// k += 4;
			}
			frc_buf_dma_flush(devp, p, paddr, data_buf_size, DMA_TO_DEVICE);
			//pr_frc(log, "link buf size:%d, data div4k size= %d\n",
			//       devp->buf.lossy_mc_y_link_buf_size[i], k);
		}
	}

	data_buf_size = roundup(devp->buf.lossy_mc_c_data_buf_size[0], ALIGN_4K);
	if (data_buf_size > 0) {
		pr_frc(log, "lossy_mc_c_data_buf_size:0x%x (%d)\n", data_buf_size, data_buf_size);
		for (i = 0; i < frm_buf_num; i++) {
			p = cma_vaddr + devp->buf.lossy_mc_c_link_buf_paddr[i];
			paddr = (cma_paddr + devp->buf.lossy_mc_y_link_buf_paddr[i]);
			linktab_vaddr = (u32 *)p;
			data_buf_addr =
			(cma_paddr + devp->buf.lossy_mc_c_data_buf_paddr[i]) & 0xfffffffff;
			// k = 0;
			for (j = 0; j < data_buf_size; j += ALIGN_4K) {
				*linktab_vaddr = (data_buf_addr + j) >> shft_bits;
				linktab_vaddr++;
				// k += 4;
			}
			frc_buf_dma_flush(devp, p, paddr, data_buf_size, DMA_TO_DEVICE);
			//pr_frc(log, "link buf size:%d, data div4k size= %d\n",
			//       devp->buf.lossy_mc_y_link_buf_size[i], k);
		}
	}

	//data_buf_size = roundup(devp->buf.lossy_mc_v_data_buf_size[0], ALIGN_4K);
	//if (data_buf_size > 0) {
	//	pr_frc(log, "lossy_mc_v_data_buf_size:0x%x (%d)\n", data_buf_size, data_buf_size);
	//	for (i = 0; i < frm_buf_num; i++) {
	//		p = cma_vaddr + devp->buf.lossy_mc_v_link_buf_paddr[i];
	//		paddr = cma_paddr + devp->buf.lossy_mc_y_link_buf_paddr[i];
	//		linktab_vaddr = (u32 *)p;
	//		data_buf_addr =
	//		(cma_paddr + devp->buf.lossy_mc_v_data_buf_paddr[i]) & 0xfffffffff;
	//		// k = 0;
	//		for (j = 0; j < data_buf_size; j += ALIGN_4K) {
	//			*linktab_vaddr = (data_buf_addr + j) >> shft_bits;
	//			linktab_vaddr++;
	//			// k += 4;
	//		}
	//		frc_buf_dma_flush(devp, p, paddr, data_buf_size, DMA_TO_DEVICE);
	//		//pr_frc(log, "link buf size:%d, data div4k size= %d\n",
	//		//       devp->buf.lossy_mc_y_link_buf_size[i], k);
	//	}
	//}

	data_buf_size = roundup(devp->buf.lossy_me_data_buf_size[0], ALIGN_4K);
	if (data_buf_size > 0) {
		pr_frc(log, "lossy_me_data_buf_size:0x%x (%d)\n", data_buf_size, data_buf_size);
		for (i = 0; i < frm_buf_num; i++) {
			p = cma_vaddr + devp->buf.lossy_me_link_buf_paddr[i];
			paddr = (cma_paddr + devp->buf.lossy_mc_y_link_buf_paddr[i]);
			linktab_vaddr = (u32 *)p;
			data_buf_addr =
			(cma_paddr + devp->buf.lossy_me_data_buf_paddr[i]) & 0xfffffffff;
			// k = 0;
			for (j = 0; j < data_buf_size; j += ALIGN_4K) {
				*linktab_vaddr = (data_buf_addr + j) >> shft_bits;
				linktab_vaddr++;
				// k += 4;
			}
			frc_buf_dma_flush(devp, p, paddr, data_buf_size, DMA_TO_DEVICE);
			//pr_frc(log, "link buf size:%d, data div4k size= %d\n",
			//       devp->buf.lossy_mc_y_link_buf_size[i], k);
		}
	}
	/*split data buffer and fill to link mcdw buffer: mcdw y*/
	if (chip == ID_T3X) {
		data_buf_size = roundup(devp->buf.lossy_mcdw_y_data_buf_size[0], ALIGN_4K);
		if (data_buf_size > 0) {
			pr_frc(log, "lossy_mcdw_y_data_buf_size:0x%x (%d)\n",
					data_buf_size, data_buf_size);
			for (i = 0; i < frm_buf_num; i++) {
				p = cma_vaddr + (devp->buf.lossy_mcdw_y_link_buf_paddr[i]);
				paddr = cma_paddr + (devp->buf.lossy_mc_y_link_buf_paddr[i]);
				linktab_vaddr = (u32 *)p;
				data_buf_addr =
					cma_paddr + devp->buf.lossy_mcdw_y_data_buf_paddr[i];
				data_buf_addr &= 0xfffffffff;
				// k = 0;
				for (j = 0; j < data_buf_size; j += ALIGN_4K) {
					*linktab_vaddr = (data_buf_addr + j) >> shft_bits;
					linktab_vaddr++;
					// k += 4;
				}
				frc_buf_dma_flush(devp, p, paddr, data_buf_size, DMA_TO_DEVICE);
				//pr_frc(log, "link buf size:%d, data div4k size= %d\n",
				//       devp->buf.lossy_mc_y_link_buf_size[i], k);
			}
		}
		data_buf_size = roundup(devp->buf.lossy_mcdw_c_data_buf_size[0], ALIGN_4K);
		if (data_buf_size > 0) {
			pr_frc(log, "lossy_mcdw_c_data_buf_size:0x%x (%d)\n",
				data_buf_size, data_buf_size);
			for (i = 0; i < frm_buf_num; i++) {
				p = cma_vaddr + devp->buf.lossy_mcdw_c_link_buf_paddr[i];
				paddr = cma_paddr + devp->buf.lossy_mc_y_link_buf_paddr[i];
				linktab_vaddr = (u32 *)p;
				data_buf_addr =
					cma_paddr + devp->buf.lossy_mcdw_c_data_buf_paddr[i];
				data_buf_addr &= 0xfffffffff;
				// k = 0;
				for (j = 0; j < data_buf_size; j += ALIGN_4K) {
					*linktab_vaddr = (data_buf_addr + j) >> shft_bits;
					linktab_vaddr++;
					// k += 4;
				}
				//pr_frc(log, "link buf size:%d, data div4k size= %d\n",
				//       devp->buf.lossy_mc_y_link_buf_size[i], k);
				frc_buf_dma_flush(devp, p, paddr, data_buf_size, DMA_TO_DEVICE);
			}
		}
	}
	frc_buf_unmap((u32 *)cma_vaddr);
	return 0;
}

/*
 * config buffer address to every hw buffer register
 *
 */
int frc_buf_config(struct frc_dev_s *devp)
{
	u32 i = 0;
	ulong base;
	u32 log = 2;
	u32 shft_bits = 0;
	enum chip_id chip;
	u32 temp;
	u8  frm_buf_num;
	u8  logo_buf_num;

	if (!devp) {
		pr_frc(0, "%s fail<devp is null>\n", __func__);
		return -1;
	} else if (!devp->buf.cma_mem_alloced) {
		pr_frc(0, "%s fail <cma alloced:%d>\n", __func__, devp->buf.cma_mem_alloced);
		return -1;
	} else if (!devp->buf.cma_mem_paddr_start) {
		pr_frc(0, "%s fail <cma_paddr is null>\n", __func__);
		return -1;
	}
	chip = get_chip_type();
	temp = READ_FRC_REG(FRC_FRAME_BUFFER_NUM);
	frm_buf_num = temp & 0x1F;
	logo_buf_num = (temp >> 8) & 0x1F;
	if (frm_buf_num == 0 || logo_buf_num == 0) {
		pr_frc(log, "buf num reg read 0");
		return -1;
	}
	base = devp->buf.cma_mem_paddr_start;
	shft_bits = devp->buf.addr_shft_bits;
	pr_frc(log, "%s cma base:0x%lx, shift_bits:%d\n", __func__, base, shft_bits);
	/*mc info buffer*/
	WRITE_FRC_REG_BY_CPU(FRC_REG_MC_YINFO_BADDR,
			(base + devp->buf.lossy_mc_y_info_buf_paddr) >> shft_bits);
	WRITE_FRC_REG_BY_CPU(FRC_REG_MC_CINFO_BADDR,
			(base + devp->buf.lossy_mc_c_info_buf_paddr) >> shft_bits);
	WRITE_FRC_REG_BY_CPU(FRC_REG_MC_VINFO_BADDR, 0);
	WRITE_FRC_REG_BY_CPU(FRC_REG_ME_XINFO_BADDR,
			(base + devp->buf.lossy_me_x_info_buf_paddr) >> shft_bits);
	/*mcdw info buffer*/
	if (chip == ID_T3X) {
		WRITE_FRC_REG_BY_CPU(FRC_REG_MCDW_YINFO_BADDR,
			(base + devp->buf.lossy_mcdw_y_info_buf_paddr) >> shft_bits);
		WRITE_FRC_REG_BY_CPU(FRC_REG_MCDW_CINFO_BADDR,
			(base + devp->buf.lossy_mcdw_c_info_buf_paddr) >> shft_bits);
	}
	/*lossy mc y,c,v data buffer, data buffer needn't config*/
	/*lossy me data buffer, data buffer needn't config*/

	/*lossy mc link buffer*/
	for (i = FRC_REG_MC_YBUF_ADDRX_0; i < FRC_REG_MC_YBUF_ADDRX_0 + frm_buf_num; i++) {
		WRITE_FRC_REG_BY_CPU(i,
	(base + devp->buf.lossy_mc_y_link_buf_paddr[i - FRC_REG_MC_YBUF_ADDRX_0]) >> shft_bits);
	}
	for (i = FRC_REG_MC_CBUF_ADDRX_0; i < FRC_REG_MC_CBUF_ADDRX_0 + frm_buf_num; i++)
		WRITE_FRC_REG_BY_CPU(i,
	(base + devp->buf.lossy_mc_c_link_buf_paddr[i - FRC_REG_MC_CBUF_ADDRX_0]) >> shft_bits);

	for (i = FRC_REG_MC_VBUF_ADDRX_0; i < FRC_REG_MC_VBUF_ADDRX_0 + frm_buf_num; i++)
		WRITE_FRC_REG_BY_CPU(i, 0);
	/*lossy me link buffer*/
	for (i = FRC_REG_ME_BUF_ADDRX_0; i < FRC_REG_ME_BUF_ADDRX_0 + frm_buf_num; i++)
		WRITE_FRC_REG_BY_CPU(i,
	(base + devp->buf.lossy_me_link_buf_paddr[i - FRC_REG_ME_BUF_ADDRX_0]) >> shft_bits);

	/*lossy mcdw link buffer*/
	if (chip == ID_T3X) {
		for (i = FRC_REG_MCDW_YBUF_ADDRX_0;
			i < FRC_REG_MCDW_YBUF_ADDRX_0 + frm_buf_num;
			i++) {
			WRITE_FRC_REG_BY_CPU(i, (base +
		devp->buf.lossy_mcdw_y_link_buf_paddr[i - FRC_REG_MCDW_YBUF_ADDRX_0]) >> shft_bits);
		}
		for (i = FRC_REG_MCDW_CBUF_ADDRX_0;
			i < FRC_REG_MCDW_CBUF_ADDRX_0 + frm_buf_num;
			i++)
			WRITE_FRC_REG_BY_CPU(i, (base +
		devp->buf.lossy_mcdw_c_link_buf_paddr[i - FRC_REG_MCDW_CBUF_ADDRX_0]) >> shft_bits);
	}
	/*t3 norm hme data buffer*/
	if (chip == ID_T3)
		for (i = FRC_REG_HME_BUF_ADDRX_0;
			i < FRC_REG_HME_BUF_ADDRX_0 + frm_buf_num; i++)
			WRITE_FRC_REG_BY_CPU(i, (base +
		devp->buf.norm_hme_data_buf_paddr[i - FRC_REG_HME_BUF_ADDRX_0]) >> shft_bits);
	else
		for (i = FRC_REG_HME_BUF_ADDRX_0;
			i < FRC_REG_HME_BUF_ADDRX_0 + frm_buf_num; i++)
			WRITE_FRC_REG_BY_CPU(i, 0);
	// clear addr value 0
	for (i = FRC_REG_MC_YBUF_ADDRX_0 + frm_buf_num; i <= FRC_REG_MC_YBUF_ADDRX_15; i++)
		WRITE_FRC_REG_BY_CPU(i, 0);
	for (i = FRC_REG_MC_CBUF_ADDRX_0 + frm_buf_num; i <= FRC_REG_MC_CBUF_ADDRX_15; i++)
		WRITE_FRC_REG_BY_CPU(i, 0);
	for (i = FRC_REG_MC_VBUF_ADDRX_0 + frm_buf_num; i <= FRC_REG_MC_VBUF_ADDRX_15; i++)
		WRITE_FRC_REG_BY_CPU(i, 0);
	/*lossy me link buffer*/
	for (i = FRC_REG_ME_BUF_ADDRX_0 + frm_buf_num; i <= FRC_REG_ME_BUF_ADDRX_15; i++)
		WRITE_FRC_REG_BY_CPU(i, 0);
	for (i = FRC_REG_MCDW_YBUF_ADDRX_0 + frm_buf_num; i <= FRC_REG_MCDW_YBUF_ADDRX_15; i++)
		WRITE_FRC_REG_BY_CPU(i, 0);
	for (i = FRC_REG_MCDW_CBUF_ADDRX_0 + frm_buf_num; i <= FRC_REG_MCDW_CBUF_ADDRX_15; i++)
		WRITE_FRC_REG_BY_CPU(i, 0);
	/*t3 norm hme data buffer*/
	if (chip == ID_T3) {
		for (i = FRC_REG_HME_BUF_ADDRX_0 + frm_buf_num; i <= FRC_REG_HME_BUF_ADDRX_15; i++)
			WRITE_FRC_REG_BY_CPU(i, 0);
	} else {
		for (i = FRC_REG_HME_BUF_ADDRX_0 + frm_buf_num; i <= FRC_REG_HME_BUF_ADDRX_15; i++)
			WRITE_FRC_REG_BY_CPU(i, 0);
	}
	/*norm memv buffer*/
	for (i = FRC_REG_ME_NC_UNI_MV_ADDRX_0; i <= FRC_REG_ME_PC_PHS_MV_ADDR; i++)
		WRITE_FRC_REG_BY_CPU(i,
	(base + devp->buf.norm_memv_buf_paddr[i - FRC_REG_ME_NC_UNI_MV_ADDRX_0]) >> shft_bits);
	/*t3 norm hmemv buffer*/
	if (chip == ID_T3) {
		for (i = FRC_REG_HME_NC_UNI_MV_ADDRX_0; i <= FRC_REG_VP_PF_UNI_MV_ADDR; i++)
			WRITE_FRC_REG_BY_CPU(i, (base +
		devp->buf.norm_hmemv_buf_paddr[i - FRC_REG_HME_NC_UNI_MV_ADDRX_0]) >> shft_bits);
	} else {
		for (i = FRC_REG_HME_NC_UNI_MV_ADDRX_0; i <= FRC_REG_VP_PF_UNI_MV_ADDR; i++)
			WRITE_FRC_REG_BY_CPU(i, 0);
	}

	/*norm mevp buffer*/
	for (i = FRC_REG_VP_MC_MV_ADDRX_0; i <= FRC_REG_VP_MC_MV_ADDRX_1; i++)
		WRITE_FRC_REG_BY_CPU(i,
	(base + devp->buf.norm_mevp_out_buf_paddr[i - FRC_REG_VP_MC_MV_ADDRX_0]) >> shft_bits);

	/*norm iplogo buffer*/
	for (i = FRC_REG_IP_LOGO_ADDRX_0; i < FRC_REG_IP_LOGO_ADDRX_0 + logo_buf_num; i++)
		WRITE_FRC_REG_BY_CPU(i,
		(base + devp->buf.norm_iplogo_buf_paddr[i - FRC_REG_IP_LOGO_ADDRX_0]) >> shft_bits);
	for (i = FRC_REG_IP_LOGO_ADDRX_0 + logo_buf_num; i <= FRC_REG_IP_LOGO_ADDRX_15; i++)
		WRITE_FRC_REG_BY_CPU(i, 0);

	/*norm logo irr buffer*/
	WRITE_FRC_REG_BY_CPU(FRC_REG_LOGO_IIR_BUF_ADDR,
		(base + devp->buf.norm_logo_irr_buf_paddr) >> shft_bits);

	/*norm logo scc buffer*/
	WRITE_FRC_REG_BY_CPU(FRC_REG_LOGO_SCC_BUF_ADDR,
		(base + devp->buf.norm_logo_scc_buf_paddr) >> shft_bits);

	/*norm iplogo buffer*/
	for (i = FRC_REG_ME_LOGO_ADDRX_0; i < FRC_REG_ME_LOGO_ADDRX_0 + logo_buf_num; i++)
		WRITE_FRC_REG_BY_CPU(i,
		(base + devp->buf.norm_melogo_buf_paddr[i - FRC_REG_ME_LOGO_ADDRX_0]) >> shft_bits);
	for (i = FRC_REG_ME_LOGO_ADDRX_0 + logo_buf_num; i <= FRC_REG_ME_LOGO_ADDRX_15; i++)
		WRITE_FRC_REG_BY_CPU(i, 0);
	frc_buf_mapping_tab_init(devp);

	//pr_frc(0, "%s success!\n", __func__);
	return 0;
}

