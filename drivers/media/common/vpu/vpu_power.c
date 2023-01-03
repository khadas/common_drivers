// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/delay.h>
#ifdef CONFIG_AMLOGIC_POWER
#include <linux/amlogic/power_domain.h>
#endif
#include <linux/amlogic/media/vpu/vpu.h>
#include "vpu_reg.h"
#include "vpu.h"

int vpu_power_init_check_dft(void)
{
	unsigned int val;
	int ret = 0;

	val = vpu_clk_getb(vpu_conf.data->vpu_clk_reg, 31, 1);
	if (val)
		val = vpu_clk_getb(vpu_conf.data->vpu_clk_reg, 24, 1);
	else
		val = vpu_clk_getb(vpu_conf.data->vpu_clk_reg, 8, 1);
	ret = (val == 0) ? 1 : 0;
	if (vpu_debug_print_flag) {
		VPUPR("%s: vpu_clk_ctrl: 0x%08x, ret=%d\n",
		      __func__, vpu_clk_read(vpu_conf.data->vpu_clk_reg), ret);
	}

	return ret;
}

int vpu_power_init_check_c3(void)
{
	unsigned int val;
	int ret = 0;

	val = vpu_clk_getb(vpu_conf.data->vpu_clk_reg, 8, 1);
	ret = (val == 0) ? 1 : 0;
	if (vpu_debug_print_flag) {
		VPUPR("%s: vpu_clk_ctrl: 0x%08x, ret=%d\n",
		      __func__, vpu_clk_read(vpu_conf.data->vpu_clk_reg), ret);
	}

	return ret;
}

void vpu_mem_pd_init_off(void)
{
}

void vpu_module_init_config(void)
{
	struct vpu_ctrl_s *ctrl_table;
	unsigned int _reg, _val, _bit, _len;
	int i = 0;

	VPUPR("%s\n", __func__);

	/* vpu clk gate init off */
	//ctrl_table = vpu_conf.data->module_init_table;
	if (vpu_conf.data->module_init_table) {
		ctrl_table = vpu_conf.data->module_init_table;
		i = 0;
		while (i < VPU_MOD_INIT_CNT_MAX) {
			if (ctrl_table[i].reg == VPU_REG_END)
				break;
			_reg = ctrl_table[i].reg;
			_val = ctrl_table[i].val;
			_bit = ctrl_table[i].bit;
			_len = ctrl_table[i].len;
			vpu_vcbus_setb(_reg, _val, _bit, _len);
			i++;
		}
	}

	/* dmc_arb_config */
	switch (vpu_conf.data->chip_type) {
	case VPU_CHIP_SC2:
	case VPU_CHIP_T5:
	case VPU_CHIP_T5D:
	case VPU_CHIP_T7:
	case VPU_CHIP_S4:
	case VPU_CHIP_S4D:
	case VPU_CHIP_T3:
	case VPU_CHIP_T5M:
		vpu_vcbus_write(VPU_RDARB_MODE_L1C1, 0x210000);
		vpu_vcbus_write(VPU_RDARB_MODE_L1C2, 0x10000);
		vpu_vcbus_write(VPU_RDARB_MODE_L2C1, 0x900000);
		/*from vlsi feijun*/
		vpu_vcbus_write(VPU_WRARB_MODE_L2C1, 0x170000/*0x20000*/);
		break;
	default:
		break;
	}

	if (vpu_debug_print_flag)
		VPUPR("%s finish\n", __func__);
}



void vpu_power_on_new(void)
{
#ifdef CONFIG_AMLOGIC_POWER
	unsigned int pwr_id;
	int i = 0;

	if (!vpu_conf.data->pwrctrl_id_table)
		return;

	while (i < VPU_PWR_ID_MAX) {
		pwr_id = vpu_conf.data->pwrctrl_id_table[i];
		if (pwr_id == VPU_PWR_ID_END)
			break;
		if (vpu_debug_print_flag)
			VPUPR("%s: pwr_id=%d\n", __func__, pwr_id);
		pwr_ctrl_psci_smc(pwr_id, 1);
		i++;
	}
	VPUPR("%s\n", __func__);
#else
	VPUERR("%s: no CONFIG_AMLOGIC_POWER\n", __func__);
#endif

	if (vpu_debug_print_flag)
		VPUPR("%s finish\n", __func__);
}

void vpu_power_off_new(void)
{
#ifdef CONFIG_AMLOGIC_POWER
	unsigned int pwr_id;
	int i = 0;

	VPUPR("%s\n", __func__);
	if (!vpu_conf.data->pwrctrl_id_table)
		return;

	while (i < VPU_PWR_ID_MAX) {
		pwr_id = vpu_conf.data->pwrctrl_id_table[i];
		if (pwr_id == VPU_PWR_ID_END)
			break;
		if (vpu_debug_print_flag)
			VPUPR("%s: pwr_id=%d\n", __func__, pwr_id);
		pwr_ctrl_psci_smc(pwr_id, 0);
		i++;
	}
#else
	VPUERR("%s: no CONFIG_AMLOGIC_POWER\n", __func__);
#endif

	if (vpu_debug_print_flag)
		VPUPR("%s finish\n", __func__);
}
