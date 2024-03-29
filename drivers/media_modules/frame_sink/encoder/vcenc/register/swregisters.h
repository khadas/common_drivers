//Copyright 2019 Verisilicon(Beijing) Co.,Ltd. All Rights Reserved.
//Author: carl.li@verisilicon.com (Carl Li)
//
//Description: Common SWHW interface structure definition
//Based on document version 1.1.2
#ifndef SWREGISTERS_H_
#define SWREGISTERS_H_

#include "actypes.h"

struct SwRegisters {
    uai16 sw_vcmd_hw_id;
    uai16 sw_vcmd_hw_version;
    uai32 sw_vcmd_hw_build_date;
    uai1 sw_vcmd_ext_abn_int_src_vcd_mmu;
    uai1 sw_vcmd_ext_abn_int_src_vcd_l2cache;
    uai1 sw_vcmd_ext_abn_int_src_vcd_dec400;
    uai1 sw_vcmd_ext_abn_int_src_vcd;
    uai1 sw_vcmd_ext_abn_int_src_cutree_mmu;
    uai1 sw_vcmd_ext_abn_int_src_vce_mmu;
    uai1 sw_vcmd_ext_abn_int_src_vce_l2cache;
    uai1 sw_vcmd_ext_abn_int_src_vce_dec400;
    uai1 sw_vcmd_ext_abn_int_src_cutree;
    uai1 sw_vcmd_ext_abn_int_src_vce;
    uai1 sw_vcmd_ext_norm_int_src_vcd_mmu;
    uai1 sw_vcmd_ext_norm_int_src_vcd_l2cache;
    uai1 sw_vcmd_ext_norm_int_src_vcd_dec400;
    uai1 sw_vcmd_ext_norm_int_src_vcd;
    uai1 sw_vcmd_ext_norm_int_src_cutree_mmu;
    uai1 sw_vcmd_ext_norm_int_src_vce_mmu;
    uai1 sw_vcmd_ext_norm_int_src_vce_l2cache;
    uai1 sw_vcmd_ext_norm_int_src_vce_dec400;
    uai1 sw_vcmd_ext_norm_int_src_cutree;
    uai1 sw_vcmd_ext_norm_int_src_vce;
    uai32 sw_vcmd_exe_cmdbuf_count;
    uai32 sw_vcmd_executing_cmd;
    uai32 sw_vcmd_executing_cmd_msb;
    uai32 sw_vcmd_axi_total_ar_len;
    uai32 sw_vcmd_axi_total_r;
    uai32 sw_vcmd_axi_total_ar;
    uai32 sw_vcmd_axi_total_r_last;
    uai32 sw_vcmd_axi_total_aw_len;
    uai32 sw_vcmd_axi_total_w;
    uai32 sw_vcmd_axi_total_aw;
    uai32 sw_vcmd_axi_total_w_last;
    uai32 sw_vcmd_axi_total_b;
    uai1 sw_vcmd_axi_ar_valid;
    uai1 sw_vcmd_axi_ar_ready;
    uai1 sw_vcmd_axi_r_valid;
    uai1 sw_vcmd_axi_r_ready;
    uai1 sw_vcmd_axi_aw_valid;
    uai1 sw_vcmd_axi_aw_ready;
    uai1 sw_vcmd_axi_w_valid;
    uai1 sw_vcmd_axi_w_ready;
    uai1 sw_vcmd_axi_b_valid;
    uai1 sw_vcmd_axi_b_ready;
    uai3 sw_vcmd_work_state;
    uai1 sw_vcmd_axi_clk_gate_disable;
    uai1 sw_vcmd_master_out_clk_gate_disable;
    uai1 sw_vcmd_core_clk_gate_disable;
    uai1 sw_vcmd_abort_mode;
    uai1 sw_vcmd_reset_core;
    uai1 sw_vcmd_reset_all;
    uai1 sw_vcmd_start_trigger;
    uai16 sw_vcmd_irq_intcmd;
    uai1 sw_vcmd_irq_jmpp;
    uai1 sw_vcmd_irq_jmpd;
    uai1 sw_vcmd_irq_reset;
    uai1 sw_vcmd_irq_abort;
    uai1 sw_vcmd_irq_cmderr;
    uai1 sw_vcmd_irq_timeout;
    uai1 sw_vcmd_irq_buserr;
    uai1 sw_vcmd_irq_endcmd;
    uai16 sw_vcmd_irq_intcmd_en;
    uai1 sw_vcmd_irq_jmpp_en;
    uai1 sw_vcmd_irq_jmpd_en;
    uai1 sw_vcmd_irq_reset_en;
    uai1 sw_vcmd_irq_abort_en;
    uai1 sw_vcmd_irq_cmderr_en;
    uai1 sw_vcmd_irq_timeout_en;
    uai1 sw_vcmd_irq_buserr_en;
    uai1 sw_vcmd_irq_endcmd_en;
    uai1 sw_vcmd_timeout_en;
    uai31 sw_vcmd_timeout_cycles;
    uai32 sw_vcmd_executing_cmd_addr;
    uai32 sw_vcmd_executing_cmd_addr_msb;
    uai16 sw_vcmd_exe_cmdbuf_length;
    uai4 sw_vcmd_cmd_swap;
    uai8 sw_vcmd_max_burst_len;
    uai8 sw_vcmd_axi_id_rd;
    uai8 sw_vcmd_axi_id_wr;
    uai32 sw_vcmd_rdy_cmdbuf_count;
    uai1 sw_vcmd_ext_abn_int_src_vcd_mmu_gate;
    uai1 sw_vcmd_ext_abn_int_src_vcd_l2cache_gate;
    uai1 sw_vcmd_ext_abn_int_src_vcd_dec400_gate;
    uai1 sw_vcmd_ext_abn_int_src_vcd_gate;
    uai1 sw_vcmd_ext_abn_int_src_cutree_mmu_gate;
    uai1 sw_vcmd_ext_abn_int_src_vce_mmu_gate;
    uai1 sw_vcmd_ext_abn_int_src_vce_l2cache_gate;
    uai1 sw_vcmd_ext_abn_int_src_vce_dec400_gate;
    uai1 sw_vcmd_ext_abn_int_src_cutree_gate;
    uai1 sw_vcmd_ext_abn_int_src_vce_gate;
    uai1 sw_vcmd_ext_norm_int_src_vcd_mmu_gate;
    uai1 sw_vcmd_ext_norm_int_src_vcd_l2cache_gate;
    uai1 sw_vcmd_ext_norm_int_src_vcd_dec400_gate;
    uai1 sw_vcmd_ext_norm_int_src_vcd_gate;
    uai1 sw_vcmd_ext_norm_int_src_cutree_mmu_gate;
    uai1 sw_vcmd_ext_norm_int_src_vce_mmu_gate;
    uai1 sw_vcmd_ext_norm_int_src_vce_l2cache_gate;
    uai1 sw_vcmd_ext_norm_int_src_vce_dec400_gate;
    uai1 sw_vcmd_ext_norm_int_src_cutree_gate;
    uai1 sw_vcmd_ext_norm_int_src_vce_gate;
    uai32 sw_vcmd_cmdbuf_executing_id;
};

#endif /*SWREGISTERS_H_*/
