/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _HOST_MODULE_H
#define _HOST_MODULE_H
#include <linux/platform_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/miscdevice.h>
#include <linux/amlogic/scpi_protocol.h>

#define SMC_REMAP_CMD 0x82000096
#define SMC_BOOT_CMD  0x82000090

struct host_module;
struct host_info_t;
struct host_data;

struct host_data {
	struct miscdevice *misc;
	char name[20];
	u8 hostid;
} __packed;

struct host_info_t {
	char id;			/*dsp_id 0,1,2...*/
	char fw_id;
	char fw_name[32];	/*name of firmware which used for dsp*/
	char fw1_name[32];	/*name of firmware which used for dsp ddr*/
	char fw2_name[32];	/*name of firmware which used for dsp sram*/
	unsigned int phy_addr;/*phy address of firmware will be loaded on*/
	unsigned int size;		/*size of reserved hifi memory*/
} __packed;

struct host_shm_info_t {
	unsigned int addr;
	unsigned int size;
} __packed;

#define HOSTFW_NAME_LEN                  30
#define REG_DSP_CFG0			(0x0)
#define REG_DSP_CFG1			(0x4)
#define REG_DSP_CFG2			(0x8)
#define REG_DSP_RESET_VEC		(0x004 << 2)
#define DSP_OTP				(0x03000000)

#define HOST_IOC_MAGIC  'H'

#define HOST_LOAD	_IOWR(HOST_IOC_MAGIC, 1, struct host_info_t)
#define HOST_START	_IOWR(HOST_IOC_MAGIC, 3, struct host_info_t)
#define HOST_STOP	_IOWR(HOST_IOC_MAGIC, 4, struct host_info_t)
#define HOST_GET_INFO	_IOWR((HOST_IOC_MAGIC), (18), \
					struct host_info_t)
#define HOST_SHM_CLEAN \
		_IOWR(HOST_IOC_MAGIC, 64, struct host_shm_info_t)
#define HOST_SHM_INV _IOWR(HOST_IOC_MAGIC, 65, struct host_shm_info_t)

/** struct host_module, the struct of host module
 * @base_reg:         Base register of host
 * @health_reg:       Health monitor register
 * @sram_addr:        Sram virtual address
 * @phys_sram_addr:   Sram physical address
 * @phys_sram_size:   Sram physical address size
 * @ddr_addr:         Ddr virtual address
 * @shm_addr:         Share memory address
 * @phys_ddr_addr:    Ddr physical address
 * @phys_ddr_size:    Ddr physical address size
 * @phys_shm_addr:    Share memory physical address
 * @phys_shm_size:    Share memory physical address size
 * @phys_remap_addr:  Ddr physical remap address
 * @phys_remap_size:  Ddr physical remap size
 * @start_pos:        Boot from sram, ddr or both
 * @logbuff_polling_period_ms:    Host logbuff polling period
 * @is_health_monitor:If support Host health monitor
 * @is_sram_remap:    Instruct long call support
 * @clk:              Host clock
 * @clk_rate:         Host clock rate
 * @pm_support:       If support power management
 * @mbox_chan:        Mbox channel, reserve for mbox development
 * @misc:             Misc device
 * @hostid:           Host id
 * @fname0:           Host firmware name0
 * @fname1:           Host firmware name1
 * @suspended:        Host suspend flag
 * @pre_cnt:          Host health monitor last value
 * @cur_cnt:          Host health monitor current value
 * @started:          Host suspend flag
 * @init_mbox_chan:   Aocpu mbox chan
 * @hang:             Host hang flag
 * @firmware_load:    Host firmware is load
 * @nb:               Host die notifier
 */
struct host_module {
	struct device *dev;
	void __iomem *base_reg;
	void __iomem *health_reg;
	void __iomem *dspsup_reg;
	void __iomem *sram_addr;
	phys_addr_t  phys_sram_addr;
	phys_addr_t  phys_sram_size;
	void __iomem *ddr_addr;
	void __iomem *shm_addr;
	phys_addr_t  phys_ddr_addr;
	phys_addr_t  phys_ddr_size;
	phys_addr_t  phys_shm_addr;
	phys_addr_t  phys_shm_size;
	phys_addr_t  phys_remap_addr;
	phys_addr_t  phys_remap_size;
	u8 start_pos;
	u32 logbuff_polling_ms;
	u32 health_polling_ms;
	bool health_monitor;
	bool addr_remap;
	struct clk *clk;
	u32 clk_rate;
	bool pm_support;
	struct mbox_chan *mbox_chan;
	struct mbox_chan *init_mbox_chan;
	struct miscdevice *misc;
	int hostid;
	char fname0[HOSTFW_NAME_LEN];
	char fname1[HOSTFW_NAME_LEN];

	u32 pre_cnt, cur_cnt;
	struct delayed_work host_monitor_work;
	struct delayed_work host_logbuff_work;
	struct workqueue_struct *host_wq;
	u32 hang;
	u32 firmware_load;
	struct host_data *host_data;
	struct notifier_block nb;
};

#endif /*_HOST_MODULE_H*/
