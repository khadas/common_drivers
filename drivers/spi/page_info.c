// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include "page_info.h"

struct boot_info *page_info;

unsigned char page_info_get_data_lanes_mode(void)
{
	return page_info->dev_cfg0.bus_width & 0x0f;
}

unsigned char page_info_get_cmd_lanes_mode(void)
{
	return page_info->dev_cfg1.ca_lanes & 0x0f;
}

unsigned char page_info_get_addr_lanes_mode(void)
{
	return (page_info->dev_cfg1.ca_lanes >> 4) & 0x0f;
}

unsigned char page_info_get_frequency_index(void)
{
	return page_info->host_cfg.frequency_index;
}

unsigned char page_info_get_adj_index(void)
{
	return page_info->host_cfg.mode_rx_adj & 0x3f;
}

unsigned char page_info_get_work_mode(void)
{
	return (page_info->host_cfg.mode_rx_adj >> 6) & 0x3;
}

unsigned char page_info_get_line_delay1(void)
{
	return page_info->host_cfg.lines_delay[0];
}

unsigned char page_info_get_line_delay2(void)
{
	return page_info->host_cfg.lines_delay[1];
}

unsigned char page_info_get_core_div(void)
{
	return page_info->host_cfg.core_div;
}

unsigned char page_info_get_bus_cycle(void)
{
	return page_info->host_cfg.bus_cycle;
}

unsigned char page_info_get_device_ecc_disable(void)
{
	return page_info->host_cfg.device_ecc_disable & 0x01;
}

unsigned int page_info_get_n2m_command(void)
{
	return page_info->host_cfg.n2m_cmd;
}

unsigned int page_info_get_page_size(void)
{
	return page_info->dev_cfg0.page_size;
}

unsigned char page_info_get_planes(void)
{
	return  page_info->dev_cfg0.planes_per_lun & 0x0f;
}

unsigned char page_info_get_plane_shift(void)
{
	return (page_info->dev_cfg0.planes_per_lun >> 4) & 0x0f;
}

unsigned char page_info_get_cache_plane_shift(void)
{
	return (page_info->dev_cfg0.bus_width >> 4) & 0x0f;
}

unsigned char page_info_get_cs_deselect_time(void)
{
	return page_info->dev_cfg1.cs_deselect_time;
}

unsigned char page_info_get_dummy_cycles(void)
{
	return page_info->dev_cfg1.dummy_cycles;
}

unsigned int page_info_get_block_size(void)
{
	return page_info->dev_cfg1.block_size;
}

unsigned short *page_info_get_bbt(void)
{
	return &page_info->dev_cfg1.bbt[0];
}

unsigned char page_info_get_enable_bbt(void)
{
	return page_info->dev_cfg1.enable_bbt;
}

unsigned char page_info_get_high_speed_mode(void)
{
	return page_info->dev_cfg1.high_speed_mode;
}

unsigned char page_info_get_layout_method(void)
{
	return page_info->boot_layout.layout_method;
}

unsigned int page_info_get_boot_size(void)
{
	return page_info->boot_layout.boot_size;
}

unsigned int page_info_get_pages_in_block(void)
{
	unsigned int block_size, page_size;
	static unsigned int pages_in_block;

	if (pages_in_block)
		return pages_in_block;

	block_size = page_info_get_block_size();
	page_size = page_info_get_page_size();
	pages_in_block = block_size / page_size;

	return pages_in_block;
}

unsigned int page_info_get_pages_in_boot(void)
{
	unsigned int page_size, boot_size;

	page_size = page_info_get_page_size();
	boot_size = page_info_get_boot_size();

	return boot_size / page_size;
}

void page_info_initialize(unsigned int default_n2m,
			  unsigned char bus_width, unsigned char ca)
{
	memset((unsigned char *)page_info,
		0, sizeof(struct boot_info));
	page_info->dev_cfg0.page_size = sizeof(struct boot_info);
	page_info->dev_cfg0.planes_per_lun = 0;
	page_info->dev_cfg0.bus_width = bus_width;
	page_info->host_cfg.frequency_index = 0xFF;
	page_info->host_cfg.n2m_cmd = default_n2m;
	page_info->dev_cfg1.ca_lanes = ca;
	page_info->dev_cfg1.cs_deselect_time = 0xFF;
	page_info->dev_cfg1.dummy_cycles = 0xFF;
}

#ifdef __PXP_DEBUG__
static void page_info_dump_info(void)
{
	unsigned char planes_per_lun, plane_shift, bus_width, cache_plane_shift;
	unsigned char high_speed_mode, cmd_lanes, addr_lanes;
	unsigned char enable_bbt;
	unsigned int block_size, page_size;
	unsigned char frequency_index, mode, rx_adj;
	unsigned char device_ecc_disable = 0;
	unsigned int n2m_cmd;

	planes_per_lun = page_info_get_planes();
	plane_shift = page_info_get_plane_shift();
	cache_plane_shift = page_info_get_cache_plane_shift();
	high_speed_mode = page_info_get_high_speed_mode();
	page_size = page_info_get_page_size();
	block_size = page_info_get_block_size();
	enable_bbt = page_info_get_enable_bbt();
	bus_width = page_info_get_data_lanes_mode();
	cmd_lanes = page_info_get_cmd_lanes_mode();
	addr_lanes = page_info_get_addr_lanes_mode();

	frequency_index = page_info_get_frequency_index();
	mode = page_info_get_work_mode();
	rx_adj = page_info_get_adj_index();
	device_ecc_disable = page_info_get_device_ecc_disable();
	n2m_cmd = page_info_get_n2m_command();

	pr_info("bus_width: 0x%x\n", bus_width);
	pr_info("cmd_lanes: 0x%x\n", cmd_lanes);
	pr_info("addr_lanes: 0x%x\n", addr_lanes);
	pr_info("page_size: 0x%x\n", page_size);
	pr_info("planes_per_lun: 0x%x\n", planes_per_lun);
	pr_info("plane_shift: 0x%x\n", plane_shift);
	pr_info("cache_plane_shift: 0x%x\n", cache_plane_shift);
	pr_info("block_size: 0x%x\n", block_size);
	pr_info("high_speed_mode: 0x%x\n", high_speed_mode);
	pr_info("enable_bbt: 0x%x\n", enable_bbt);

	pr_info("frequency_index: 0x%x\n", frequency_index);
	pr_info("mode: 0x%x\n", mode);
	pr_info("rx_adj: 0x%x\n", rx_adj);
	pr_info("device_ecc_disable: 0x%x\n", device_ecc_disable);
	pr_info("n2m_cmd: 0x%x\n", n2m_cmd);
}
#endif

int page_info_pre_init(u8 *boot_info)
{
	page_info = (struct boot_info *)boot_info;
	page_info_initialize(DEFAULT_ECC_MODE, 0, 0);
	return 0;
}