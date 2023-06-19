/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#define NEW_REG_CHECK_MASK  (0xffff0000)
/*new reg mask, bit20~bit27 chipid, bit16~bit19 subid*/
#define MASK_S5_NEW_REGS   ((0x3e << 20) & 0xffff0000)

#define EE_ASSIST_MBOX0_IRQ_REG          0x3f70
#define EE_ASSIST_MBOX0_CLR_REG          0x3f71
#define EE_ASSIST_MBOX0_MASK             0x3f72
#define EE_ASSIST_MBOX0_FIQ_SEL          0x3f73

#ifndef AMRISC_REGS_HEADER_
#define AMRISC_REGS_HEADER_

#define MSP                 0x0300
#define MPSR                0x0301
#define MINT_VEC_BASE       0x0302
#define MCPU_INTR_GRP       0x0303
#define MCPU_INTR_MSK       0x0304
#define MCPU_INTR_REQ       0x0305
#define MPC_P               0x0306
#define MPC_D               0x0307
#define MPC_E               0x0308
#define MPC_W               0x0309
#define CPSR                0x0321
#define CINT_VEC_BASE       0x0322
#define CPC_P               0x0326
#define CPC_D               0x0327
#define CPC_E               0x0328
#define CPC_W               0x0329
#define IMEM_DMA_CTRL       0x0340
#define IMEM_DMA_ADR        0x0341
#define IMEM_DMA_COUNT      0x0342
#define WRRSP_IMEM          0x0343
#define LMEM_DMA_CTRL       0x0350
#define LMEM_DMA_ADR        0x0351
#define LMEM_DMA_COUNT      0x0352
#define WRRSP_LMEM          0x0353
#define MAC_CTRL1           0x0360
#define ACC0REG1            0x0361
#define ACC1REG1            0x0362
#define MAC_CTRL2           0x0370
#define ACC0REG2            0x0371
#define ACC1REG2            0x0372
#define CPU_TRACE           0x0380

#endif
/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef DOS_REGS_HEADERS__
#define DOS_REGS_HEADERS__

#define MC_CTRL_REG                  0x0900
#define MC_MB_INFO                   0x0901
#define MC_PIC_INFO                  0x0902
#define MC_HALF_PEL_ONE              0x0903
#define MC_HALF_PEL_TWO              0x0904
#define POWER_CTL_MC                 0x0905
#define MC_CMD                       0x0906
#define MC_CTRL0                     0x0907
#define MC_PIC_W_H                   0x0908
#define MC_STATUS0                   0x0909
#define MC_STATUS1                   0x090a
#define MC_CTRL1                     0x090b
#define MC_MIX_RATIO0                0x090c
#define MC_MIX_RATIO1                0x090d
#define MC_DP_MB_XY                  0x090e
#define MC_OM_MB_XY                  0x090f
#define PSCALE_RST                   0x0910
#define PSCALE_CTRL                  0x0911
#define PSCALE_PICI_W                0x912
#define PSCALE_PICI_H                0x913
#define PSCALE_PICO_W                0x914
#define PSCALE_PICO_H                0x915
#define PSCALE_BMEM_ADDR             0x091f
#define PSCALE_BMEM_DAT              0x0920

#define PSCALE_RBUF_START_BLKX       0x925
#define PSCALE_RBUF_START_BLKY       0x926
#define PSCALE_PICO_SHIFT_XY         0x928
#define PSCALE_CTRL1                 0x929
#define PSCALE_SRCKEY_CTRL0          0x92a
#define PSCALE_SRCKEY_CTRL1          0x92b
#define PSCALE_CANVAS_RD_ADDR        0x92c
#define PSCALE_CANVAS_WR_ADDR        0x92d
#define PSCALE_CTRL2                 0x92e

/**/
#define MC_MPORT_CTRL                0x0940
#define MC_MPORT_DAT                 0x0941
#define MC_WT_PRED_CTRL              0x0942
#define MC_MBBOT_ST_EVEN_ADDR        0x0944
#define MC_MBBOT_ST_ODD_ADDR         0x0945
#define MC_DPDN_MB_XY                0x0946
#define MC_OMDN_MB_XY                0x0947
#define MC_HCMDBUF_H                 0x0948
#define MC_HCMDBUF_L                 0x0949
#define MC_HCMD_H                    0x094a
#define MC_HCMD_L                    0x094b
#define MC_IDCT_DAT                  0x094c
#define MC_CTRL_GCLK_CTRL            0x094d
#define MC_OTHER_GCLK_CTRL           0x094e
#define MC_CTRL2                     0x094f
#define MDEC_PIC_DC_MUX_CTRL         0x98d
#define MDEC_PIC_DC_CTRL             0x098e
#define MDEC_PIC_DC_STATUS           0x098f
#define ANC0_CANVAS_ADDR             0x0990
#define ANC1_CANVAS_ADDR             0x0991
#define ANC2_CANVAS_ADDR             0x0992
#define ANC3_CANVAS_ADDR             0x0993
#define ANC4_CANVAS_ADDR             0x0994
#define ANC5_CANVAS_ADDR             0x0995
#define ANC6_CANVAS_ADDR             0x0996
#define ANC7_CANVAS_ADDR             0x0997
#define ANC8_CANVAS_ADDR             0x0998
#define ANC9_CANVAS_ADDR             0x0999
#define ANC10_CANVAS_ADDR            0x099a
#define ANC11_CANVAS_ADDR            0x099b
#define ANC12_CANVAS_ADDR            0x099c
#define ANC13_CANVAS_ADDR            0x099d
#define ANC14_CANVAS_ADDR            0x099e
#define ANC15_CANVAS_ADDR            0x099f
#define ANC16_CANVAS_ADDR            0x09a0
#define ANC17_CANVAS_ADDR            0x09a1
#define ANC18_CANVAS_ADDR            0x09a2
#define ANC19_CANVAS_ADDR            0x09a3
#define ANC20_CANVAS_ADDR            0x09a4
#define ANC21_CANVAS_ADDR            0x09a5
#define ANC22_CANVAS_ADDR            0x09a6
#define ANC23_CANVAS_ADDR            0x09a7
#define ANC24_CANVAS_ADDR            0x09a8
#define ANC25_CANVAS_ADDR            0x09a9
#define ANC26_CANVAS_ADDR            0x09aa
#define ANC27_CANVAS_ADDR            0x09ab
#define ANC28_CANVAS_ADDR            0x09ac
#define ANC29_CANVAS_ADDR            0x09ad
#define ANC30_CANVAS_ADDR            0x09ae
#define ANC31_CANVAS_ADDR            0x09af
#define DBKR_CANVAS_ADDR             0x09b0
#define DBKW_CANVAS_ADDR             0x09b1
#define REC_CANVAS_ADDR              0x09b2
#define CURR_CANVAS_CTRL             0x09b3
#define MDEC_PIC_DC_THRESH           0x09b8
#define MDEC_PICR_BUF_STATUS         0x09b9
#define MDEC_PICW_BUF_STATUS         0x09ba
#define MCW_DBLK_WRRSP_CNT           0x09bb
#define MC_MBBOT_WRRSP_CNT           0x09bc
#define MDEC_PICW_BUF2_STATUS        0x09bd
#define WRRSP_FIFO_PICW_DBK          0x09be
#define WRRSP_FIFO_PICW_MC           0x09bf
#define AV_SCRATCH_0                 0x09c0
#define AV_SCRATCH_1                 0x09c1
#define AV_SCRATCH_2                 0x09c2
#define AV_SCRATCH_3                 0x09c3
#define AV_SCRATCH_4                 0x09c4
#define AV_SCRATCH_5                 0x09c5
#define AV_SCRATCH_6                 0x09c6
#define AV_SCRATCH_7                 0x09c7
#define AV_SCRATCH_8                 0x09c8
#define AV_SCRATCH_9                 0x09c9
#define AV_SCRATCH_A                 0x09ca
#define AV_SCRATCH_B                 0x09cb
#define AV_SCRATCH_C                 0x09cc
#define AV_SCRATCH_D                 0x09cd
#define AV_SCRATCH_E                 0x09ce
#define AV_SCRATCH_F                 0x09cf
#define AV_SCRATCH_G                 0x09d0
#define AV_SCRATCH_H                 0x09d1
#define AV_SCRATCH_I                 0x09d2
#define AV_SCRATCH_J                 0x09d3
#define AV_SCRATCH_K                 0x09d4
#define AV_SCRATCH_L                 0x09d5
#define AV_SCRATCH_M                 0x09d6
#define AV_SCRATCH_N                 0x09d7
#define WRRSP_CO_MB                  0x09d8
#define WRRSP_DCAC                   0x09d9
/*add from M8M2*/
#define WRRSP_VLD                    0x09da
#define MDEC_DOUBLEW_CFG0            0x09db
#define MDEC_DOUBLEW_CFG1            0x09dc
#define MDEC_DOUBLEW_CFG2            0x09dd
#define MDEC_DOUBLEW_CFG3            0x09de
#define MDEC_DOUBLEW_CFG4            0x09df
#define MDEC_DOUBLEW_CFG5            0x09e0
#define MDEC_DOUBLEW_CFG6            0x09e1
#define MDEC_DOUBLEW_CFG7            0x09e2
#define MDEC_DOUBLEW_STATUS          0x09e3
#define MDEC_EXTIF_CFG0              0x09e4
#define MDEC_EXTIF_CFG1              0x09e5
/*add from t7*/
#define MDEC_EXTIF_CFG2              0x09e6
#define MDEC_EXTIF_STS0              0x09e7
#define MDEC_PICW_BUFDW_CFG0         0x09e8
#define MDEC_PICW_BUFDW_CFG1         0x09e9
#define MDEC_CAV_LUT_DATAL           0x09ea
#define MDEC_CAV_LUT_DATAH           0x09eb
#define MDEC_CAV_LUT_ADDR            0x09ec
#define MDEC_CAV_CFG0                0x09ed
/*add from s5 */
#define MDEC_CRCW                    (0x09ee | MASK_S5_NEW_REGS)

/* 0f73ae529d2c1 rico.yang need add 0x1000 ? */
#define HCODEC_MDEC_CAV_LUT_DATAL    0x09ea
#define HCODEC_MDEC_CAV_LUT_DATAH    0x09eb
#define HCODEC_MDEC_CAV_LUT_ADDR     0x09ec
#define HCODEC_MDEC_CAV_CFG0         0x09ed

/**/
#define DBLK_RST                     0x0950
#define DBLK_CTRL                    0x0951
#define DBLK_MB_WID_HEIGHT           0x0952
#define DBLK_STATUS                  0x0953
#define DBLK_CMD_CTRL                0x0954
#define MCRCC_CTL1                   0x0980
#define MCRCC_CTL2                   0x0981
#define MCRCC_CTL3                   0x0982
#define GCLK_EN                      0x0983
#define MDEC_SW_RESET                0x0984
#define VLD_STATUS_CTRL              0x0c00
#define MPEG1_2_REG                  0x0c01
#define F_CODE_REG                   0x0c02
#define PIC_HEAD_INFO                0x0c03
#define SLICE_VER_POS_PIC_TYPE       0x0c04
#define QP_VALUE_REG                 0x0c05
#define MBA_INC                      0x0c06
#define MB_MOTION_MODE               0x0c07
#define POWER_CTL_VLD                0x0c08
#define MB_WIDTH                     0x0c09
#define SLICE_QP                     0x0c0a
#define PRE_START_CODE               0x0c0b
#define SLICE_START_BYTE_01          0x0c0c
#define SLICE_START_BYTE_23          0x0c0d
#define RESYNC_MARKER_LENGTH         0x0c0e
#define DECODER_BUFFER_INFO          0x0c0f
#define FST_FOR_MV_X                 0x0c10
#define FST_FOR_MV_Y                 0x0c11
#define SCD_FOR_MV_X                 0x0c12
#define SCD_FOR_MV_Y                 0x0c13
#define FST_BAK_MV_X                 0x0c14
#define FST_BAK_MV_Y                 0x0c15
#define SCD_BAK_MV_X                 0x0c16
#define SCD_BAK_MV_Y                 0x0c17
#define VLD_DECODE_CONTROL           0x0c18
#define VLD_REVERVED_19              0x0c19
#define VIFF_BIT_CNT                 0x0c1a
#define BYTE_ALIGN_PEAK_HI           0x0c1b
#define BYTE_ALIGN_PEAK_LO           0x0c1c
#define NEXT_ALIGN_PEAK              0x0c1d
#define VC1_CONTROL_REG              0x0c1e
#define PMV1_X                       0x0c20
#define PMV1_Y                       0x0c21
#define PMV2_X                       0x0c22
#define PMV2_Y                       0x0c23
#define PMV3_X                       0x0c24
#define PMV3_Y                       0x0c25
#define PMV4_X                       0x0c26
#define PMV4_Y                       0x0c27
#define M4_TABLE_SELECT              0x0c28
#define M4_CONTROL_REG               0x0c29
#define BLOCK_NUM                    0x0c2a
#define PATTERN_CODE                 0x0c2b
#define MB_INFO                      0x0c2c
#define VLD_DC_PRED                  0x0c2d
#define VLD_ERROR_MASK               0x0c2e
#define VLD_DC_PRED_C                0x0c2f
#define LAST_SLICE_MV_ADDR           0x0c30
#define LAST_MVX                     0x0c31
#define LAST_MVY                     0x0c32
#define VLD_C38                      0x0c38
#define VLD_C39                      0x0c39
#define VLD_STATUS                   0x0c3a
#define VLD_SHIFT_STATUS             0x0c3b
#define VOFF_STATUS                  0x0c3c
#define VLD_C3D                      0x0c3d
#define VLD_DBG_INDEX                0x0c3e
#define VLD_DBG_DATA                 0x0c3f
#define VLD_MEM_VIFIFO_START_PTR     0x0c40
#define VLD_MEM_VIFIFO_CURR_PTR      0x0c41
#define VLD_MEM_VIFIFO_END_PTR       0x0c42
#define VLD_MEM_VIFIFO_BYTES_AVAIL   0x0c43
#define VLD_MEM_VIFIFO_CONTROL       0x0c44
#define VLD_MEM_VIFIFO_WP            0x0c45
#define VLD_MEM_VIFIFO_RP            0x0c46
#define VLD_MEM_VIFIFO_LEVEL         0x0c47
#define VLD_MEM_VIFIFO_BUF_CNTL      0x0c48
#define VLD_TIME_STAMP_CNTL          0x0c49
#define VLD_TIME_STAMP_SYNC_0        0x0c4a
#define VLD_TIME_STAMP_SYNC_1        0x0c4b
#define VLD_TIME_STAMP_0             0x0c4c
#define VLD_TIME_STAMP_1             0x0c4d
#define VLD_TIME_STAMP_2             0x0c4e
#define VLD_TIME_STAMP_3             0x0c4f
#define VLD_TIME_STAMP_LENGTH        0x0c50
#define VLD_MEM_VIFIFO_WRAP_COUNT    0x0c51
#define VLD_MEM_VIFIFO_MEM_CTL       0x0c52
#define VLD_MEM_VBUF_RD_PTR          0x0c53
#define VLD_MEM_VBUF2_RD_PTR         0x0c54
#define VLD_MEM_SWAP_ADDR            0x0c55
#define VLD_MEM_SWAP_CTL             0x0c56
// bit[12]  -- zero_use_cbp_blk
// bit[11]  -- mv_use_abs (only calculate abs)
// bit[10]  -- mv_use_simple_mode (every size count has same weight)
// bit[9]   -- use_simple_mode (every size count has same weight)
// bit[8]   -- reset_all_count // write only
// bit[7:5] Reserved
// bit[4:0] pic_quality_rd_idx
#define VDEC_PIC_QUALITY_CTRL        0x0c57
// idx  -- read out
//   0  -- blk88_y_count // 4k will use 20 bits
//   1  -- qp_y_sum // 4k use 27 bits
//   2  -- intra_y_count // 4k use 20 bits
//   3  -- skipped_y_count // 4k use 20 bits
//   4  -- coeff_non_zero_y_count // 4k use 20 bits
//   5  -- blk66_c_count // 4k will use 20 bits
//   6  -- qp_c_sum // 4k use 26 bits
//   7  -- intra_c_count // 4k use 20 bits
//   8  -- skipped_cu_c_count // 4k use 20 bits
//   9  -- coeff_non_zero_c_count // 4k use 20 bits
//  10  -- { 1'h0, qp_c_max[6:0], 1'h0, qp_c_min[6:0],
//			1'h0, qp_y_max[6:0], 1'h0, qp_y_min[6:0]}
//  11  -- blk22_mv_count
//  12  -- {mvy_L1_count[39:32], mvx_L1_count[39:32],
//			mvy_L0_count[39:32], mvx_L0_count[39:32]}
//  13  -- mvx_L0_count[31:0]
//  14  -- mvy_L0_count[31:0]
//  15  -- mvx_L1_count[31:0]
//  16  -- mvy_L1_count[31:0]
//  17  -- {mvx_L0_max, mvx_L0_min} // format : {sign, abs[14:0]}
//  18  -- {mvy_L0_max, mvy_L0_min}
//  19  -- {mvx_L1_max, mvx_L1_min}
//  20  -- {mvy_L1_max, mvy_L1_min}
#define VDEC_PIC_QUALITY_DATA        0x0c58
/*from s5*/
#define VDEC_STREAM_CRC              (0x0c59 | MASK_S5_NEW_REGS)
#define VDEC_H264_TOP_BUFF_START     (0x0c5a | MASK_S5_NEW_REGS)
#define VDEC_H264_TOP_CFG            (0x0c5b | MASK_S5_NEW_REGS)
#define VDEC_H264_TOP_OFFSET         (0x0c5c | MASK_S5_NEW_REGS)
#define VDEC_H264_TOP_CTRL           (0x0c5d | MASK_S5_NEW_REGS)

#define VCOP_CTRL_REG                0x0e00
#define QP_CTRL_REG                  0x0e01
#define INTRA_QUANT_MATRIX           0x0e02
#define NON_I_QUANT_MATRIX           0x0e03
#define DC_SCALER                    0x0e04
#define DC_AC_CTRL                   0x0e05
#define DC_AC_SCALE_MUL              0x0e06
#define DC_AC_SCALE_DIV              0x0e07
#define POWER_CTL_IQIDCT             0x0e08
#define RV_AI_Y_X                    0x0e09
#define RV_AI_U_X                    0x0e0a
#define RV_AI_V_X                    0x0e0b
#define RV_AI_MB_COUNT               0x0e0c
#define NEXT_INTRA_DMA_ADDRESS       0x0e0d
#define IQIDCT_CONTROL               0x0e0e
#define IQIDCT_DEBUG_INFO_0          0x0e0f
#define DEBLK_CMD                    0x0e10
#define IQIDCT_DEBUG_IDCT            0x0e11
#define DCAC_DMA_CTRL                0x0e12
#define DCAC_DMA_ADDRESS             0x0e13
#define DCAC_CPU_ADDRESS             0x0e14
#define DCAC_CPU_DATA                0x0e15
#define DCAC_MB_COUNT                0x0e16
#define IQ_QUANT                     0x0e17
#define VC1_BITPLANE_CTL             0x0e18
#define AVSP_IQ_WQ_PARAM_01          0x0e19
#define AVSP_IQ_WQ_PARAM_23          0x0e1a
#define AVSP_IQ_WQ_PARAM_45          0x0e1b
#define AVSP_IQ_CTL                  0x0e1c
#define DCAC_DDR_BYTE64_CTL          0x0e1d
/*from s5*/
#define DCAC_DMA_MIN_ADDR            (0x0e1e | MASK_S5_NEW_REGS)
#define DCAC_DMA_MAX_ADDR            (0x0e1f | MASK_S5_NEW_REGS)
#define DCAC_DMA_HW_CTL_CFG          (0x0e20 | MASK_S5_NEW_REGS)
#define I_PIC_MB_COUNT_HW            (0x0e21 | MASK_S5_NEW_REGS)
#define DCAC_DMA_HW_INFO             (0x0e22 | MASK_S5_NEW_REGS)
#define DCAC_DMA_HW_OFFSET           (0x0e23 | MASK_S5_NEW_REGS)
#define DCAC_DMA_HW_BUFF_START       (0x0e24 | MASK_S5_NEW_REGS)
#define NEXT_INTRA_READ_ADDR_HW      (0x0e25 | MASK_S5_NEW_REGS)
#define DCAC_DMA_HW_CTL              (0x0e26 | MASK_S5_NEW_REGS)
#define DCAC_DMA_HW_MBBOT_ADDR       (0x0e27 | MASK_S5_NEW_REGS)
#define DCAC_DMA_HW_MBXY             (0x0e28 | MASK_S5_NEW_REGS)
#define SET_HW_TLR                   (0x0e29 | MASK_S5_NEW_REGS)

#define DOS_SW_RESET0                0x3f00
#define DOS_GCLK_EN0                 0x3f01
#define DOS_GEN_CTRL0                0x3f02
#define DOS_APB_ERR_CTRL             0x3f03
#define DOS_APB_ERR_STAT             0x3f04
#define DOS_VDEC_INT_EN              0x3f05
#define DOS_HCODEC_INT_EN            0x3f06
#define DOS_SW_RESET1                0x3f07
#define DOS_SW_RESET2                0x3f08
#define DOS_GCLK_EN1                 0x3f09
#define DOS_VDEC2_INT_EN             0x3f0a
#define DOS_VDIN_LCNT                0x3f0b
#define DOS_VDIN_FCNT                0x3f0c
#define DOS_VDIN_CCTL                0x3f0d
#define DOS_SCRATCH0                 0x3f10
#define DOS_SCRATCH1                 0x3f11
#define DOS_SCRATCH2                 0x3f12
#define DOS_SCRATCH3                 0x3f13
#define DOS_SCRATCH4                 0x3f14
#define DOS_SCRATCH5                 0x3f15
#define DOS_SCRATCH6                 0x3f16
#define DOS_SCRATCH7                 0x3f17
#define DOS_SCRATCH8                 0x3f18
#define DOS_SCRATCH9                 0x3f19
#define DOS_SCRATCH10                0x3f1a
#define DOS_SCRATCH11                0x3f1b
#define DOS_SCRATCH12                0x3f1c
#define DOS_SCRATCH13                0x3f1d
#define DOS_SCRATCH14                0x3f1e
#define DOS_SCRATCH15                0x3f1f
#define DOS_SCRATCH16                0x3f20
#define DOS_SCRATCH17                0x3f21
#define DOS_SCRATCH18                0x3f22
#define DOS_SCRATCH19                0x3f23
#define DOS_SCRATCH20                0x3f24
#define DOS_SCRATCH21                0x3f25
#define DOS_SCRATCH22                0x3f26
#define DOS_SCRATCH23                0x3f27
#define DOS_SCRATCH24                0x3f28
#define DOS_SCRATCH25                0x3f29
#define DOS_SCRATCH26                0x3f2a
#define DOS_SCRATCH27                0x3f2b
#define DOS_SCRATCH28                0x3f2c
#define DOS_SCRATCH29                0x3f2d
#define DOS_SCRATCH30                0x3f2e
#define DOS_SCRATCH31                0x3f2f
#define DOS_MEM_PD_VDEC              0x3f30
#define DOS_MEM_PD_VDEC2             0x3f31
#define DOS_MEM_PD_HCODEC            0x3f32
/*add from M8M2*/
#define DOS_MEM_PD_HEVC              0x3f33
/* from s5 */
#define DOS_MEM_PD_HEVC_DBE          (0x3f3b | MASK_S5_NEW_REGS)

#define DOS_SW_RESET3                0x3f34
#define DOS_GCLK_EN3                 0x3f35
#define DOS_HEVC_INT_EN              0x3f36

/**/
#define DOS_SW_RESET4                0x3f37
#define DOS_GCLK_EN4                 0x3f38
#define DOS_MEM_PD_WAVE420L          0x3f39
#define DOS_WAVE420L_CNTL_STAT       0x3f3a

/**/
#define DOS_VDEC_MCRCC_STALL_CTRL    0x3f40
#define DOS_VDEC_MCRCC_STALL2_CTRL   0x3f42
#define DOS_VDEC2_MCRCC_STALL_CTRL   0x3f41
#define DOS_VDEC2_MCRCC_STALL2_CTRL  0x3f43

/* from s5 */
#define DOS_VDEC_WR_MAX_SIZE_CTL     (0x3f84 | MASK_S5_NEW_REGS)
#define DOS_VDEC_DW_MAX_SIZE_CTL     (0x3f85 | MASK_S5_NEW_REGS)
#define DOS_HEVC_WR_MAX_SIZE_CTL     (0x3f86 | MASK_S5_NEW_REGS)
#define DOS_HEVC_DW_MAX_SIZE_CTL     (0x3f87 | MASK_S5_NEW_REGS)
#define DOS_HEVC_WR_MAX_SIZE_CTL1    (0x3f8a | MASK_S5_NEW_REGS)
#define DOS_HEVC_DW_MAX_SIZE_CTL1    (0x3f8b | MASK_S5_NEW_REGS)
#define DOS_HEVC_PATH_CTL            (0x3f8c | MASK_S5_NEW_REGS)
#define DOS_AXI_ID_MAP_INDEX         (0x3f88 | MASK_S5_NEW_REGS)
#define DOS_AXI_ID_MAP_DATA          (0x3f89 | MASK_S5_NEW_REGS)

#endif

/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef HEVC_REGS_HEADERS__
#define HEVC_REGS_HEADERS__
/*add from M8M2*/
#define HEVC_ASSIST_AFIFO_CTRL              0x3001
#define HEVC_ASSIST_AFIFO_CTRL1             0x3002
#define HEVC_ASSIST_GCLK_EN                 0x3003
#define HEVC_ASSIST_SW_RESET                0x3004
/* from s5 */
#define HEVC_ASSIST_AXIADDR_PREFIX          (0x300f | MASK_S5_NEW_REGS)
#define HEVC_PARSER_IQIT_BUFF_CTL           (0x3010 | MASK_S5_NEW_REGS)
#define HEVC_PARSER_IQIT_BUFF_STATUS        (0x3011 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_MMU_MAP_ADDR_DBE1       (0x3012 | MASK_S5_NEW_REGS)

#define HEVC_ASSIST_AMR1_INT0               0x3025
#define HEVC_ASSIST_AMR1_INT1               0x3026
#define HEVC_ASSIST_AMR1_INT2               0x3027
#define HEVC_ASSIST_AMR1_INT3               0x3028
#define HEVC_ASSIST_AMR1_INT4               0x3029
#define HEVC_ASSIST_AMR1_INT5               0x302a
#define HEVC_ASSIST_AMR1_INT6               0x302b
#define HEVC_ASSIST_AMR1_INT7               0x302c
#define HEVC_ASSIST_AMR1_INT8               0x302d
#define HEVC_ASSIST_AMR1_INT9               0x302e
#define HEVC_ASSIST_AMR1_INTA               0x302f
#define HEVC_ASSIST_AMR1_INTB               0x3030
#define HEVC_ASSIST_AMR1_INTC               0x3031
#define HEVC_ASSIST_AMR1_INTD               0x3032
#define HEVC_ASSIST_AMR1_INTE               0x3033
#define HEVC_ASSIST_AMR1_INTF               0x3034
#define HEVC_ASSIST_AMR2_INT0               0x3035
#define HEVC_ASSIST_AMR2_INT1               0x3036
#define HEVC_ASSIST_AMR2_INT2               0x3037
#define HEVC_ASSIST_AMR2_INT3               0x3038
#define HEVC_ASSIST_AMR2_INT4               0x3039
#define HEVC_ASSIST_AMR2_INT5               0x303a
#define HEVC_ASSIST_AMR2_INT6               0x303b
#define HEVC_ASSIST_AMR2_INT7               0x303c
#define HEVC_ASSIST_AMR2_INT8               0x303d
#define HEVC_ASSIST_AMR2_INT9               0x303e
#define HEVC_ASSIST_AMR2_INTA               0x303f
#define HEVC_ASSIST_AMR2_INTB               0x3040
#define HEVC_ASSIST_AMR2_INTC               0x3041
#define HEVC_ASSIST_AMR2_INTD               0x3042
#define HEVC_ASSIST_AMR2_INTE               0x3043
#define HEVC_ASSIST_AMR2_INTF               0x3044
#define HEVC_ASSIST_MBX_SSEL                0x3045
#define HEVC_ASSIST_TIMER0_LO               0x3060
#define HEVC_ASSIST_TIMER0_HI               0x3061
#define HEVC_ASSIST_TIMER1_LO               0x3062
#define HEVC_ASSIST_TIMER1_HI               0x3063
#define HEVC_ASSIST_DMA_INT                 0x3064
#define HEVC_ASSIST_DMA_INT_MSK             0x3065
#define HEVC_ASSIST_DMA_INT2                0x3066
#define HEVC_ASSIST_DMA_INT_MSK2            0x3067
#define HEVC_ASSIST_MBOX0_IRQ_REG           0x3070
#define HEVC_ASSIST_MBOX0_CLR_REG           0x3071
#define HEVC_ASSIST_MBOX0_MASK              0x3072
#define HEVC_ASSIST_MBOX0_FIQ_SEL           0x3073
#define HEVC_ASSIST_MBOX1_IRQ_REG           0x3074
#define HEVC_ASSIST_MBOX1_CLR_REG           0x3075
#define HEVC_ASSIST_MBOX1_MASK              0x3076
#define HEVC_ASSIST_MBOX1_FIQ_SEL           0x3077
#define HEVC_ASSIST_MBOX2_IRQ_REG           0x3078
#define HEVC_ASSIST_MBOX2_CLR_REG           0x3079
#define HEVC_ASSIST_MBOX2_MASK              0x307a
#define HEVC_ASSIST_MBOX2_FIQ_SEL           0x307b
#define HEVC_ASSIST_AXI_CTRL                0x307c
#define HEVC_ASSIST_AXI_STATUS              0x307d
#define HEVC_ASSIST_AXI_STATUS2_HI          0x307e
#define HEVC_ASSIST_AXI_STATUS2_LO          0x307f
/*from s5*/
#define HEVC_ASSIST_FB_CTL                         (0x3050 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_W_CTL                       (0x3051 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_W_CTL1                      (0x3052 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_WID                         (0x3053 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_R_CTL                       (0x3054 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_R_CTL1                      (0x3055 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_RID                         (0x3056 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_PIC_SIZE_FB_READ1              (0x3057 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_PIC_CLR                     (0x3058 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_BACKCORE_INT_STATUS            (0x3059 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_TILE_INFO_WPTR              (0x3060 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_TILE_INFO_RPTR              (0x3061 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_WR_ADDR0                    (0x3062 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_WR_ADDR1                    (0x3063 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_RD_ADDR0                    (0x3064 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_RD_ADDR1                    (0x3065 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_MMU_MAP_ADDR0               (0x3066 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_MMU_MAP_ADDR1               (0x3067 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FBD_MMU_MAP_ADDR0              (0x3068 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FBD_MMU_MAP_ADDR1              (0x3069 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_MMU_MAP_ADDR0_START         (0x306a | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_MMU_MAP_ADDR0_END           (0x306b | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_MMU_MAP_ADDR1_START         (0x306c | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_MMU_MAP_ADDR1_END           (0x306d | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_PARSER_SAO_WOFFSET0         (0x306e | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_PARSER_SAO_WOFFSET1         (0x306f | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_PARSER_SAO_ROFFSET0         (0x3070 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_PARSER_SAO_ROFFSET1         (0x3071 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FBD_MMU_MAP_ADDR0_START        (0x3072 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FBD_MMU_MAP_ADDR0_END          (0x3073 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FBD_MMU_MAP_ADDR1_START        (0x3074 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FBD_MMU_MAP_ADDR1_END          (0x3075 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_MPRED_IMP_WOFFSET0          (0x3076 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_MPRED_IMP_WOFFSET1          (0x3077 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_MPRED_IMP_ROFFSET0          (0x3078 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_MPRED_IMP_ROFFSET1          (0x3079 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_AV1_CU_WOFFSET0             (0x307a | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_AV1_CU_WOFFSET1             (0x307b | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_AV1_CU_ROFFSET0             (0x307c | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_AV1_CU_ROFFSET1             (0x307d | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_AV1_GMWM_WOFFSET            (0x307e | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_AV1_GMWM_ROFFSET            (0x307f | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_AV1_LRF_WOFFSET             (0x3080 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_AV1_LRF_ROFFSET             (0x3081 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_MPRED_TLDAT_WOFFSET0        (0x3082 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_MPRED_TLDAT_WOFFSET1        (0x3083 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_MPRED_TLDAT_ROFFSET0        (0x3084 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_FB_MPRED_TLDAT_ROFFSET1        (0x3085 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_AXI_STATUS_DFE_HI              (0x3086 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_AXI_STATUS_DFE_LO              (0x3087 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_AXI_STATUS_DBE0_HI             (0x3088 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_AXI_STATUS_DBE0_LO             (0x3089 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_AXI_STATUS_DBE1_HI             (0x308a | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_AXI_STATUS_DBE1_LO             (0x308b | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_AXI_STATUS_DMC_HI              (0x308c | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_AXI_STATUS_DMC_LO              (0x308d | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_AXI_STATUS_FB_HI               (0x308e | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_AXI_STATUS_FB_LO               (0x308f | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_RING_F_INDEX                   (0x30a0 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_RING_F_START                   (0x30a1 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_RING_F_END                     (0x30a2 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_RING_F_RPTR                    (0x30a3 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_RING_F_WPTR                    (0x30a4 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_RING_F_THRESHOLD               (0x30a5 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_RING_B_INDEX                   (0x30a6 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_RING_B_START                   (0x30a7 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_RING_B_END                     (0x30a8 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_RING_B_RPTR                    (0x30a9 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_RING_B_THRESHOLD               (0x30aa | MASK_S5_NEW_REGS)


#define HEVC_ASSIST_SCRATCH_0               0x30c0
#define HEVC_ASSIST_SCRATCH_1               0x30c1
#define HEVC_ASSIST_SCRATCH_2               0x30c2
#define HEVC_ASSIST_SCRATCH_3               0x30c3
#define HEVC_ASSIST_SCRATCH_4               0x30c4
#define HEVC_ASSIST_SCRATCH_5               0x30c5
#define HEVC_ASSIST_SCRATCH_6               0x30c6
#define HEVC_ASSIST_SCRATCH_7               0x30c7
#define HEVC_ASSIST_SCRATCH_8               0x30c8
#define HEVC_ASSIST_SCRATCH_9               0x30c9
#define HEVC_ASSIST_SCRATCH_A               0x30ca
#define HEVC_ASSIST_SCRATCH_B               0x30cb
#define HEVC_ASSIST_SCRATCH_C               0x30cc
#define HEVC_ASSIST_SCRATCH_D               0x30cd
#define HEVC_ASSIST_SCRATCH_E               0x30ce
#define HEVC_ASSIST_SCRATCH_F               0x30cf
#define HEVC_ASSIST_SCRATCH_G               0x30d0
#define HEVC_ASSIST_SCRATCH_H               0x30d1
#define HEVC_ASSIST_SCRATCH_I               0x30d2
#define HEVC_ASSIST_SCRATCH_J               0x30d3
#define HEVC_ASSIST_SCRATCH_K               0x30d4
#define HEVC_ASSIST_SCRATCH_L               0x30d5
#define HEVC_ASSIST_SCRATCH_M               0x30d6
#define HEVC_ASSIST_SCRATCH_N               0x30d7
/*from s5*/
#define HEVC_ASSIST_SCRATCH_O               (0x30c8 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_P               (0x30c9 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_Q               (0x30ca | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_R               (0x30cb | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_S               (0x30cc | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_T               (0x30cd | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_U               (0x30ce | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_V               (0x30cf | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_W               (0x30d0 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_X               (0x30d1 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_Y               (0x30d2 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_Z               (0x30d3 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_10              (0x30d4 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_11              (0x30d5 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_12              (0x30d6 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_13              (0x30d7 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_14              (0x30d8 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_15              (0x30d9 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_16              (0x30da | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_17              (0x30db | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_18              (0x30dc | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_SCRATCH_19              (0x30dd | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_DEBUG_0                 (0x30e0 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_DEBUG_1                 (0x30e1 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_DEBUG_2                 (0x30e2 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_DEBUG_3                 (0x30e3 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_DEBUG_4                 (0x30e4 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_DEBUG_5                 (0x30e5 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_DEBUG_6                 (0x30e6 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_DEBUG_7                 (0x30e7 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_DEBUG_8                 (0x30e8 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_DEBUG_9                 (0x30e9 | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_DEBUG_A                 (0x30ea | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_DEBUG_B                 (0x30eb | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_DEBUG_C                 (0x30ec | MASK_S5_NEW_REGS)
#define HEVC_ASSIST_MIRROR_CONFIG           (0x30ed | MASK_S5_NEW_REGS)
#define HEVC_RDMA_F_CTRL                    0x30f0
#define HEVC_RDMA_F_START_ADDR              0x30f1
#define HEVC_RDMA_F_END_ADDR                0x30f2
#define HEVC_RDMA_F_STATUS0                 0x30f3
#define HEVC_RDMA_F_STATUS1                 0x30f4
#define HEVC_RDMA_F_STATUS2                 0x30f5
#define HEVC_RDMA_B_CTRL                    0x30f8
#define HEVC_RDMA_B_START_ADDR              0x30f9
#define HEVC_RDMA_B_END_ADDR                0x30fa
#define HEVC_RDMA_B_STATUS0                 0x30fb
#define HEVC_RDMA_B_STATUS1                 0x30fc
#define HEVC_RDMA_B_STATUS2                 0x30fd
/* from s5 */

#define HEVC_MSP_DBE                      (0x3800 | MASK_S5_NEW_REGS)
#define HEVC_MPSR_DBE                     (0x3801 | MASK_S5_NEW_REGS)
#define HEVC_MINT_VEC_BASE_DBE            (0x3802 | MASK_S5_NEW_REGS)
#define HEVC_MCPU_INTR_GRP_DBE            (0x3803 | MASK_S5_NEW_REGS)
#define HEVC_MCPU_INTR_MSK_DBE            (0x3804 | MASK_S5_NEW_REGS)
#define HEVC_MCPU_INTR_REQ_DBE            (0x3805 | MASK_S5_NEW_REGS)
#define HEVC_MPC_P_DBE                    (0x3806 | MASK_S5_NEW_REGS)
#define HEVC_MPC_D_DBE                    (0x3807 | MASK_S5_NEW_REGS)
#define HEVC_MPC_E_DBE                    (0x3808 | MASK_S5_NEW_REGS)
#define HEVC_MPC_W_DBE                    (0x3809 | MASK_S5_NEW_REGS)
#define HEVC_MINDEX0_REG_DBE              (0x380a | MASK_S5_NEW_REGS)
#define HEVC_MINDEX1_REG_DBE              (0x380b | MASK_S5_NEW_REGS)
#define HEVC_MINDEX2_REG_DBE              (0x380c | MASK_S5_NEW_REGS)
#define HEVC_MINDEX3_REG_DBE              (0x380d | MASK_S5_NEW_REGS)
#define HEVC_MINDEX4_REG_DBE              (0x380e | MASK_S5_NEW_REGS)
#define HEVC_MINDEX5_REG_DBE              (0x380f | MASK_S5_NEW_REGS)
#define HEVC_MINDEX6_REG_DBE              (0x3810 | MASK_S5_NEW_REGS)
#define HEVC_MINDEX7_REG_DBE              (0x3811 | MASK_S5_NEW_REGS)
#define HEVC_MMIN_REG_DBE                 (0x3812 | MASK_S5_NEW_REGS)
#define HEVC_MMAX_REG_DBE                 (0x3813 | MASK_S5_NEW_REGS)
#define HEVC_MBREAK0_REG_DBE              (0x3814 | MASK_S5_NEW_REGS)
#define HEVC_MBREAK1_REG_DBE              (0x3815 | MASK_S5_NEW_REGS)
#define HEVC_MBREAK2_REG_DBE              (0x3816 | MASK_S5_NEW_REGS)
#define HEVC_MBREAK3_REG_DBE              (0x3817 | MASK_S5_NEW_REGS)
#define HEVC_MBREAK_TYPE_DBE              (0x3818 | MASK_S5_NEW_REGS)
#define HEVC_MBREAK_CTRL_DBE              (0x3819 | MASK_S5_NEW_REGS)
#define HEVC_MBREAK_STAUTS_DBE            (0x381a | MASK_S5_NEW_REGS)
#define HEVC_MDB_ADDR_REG_DBE             (0x381b | MASK_S5_NEW_REGS)
#define HEVC_MDB_DATA_REG_DBE             (0x381c | MASK_S5_NEW_REGS)
#define HEVC_MDB_CTRL_DBE                 (0x381d | MASK_S5_NEW_REGS)
#define HEVC_MSFTINT0_DBE                 (0x381e | MASK_S5_NEW_REGS)
#define HEVC_MSFTINT1_DBE                 (0x381f | MASK_S5_NEW_REGS)
#define HEVC_CSP_DBE                      (0x3820 | MASK_S5_NEW_REGS)
#define HEVC_CPSR_DBE                     (0x3821 | MASK_S5_NEW_REGS)
#define HEVC_CINT_VEC_BASE_DBE            (0x3822 | MASK_S5_NEW_REGS)
#define HEVC_CCPU_INTR_GRP_DBE            (0x3823 | MASK_S5_NEW_REGS)
#define HEVC_CCPU_INTR_MSK_DBE            (0x3824 | MASK_S5_NEW_REGS)
#define HEVC_CCPU_INTR_REQ_DBE            (0x3825 | MASK_S5_NEW_REGS)
#define HEVC_CPC_P_DBE                    (0x3826 | MASK_S5_NEW_REGS)
#define HEVC_CPC_D_DBE                    (0x3827 | MASK_S5_NEW_REGS)
#define HEVC_CPC_E_DBE                    (0x3828 | MASK_S5_NEW_REGS)
#define HEVC_CPC_W_DBE                    (0x3829 | MASK_S5_NEW_REGS)
#define HEVC_CINDEX0_REG_DBE              (0x382a | MASK_S5_NEW_REGS)
#define HEVC_CINDEX1_REG_DBE              (0x382b | MASK_S5_NEW_REGS)
#define HEVC_CINDEX2_REG_DBE              (0x382c | MASK_S5_NEW_REGS)
#define HEVC_CINDEX3_REG_DBE              (0x382d | MASK_S5_NEW_REGS)
#define HEVC_CINDEX4_REG_DBE              (0x382e | MASK_S5_NEW_REGS)
#define HEVC_CINDEX5_REG_DBE              (0x382f | MASK_S5_NEW_REGS)
#define HEVC_CINDEX6_REG_DBE              (0x3830 | MASK_S5_NEW_REGS)
#define HEVC_CINDEX7_REG_DBE              (0x3831 | MASK_S5_NEW_REGS)
#define HEVC_CMIN_REG_DBE                 (0x3832 | MASK_S5_NEW_REGS)
#define HEVC_CMAX_REG_DBE                 (0x3833 | MASK_S5_NEW_REGS)
#define HEVC_CBREAK0_REG_DBE              (0x3834 | MASK_S5_NEW_REGS)
#define HEVC_CBREAK1_REG_DBE              (0x3835 | MASK_S5_NEW_REGS)
#define HEVC_CBREAK2_REG_DBE              (0x3836 | MASK_S5_NEW_REGS)
#define HEVC_CBREAK3_REG_DBE              (0x3837 | MASK_S5_NEW_REGS)
#define HEVC_CBREAK_TYPE_DBE              (0x3838 | MASK_S5_NEW_REGS)
#define HEVC_CBREAK_CTRL_DBE              (0x3839 | MASK_S5_NEW_REGS)
#define HEVC_CBREAK_STAUTS_DBE            (0x383a | MASK_S5_NEW_REGS)
#define HEVC_CDB_ADDR_REG_DBE             (0x383b | MASK_S5_NEW_REGS)
#define HEVC_CDB_DATA_REG_DBE             (0x383c | MASK_S5_NEW_REGS)
#define HEVC_CDB_CTRL_DBE                 (0x383d | MASK_S5_NEW_REGS)
#define HEVC_CSFTINT0_DBE                 (0x383e | MASK_S5_NEW_REGS)
#define HEVC_CSFTINT1_DBE                 (0x383f | MASK_S5_NEW_REGS)
#define HEVC_IMEM_DMA_CTRL_DBE            (0x3840 | MASK_S5_NEW_REGS)
#define HEVC_IMEM_DMA_ADR_DBE             (0x3841 | MASK_S5_NEW_REGS)
#define HEVC_IMEM_DMA_COUNT_DBE           (0x3842 | MASK_S5_NEW_REGS)
#define HEVC_WRRSP_IMEM_DBE               (0x3843 | MASK_S5_NEW_REGS)
#define HEVC_LMEM_DMA_CTRL_DBE            (0x3850 | MASK_S5_NEW_REGS)
#define HEVC_LMEM_DMA_ADR_DBE             (0x3851 | MASK_S5_NEW_REGS)
#define HEVC_LMEM_DMA_COUNT_DBE           (0x3852 | MASK_S5_NEW_REGS)
#define HEVC_WRRSP_LMEM_DBE               (0x3853 | MASK_S5_NEW_REGS)
#define HEVC_MAC_CTRL1_DBE                (0x3860 | MASK_S5_NEW_REGS)
#define HEVC_ACC0REG1_DBE                 (0x3861 | MASK_S5_NEW_REGS)
#define HEVC_ACC1REG1_DBE                 (0x3862 | MASK_S5_NEW_REGS)
#define HEVC_MAC_CTRL2_DBE                (0x3870 | MASK_S5_NEW_REGS)
#define HEVC_ACC0REG2_DBE                 (0x3871 | MASK_S5_NEW_REGS)
#define HEVC_ACC1REG2_DBE                 (0x3872 | MASK_S5_NEW_REGS)
#define HEVC_CPU_TRACE_DBE                (0x3880 | MASK_S5_NEW_REGS)


#define HEVC_PARSER_VERSION                 0x3100
#define HEVC_STREAM_CONTROL                 0x3101
#define HEVC_STREAM_START_ADDR              0x3102
#define HEVC_STREAM_END_ADDR                0x3103
#define HEVC_STREAM_WR_PTR                  0x3104
#define HEVC_STREAM_RD_PTR                  0x3105
#define HEVC_STREAM_LEVEL                   0x3106
#define HEVC_STREAM_FIFO_CTL                0x3107
#define HEVC_SHIFT_CONTROL                  0x3108
#define HEVC_SHIFT_STARTCODE                0x3109
#define HEVC_SHIFT_EMULATECODE              0x310a
#define HEVC_SHIFT_STATUS                   0x310b
#define HEVC_SHIFTED_DATA                   0x310c
#define HEVC_SHIFT_BYTE_COUNT               0x310d
#define HEVC_SHIFT_COMMAND                  0x310e
#define HEVC_ELEMENT_RESULT                 0x310f
#define HEVC_CABAC_CONTROL                  0x3110
#define HEVC_PARSER_SLICE_INFO              0x3111
#define HEVC_PARSER_CMD_WRITE               0x3112
#define HEVC_PARSER_CORE_CONTROL            0x3113
#define HEVC_PARSER_CMD_FETCH               0x3114
#define HEVC_PARSER_CMD_STATUS              0x3115
#define HEVC_PARSER_LCU_INFO                0x3116
#define HEVC_PARSER_HEADER_INFO             0x3117
#define HEVC_PARSER_RESULT_0                0x3118
#define HEVC_PARSER_RESULT_1                0x3119
#define HEVC_PARSER_RESULT_2                0x311a
#define HEVC_PARSER_RESULT_3                0x311b
#define HEVC_CABAC_TOP_INFO                 0x311c
#define HEVC_CABAC_TOP_INFO_2               0x311d
#define HEVC_CABAC_LEFT_INFO                0x311e
#define HEVC_CABAC_LEFT_INFO_2              0x311f
#define HEVC_PARSER_INT_CONTROL             0x3120
#define HEVC_PARSER_INT_STATUS              0x3121
#define HEVC_PARSER_IF_CONTROL              0x3122
#define HEVC_PARSER_PICTURE_SIZE            0x3123
#define HEVC_PARSER_LCU_START               0x3124
#define HEVC_PARSER_HEADER_INFO2            0x3125
#define HEVC_PARSER_QUANT_READ              0x3126
#define HEVC_PARSER_RESERVED_27             0x3127
#define HEVC_PARSER_CMD_SKIP_0              0x3128
#define HEVC_PARSER_CMD_SKIP_1              0x3129
#define HEVC_PARSER_CMD_SKIP_2              0x312a
#define HEVC_PARSER_MANUAL_CMD              0x312b
#define HEVC_PARSER_MEM_RD_ADDR             0x312c
#define HEVC_PARSER_MEM_WR_ADDR             0x312d
#define HEVC_PARSER_MEM_RW_DATA             0x312e
#define HEVC_SAO_IF_STATUS                  0x3130
#define HEVC_SAO_IF_DATA_Y                  0x3131
#define HEVC_SAO_IF_DATA_U                  0x3132
#define HEVC_SAO_IF_DATA_V                  0x3133
#define HEVC_STREAM_SWAP_ADDR               0x3134
#define HEVC_STREAM_SWAP_CTRL               0x3135
#define HEVC_IQIT_IF_WAIT_CNT               0x3136
#define HEVC_MPRED_IF_WAIT_CNT              0x3137
#define HEVC_SAO_IF_WAIT_CNT                0x3138
/* from s5 */
#define HEVC_PARSER_IF_MONITOR_CTRL         (0x3136 | MASK_S5_NEW_REGS)
#define HEVC_PARSER_IF_MONITOR_DATA         (0x3137 | MASK_S5_NEW_REGS)
#define HEVC_STREAM_PACKET_LENGTH           (0x3139 | MASK_S5_NEW_REGS)


#define HEVC_PARSER_DEBUG_IDX               0x313e
#define HEVC_PARSER_DEBUG_DAT               0x313f
/* from s5*/
#define VP9_CONTROL                         (0x3140 | MASK_S5_NEW_REGS)
#define VP9_QUANT_WR                        (0x3146 | MASK_S5_NEW_REGS)

#define HEVC_SLICE_DATA_CTL                 (0x3172 | MASK_S5_NEW_REGS)
#define HEVC_STREAM_CRC                     (0x3175 | MASK_S5_NEW_REGS)
#define VP9_ACP_CTRL                        (0x3176 | MASK_S5_NEW_REGS)

#define HEVC_MPRED_VERSION                  0x3200
#define HEVC_MPRED_CTRL0                    0x3201
#define HEVC_MPRED_CTRL1                    0x3202
#define HEVC_MPRED_INT_EN                   0x3203
#define HEVC_MPRED_INT_STATUS               0x3204
#define HEVC_MPRED_PIC_SIZE                 0x3205
#define HEVC_MPRED_PIC_SIZE_LCU             0x3206
#define HEVC_MPRED_TILE_START               0x3207
#define HEVC_MPRED_TILE_SIZE_LCU            0x3208
#define HEVC_MPRED_REF_NUM                  0x3209
#define HEVC_MPRED_LT_REF                   0x320a
#define HEVC_MPRED_LT_COLREF                0x320b
#define HEVC_MPRED_REF_EN_L0                0x320c
#define HEVC_MPRED_REF_EN_L1                0x320d
#define HEVC_MPRED_COLREF_EN_L0             0x320e
#define HEVC_MPRED_COLREF_EN_L1             0x320f
#define HEVC_MPRED_AXI_WCTRL                0x3210
#define HEVC_MPRED_AXI_RCTRL                0x3211
#define HEVC_MPRED_ABV_START_ADDR           0x3212
#define HEVC_MPRED_MV_WR_START_ADDR         0x3213
#define HEVC_MPRED_MV_RD_START_ADDR         0x3214
#define HEVC_MPRED_MV_WPTR                  0x3215
#define HEVC_MPRED_MV_RPTR                  0x3216
#define HEVC_MPRED_MV_WR_ROW_JUMP           0x3217
#define HEVC_MPRED_MV_RD_ROW_JUMP           0x3218
#define HEVC_MPRED_CURR_LCU                 0x3219
#define HEVC_MPRED_ABV_WPTR                 0x321a
#define HEVC_MPRED_ABV_RPTR                 0x321b
#define HEVC_MPRED_CTRL2                    0x321c
#define HEVC_MPRED_CTRL3                    0x321d
#define HEVC_MPRED_MV_WLCUY                 0x321e
#define HEVC_MPRED_MV_RLCUY                 0x321f
#define HEVC_MPRED_L0_REF00_POC             0x3220
#define HEVC_MPRED_L0_REF01_POC             0x3221
#define HEVC_MPRED_L0_REF02_POC             0x3222
#define HEVC_MPRED_L0_REF03_POC             0x3223
#define HEVC_MPRED_L0_REF04_POC             0x3224
#define HEVC_MPRED_L0_REF05_POC             0x3225
#define HEVC_MPRED_L0_REF06_POC             0x3226
#define HEVC_MPRED_L0_REF07_POC             0x3227
#define HEVC_MPRED_L0_REF08_POC             0x3228
#define HEVC_MPRED_L0_REF09_POC             0x3229
#define HEVC_MPRED_L0_REF10_POC             0x322a
#define HEVC_MPRED_L0_REF11_POC             0x322b
#define HEVC_MPRED_L0_REF12_POC             0x322c
#define HEVC_MPRED_L0_REF13_POC             0x322d
#define HEVC_MPRED_L0_REF14_POC             0x322e
#define HEVC_MPRED_L0_REF15_POC             0x322f
#define HEVC_MPRED_L1_REF00_POC             0x3230
#define HEVC_MPRED_L1_REF01_POC             0x3231
#define HEVC_MPRED_L1_REF02_POC             0x3232
#define HEVC_MPRED_L1_REF03_POC             0x3233
#define HEVC_MPRED_L1_REF04_POC             0x3234
#define HEVC_MPRED_L1_REF05_POC             0x3235
#define HEVC_MPRED_L1_REF06_POC             0x3236
#define HEVC_MPRED_L1_REF07_POC             0x3237
#define HEVC_MPRED_L1_REF08_POC             0x3238
#define HEVC_MPRED_L1_REF09_POC             0x3239
#define HEVC_MPRED_L1_REF10_POC             0x323a
#define HEVC_MPRED_L1_REF11_POC             0x323b
#define HEVC_MPRED_L1_REF12_POC             0x323c
#define HEVC_MPRED_L1_REF13_POC             0x323d
#define HEVC_MPRED_L1_REF14_POC             0x323e
#define HEVC_MPRED_L1_REF15_POC             0x323f
#define HEVC_MPRED_PIC_SIZE_EXT             0x3240
#define HEVC_MPRED_DBG_MODE0                0x3241
#define HEVC_MPRED_DBG_MODE1                0x3242
#define HEVC_MPRED_DBG2_MODE                0x3243
#define HEVC_MPRED_IMP_CMD0                 0x3244
#define HEVC_MPRED_IMP_CMD1                 0x3245
#define HEVC_MPRED_IMP_CMD2                 0x3246
#define HEVC_MPRED_IMP_CMD3                 0x3247
#define HEVC_MPRED_DBG2_DATA_0              0x3248
#define HEVC_MPRED_DBG2_DATA_1              0x3249
#define HEVC_MPRED_DBG2_DATA_2              0x324a
#define HEVC_MPRED_DBG2_DATA_3              0x324b
#define HEVC_MPRED_DBG_DATA_0               0x3250
#define HEVC_MPRED_DBG_DATA_1               0x3251
#define HEVC_MPRED_DBG_DATA_2               0x3252
#define HEVC_MPRED_DBG_DATA_3               0x3253
#define HEVC_MPRED_DBG_DATA_4               0x3254
#define HEVC_MPRED_DBG_DATA_5               0x3255
#define HEVC_MPRED_DBG_DATA_6               0x3256
#define HEVC_MPRED_DBG_DATA_7               0x3257

/* from s5 */
#define HEVC_MPRED_POC24_CTRL0                     (0x324e | MASK_S5_NEW_REGS)
#define HEVC_MPRED_POC24_CTRL1                     (0x324f | MASK_S5_NEW_REGS)

#define HEVC_MPRED_CTRL6                           (0x3258 | MASK_S5_NEW_REGS)
#define HEVC_MPRED_CTRL7                           (0x3259 | MASK_S5_NEW_REGS)
#define HEVC_MPRED_CTRL8                           (0x325a | MASK_S5_NEW_REGS)
#define HEVC_MPRED_CTRL9                           (0x325b)
#define HEVC_MPRED_MCR_CNT_CTL                     (0x325c | MASK_S5_NEW_REGS)
#define HEVC_MPRED_MCR_CNT_DATA                    (0x325d | MASK_S5_NEW_REGS)
#define HEVC_MPRED_CTRL10                          (0x325e | MASK_S5_NEW_REGS)
#define HEVC_MPRED_CTRL11                          (0x325f | MASK_S5_NEW_REGS)

#define HEVC_MPRED_CUR_POC                  0x3260
#define HEVC_MPRED_COL_POC                  0x3261
#define HEVC_MPRED_MV_RD_END_ADDR           0x3262
#define HEVCD_IPP_TOP_CNTL                  0x3400
#define HEVCD_IPP_TOP_STATUS                0x3401
#define HEVCD_IPP_TOP_FRMCONFIG             0x3402
#define HEVCD_IPP_TOP_TILECONFIG1           0x3403
#define HEVCD_IPP_TOP_TILECONFIG2           0x3404
#define HEVCD_IPP_TOP_TILECONFIG3           0x3405
#define HEVCD_IPP_TOP_LCUCONFIG             0x3406
#define HEVCD_IPP_TOP_FRMCTL                0x3407
#define HEVCD_IPP_CONFIG                    0x3408
#define HEVCD_IPP_LINEBUFF_BASE             0x3409
#define HEVCD_IPP_INTR_MASK                 0x340a
#define HEVCD_IPP_AXIIF_CONFIG              0x340b
#define HEVCD_IPP_BITDEPTH_CONFIG           0x340c
#define HEVCD_IPP_SWMPREDIF_CONFIG          0x3410
#define HEVCD_IPP_SWMPREDIF_STATUS          0x3411
#define HEVCD_IPP_SWMPREDIF_CTBINFO         0x3412
#define HEVCD_IPP_SWMPREDIF_PUINFO0         0x3413
#define HEVCD_IPP_SWMPREDIF_PUINFO1         0x3414
#define HEVCD_IPP_SWMPREDIF_PUINFO2         0x3415
#define HEVCD_IPP_SWMPREDIF_PUINFO3         0x3416
#define HEVCD_IPP_DYNCLKGATE_CONFIG         0x3420
#define HEVCD_IPP_DYNCLKGATE_STATUS         0x3421
#define HEVCD_IPP_DBG_SEL                   0x3430
#define HEVCD_IPP_DBG_DATA                  0x3431
#define HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR     0x3460
#define HEVCD_MPP_ANC2AXI_TBL_CMD_ADDR      0x3461
#define HEVCD_MPP_ANC2AXI_TBL_WDATA_ADDR    0x3462
#define HEVCD_MPP_ANC2AXI_TBL_RDATA_ADDR    0x3463
#define HEVCD_MPP_WEIGHTPRED_CNTL_ADDR      0x347b
#define HEVCD_MPP_L0_WEIGHT_FLAG_ADDR       0x347c
#define HEVCD_MPP_L1_WEIGHT_FLAG_ADDR       0x347d
#define HEVCD_MPP_YLOG2WGHTDENOM_ADDR       0x347e
#define HEVCD_MPP_DELTACLOG2WGHTDENOM_ADDR  0x347f
#define HEVCD_MPP_WEIGHT_ADDR               0x3480
#define HEVCD_MPP_WEIGHT_DATA               0x3481
#define HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR 0x34c0
#define HEVCD_MPP_ANC_CANVAS_DATA_ADDR      0x34c1
#define HEVCD_MPP_DECOMP_CTL1               0x34c2
#define HEVCD_MPP_DECOMP_CTL2               0x34c3
#define HEVCD_MPP_DECOMP_PERFMON_CTL        0x34c5
#define HEVCD_MPP_DECOMP_PERFMON_DATA       0x34c6
#define HEVCD_MCRCC_CTL1                    0x34f0
#define HEVCD_MCRCC_CTL2                    0x34f1
#define HEVCD_MCRCC_CTL3                    0x34f2
#define HEVCD_MCRCC_PERFMON_CTL             0x34f3
#define HEVCD_MCRCC_PERFMON_DATA            0x34f4
#define HEVC_DBLK_CFG0                      0x3500
#define HEVC_DBLK_CFG1                      0x3501
#define HEVC_DBLK_CFG2                      0x3502
#define HEVC_DBLK_CFG3                      0x3503
#define HEVC_DBLK_CFG4                      0x3504
#define HEVC_DBLK_CFG5                      0x3505
#define HEVC_DBLK_CFG6                      0x3506
#define HEVC_DBLK_CFG7                      0x3507
#define HEVC_DBLK_CFG8                      0x3508
#define HEVC_DBLK_CFG9                      0x3509
#define HEVC_DBLK_CFGA                      0x350a
#define HEVC_DBLK_CFGE                      0x350e
#define HEVC_DBLK_STS0                      0x350b
/* changes the val to 0x350f on g12a */
#define HEVC_DBLK_STS1                      0x350c
/* changes the val to 0x3510 on g12a */
#define HEVC_SAO_VERSION                    0x3600
#define HEVC_SAO_CTRL0                      0x3601
#define HEVC_SAO_CTRL1                      0x3602
#define HEVC_SAO_INT_EN                     0x3603
#define HEVC_SAO_INT_STATUS                 0x3604
#define HEVC_SAO_PIC_SIZE                   0x3605
#define HEVC_SAO_PIC_SIZE_LCU               0x3606
#define HEVC_SAO_TILE_START                 0x3607
#define HEVC_SAO_TILE_SIZE_LCU              0x3608
#define HEVC_SAO_AXI_WCTRL                  0x3609
#define HEVC_SAO_AXI_RCTRL                  0x360a
#define HEVC_SAO_Y_START_ADDR               0x360b
#define HEVC_SAO_Y_LENGTH                   0x360c
#define HEVC_SAO_C_START_ADDR               0x360d
#define HEVC_SAO_C_LENGTH                   0x360e
#define HEVC_SAO_Y_WPTR                     0x360f
#define HEVC_SAO_C_WPTR                     0x3610
#define HEVC_SAO_ABV_START_ADDR             0x3611
#define HEVC_SAO_VB_WR_START_ADDR           0x3612
#define HEVC_SAO_VB_RD_START_ADDR           0x3613
#define HEVC_SAO_ABV_WPTR                   0x3614
#define HEVC_SAO_ABV_RPTR                   0x3615
#define HEVC_SAO_VB_WPTR                    0x3616
#define HEVC_SAO_VB_RPTR                    0x3617
#define HEVC_SAO_DBG_MODE0                  0x361e
#define HEVC_SAO_DBG_MODE1                  0x361f
#define HEVC_SAO_CTRL2                      0x3620
#define HEVC_SAO_CTRL3                      0x3621
#define HEVC_SAO_CTRL4                      0x3622
#define HEVC_SAO_CTRL5                      0x3623
#define HEVC_SAO_CTRL6                      0x3624
#define HEVC_SAO_CTRL7                      0x3625
#define HEVC_SAO_CTRL8                      0x362c
//axi_idle_thred=sao_ctrl8[15:0]
#define HEVC_SAO_CTRL9                      0x362d


#define HEVC_SAO_CTRL26                     0x3677
#define HEVC_SAO_DBG_DATA_0                 0x3630
#define HEVC_SAO_DBG_DATA_1                 0x3631
#define HEVC_SAO_DBG_DATA_2                 0x3632
#define HEVC_SAO_DBG_DATA_3                 0x3633
#define HEVC_SAO_DBG_DATA_4                 0x3634
#define HEVC_SAO_DBG_DATA_5                 0x3635
#define HEVC_SAO_DBG_DATA_6                 0x3636
#define HEVC_SAO_DBG_DATA_7                 0x3637
#define HEVC_SAO_MMU_STATUS                 0x3639
#define HEVC_SAO_MMU_DMA_CTRL               0x363e
#define HEVC_SAO_MMU_DMA_STATUS             0x363f
#define HEVC_CM_CORE_STATUS                 0x3640
#define HEVC_SAO_MMU_RESET_CTRL             0x3641

/* T3X triple write */
#define HEVC_SAO_Y_START_ADDR3              0x3698
#define HEVC_SAO_Y_LENGTH3                  0x3699
#define HEVC_SAO_C_START_ADDR3              0x369a
#define HEVC_SAO_C_LENGTH3                  0x369b

//SAO EXT BUF config for S1A
#define HEVC_SAO_Y2_START_ADDR              0x36a0
#define HEVC_SAO_Y2_LENGTH                  0x36a1
#define HEVC_SAO_C2_START_ADDR              0x36a2
#define HEVC_SAO_C2_LENGTH                  0x36a3
#define HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR_EXTRA 0x3495
#define HEVCD_MPP_ANC2AXI_TBL_DATA_EXTRA    0x3496

#define HEVC_IQIT_CLK_RST_CTRL              0x3700
#define HEVC_IQIT_DEQUANT_CTRL              0x3701
#define HEVC_IQIT_SCALELUT_WR_ADDR          0x3702
#define HEVC_IQIT_SCALELUT_RD_ADDR          0x3703
#define HEVC_IQIT_SCALELUT_DATA             0x3704
#define HEVC_IQIT_SCALELUT_IDX_4            0x3705
#define HEVC_IQIT_SCALELUT_IDX_8            0x3706
#define HEVC_IQIT_SCALELUT_IDX_16_32        0x3707
#define HEVC_IQIT_STAT_GEN0                 0x3708
#define HEVC_QP_WRITE                       0x3709
#define HEVC_IQIT_STAT_GEN1                 0x370a
#define HEVC_IQIT_BITDEPTH                  0x370b
#define HEVC_IQIT_STAT_GEN2                 0x370c
#define HEVC_IQIT_AVS2_WQP_0123             0x370d
#define HEVC_IQIT_AVS2_WQP_45               0x370e
#define HEVC_IQIT_AVS2_QP_DELTA             0x370f
#define HEVC_PIC_QUALITY_CTRL               0x3710
#define HEVC_PIC_QUALITY_DATA               0x3711

/*from t3 0x3419*/
#define AV1D_IPP_DIR_CFG                    0x3490
/* add from s5 */
#define HEVCD_IPP_AXIADDR_PREFIX                   (0x3418 | MASK_S5_NEW_REGS)
#define VP9D_MPP_REF_SCALE_ENBL                    (0x3441)
#define VP9D_MPP_REFINFO_TBL_ACCCONFIG             (0x3442)
#define VP9D_MPP_REFINFO_DATA                      (0x3443)


#define HEVCD_IPP_MULTICORE_CFG                    (0x34a0 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_MULTICORE_LINE_CTL               (0x34a1 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_LINEBUFF_BASE2                   (0x34a2 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_DYN_CACHE                        (0x34a3 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_CTRL0                            (0x34a4 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_CTRL1                            (0x34a5 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_CTRL2                            (0x34a6 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_CTRL3                            (0x34a7 | MASK_S5_NEW_REGS)

#define HEVCD_MPP_DECOMP_AXIURG_CTL                0x34c7

#define HEVC_DBLK_MCP                              (0x3529 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_SLICNT                           (0x352a | MASK_S5_NEW_REGS)
#define HEVC_DBLK_INTRPT                           (0x352b | MASK_S5_NEW_REGS)
#define HEVC_OW_FRAME_CNT                          (0x3668 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL12                            (0x3669 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL13                            (0x366a | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL14                            (0x366b | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL15                            (0x366c | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL16                            (0x366d | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL17                            (0x366e | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL18                            (0x366f | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL19                            (0x3670 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL20                            (0x3671 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL21                            (0x3672 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL22                            (0x3673 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL23                            (0x3674 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL24                            (0x3675 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL25                            (0x3676 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL27                            (0x3678 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL28                            (0x3679 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL29                            (0x367a | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL30                            (0x367b | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL31                            (0x367c | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL32                            (0x367d | MASK_S5_NEW_REGS)
#define HEVC_OW_AXIADDR_PREFIX                     (0x367e | MASK_S5_NEW_REGS)
#define HEVC_OW_AXIADDR_PREFIX2                    (0x367f | MASK_S5_NEW_REGS)
#define HEVC_SAO_STATUS0                           (0x3680 | MASK_S5_NEW_REGS)
#define HEVC_SAO_STATUS1                           (0x3681 | MASK_S5_NEW_REGS)
#define HEVC_SAO_STATUS2                           (0x3682 | MASK_S5_NEW_REGS)
#define HEVC_SAO_STATUS3                           (0x3683 | MASK_S5_NEW_REGS)
#define HEVC_SAO_STATUS4                           (0x3684 | MASK_S5_NEW_REGS)
#define HEVC_SAO_STATUS5                           (0x3685 | MASK_S5_NEW_REGS)
#define HEVC_SAO_STATUS6                           (0x3686 | MASK_S5_NEW_REGS)
#define HEVC_SAO_STATUS7                           (0x3687 | MASK_S5_NEW_REGS)
#define COPY_REG_R0                                (0x3688 | MASK_S5_NEW_REGS)
#define COPY_REG_R1                                (0x3689 | MASK_S5_NEW_REGS)
#define COPY_SEL                                   (0x368a | MASK_S5_NEW_REGS)
#define COPY_REG_R3                                (0x368b | MASK_S5_NEW_REGS)
#define COPY_REG_R4                                (0x368c | MASK_S5_NEW_REGS)
#define COPY_REG_R5                                (0x368d | MASK_S5_NEW_REGS)
#define HEVC_SAO_SHADOWMODE_CNTL                   (0x368e | MASK_S5_NEW_REGS)
#define HEVC_SAO_CRC                               (0x3690 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CRC_Y                             (0x3691 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CRC_C                             (0x3692 | MASK_S5_NEW_REGS)
#define HEVC_IQIT_STAT_GEN4                        (0x3721 | MASK_S5_NEW_REGS)
#define HEVC_IQIT_STAT_GEN5                        (0x3722 | MASK_S5_NEW_REGS)

#define HEVC_PATH_MONITOR_CTRL                     (0x3712 | MASK_S5_NEW_REGS)
#define HEVC_PATH_MONITOR_DATA                     (0x3713 | MASK_S5_NEW_REGS)

/* for s5 back end */
#define HEVCD_IPP_TOP_CNTL_DBE1                    (0x3900 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_TOP_STATUS_DBE1                  (0x3901 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_TOP_FRMCONFIG_DBE1               (0x3902 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_TOP_TILECONFIG1_DBE1             (0x3903 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_TOP_TILECONFIG2_DBE1             (0x3904 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_TOP_TILECONFIG3_DBE1             (0x3905 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_TOP_LCUCONFIG_DBE1               (0x3906 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_TOP_FRMCTL_DBE1                  (0x3907 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_CONFIG_DBE1                      (0x3908 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_LINEBUFF_BASE_DBE1               (0x3909 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_INTR_MASK_DBE1                   (0x390a | MASK_S5_NEW_REGS)
#define HEVCD_IPP_AXIIF_CONFIG_DBE1                (0x390b | MASK_S5_NEW_REGS)
#define HEVCD_IPP_BITDEPTH_CONFIG_DBE1             (0x390c | MASK_S5_NEW_REGS)
#define HEVCD_IPP_RTL_CONFIG_DBE1                  (0x390d | MASK_S5_NEW_REGS)
#define HEVCD_IPP_SHADOWMODE_CNTL_DBE1             (0x390e | MASK_S5_NEW_REGS)
#define HEVCD_IPP_AXIADDR_PREFIX_DBE1              (0x3918 | MASK_S5_NEW_REGS)
#define AV1D_IPP_DIR_CFG_DBE1                      (0x3919 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_DYNCLKGATE_CONFIG_DBE1           (0x3920 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_DYNCLKGATE_STATUS_DBE1           (0x3921 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_DBG_SEL_DBE1                     (0x3930 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_DBG_DATA_DBE1                    (0x3931 | MASK_S5_NEW_REGS)
#define HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR_DBE1       (0x3960 | MASK_S5_NEW_REGS)
#define HEVCD_MPP_ANC2AXI_TBL_DATA_DBE1            (0x3964 | MASK_S5_NEW_REGS)
#define HEVCD_MPP_WEIGHTPRED_CNTL_ADDR_DBE1        (0x397b | MASK_S5_NEW_REGS)
#define HEVCD_MPP_L0_WEIGHT_FLAG_ADDR_DBE1         (0x397c | MASK_S5_NEW_REGS)
#define HEVCD_MPP_L1_WEIGHT_FLAG_ADDR_DBE1         (0x397d | MASK_S5_NEW_REGS)
#define HEVCD_MPP_YLOG2WGHTDENOM_ADDR_DBE1         (0x397e | MASK_S5_NEW_REGS)
#define HEVCD_MPP_DELTACLOG2WGHTDENOM_ADDR_DBE1    (0x397f | MASK_S5_NEW_REGS)
#define HEVCD_MPP_WEIGHT_ADDR_DBE1                 (0x3980 | MASK_S5_NEW_REGS)
#define HEVCD_MPP_WEIGHT_DATA_DBE1                 (0x3981 | MASK_S5_NEW_REGS)
#define HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR_DBE1   (0x39c0 | MASK_S5_NEW_REGS)
#define HEVCD_MPP_ANC_CANVAS_DATA_ADDR_DBE1        (0x39c1 | MASK_S5_NEW_REGS)
#define HEVCD_MPP_DECOMP_CTL1_DBE1                 (0x39c2 | MASK_S5_NEW_REGS)
#define HEVCD_MPP_DECOMP_CTL2_DBE1                 (0x39c3 | MASK_S5_NEW_REGS)
#define HEVCD_MPP_DECOMP_CTL3_DBE1                 (0x39c4 | MASK_S5_NEW_REGS)
#define HEVCD_MPP_DECOMP_PERFMON_CTL_DBE1          (0x39c5 | MASK_S5_NEW_REGS)
#define HEVCD_MPP_DECOMP_PERFMON_DATA_DBE1         (0x39c6 | MASK_S5_NEW_REGS)
#define HEVCD_MPP_DECOMP_AXIURG_CTL_DBE1           (0x39c7 | MASK_S5_NEW_REGS)
#define HEVCD_MPP_VDEC_MCR_CTL_DBE1                (0x39c8 | MASK_S5_NEW_REGS)
#define HEVCD_MCRCC_CTL1_DBE1                      (0x39f0 | MASK_S5_NEW_REGS)
#define HEVCD_MCRCC_CTL2_DBE1                      (0x39f1 | MASK_S5_NEW_REGS)
#define HEVCD_MCRCC_CTL3_DBE1                      (0x39f2 | MASK_S5_NEW_REGS)
#define HEVCD_MCRCC_PERFMON_CTL_DBE1               (0x39f3 | MASK_S5_NEW_REGS)
#define HEVCD_MCRCC_PERFMON_DATA_DBE1              (0x39f4 | MASK_S5_NEW_REGS)
#define HEVCD_MCRCC_STALL_ADJUST_DBE1              (0x39f5 | MASK_S5_NEW_REGS)
#define VP9D_MPP_INTERPOL_CFG0_DBE1                (0x3940 | MASK_S5_NEW_REGS)
#define VP9D_MPP_REF_SCALE_ENBL_DBE1               (0x3941 | MASK_S5_NEW_REGS)
#define VP9D_MPP_REFINFO_TBL_ACCCONFIG_DBE1        (0x3942 | MASK_S5_NEW_REGS)
#define VP9D_MPP_REFINFO_DATA_DBE1                 (0x3943 | MASK_S5_NEW_REGS)
#define AV1D_MPP_REF2WMMAT_TBL_CONF_ADDR_DBE1      (0x3991 | MASK_S5_NEW_REGS)
#define AV1D_MPP_REF2WMMAT_TBL_DATA_DBE1           (0x3992 | MASK_S5_NEW_REGS)
#define AV1D_MPP_ORDERHINT_CFG_DBE1                (0x3993 | MASK_S5_NEW_REGS)
#define AV1D_MPP_MISC_CFG_DBE1                     (0x3994 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_MULTICORE_CFG_DBE1               (0x39a0 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_MULTICORE_LINE_CTL_DBE1          (0x39a1 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_LINEBUFF_BASE2_DBE1              (0x39a2 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_DYN_CACHE_DBE1                   (0x39a3 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_CTRL0_DBE1                       (0x39a4 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_CTRL1_DBE1                       (0x39a5 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_CTRL2_DBE1                       (0x39a6 | MASK_S5_NEW_REGS)
#define HEVCD_IPP_CTRL3_DBE1                       (0x39a7 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CFG0_DBE1                        (0x3a00 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CFG1_DBE1                        (0x3a01 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CFG2_DBE1                        (0x3a02 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CFG3_DBE1                        (0x3a03 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CFG4_DBE1                        (0x3a04 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CFG5_DBE1                        (0x3a05 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CFG6_DBE1                        (0x3a06 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CFG7_DBE1                        (0x3a07 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CFG8_DBE1                        (0x3a08 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CFG9_DBE1                        (0x3a09 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CFGA_DBE1                        (0x3a0a | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CFGB_DBE1                        (0x3a0b | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CFGC_DBE1                        (0x3a0c | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CFGD_DBE1                        (0x3a0d | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CFGE_DBE1                        (0x3a0e | MASK_S5_NEW_REGS)
#define HEVC_DBLK_STS0_DBE1                        (0x3a0f | MASK_S5_NEW_REGS)
#define HEVC_DBLK_STS1_DBE1                        (0x3a10 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CFG11_DBE1                       (0x3a11 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CFG12_DBE1                       (0x3a12 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CFG13_DBE1                       (0x3a13 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CFG14_DBE1                       (0x3a14 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CDEF0_DBE1                       (0x3a15 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CDEF1_DBE1                       (0x3a16 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CDEF2_DBE1                       (0x3a17 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CDEF3_DBE1                       (0x3a18 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CDEF4_DBE1                       (0x3a19 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_CDEF5_DBE1                       (0x3a1a | MASK_S5_NEW_REGS)
#define HEVC_DBLK_UPS0_DBE1                        (0x3a1b | MASK_S5_NEW_REGS)
#define HEVC_DBLK_UPS1_DBE1                        (0x3a1c | MASK_S5_NEW_REGS)
#define HEVC_DBLK_UPS2_DBE1                        (0x3a1d | MASK_S5_NEW_REGS)
#define HEVC_DBLK_UPS3_DBE1                        (0x3a1e | MASK_S5_NEW_REGS)
#define HEVC_DBLK_UPS4_DBE1                        (0x3a1f | MASK_S5_NEW_REGS)
#define HEVC_DBLK_UPS5_DBE1                        (0x3a20 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_LRF0_DBE1                        (0x3a21 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_LRF1_DBE1                        (0x3a22 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_DBLK0_DBE1                       (0x3a23 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_DBLK1_DBE1                       (0x3a24 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_DBLK2_DBE1                       (0x3a25 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_PREFIX_DBE1                      (0x3a26 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_BUSYSEL_DBE1                     (0x3a27 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_SHADOWMODE_CNTL_DBE1             (0x3a28 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_MCP_DBE1                         (0x3a29 | MASK_S5_NEW_REGS)
#define HEVC_DBLK_SLICNT_DBE1                      (0x3a2a | MASK_S5_NEW_REGS)
#define HEVC_DBLK_INTRPT_DBE1                      (0x3a2b | MASK_S5_NEW_REGS)
#define HEVC_SAO_VERSION_DBE1                      (0x3b00 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL0_DBE1                        (0x3b01 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL1_DBE1                        (0x3b02 | MASK_S5_NEW_REGS)

#define HEVC_SAO_INT_EN_DBE1                       (0x3b03 | MASK_S5_NEW_REGS)
#define HEVC_SAO_INT_STATUS_DBE1                   (0x3b04 | MASK_S5_NEW_REGS)
#define HEVC_SAO_PIC_SIZE_DBE1                     (0x3b05 | MASK_S5_NEW_REGS)
#define HEVC_SAO_PIC_SIZE_LCU_DBE1                 (0x3b06 | MASK_S5_NEW_REGS)
#define HEVC_SAO_TILE_START_DBE1                   (0x3b07 | MASK_S5_NEW_REGS)
#define HEVC_SAO_TILE_SIZE_LCU_DBE1                (0x3b08 | MASK_S5_NEW_REGS)
#define HEVC_SAO_AXI_WCTRL_DBE1                    (0x3b09 | MASK_S5_NEW_REGS)
#define HEVC_SAO_AXI_RCTRL_DBE1                    (0x3b0a | MASK_S5_NEW_REGS)
#define HEVC_SAO_Y_START_ADDR_DBE1                 (0x3b0b | MASK_S5_NEW_REGS)
#define HEVC_SAO_Y_LENGTH_DBE1                     (0x3b0c | MASK_S5_NEW_REGS)
#define HEVC_SAO_C_START_ADDR_DBE1                 (0x3b0d | MASK_S5_NEW_REGS)
#define HEVC_SAO_C_LENGTH_DBE1                     (0x3b0e | MASK_S5_NEW_REGS)
#define HEVC_SAO_Y_WPTR_DBE1                       (0x3b0f | MASK_S5_NEW_REGS)
#define HEVC_SAO_C_WPTR_DBE1                       (0x3b10 | MASK_S5_NEW_REGS)
#define HEVC_SAO_ABV_START_ADDR_DBE1               (0x3b11 | MASK_S5_NEW_REGS)
#define HEVC_SAO_VB_WR_START_ADDR_DBE1             (0x3b12 | MASK_S5_NEW_REGS)
#define HEVC_SAO_VB_RD_START_ADDR_DBE1             (0x3b13 | MASK_S5_NEW_REGS)
#define HEVC_SAO_ABV_WPTR_DBE1                     (0x3b14 | MASK_S5_NEW_REGS)
#define HEVC_SAO_ABV_RPTR_DBE1                     (0x3b15 | MASK_S5_NEW_REGS)
#define HEVC_SAO_VB_WPTR_DBE1                      (0x3b16 | MASK_S5_NEW_REGS)
#define HEVC_SAO_VB_RPTR_DBE1                      (0x3b17 | MASK_S5_NEW_REGS)
#define HEVC_SAO_DBG_MODE0_DBE1                    (0x3b1e | MASK_S5_NEW_REGS)
#define HEVC_SAO_DBG_MODE1_DBE1                    (0x3b1f | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL2_DBE1                        (0x3b20 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL3_DBE1                        (0x3b21 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL4_DBE1                        (0x3b22 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL5_DBE1                        (0x3b23 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL6_DBE1                        (0x3b24 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL7_DBE1                        (0x3b25 | MASK_S5_NEW_REGS)
#define HEVC_CM_BODY_START_ADDR_DBE1               (0x3b26 | MASK_S5_NEW_REGS)
#define HEVC_CM_BODY_LENGTH_DBE1                   (0x3b27 | MASK_S5_NEW_REGS)
#define HEVC_CM_HEADER_START_ADDR_DBE1             (0x3b28 | MASK_S5_NEW_REGS)
#define HEVC_CM_HEADER_LENGTH_DBE1                 (0x3b29 | MASK_S5_NEW_REGS)
#define HEVC_CM_COLOR_DBE1                         (0x3b2a | MASK_S5_NEW_REGS)
#define HEVC_CM_HEADER_OFFSET_DBE1                 (0x3b2b | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL8_DBE1                        (0x3b2c | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL9_DBE1                        (0x3b2d | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL10_DBE1                       (0x3b2e | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL11_DBE1                       (0x3b2f | MASK_S5_NEW_REGS)
#define HEVC_SAO_DBG_DATA_0_DBE1                   (0x3b30 | MASK_S5_NEW_REGS)
#define HEVC_SAO_DBG_DATA_1_DBE1                   (0x3b31 | MASK_S5_NEW_REGS)
#define HEVC_SAO_DBG_DATA_2_DBE1                   (0x3b32 | MASK_S5_NEW_REGS)
#define HEVC_SAO_DBG_DATA_3_DBE1                   (0x3b33 | MASK_S5_NEW_REGS)
#define HEVC_SAO_DBG_DATA_4_DBE1                   (0x3b34 | MASK_S5_NEW_REGS)
#define HEVC_SAO_DBG_DATA_5_DBE1                   (0x3b35 | MASK_S5_NEW_REGS)
#define HEVC_SAO_DBG_DATA_6_DBE1                   (0x3b36 | MASK_S5_NEW_REGS)
#define HEVC_SAO_DBG_DATA_7_DBE1                   (0x3b37 | MASK_S5_NEW_REGS)
#define HEVC_SAO_MMU_WR_DBE1                       (0x3b38 | MASK_S5_NEW_REGS)
#define HEVC_SAO_MMU_STATUS_DBE1                   (0x3b39 | MASK_S5_NEW_REGS)
#define HEVC_SAO_MMU_VH0_ADDR_DBE1                 (0x3b3a | MASK_S5_NEW_REGS)
#define HEVC_SAO_MMU_VH1_ADDR_DBE1                 (0x3b3b | MASK_S5_NEW_REGS)
#define HEVC_SAO_MMU_WPTR_DBE1                     (0x3b3c | MASK_S5_NEW_REGS)
#define HEVC_SAO_MMU_RPTR_DBE1                     (0x3b3d | MASK_S5_NEW_REGS)
#define HEVC_SAO_MMU_DMA_CTRL_DBE1                 (0x3b3e | MASK_S5_NEW_REGS)
#define HEVC_SAO_MMU_DMA_STATUS_DBE1               (0x3b3f | MASK_S5_NEW_REGS)
#define HEVC_CM_CORE_STATUS_DBE1                   (0x3b40 | MASK_S5_NEW_REGS)
#define HEVC_SAO_MMU_RESET_CTRL_DBE1               (0x3b41 | MASK_S5_NEW_REGS)
#define HEVC_CM_LOSSY_QUANT_CTRL_DBE1              (0x3b42 | MASK_S5_NEW_REGS)
#define HEVC_CM_LOSSY_RQUANT_YCLUT_ACCCONFIG_DBE1  (0x3b43 | MASK_S5_NEW_REGS)
#define HEVC_CM_LOSSY_RQUANT_YCLUT_DATA_DBE1       (0x3b44 | MASK_S5_NEW_REGS)
#define HEVC_CM_LOSSY_IQUANT_YCLUT_ACCCONFIG_DBE1  (0x3b45 | MASK_S5_NEW_REGS)
#define HEVC_CM_LOSSY_IQUANT_YCLUT_DATA_DBE1       (0x3b46 | MASK_S5_NEW_REGS)
#define HEVC_CM_AV1_TILE_LOC_X_DBE1                (0x3b47 | MASK_S5_NEW_REGS)
#define HEVC_CM_AV1_TILE_LOC_Y_DBE1                (0x3b48 | MASK_S5_NEW_REGS)
#define HEVC_CM_CORE_CTRL_DBE1                     (0x3b49 | MASK_S5_NEW_REGS)
#define HEVC_CM_HEADER_START_ADDR2_DBE1            (0x3b4a | MASK_S5_NEW_REGS)
#define HEVC_SAO_MMU_WR2_DBE1                      (0x3b4b | MASK_S5_NEW_REGS)
#define HEVC_SAO_MMU_DMA_CTRL2_DBE1                (0x3b4c | MASK_S5_NEW_REGS)
#define HEVC_SAO_MMU_VH0_ADDR2_DBE1                (0x3b4d | MASK_S5_NEW_REGS)
#define HEVC_SAO_MMU_VH1_ADDR2_DBE1                (0x3b4e | MASK_S5_NEW_REGS)
#define HEVC_CM_CORE_STATUS2_DBE1                  (0x3b4f | MASK_S5_NEW_REGS)
#define HEVC_SAO_MMU_STATUS2_DBE1                  (0x3b50 | MASK_S5_NEW_REGS)
#define HEVC_CM_LOSSY_QUANT_CTRL2_DBE1             (0x3b52 | MASK_S5_NEW_REGS)
#define HEVC_CM_LOSSY_RQUANT_YCLUT_ACCCONFIG2_DBE1 (0x3b53 | MASK_S5_NEW_REGS)
#define HEVC_CM_LOSSY_RQUANT_YCLUT_DATA2_DBE1      (0x3b54 | MASK_S5_NEW_REGS)
#define HEVC_CM_LOSSY_IQUANT_YCLUT_ACCCONFIG2_DBE1 (0x3b55 | MASK_S5_NEW_REGS)
#define HEVC_CM_LOSSY_IQUANT_YCLUT_DATA2_DBE1      (0x3b56 | MASK_S5_NEW_REGS)
#define HEVC_CM_AV1_TILE_LOC_X2_DBE1               (0x3b57 | MASK_S5_NEW_REGS)
#define HEVC_CM_AV1_TILE_LOC_Y2_DBE1               (0x3b58 | MASK_S5_NEW_REGS)
#define HEVC_CM_CORE_CTRL2_DBE1                    (0x3b59 | MASK_S5_NEW_REGS)
#define HEVC_CM_BODY_START_ADDR2_DBE1              (0x3b5a | MASK_S5_NEW_REGS)
#define HEVC_FORCE_YUV_CTRL_DBE1                   (0x3b5b | MASK_S5_NEW_REGS)
#define HEVC_FORCE_YUV_0_DBE1                      (0x3b5c | MASK_S5_NEW_REGS)
#define HEVC_FORCE_YUV_1_DBE1                      (0x3b5d | MASK_S5_NEW_REGS)
#define HEVC_DW_VH0_ADDDR_DBE1                     (0x3b5e | MASK_S5_NEW_REGS)
#define HEVC_DW_VH1_ADDDR_DBE1                     (0x3b5f | MASK_S5_NEW_REGS)
#define HEVC_FGS_IDX_DBE1                          (0x3b60 | MASK_S5_NEW_REGS)
#define HEVC_FGS_DATA_DBE1                         (0x3b61 | MASK_S5_NEW_REGS)
#define HEVC_FGS_CTRL_DBE1                         (0x3b62 | MASK_S5_NEW_REGS)
#define HEVC_CM_BODY_LENGTH2_DBE1                  (0x3b63 | MASK_S5_NEW_REGS)
#define HEVC_CM_HEADER_OFFSET2_DBE1                (0x3b64 | MASK_S5_NEW_REGS)
#define HEVC_CM_HEADER_LENGTH2_DBE1                (0x3b65 | MASK_S5_NEW_REGS)
#define HEVC_FGS_TABLE_START_DBE1                  (0x3b66 | MASK_S5_NEW_REGS)
#define HEVC_FGS_TABLE_LENGTH_DBE1                 (0x3b67 | MASK_S5_NEW_REGS)
#define HEVC_OW_FRAME_CNT_DBE1                     (0x3b68 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL12_DBE1                       (0x3b69 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL13_DBE1                       (0x3b6a | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL14_DBE1                       (0x3b6b | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL15_DBE1                       (0x3b6c | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL16_DBE1                       (0x3b6d | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL17_DBE1                       (0x3b6e | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL18_DBE1                       (0x3b6f | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL19_DBE1                       (0x3b70 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL20_DBE1                       (0x3b71 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL21_DBE1                       (0x3b72 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL22_DBE1                       (0x3b73 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL23_DBE1                       (0x3b74 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL24_DBE1                       (0x3b75 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL25_DBE1                       (0x3b76 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL26_DBE1                       (0x3b77 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL27_DBE1                       (0x3b78 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL28_DBE1                       (0x3b79 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL29_DBE1                       (0x3b7a | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL30_DBE1                       (0x3b7b | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL31_DBE1                       (0x3b7c | MASK_S5_NEW_REGS)
#define HEVC_SAO_CTRL32_DBE1                       (0x3b7d | MASK_S5_NEW_REGS)
#define HEVC_OW_AXIADDR_PREFIX_DBE1                (0x3b7e | MASK_S5_NEW_REGS)
#define HEVC_OW_AXIADDR_PREFIX2_DBE1               (0x3b7f | MASK_S5_NEW_REGS)
#define HEVC_SAO_STATUS0_DBE1                      (0x3b80 | MASK_S5_NEW_REGS)
#define HEVC_SAO_STATUS1_DBE1                      (0x3b81 | MASK_S5_NEW_REGS)
#define HEVC_SAO_STATUS2_DBE1                      (0x3b82 | MASK_S5_NEW_REGS)
#define HEVC_SAO_STATUS3_DBE1                      (0x3b83 | MASK_S5_NEW_REGS)
#define HEVC_SAO_STATUS4_DBE1                      (0x3b84 | MASK_S5_NEW_REGS)
#define HEVC_SAO_STATUS5_DBE1                      (0x3b85 | MASK_S5_NEW_REGS)
#define HEVC_SAO_STATUS6_DBE1                      (0x3b86 | MASK_S5_NEW_REGS)
#define HEVC_SAO_STATUS7_DBE1                      (0x3b87 | MASK_S5_NEW_REGS)
#define COPY_REG_R0_DBE1                           (0x3b88 | MASK_S5_NEW_REGS)
#define COPY_REG_R1_DBE1                           (0x3b89 | MASK_S5_NEW_REGS)
#define COPY_SEL_DBE1                              (0x3b8a | MASK_S5_NEW_REGS)
#define COPY_REG_R3_DBE1                           (0x3b8b | MASK_S5_NEW_REGS)
#define COPY_REG_R4_DBE1                           (0x3b8c | MASK_S5_NEW_REGS)
#define COPY_REG_R5_DBE1                           (0x3b8d | MASK_S5_NEW_REGS)
#define HEVC_SAO_SHADOWMODE_CNTL_DBE1              (0x3b8e | MASK_S5_NEW_REGS)
#define HEVC_SAO_CRC_DBE1                          (0x3b90 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CRC_Y_DBE1                        (0x3b91 | MASK_S5_NEW_REGS)
#define HEVC_SAO_CRC_C_DBE1                        (0x3b92 | MASK_S5_NEW_REGS)
#define HEVC_IQIT_CLK_RST_CTRL_DBE1                (0x3c00 | MASK_S5_NEW_REGS)
#define HEVC_IQIT_DEQUANT_CTRL_DBE1                (0x3c01 | MASK_S5_NEW_REGS)
#define HEVC_IQIT_SCALELUT_WR_ADDR_DBE1            (0x3c02 | MASK_S5_NEW_REGS)
#define HEVC_IQIT_SCALELUT_RD_ADDR_DBE1            (0x3c03 | MASK_S5_NEW_REGS)
#define HEVC_IQIT_SCALELUT_DATA_DBE1               (0x3c04 | MASK_S5_NEW_REGS)
#define HEVC_IQIT_SCALELUT_IDX_4_DBE1              (0x3c05 | MASK_S5_NEW_REGS)
#define HEVC_IQIT_SCALELUT_IDX_8_DBE1              (0x3c06 | MASK_S5_NEW_REGS)
#define HEVC_IQIT_SCALELUT_IDX_16_32_DBE1          (0x3c07 | MASK_S5_NEW_REGS)
#define HEVC_IQIT_STAT_GEN0_DBE1                   (0x3c08 | MASK_S5_NEW_REGS)
#define HEVC_QP_WRITE_DBE1                         (0x3c09 | MASK_S5_NEW_REGS)
#define HEVC_IQIT_STAT_GEN1_DBE1                   (0x3c0a | MASK_S5_NEW_REGS)
#define HEVC_IQIT_BITDEPTH_DBE1                    (0x3c0b | MASK_S5_NEW_REGS)
#define HEVC_IQIT_STAT_GEN2_DBE1                   (0x3c0c | MASK_S5_NEW_REGS)
#define HEVC_IQIT_AVS2_WQP_0123_DBE1               (0x3c0d | MASK_S5_NEW_REGS)
#define HEVC_IQIT_AVS2_WQP_45_DBE1                 (0x3c0e | MASK_S5_NEW_REGS)
#define HEVC_IQIT_AVS2_QP_DELTA_DBE1               (0x3c0f | MASK_S5_NEW_REGS)
#define HEVC_PIC_QUALITY_CTRL_DBE1                 (0x3c10 | MASK_S5_NEW_REGS)
#define HEVC_PIC_QUALITY_DATA_DBE1                 (0x3c11 | MASK_S5_NEW_REGS)
#define HEVC_PATH_MONITOR_CTRL_DBE1                (0x3c12 | MASK_S5_NEW_REGS)
#define HEVC_PATH_MONITOR_DATA_DBE1                (0x3c13 | MASK_S5_NEW_REGS)
#define HEVC_IQIT_STAT_GEN3_DBE1                   (0x3c20 | MASK_S5_NEW_REGS)
#define HEVC_IQIT_STAT_GEN4_DBE1                   (0x3c21 | MASK_S5_NEW_REGS)
#define HEVC_IQIT_STAT_GEN5_DBE1                   (0x3c22 | MASK_S5_NEW_REGS)

/*add from M8M2*/
#define HEVC_MC_CTRL_REG                    0x3900
#define HEVC_MC_MB_INFO                     0x3901
#define HEVC_MC_PIC_INFO                    0x3902
#define HEVC_MC_HALF_PEL_ONE                0x3903
#define HEVC_MC_HALF_PEL_TWO                0x3904
#define HEVC_POWER_CTL_MC                   0x3905
#define HEVC_MC_CMD                         0x3906
#define HEVC_MC_CTRL0                       0x3907
#define HEVC_MC_PIC_W_H                     0x3908
#define HEVC_MC_STATUS0                     0x3909
#define HEVC_MC_STATUS1                     0x390a
#define HEVC_MC_CTRL1                       0x390b
#define HEVC_MC_MIX_RATIO0                  0x390c
#define HEVC_MC_MIX_RATIO1                  0x390d
#define HEVC_MC_DP_MB_XY                    0x390e
#define HEVC_MC_OM_MB_XY                    0x390f
#define HEVC_PSCALE_RST                     0x3910
#define HEVC_PSCALE_CTRL                    0x3911
#define HEVC_PSCALE_PICI_W                  0x3912
#define HEVC_PSCALE_PICI_H                  0x3913
#define HEVC_PSCALE_PICO_W                  0x3914
#define HEVC_PSCALE_PICO_H                  0x3915
#define HEVC_PSCALE_PICO_START_X            0x3916
#define HEVC_PSCALE_PICO_START_Y            0x3917
#define HEVC_PSCALE_DUMMY                   0x3918
#define HEVC_PSCALE_FILT0_COEF0             0x3919
#define HEVC_PSCALE_FILT0_COEF1             0x391a
#define HEVC_PSCALE_CMD_CTRL                0x391b
#define HEVC_PSCALE_CMD_BLK_X               0x391c
#define HEVC_PSCALE_CMD_BLK_Y               0x391d
#define HEVC_PSCALE_STATUS                  0x391e
#define HEVC_PSCALE_BMEM_ADDR               0x391f
#define HEVC_PSCALE_BMEM_DAT                0x3920
#define HEVC_PSCALE_DRAM_BUF_CTRL           0x3921
#define HEVC_PSCALE_MCMD_CTRL               0x3922
#define HEVC_PSCALE_MCMD_XSIZE              0x3923
#define HEVC_PSCALE_MCMD_YSIZE              0x3924
#define HEVC_PSCALE_RBUF_START_BLKX         0x3925
#define HEVC_PSCALE_RBUF_START_BLKY         0x3926
#define HEVC_PSCALE_PICO_SHIFT_XY           0x3928
#define HEVC_PSCALE_CTRL1                   0x3929
#define HEVC_PSCALE_SRCKEY_CTRL0            0x392a
#define HEVC_PSCALE_SRCKEY_CTRL1            0x392b
#define HEVC_PSCALE_CANVAS_RD_ADDR          0x392c
#define HEVC_PSCALE_CANVAS_WR_ADDR          0x392d
#define HEVC_PSCALE_CTRL2                   0x392e
#define HEVC_HDEC_MC_OMEM_AUTO              0x3930
#define HEVC_HDEC_MC_MBRIGHT_IDX            0x3931
#define HEVC_HDEC_MC_MBRIGHT_RD             0x3932
#define HEVC_MC_MPORT_CTRL                  0x3940
#define HEVC_MC_MPORT_DAT                   0x3941
#define HEVC_MC_WT_PRED_CTRL                0x3942
#define HEVC_MC_MBBOT_ST_EVEN_ADDR          0x3944
#define HEVC_MC_MBBOT_ST_ODD_ADDR           0x3945
#define HEVC_MC_DPDN_MB_XY                  0x3946
#define HEVC_MC_OMDN_MB_XY                  0x3947
#define HEVC_MC_HCMDBUF_H                   0x3948
#define HEVC_MC_HCMDBUF_L                   0x3949
#define HEVC_MC_HCMD_H                      0x394a
#define HEVC_MC_HCMD_L                      0x394b
#define HEVC_MC_IDCT_DAT                    0x394c
#define HEVC_MC_CTRL_GCLK_CTRL              0x394d
#define HEVC_MC_OTHER_GCLK_CTRL             0x394e
#define HEVC_MC_CTRL2                       0x394f
#define HEVC_MDEC_PIC_DC_CTRL               0x398e
#define HEVC_MDEC_PIC_DC_STATUS             0x398f
#define HEVC_ANC0_CANVAS_ADDR               0x3990
#define HEVC_ANC1_CANVAS_ADDR               0x3991
#define HEVC_ANC2_CANVAS_ADDR               0x3992
#define HEVC_ANC3_CANVAS_ADDR               0x3993
#define HEVC_ANC4_CANVAS_ADDR               0x3994
#define HEVC_ANC5_CANVAS_ADDR               0x3995
#define HEVC_ANC6_CANVAS_ADDR               0x3996
#define HEVC_ANC7_CANVAS_ADDR               0x3997
#define HEVC_ANC8_CANVAS_ADDR               0x3998
#define HEVC_ANC9_CANVAS_ADDR               0x3999
#define HEVC_ANC10_CANVAS_ADDR              0x399a
#define HEVC_ANC11_CANVAS_ADDR              0x399b
#define HEVC_ANC12_CANVAS_ADDR              0x399c
#define HEVC_ANC13_CANVAS_ADDR              0x399d
#define HEVC_ANC14_CANVAS_ADDR              0x399e
#define HEVC_ANC15_CANVAS_ADDR              0x399f
#define HEVC_ANC16_CANVAS_ADDR              0x39a0
#define HEVC_ANC17_CANVAS_ADDR              0x39a1
#define HEVC_ANC18_CANVAS_ADDR              0x39a2
#define HEVC_ANC19_CANVAS_ADDR              0x39a3
#define HEVC_ANC20_CANVAS_ADDR              0x39a4
#define HEVC_ANC21_CANVAS_ADDR              0x39a5
#define HEVC_ANC22_CANVAS_ADDR              0x39a6
#define HEVC_ANC23_CANVAS_ADDR              0x39a7
#define HEVC_ANC24_CANVAS_ADDR              0x39a8
#define HEVC_ANC25_CANVAS_ADDR              0x39a9
#define HEVC_ANC26_CANVAS_ADDR              0x39aa
#define HEVC_ANC27_CANVAS_ADDR              0x39ab
#define HEVC_ANC28_CANVAS_ADDR              0x39ac
#define HEVC_ANC29_CANVAS_ADDR              0x39ad
#define HEVC_ANC30_CANVAS_ADDR              0x39ae
#define HEVC_ANC31_CANVAS_ADDR              0x39af
#define HEVC_DBKR_CANVAS_ADDR               0x39b0
#define HEVC_DBKW_CANVAS_ADDR               0x39b1
#define HEVC_REC_CANVAS_ADDR                0x39b2
#define HEVC_CURR_CANVAS_CTRL               0x39b3
#define HEVC_MDEC_PIC_DC_THRESH             0x39b8
#define HEVC_MDEC_PICR_BUF_STATUS           0x39b9
#define HEVC_MDEC_PICW_BUF_STATUS           0x39ba
#define HEVC_MCW_DBLK_WRRSP_CNT             0x39bb
#define HEVC_MC_MBBOT_WRRSP_CNT             0x39bc
#define HEVC_MDEC_PICW_BUF2_STATUS          0x39bd
#define HEVC_WRRSP_FIFO_PICW_DBK            0x39be
#define HEVC_WRRSP_FIFO_PICW_MC             0x39bf
#define HEVC_AV_SCRATCH_0                   0x39c0
#define HEVC_AV_SCRATCH_1                   0x39c1
#define HEVC_AV_SCRATCH_2                   0x39c2
#define HEVC_AV_SCRATCH_3                   0x39c3
#define HEVC_AV_SCRATCH_4                   0x39c4
#define HEVC_AV_SCRATCH_5                   0x39c5
#define HEVC_AV_SCRATCH_6                   0x39c6
#define HEVC_AV_SCRATCH_7                   0x39c7
#define HEVC_AV_SCRATCH_8                   0x39c8
#define HEVC_AV_SCRATCH_9                   0x39c9
#define HEVC_AV_SCRATCH_A                   0x39ca
#define HEVC_AV_SCRATCH_B                   0x39cb
#define HEVC_AV_SCRATCH_C                   0x39cc
#define HEVC_AV_SCRATCH_D                   0x39cd
#define HEVC_AV_SCRATCH_E                   0x39ce
#define HEVC_AV_SCRATCH_F                   0x39cf
#define HEVC_AV_SCRATCH_G                   0x39d0
#define HEVC_AV_SCRATCH_H                   0x39d1
#define HEVC_AV_SCRATCH_I                   0x39d2
#define HEVC_AV_SCRATCH_J                   0x39d3
#define HEVC_AV_SCRATCH_K                   0x39d4
#define HEVC_AV_SCRATCH_L                   0x39d5
#define HEVC_AV_SCRATCH_M                   0x39d6
#define HEVC_AV_SCRATCH_N                   0x39d7
#define HEVC_WRRSP_CO_MB                    0x39d8
#define HEVC_WRRSP_DCAC                     0x39d9
#define HEVC_WRRSP_VLD                      0x39da
#define HEVC_MDEC_DOUBLEW_CFG0              0x39db
#define HEVC_MDEC_DOUBLEW_CFG1              0x39dc
#define HEVC_MDEC_DOUBLEW_CFG2              0x39dd
#define HEVC_MDEC_DOUBLEW_CFG3              0x39de
#define HEVC_MDEC_DOUBLEW_CFG4              0x39df
#define HEVC_MDEC_DOUBLEW_CFG5              0x39e0
#define HEVC_MDEC_DOUBLEW_CFG6              0x39e1
#define HEVC_MDEC_DOUBLEW_CFG7              0x39e2
#define HEVC_MDEC_DOUBLEW_STATUS            0x39e3
#define HEVC_DBLK_RST                       0x3950
#define HEVC_DBLK_CTRL                      0x3951
#define HEVC_DBLK_MB_WID_HEIGHT             0x3952
#define HEVC_DBLK_STATUS                    0x3953
#define HEVC_DBLK_CMD_CTRL                  0x3954
#define HEVC_DBLK_MB_XY                     0x3955
#define HEVC_DBLK_QP                        0x3956
#define HEVC_DBLK_Y_BHFILT                  0x3957
#define HEVC_DBLK_Y_BHFILT_HIGH             0x3958
#define HEVC_DBLK_Y_BVFILT                  0x3959
#define HEVC_DBLK_CB_BFILT                  0x395a
#define HEVC_DBLK_CR_BFILT                  0x395b
#define HEVC_DBLK_Y_HFILT                   0x395c
#define HEVC_DBLK_Y_HFILT_HIGH              0x395d
#define HEVC_DBLK_Y_VFILT                   0x395e
#define HEVC_DBLK_CB_FILT                   0x395f
#define HEVC_DBLK_CR_FILT                   0x3960
#define HEVC_DBLK_BETAX_QP_SEL              0x3961
#define HEVC_DBLK_CLIP_CTRL0                0x3962
#define HEVC_DBLK_CLIP_CTRL1                0x3963
#define HEVC_DBLK_CLIP_CTRL2                0x3964
#define HEVC_DBLK_CLIP_CTRL3                0x3965
#define HEVC_DBLK_CLIP_CTRL4                0x3966
#define HEVC_DBLK_CLIP_CTRL5                0x3967
#define HEVC_DBLK_CLIP_CTRL6                0x3968
#define HEVC_DBLK_CLIP_CTRL7                0x3969
#define HEVC_DBLK_CLIP_CTRL8                0x396a
#define HEVC_DBLK_STATUS1                   0x396b
#define HEVC_DBLK_GCLK_FREE                 0x396c
#define HEVC_DBLK_GCLK_OFF                  0x396d
#define HEVC_DBLK_AVSFLAGS                  0x396e
#define HEVC_DBLK_CBPY                      0x3970
#define HEVC_DBLK_CBPY_ADJ                  0x3971
#define HEVC_DBLK_CBPC                      0x3972
#define HEVC_DBLK_CBPC_ADJ                  0x3973
#define HEVC_DBLK_VHMVD                     0x3974
#define HEVC_DBLK_STRONG                    0x3975
#define HEVC_DBLK_RV8_QUANT                 0x3976
#define HEVC_DBLK_CBUS_HCMD2                0x3977
#define HEVC_DBLK_CBUS_HCMD1                0x3978
#define HEVC_DBLK_CBUS_HCMD0                0x3979
#define HEVC_DBLK_VLD_HCMD2                 0x397a
#define HEVC_DBLK_VLD_HCMD1                 0x397b
#define HEVC_DBLK_VLD_HCMD0                 0x397c
#define HEVC_DBLK_OST_YBASE                 0x397d
#define HEVC_DBLK_OST_CBCRDIFF              0x397e
#define HEVC_DBLK_CTRL1                     0x397f
#define HEVC_MCRCC_CTL1                     0x3980
#define HEVC_MCRCC_CTL2                     0x3981
#define HEVC_MCRCC_CTL3                     0x3982
#define HEVC_GCLK_EN                        0x3983
#define HEVC_MDEC_SW_RESET                  0x3984
/* add from s5 */
#define HEVC_MDEC_CAV_LUT_DATAL             (0x39ea | MASK_S5_NEW_REGS)
#define HEVC_MDEC_CAV_LUT_DATAH             (0x39eb | MASK_S5_NEW_REGS)
#define HEVC_MDEC_CAV_LUT_ADDR              (0x39ec | MASK_S5_NEW_REGS)
#define HEVC_MDEC_CAV_CFG0                  (0x39ed | MASK_S5_NEW_REGS)
#define HEVC_MDEC_CRCW                      (0x39ee | MASK_S5_NEW_REGS)

/*add from M8M2*/
#define HEVC_VLD_STATUS_CTRL                0x3c00
#define HEVC_MPEG1_2_REG                    0x3c01
#define HEVC_F_CODE_REG                     0x3c02
#define HEVC_PIC_HEAD_INFO                  0x3c03
#define HEVC_SLICE_VER_POS_PIC_TYPE         0x3c04
#define HEVC_QP_VALUE_REG                   0x3c05
#define HEVC_MBA_INC                        0x3c06
#define HEVC_MB_MOTION_MODE                 0x3c07
#define HEVC_POWER_CTL_VLD                  0x3c08
#define HEVC_MB_WIDTH                       0x3c09
#define HEVC_SLICE_QP                       0x3c0a
#define HEVC_PRE_START_CODE                 0x3c0b
#define HEVC_SLICE_START_BYTE_01            0x3c0c
#define HEVC_SLICE_START_BYTE_23            0x3c0d
#define HEVC_RESYNC_MARKER_LENGTH           0x3c0e
#define HEVC_DECODER_BUFFER_INFO            0x3c0f
#define HEVC_FST_FOR_MV_X                   0x3c10
#define HEVC_FST_FOR_MV_Y                   0x3c11
#define HEVC_SCD_FOR_MV_X                   0x3c12
#define HEVC_SCD_FOR_MV_Y                   0x3c13
#define HEVC_FST_BAK_MV_X                   0x3c14
#define HEVC_FST_BAK_MV_Y                   0x3c15
#define HEVC_SCD_BAK_MV_X                   0x3c16
#define HEVC_SCD_BAK_MV_Y                   0x3c17
#define HEVC_VLD_DECODE_CONTROL             0x3c18
#define HEVC_VLD_REVERVED_19                0x3c19
#define HEVC_VIFF_BIT_CNT                   0x3c1a
#define HEVC_BYTE_ALIGN_PEAK_HI             0x3c1b
#define HEVC_BYTE_ALIGN_PEAK_LO             0x3c1c
#define HEVC_NEXT_ALIGN_PEAK                0x3c1d
#define HEVC_VC1_CONTROL_REG                0x3c1e
#define HEVC_PMV1_X                         0x3c20
#define HEVC_PMV1_Y                         0x3c21
#define HEVC_PMV2_X                         0x3c22
#define HEVC_PMV2_Y                         0x3c23
#define HEVC_PMV3_X                         0x3c24
#define HEVC_PMV3_Y                         0x3c25
#define HEVC_PMV4_X                         0x3c26
#define HEVC_PMV4_Y                         0x3c27
#define HEVC_M4_TABLE_SELECT                0x3c28
#define HEVC_M4_CONTROL_REG                 0x3c29
#define HEVC_BLOCK_NUM                      0x3c2a
#define HEVC_PATTERN_CODE                   0x3c2b
#define HEVC_MB_INFO                        0x3c2c
#define HEVC_VLD_DC_PRED                    0x3c2d
#define HEVC_VLD_ERROR_MASK                 0x3c2e
#define HEVC_VLD_DC_PRED_C                  0x3c2f
#define HEVC_LAST_SLICE_MV_ADDR             0x3c30
#define HEVC_LAST_MVX                       0x3c31
#define HEVC_LAST_MVY                       0x3c32
#define HEVC_VLD_C38                        0x3c38
#define HEVC_VLD_C39                        0x3c39
#define HEVC_VLD_STATUS                     0x3c3a
#define HEVC_VLD_SHIFT_STATUS               0x3c3b
#define HEVC_VOFF_STATUS                    0x3c3c
#define HEVC_VLD_C3D                        0x3c3d
#define HEVC_VLD_DBG_INDEX                  0x3c3e
#define HEVC_VLD_DBG_DATA                   0x3c3f
#define HEVC_VLD_MEM_VIFIFO_START_PTR       0x3c40
#define HEVC_VLD_MEM_VIFIFO_CURR_PTR        0x3c41
#define HEVC_VLD_MEM_VIFIFO_END_PTR         0x3c42
#define HEVC_VLD_MEM_VIFIFO_BYTES_AVAIL     0x3c43
#define HEVC_VLD_MEM_VIFIFO_CONTROL         0x3c44
#define HEVC_VLD_MEM_VIFIFO_WP              0x3c45
#define HEVC_VLD_MEM_VIFIFO_RP              0x3c46
#define HEVC_VLD_MEM_VIFIFO_LEVEL           0x3c47
#define HEVC_VLD_MEM_VIFIFO_BUF_CNTL        0x3c48
#define HEVC_VLD_TIME_STAMP_CNTL            0x3c49
#define HEVC_VLD_TIME_STAMP_SYNC_0          0x3c4a
#define HEVC_VLD_TIME_STAMP_SYNC_1          0x3c4b
#define HEVC_VLD_TIME_STAMP_0               0x3c4c
#define HEVC_VLD_TIME_STAMP_1               0x3c4d
#define HEVC_VLD_TIME_STAMP_2               0x3c4e
#define HEVC_VLD_TIME_STAMP_3               0x3c4f
#define HEVC_VLD_TIME_STAMP_LENGTH          0x3c50
#define HEVC_VLD_MEM_VIFIFO_WRAP_COUNT      0x3c51
#define HEVC_VLD_MEM_VIFIFO_MEM_CTL         0x3c52
#define HEVC_VLD_MEM_VBUF_RD_PTR            0x3c53
#define HEVC_VLD_MEM_VBUF2_RD_PTR           0x3c54
#define HEVC_VLD_MEM_SWAP_ADDR              0x3c55
#define HEVC_VLD_MEM_SWAP_CTL               0x3c56
/* from s5 */
#define HEVC_VDEC_STREAM_CRC                (0x3c59 | MASK_S5_NEW_REGS)
#define HEVC_VDEC_H264_TOP_BUFF_START       (0x3c5a | MASK_S5_NEW_REGS)
#define HEVC_VDEC_H264_TOP_CFG              (0x3c5b | MASK_S5_NEW_REGS)
#define HEVC_VDEC_H264_TOP_OFFSET           (0x3c5c | MASK_S5_NEW_REGS)
#define HEVC_VDEC_H264_TOP_CTRL             (0x3c5d | MASK_S5_NEW_REGS)

/*add from M8M2*/
#define HEVC_VCOP_CTRL_REG                  0x3e00
#define HEVC_QP_CTRL_REG                    0x3e01
#define HEVC_INTRA_QUANT_MATRIX             0x3e02
#define HEVC_NON_I_QUANT_MATRIX             0x3e03
#define HEVC_DC_SCALER                      0x3e04
#define HEVC_DC_AC_CTRL                     0x3e05
#define HEVC_DC_AC_SCALE_MUL                0x3e06
#define HEVC_DC_AC_SCALE_DIV                0x3e07
#define HEVC_POWER_CTL_IQIDCT               0x3e08
#define HEVC_RV_AI_Y_X                      0x3e09
#define HEVC_RV_AI_U_X                      0x3e0a
#define HEVC_RV_AI_V_X                      0x3e0b
#define HEVC_RV_AI_MB_COUNT                 0x3e0c
#define HEVC_NEXT_INTRA_DMA_ADDRESS         0x3e0d
#define HEVC_IQIDCT_CONTROL                 0x3e0e
#define HEVC_IQIDCT_DEBUG_INFO_0            0x3e0f
#define HEVC_DEBLK_CMD                      0x3e10
#define HEVC_IQIDCT_DEBUG_IDCT              0x3e11
#define HEVC_DCAC_DMA_CTRL                  0x3e12
#define HEVC_DCAC_DMA_ADDRESS               0x3e13
#define HEVC_DCAC_CPU_ADDRESS               0x3e14
#define HEVC_DCAC_CPU_DATA                  0x3e15
#define HEVC_DCAC_MB_COUNT                  0x3e16
#define HEVC_IQ_QUANT                       0x3e17
#define HEVC_VC1_BITPLANE_CTL               0x3e18
/*from s5 */
#define HEVC_DCAC_DMA_MIN_ADDR              (0x3e1e | MASK_S5_NEW_REGS)
#define HEVC_DCAC_DMA_MAX_ADDR              (0x3e1f | MASK_S5_NEW_REGS)
#define HEVC_DCAC_DMA_HW_CTL_CFG            (0x3e20 | MASK_S5_NEW_REGS)
#define HEVC_I_PIC_MB_COUNT_HW              (0x3e21 | MASK_S5_NEW_REGS)
#define HEVC_DCAC_DMA_HW_INFO               (0x3e22 | MASK_S5_NEW_REGS)
#define HEVC_DCAC_DMA_HW_OFFSET             (0x3e23 | MASK_S5_NEW_REGS)
#define HEVC_DCAC_DMA_HW_BUFF_START         (0x3e24 | MASK_S5_NEW_REGS)
#define HEVC_NEXT_INTRA_READ_ADDR_HW        (0x3e25 | MASK_S5_NEW_REGS)
#define HEVC_DCAC_DMA_HW_CTL                (0x3e26 | MASK_S5_NEW_REGS)
#define HEVC_DCAC_DMA_HW_MBBOT_ADDR         (0x3e27 | MASK_S5_NEW_REGS)
#define HEVC_DCAC_DMA_HW_MBXY               (0x3e28 | MASK_S5_NEW_REGS)
#define HEVC_SET_HW_TLR                     (0x3e29 | MASK_S5_NEW_REGS)

#define DOS_HEVC_STALL_START                       (0x3f50 | MASK_S5_NEW_REGS)
/*add from M8M2*/
#define HEVC_MSP                            0x3300
#define HEVC_MPSR                           0x3301
#define HEVC_MINT_VEC_BASE                  0x3302
#define HEVC_MCPU_INTR_GRP                  0x3303
#define HEVC_MCPU_INTR_MSK                  0x3304
#define HEVC_MCPU_INTR_REQ                  0x3305
#define HEVC_MPC_P                          0x3306
#define HEVC_MPC_D                          0x3307
#define HEVC_MPC_E                          0x3308
#define HEVC_MPC_W                          0x3309
#define HEVC_MINDEX0_REG                    0x330a
#define HEVC_MINDEX1_REG                    0x330b
#define HEVC_MINDEX2_REG                    0x330c
#define HEVC_MINDEX3_REG                    0x330d
#define HEVC_MINDEX4_REG                    0x330e
#define HEVC_MINDEX5_REG                    0x330f
#define HEVC_MINDEX6_REG                    0x3310
#define HEVC_MINDEX7_REG                    0x3311
#define HEVC_MMIN_REG                       0x3312
#define HEVC_MMAX_REG                       0x3313
#define HEVC_MBREAK0_REG                    0x3314
#define HEVC_MBREAK1_REG                    0x3315
#define HEVC_MBREAK2_REG                    0x3316
#define HEVC_MBREAK3_REG                    0x3317
#define HEVC_MBREAK_TYPE                    0x3318
#define HEVC_MBREAK_CTRL                    0x3319
#define HEVC_MBREAK_STAUTS                  0x331a
#define HEVC_MDB_ADDR_REG                   0x331b
#define HEVC_MDB_DATA_REG                   0x331c
#define HEVC_MDB_CTRL                       0x331d
#define HEVC_MSFTINT0                       0x331e
#define HEVC_MSFTINT1                       0x331f
#define HEVC_CSP                            0x3320
#define HEVC_CPSR                           0x3321
#define HEVC_CINT_VEC_BASE                  0x3322
#define HEVC_CCPU_INTR_GRP                  0x3323
#define HEVC_CCPU_INTR_MSK                  0x3324
#define HEVC_CCPU_INTR_REQ                  0x3325
#define HEVC_CPC_P                          0x3326
#define HEVC_CPC_D                          0x3327
#define HEVC_CPC_E                          0x3328
#define HEVC_CPC_W                          0x3329
#define HEVC_CINDEX0_REG                    0x332a
#define HEVC_CINDEX1_REG                    0x332b
#define HEVC_CINDEX2_REG                    0x332c
#define HEVC_CINDEX3_REG                    0x332d
#define HEVC_CINDEX4_REG                    0x332e
#define HEVC_CINDEX5_REG                    0x332f
#define HEVC_CINDEX6_REG                    0x3330
#define HEVC_CINDEX7_REG                    0x3331
#define HEVC_CMIN_REG                       0x3332
#define HEVC_CMAX_REG                       0x3333
#define HEVC_CBREAK0_REG                    0x3334
#define HEVC_CBREAK1_REG                    0x3335
#define HEVC_CBREAK2_REG                    0x3336
#define HEVC_CBREAK3_REG                    0x3337
#define HEVC_CBREAK_TYPE                    0x3338
#define HEVC_CBREAK_CTRL                    0x3339
#define HEVC_CBREAK_STAUTS                  0x333a
#define HEVC_CDB_ADDR_REG                   0x333b
#define HEVC_CDB_DATA_REG                   0x333c
#define HEVC_CDB_CTRL                       0x333d
#define HEVC_CSFTINT0                       0x333e
#define HEVC_CSFTINT1                       0x333f
#define HEVC_IMEM_DMA_CTRL                  0x3340
#define HEVC_IMEM_DMA_ADR                   0x3341
#define HEVC_IMEM_DMA_COUNT                 0x3342
#define HEVC_WRRSP_IMEM                     0x3343
#define HEVC_LMEM_DMA_CTRL                  0x3350
#define HEVC_LMEM_DMA_ADR                   0x3351
#define HEVC_LMEM_DMA_COUNT                 0x3352
#define HEVC_WRRSP_LMEM                     0x3353
#define HEVC_MAC_CTRL1                      0x3360
#define HEVC_ACC0REG1                       0x3361
#define HEVC_ACC1REG1                       0x3362
#define HEVC_MAC_CTRL2                      0x3370
#define HEVC_ACC0REG2                       0x3371
#define HEVC_ACC1REG2                       0x3372
#define HEVC_CPU_TRACE                      0x3380
/**/

#endif

/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef PARSER_REGS_HEADER_
#define PARSER_REGS_HEADER_

/*
 * pay attention : the regs range has
 * changed to 0x38xx in txlx, it was
 * converted automatically based on
 * the platform at init.
 * #define PARSER_CONTROL 0x3860
 */
#define PARSER_CONTROL            0x2960
#define PARSER_FETCH_ADDR         0x2961
#define PARSER_FETCH_CMD          0x2962
#define PARSER_FETCH_STOP_ADDR    0x2963
#define PARSER_FETCH_LEVEL        0x2964
#define PARSER_CONFIG             0x2965
#define PFIFO_WR_PTR              0x2966
#define PFIFO_RD_PTR              0x2967
#define PFIFO_DATA                0x2968
#define PARSER_SEARCH_PATTERN     0x2969
#define PARSER_SEARCH_MASK        0x296a
#define PARSER_INT_ENABLE         0x296b
#define PARSER_INT_STATUS         0x296c
#define PARSER_SCR_CTL            0x296d
#define PARSER_SCR                0x296e
#define PARSER_PARAMETER          0x296f
#define PARSER_INSERT_DATA        0x2970
#define VAS_STREAM_ID             0x2971
#define VIDEO_DTS                 0x2972
#define VIDEO_PTS                 0x2973
#define VIDEO_PTS_DTS_WR_PTR      0x2974
#define AUDIO_PTS                 0x2975
#define AUDIO_PTS_WR_PTR          0x2976
#define PARSER_ES_CONTROL         0x2977
#define PFIFO_MONITOR             0x2978
#define PARSER_VIDEO_START_PTR    0x2980
#define PARSER_VIDEO_END_PTR      0x2981
#define PARSER_VIDEO_WP           0x2982
#define PARSER_VIDEO_RP           0x2983
#define PARSER_VIDEO_HOLE         0x2984
#define PARSER_AUDIO_START_PTR    0x2985
#define PARSER_AUDIO_END_PTR      0x2986
#define PARSER_AUDIO_WP           0x2987
#define PARSER_AUDIO_RP           0x2988
#define PARSER_AUDIO_HOLE         0x2989
#define PARSER_SUB_START_PTR      0x298a
#define PARSER_SUB_END_PTR        0x298b
#define PARSER_SUB_WP             0x298c
#define PARSER_SUB_RP             0x298d
#define PARSER_SUB_HOLE           0x298e
#define PARSER_FETCH_INFO         0x298f
#define PARSER_STATUS             0x2990
#define PARSER_AV_WRAP_COUNT      0x2991
#define WRRSP_PARSER              0x2992
#define PARSER_VIDEO2_START_PTR   0x2993
#define PARSER_VIDEO2_END_PTR     0x2994
#define PARSER_VIDEO2_WP          0x2995
#define PARSER_VIDEO2_RP          0x2996
#define PARSER_VIDEO2_HOLE        0x2997
#define PARSER_AV2_WRAP_COUNT     0x2998

#endif

/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef VDEC_REGS_HEADER_
#define VDEC_REGS_HEADER_

#define VDEC_ASSIST_MMC_CTRL0       0x0001
#define VDEC_ASSIST_MMC_CTRL1       0x0002
/*add from M8M2*/
#define VDEC_ASSIST_MMC_CTRL2       0x0003
#define VDEC_ASSIST_MMC_CTRL3       0x0004
/**/
#define VDEC_ASSIST_AMR1_INT0       0x0025
#define VDEC_ASSIST_AMR1_INT1       0x0026
#define VDEC_ASSIST_AMR1_INT2       0x0027
#define VDEC_ASSIST_AMR1_INT3       0x0028
#define VDEC_ASSIST_AMR1_INT4       0x0029
#define VDEC_ASSIST_AMR1_INT5       0x002a
#define VDEC_ASSIST_AMR1_INT6       0x002b
#define VDEC_ASSIST_AMR1_INT7       0x002c
#define VDEC_ASSIST_AMR1_INT8       0x002d
#define VDEC_ASSIST_AMR1_INT9       0x002e
#define VDEC_ASSIST_AMR1_INTA       0x002f
#define VDEC_ASSIST_AMR1_INTB       0x0030
#define VDEC_ASSIST_AMR1_INTC       0x0031
#define VDEC_ASSIST_AMR1_INTD       0x0032
#define VDEC_ASSIST_AMR1_INTE       0x0033
#define VDEC_ASSIST_AMR1_INTF       0x0034
#define VDEC_ASSIST_AMR2_INT0       0x0035
#define VDEC_ASSIST_AMR2_INT1       0x0036
#define VDEC_ASSIST_AMR2_INT2       0x0037
#define VDEC_ASSIST_AMR2_INT3       0x0038
#define VDEC_ASSIST_AMR2_INT4       0x0039
#define VDEC_ASSIST_AMR2_INT5       0x003a
#define VDEC_ASSIST_AMR2_INT6       0x003b
#define VDEC_ASSIST_AMR2_INT7       0x003c
#define VDEC_ASSIST_AMR2_INT8       0x003d
#define VDEC_ASSIST_AMR2_INT9       0x003e
#define VDEC_ASSIST_AMR2_INTA       0x003f
#define VDEC_ASSIST_AMR2_INTB       0x0040
#define VDEC_ASSIST_AMR2_INTC       0x0041
#define VDEC_ASSIST_AMR2_INTD       0x0042
#define VDEC_ASSIST_AMR2_INTE       0x0043
#define VDEC_ASSIST_AMR2_INTF       0x0044
#define VDEC_ASSIST_MBX_SSEL        0x0045
#define VDEC_ASSIST_DBUS_DISABLE    0x0046

/*from s5*/
#define VDEC_AXI34_CONFIG_0         (0x0050 | MASK_S5_NEW_REGS)
#define VDEC_AXI34_CONFIG_1         (0x0051 | MASK_S5_NEW_REGS)
#define VDEC_AXI34_CONFIG_2         (0x0052 | MASK_S5_NEW_REGS)
#define VDEC_AXI34_CONFIG_3         (0x0053 | MASK_S5_NEW_REGS)
#define VDEC_AXI34_CONFIG_4         (0x0054 | MASK_S5_NEW_REGS)
#define VDEC_AXI34_CONFIG_5         (0x0055 | MASK_S5_NEW_REGS)
#define VDEC_AXI34_CONFIG_6         (0x0056 | MASK_S5_NEW_REGS)
#define VDEC_AXI34_CONFIG_7         (0x0057 | MASK_S5_NEW_REGS)

#define VDEC_ASSIST_TIMER0_LO       0x0060
#define VDEC_ASSIST_TIMER0_HI       0x0061
#define VDEC_ASSIST_TIMER1_LO       0x0062
#define VDEC_ASSIST_TIMER1_HI       0x0063
#define VDEC_ASSIST_DMA_INT         0x0064
#define VDEC_ASSIST_DMA_INT_MSK     0x0065
#define VDEC_ASSIST_DMA_INT2        0x0066
#define VDEC_ASSIST_DMA_INT_MSK2    0x0067
#define VDEC_ASSIST_MBOX0_IRQ_REG   0x0070
#define VDEC_ASSIST_MBOX0_CLR_REG   0x0071
#define VDEC_ASSIST_MBOX0_MASK      0x0072
#define VDEC_ASSIST_MBOX0_FIQ_SEL   0x0073
#define VDEC_ASSIST_MBOX1_IRQ_REG   0x0074
#define VDEC_ASSIST_MBOX1_CLR_REG   0x0075
#define VDEC_ASSIST_MBOX1_MASK      0x0076
#define VDEC_ASSIST_MBOX1_FIQ_SEL   0x0077
#define VDEC_ASSIST_MBOX2_IRQ_REG   0x0078
#define VDEC_ASSIST_MBOX2_CLR_REG   0x0079
#define VDEC_ASSIST_MBOX2_MASK      0x007a
#define VDEC_ASSIST_MBOX2_FIQ_SEL   0x007b

#endif
/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#ifndef VDEC2_REGS_HEADER_
#define VDEC2_REGS_HEADER_

#define VDEC2_ASSIST_MMC_CTRL0           0x2001
#define VDEC2_ASSIST_MMC_CTRL1           0x2002
#define VDEC2_ASSIST_AMR1_INT0           0x2025
#define VDEC2_ASSIST_AMR1_INT1           0x2026
#define VDEC2_ASSIST_AMR1_INT2           0x2027
#define VDEC2_ASSIST_AMR1_INT3           0x2028
#define VDEC2_ASSIST_AMR1_INT4           0x2029
#define VDEC2_ASSIST_AMR1_INT5           0x202a
#define VDEC2_ASSIST_AMR1_INT6           0x202b
#define VDEC2_ASSIST_AMR1_INT7           0x202c
#define VDEC2_ASSIST_AMR1_INT8           0x202d
#define VDEC2_ASSIST_AMR1_INT9           0x202e
#define VDEC2_ASSIST_AMR1_INTA           0x202f
#define VDEC2_ASSIST_AMR1_INTB           0x2030
#define VDEC2_ASSIST_AMR1_INTC           0x2031
#define VDEC2_ASSIST_AMR1_INTD           0x2032
#define VDEC2_ASSIST_AMR1_INTE           0x2033
#define VDEC2_ASSIST_AMR1_INTF           0x2034
#define VDEC2_ASSIST_AMR2_INT0           0x2035
#define VDEC2_ASSIST_AMR2_INT1           0x2036
#define VDEC2_ASSIST_AMR2_INT2           0x2037
#define VDEC2_ASSIST_AMR2_INT3           0x2038
#define VDEC2_ASSIST_AMR2_INT4           0x2039
#define VDEC2_ASSIST_AMR2_INT5           0x203a
#define VDEC2_ASSIST_AMR2_INT6           0x203b
#define VDEC2_ASSIST_AMR2_INT7           0x203c
#define VDEC2_ASSIST_AMR2_INT8           0x203d
#define VDEC2_ASSIST_AMR2_INT9           0x203e
#define VDEC2_ASSIST_AMR2_INTA           0x203f
#define VDEC2_ASSIST_AMR2_INTB           0x2040
#define VDEC2_ASSIST_AMR2_INTC           0x2041
#define VDEC2_ASSIST_AMR2_INTD           0x2042
#define VDEC2_ASSIST_AMR2_INTE           0x2043
#define VDEC2_ASSIST_AMR2_INTF           0x2044
#define VDEC2_ASSIST_MBX_SSEL            0x2045
#define VDEC2_ASSIST_TIMER0_LO           0x2060
#define VDEC2_ASSIST_TIMER0_HI           0x2061
#define VDEC2_ASSIST_TIMER1_LO           0x2062
#define VDEC2_ASSIST_TIMER1_HI           0x2063
#define VDEC2_ASSIST_DMA_INT             0x2064
#define VDEC2_ASSIST_DMA_INT_MSK         0x2065
#define VDEC2_ASSIST_DMA_INT2            0x2066
#define VDEC2_ASSIST_DMA_INT_MSK2        0x2067
#define VDEC2_ASSIST_MBOX0_IRQ_REG       0x2070
#define VDEC2_ASSIST_MBOX0_CLR_REG       0x2071
#define VDEC2_ASSIST_MBOX0_MASK          0x2072
#define VDEC2_ASSIST_MBOX0_FIQ_SEL       0x2073
#define VDEC2_ASSIST_MBOX1_IRQ_REG       0x2074
#define VDEC2_ASSIST_MBOX1_CLR_REG       0x2075
#define VDEC2_ASSIST_MBOX1_MASK          0x2076
#define VDEC2_ASSIST_MBOX1_FIQ_SEL       0x2077
#define VDEC2_ASSIST_MBOX2_IRQ_REG       0x2078
#define VDEC2_ASSIST_MBOX2_CLR_REG       0x2079
#define VDEC2_ASSIST_MBOX2_MASK          0x207a
#define VDEC2_ASSIST_MBOX2_FIQ_SEL       0x207b

#define VDEC2_MSP                        0x2300
#define VDEC2_MPSR                       0x2301
#define VDEC2_MINT_VEC_BASE              0x2302
#define VDEC2_MCPU_INTR_GRP              0x2303
#define VDEC2_MCPU_INTR_MSK              0x2304
#define VDEC2_MCPU_INTR_REQ              0x2305
#define VDEC2_MPC_P                      0x2306
#define VDEC2_MPC_D                      0x2307
#define VDEC2_MPC_E                      0x2308
#define VDEC2_MPC_W                      0x2309
#define VDEC2_MINDEX0_REG                0x230a
#define VDEC2_MINDEX1_REG                0x230b
#define VDEC2_MINDEX2_REG                0x230c
#define VDEC2_MINDEX3_REG                0x230d
#define VDEC2_MINDEX4_REG                0x230e
#define VDEC2_MINDEX5_REG                0x230f
#define VDEC2_MINDEX6_REG                0x2310
#define VDEC2_MINDEX7_REG                0x2311
#define VDEC2_MMIN_REG                   0x2312
#define VDEC2_MMAX_REG                   0x2313
#define VDEC2_MBREAK0_REG                0x2314
#define VDEC2_MBREAK1_REG                0x2315
#define VDEC2_MBREAK2_REG                0x2316
#define VDEC2_MBREAK3_REG                0x2317
#define VDEC2_MBREAK_TYPE                0x2318
#define VDEC2_MBREAK_CTRL                0x2319
#define VDEC2_MBREAK_STAUTS              0x231a
#define VDEC2_MDB_ADDR_REG               0x231b
#define VDEC2_MDB_DATA_REG               0x231c
#define VDEC2_MDB_CTRL                   0x231d
#define VDEC2_MSFTINT0                   0x231e
#define VDEC2_MSFTINT1                   0x231f
#define VDEC2_CSP                        0x2320
#define VDEC2_CPSR                       0x2321
#define VDEC2_CINT_VEC_BASE              0x2322
#define VDEC2_CCPU_INTR_GRP              0x2323
#define VDEC2_CCPU_INTR_MSK              0x2324
#define VDEC2_CCPU_INTR_REQ              0x2325
#define VDEC2_CPC_P                      0x2326
#define VDEC2_CPC_D                      0x2327
#define VDEC2_CPC_E                      0x2328
#define VDEC2_CPC_W                      0x2329
#define VDEC2_CINDEX0_REG                0x232a
#define VDEC2_CINDEX1_REG                0x232b
#define VDEC2_CINDEX2_REG                0x232c
#define VDEC2_CINDEX3_REG                0x232d
#define VDEC2_CINDEX4_REG                0x232e
#define VDEC2_CINDEX5_REG                0x232f
#define VDEC2_CINDEX6_REG                0x2330
#define VDEC2_CINDEX7_REG                0x2331
#define VDEC2_CMIN_REG                   0x2332
#define VDEC2_CMAX_REG                   0x2333
#define VDEC2_CBREAK0_REG                0x2334
#define VDEC2_CBREAK1_REG                0x2335
#define VDEC2_CBREAK2_REG                0x2336
#define VDEC2_CBREAK3_REG                0x2337
#define VDEC2_CBREAK_TYPE                0x2338
#define VDEC2_CBREAK_CTRL                0x2339
#define VDEC2_CBREAK_STAUTS              0x233a
#define VDEC2_CDB_ADDR_REG               0x233b
#define VDEC2_CDB_DATA_REG               0x233c
#define VDEC2_CDB_CTRL                   0x233d
#define VDEC2_CSFTINT0                   0x233e
#define VDEC2_CSFTINT1                   0x233f
#define VDEC2_IMEM_DMA_CTRL              0x2340
#define VDEC2_IMEM_DMA_ADR               0x2341
#define VDEC2_IMEM_DMA_COUNT             0x2342
#define VDEC2_WRRSP_IMEM                 0x2343
#define VDEC2_LMEM_DMA_CTRL              0x2350
#define VDEC2_LMEM_DMA_ADR               0x2351
#define VDEC2_LMEM_DMA_COUNT             0x2352
#define VDEC2_WRRSP_LMEM                 0x2353
#define VDEC2_MAC_CTRL1                  0x2360
#define VDEC2_ACC0REG1                   0x2361
#define VDEC2_ACC1REG1                   0x2362
#define VDEC2_MAC_CTRL2                  0x2370
#define VDEC2_ACC0REG2                   0x2371
#define VDEC2_ACC1REG2                   0x2372
#define VDEC2_CPU_TRACE                  0x2380

#define VDEC2_MC_CTRL_REG                0x2900
#define VDEC2_MC_MB_INFO                 0x2901
#define VDEC2_MC_PIC_INFO                0x2902
#define VDEC2_MC_HALF_PEL_ONE            0x2903
#define VDEC2_MC_HALF_PEL_TWO            0x2904
#define VDEC2_POWER_CTL_MC               0x2905
#define VDEC2_MC_CMD                     0x2906
#define VDEC2_MC_CTRL0                   0x2907
#define VDEC2_MC_PIC_W_H                 0x2908
#define VDEC2_MC_STATUS0                 0x2909
#define VDEC2_MC_STATUS1                 0x290a
#define VDEC2_MC_CTRL1                   0x290b
#define VDEC2_MC_MIX_RATIO0              0x290c
#define VDEC2_MC_MIX_RATIO1              0x290d
#define VDEC2_MC_DP_MB_XY                0x290e
#define VDEC2_MC_OM_MB_XY                0x290f
#define VDEC2_PSCALE_RST                 0x2910
#define VDEC2_PSCALE_CTRL                0x2911
#define VDEC2_PSCALE_PICI_W              0x2912
#define VDEC2_PSCALE_PICI_H              0x2913
#define VDEC2_PSCALE_PICO_W              0x2914
#define VDEC2_PSCALE_PICO_H              0x2915
#define VDEC2_PSCALE_PICO_START_X        0x2916
#define VDEC2_PSCALE_PICO_START_Y        0x2917
#define VDEC2_PSCALE_DUMMY               0x2918
#define VDEC2_PSCALE_FILT0_COEF0         0x2919
#define VDEC2_PSCALE_FILT0_COEF1         0x291a
#define VDEC2_PSCALE_CMD_CTRL            0x291b
#define VDEC2_PSCALE_CMD_BLK_X           0x291c
#define VDEC2_PSCALE_CMD_BLK_Y           0x291d
#define VDEC2_PSCALE_STATUS              0x291e
#define VDEC2_PSCALE_BMEM_ADDR           0x291f
#define VDEC2_PSCALE_BMEM_DAT            0x2920
#define VDEC2_PSCALE_DRAM_BUF_CTRL       0x2921
#define VDEC2_PSCALE_MCMD_CTRL           0x2922
#define VDEC2_PSCALE_MCMD_XSIZE          0x2923
#define VDEC2_PSCALE_MCMD_YSIZE          0x2924
#define VDEC2_PSCALE_RBUF_START_BLKX     0x2925
#define VDEC2_PSCALE_RBUF_START_BLKY     0x2926
#define VDEC2_PSCALE_PICO_SHIFT_XY       0x2928
#define VDEC2_PSCALE_CTRL1               0x2929
#define VDEC2_PSCALE_SRCKEY_CTRL0        0x292a
#define VDEC2_PSCALE_SRCKEY_CTRL1        0x292b
#define VDEC2_PSCALE_CANVAS_RD_ADDR      0x292c
#define VDEC2_PSCALE_CANVAS_WR_ADDR      0x292d
#define VDEC2_PSCALE_CTRL2               0x292e
/*add from M8M2*/
#define VDEC2_HDEC_MC_OMEM_AUTO          0x2930
#define VDEC2_HDEC_MC_MBRIGHT_IDX        0x2931
#define VDEC2_HDEC_MC_MBRIGHT_RD         0x2932
/**/
#define VDEC2_MC_MPORT_CTRL              0x2940
#define VDEC2_MC_MPORT_DAT               0x2941
#define VDEC2_MC_WT_PRED_CTRL            0x2942
#define VDEC2_MC_MBBOT_ST_EVEN_ADDR      0x2944
#define VDEC2_MC_MBBOT_ST_ODD_ADDR       0x2945
#define VDEC2_MC_DPDN_MB_XY              0x2946
#define VDEC2_MC_OMDN_MB_XY              0x2947
#define VDEC2_MC_HCMDBUF_H               0x2948
#define VDEC2_MC_HCMDBUF_L               0x2949
#define VDEC2_MC_HCMD_H                  0x294a
#define VDEC2_MC_HCMD_L                  0x294b
#define VDEC2_MC_IDCT_DAT                0x294c
#define VDEC2_MC_CTRL_GCLK_CTRL          0x294d
#define VDEC2_MC_OTHER_GCLK_CTRL         0x294e
#define VDEC2_MC_CTRL2                   0x294f
#define VDEC2_MDEC_PIC_DC_CTRL           0x298e
#define VDEC2_MDEC_PIC_DC_STATUS         0x298f
#define VDEC2_ANC0_CANVAS_ADDR           0x2990
#define VDEC2_ANC1_CANVAS_ADDR           0x2991
#define VDEC2_ANC2_CANVAS_ADDR           0x2992
#define VDEC2_ANC3_CANVAS_ADDR           0x2993
#define VDEC2_ANC4_CANVAS_ADDR           0x2994
#define VDEC2_ANC5_CANVAS_ADDR           0x2995
#define VDEC2_ANC6_CANVAS_ADDR           0x2996
#define VDEC2_ANC7_CANVAS_ADDR           0x2997
#define VDEC2_ANC8_CANVAS_ADDR           0x2998
#define VDEC2_ANC9_CANVAS_ADDR           0x2999
#define VDEC2_ANC10_CANVAS_ADDR          0x299a
#define VDEC2_ANC11_CANVAS_ADDR          0x299b
#define VDEC2_ANC12_CANVAS_ADDR          0x299c
#define VDEC2_ANC13_CANVAS_ADDR          0x299d
#define VDEC2_ANC14_CANVAS_ADDR          0x299e
#define VDEC2_ANC15_CANVAS_ADDR          0x299f
#define VDEC2_ANC16_CANVAS_ADDR          0x29a0
#define VDEC2_ANC17_CANVAS_ADDR          0x29a1
#define VDEC2_ANC18_CANVAS_ADDR          0x29a2
#define VDEC2_ANC19_CANVAS_ADDR          0x29a3
#define VDEC2_ANC20_CANVAS_ADDR          0x29a4
#define VDEC2_ANC21_CANVAS_ADDR          0x29a5
#define VDEC2_ANC22_CANVAS_ADDR          0x29a6
#define VDEC2_ANC23_CANVAS_ADDR          0x29a7
#define VDEC2_ANC24_CANVAS_ADDR          0x29a8
#define VDEC2_ANC25_CANVAS_ADDR          0x29a9
#define VDEC2_ANC26_CANVAS_ADDR          0x29aa
#define VDEC2_ANC27_CANVAS_ADDR          0x29ab
#define VDEC2_ANC28_CANVAS_ADDR          0x29ac
#define VDEC2_ANC29_CANVAS_ADDR          0x29ad
#define VDEC2_ANC30_CANVAS_ADDR          0x29ae
#define VDEC2_ANC31_CANVAS_ADDR          0x29af
#define VDEC2_DBKR_CANVAS_ADDR           0x29b0
#define VDEC2_DBKW_CANVAS_ADDR           0x29b1
#define VDEC2_REC_CANVAS_ADDR            0x29b2
#define VDEC2_CURR_CANVAS_CTRL           0x29b3
#define VDEC2_MDEC_PIC_DC_THRESH         0x29b8
#define VDEC2_MDEC_PICR_BUF_STATUS       0x29b9
#define VDEC2_MDEC_PICW_BUF_STATUS       0x29ba
#define VDEC2_MCW_DBLK_WRRSP_CNT         0x29bb
#define VDEC2_MC_MBBOT_WRRSP_CNT         0x29bc
#define VDEC2_MDEC_PICW_BUF2_STATUS      0x29bd
#define VDEC2_WRRSP_FIFO_PICW_DBK        0x29be
#define VDEC2_WRRSP_FIFO_PICW_MC         0x29bf
#define VDEC2_AV_SCRATCH_0               0x29c0
#define VDEC2_AV_SCRATCH_1               0x29c1
#define VDEC2_AV_SCRATCH_2               0x29c2
#define VDEC2_AV_SCRATCH_3               0x29c3
#define VDEC2_AV_SCRATCH_4               0x29c4
#define VDEC2_AV_SCRATCH_5               0x29c5
#define VDEC2_AV_SCRATCH_6               0x29c6
#define VDEC2_AV_SCRATCH_7               0x29c7
#define VDEC2_AV_SCRATCH_8               0x29c8
#define VDEC2_AV_SCRATCH_9               0x29c9
#define VDEC2_AV_SCRATCH_A               0x29ca
#define VDEC2_AV_SCRATCH_B               0x29cb
#define VDEC2_AV_SCRATCH_C               0x29cc
#define VDEC2_AV_SCRATCH_D               0x29cd
#define VDEC2_AV_SCRATCH_E               0x29ce
#define VDEC2_AV_SCRATCH_F               0x29cf
#define VDEC2_AV_SCRATCH_G               0x29d0
#define VDEC2_AV_SCRATCH_H               0x29d1
#define VDEC2_AV_SCRATCH_I               0x29d2
#define VDEC2_AV_SCRATCH_J               0x29d3
#define VDEC2_AV_SCRATCH_K               0x29d4
#define VDEC2_AV_SCRATCH_L               0x29d5
#define VDEC2_AV_SCRATCH_M               0x29d6
#define VDEC2_AV_SCRATCH_N               0x29d7
#define VDEC2_WRRSP_CO_MB                0x29d8
#define VDEC2_WRRSP_DCAC                 0x29d9
/*add from M8M2*/
#define VDEC2_WRRSP_VLD                  0x29da
#define VDEC2_MDEC_DOUBLEW_CFG0          0x29db
#define VDEC2_MDEC_DOUBLEW_CFG1          0x29dc
#define VDEC2_MDEC_DOUBLEW_CFG2          0x29dd
#define VDEC2_MDEC_DOUBLEW_CFG3          0x29de
#define VDEC2_MDEC_DOUBLEW_CFG4          0x29df
#define VDEC2_MDEC_DOUBLEW_CFG5          0x29e0
#define VDEC2_MDEC_DOUBLEW_CFG6          0x29e1
#define VDEC2_MDEC_DOUBLEW_CFG7          0x29e2
#define VDEC2_MDEC_DOUBLEW_STATUS        0x29e3
/**/
#define VDEC2_DBLK_RST                   0x2950
#define VDEC2_DBLK_CTRL                  0x2951
#define VDEC2_DBLK_MB_WID_HEIGHT         0x2952
#define VDEC2_DBLK_STATUS                0x2953
#define VDEC2_DBLK_CMD_CTRL              0x2954
#define VDEC2_DBLK_MB_XY                 0x2955
#define VDEC2_DBLK_QP                    0x2956
#define VDEC2_DBLK_Y_BHFILT              0x2957
#define VDEC2_DBLK_Y_BHFILT_HIGH         0x2958
#define VDEC2_DBLK_Y_BVFILT              0x2959
#define VDEC2_DBLK_CB_BFILT              0x295a
#define VDEC2_DBLK_CR_BFILT              0x295b
#define VDEC2_DBLK_Y_HFILT               0x295c
#define VDEC2_DBLK_Y_HFILT_HIGH          0x295d
#define VDEC2_DBLK_Y_VFILT               0x295e
#define VDEC2_DBLK_CB_FILT               0x295f
#define VDEC2_DBLK_CR_FILT               0x2960
#define VDEC2_DBLK_BETAX_QP_SEL          0x2961
#define VDEC2_DBLK_CLIP_CTRL0            0x2962
#define VDEC2_DBLK_CLIP_CTRL1            0x2963
#define VDEC2_DBLK_CLIP_CTRL2            0x2964
#define VDEC2_DBLK_CLIP_CTRL3            0x2965
#define VDEC2_DBLK_CLIP_CTRL4            0x2966
#define VDEC2_DBLK_CLIP_CTRL5            0x2967
#define VDEC2_DBLK_CLIP_CTRL6            0x2968
#define VDEC2_DBLK_CLIP_CTRL7            0x2969
#define VDEC2_DBLK_CLIP_CTRL8            0x296a
#define VDEC2_DBLK_STATUS1               0x296b
#define VDEC2_DBLK_GCLK_FREE             0x296c
#define VDEC2_DBLK_GCLK_OFF              0x296d
#define VDEC2_DBLK_AVSFLAGS              0x296e
#define VDEC2_DBLK_CBPY                  0x2970
#define VDEC2_DBLK_CBPY_ADJ              0x2971
#define VDEC2_DBLK_CBPC                  0x2972
#define VDEC2_DBLK_CBPC_ADJ              0x2973
#define VDEC2_DBLK_VHMVD                 0x2974
#define VDEC2_DBLK_STRONG                0x2975
#define VDEC2_DBLK_RV8_QUANT             0x2976
#define VDEC2_DBLK_CBUS_HCMD2            0x2977
#define VDEC2_DBLK_CBUS_HCMD1            0x2978
#define VDEC2_DBLK_CBUS_HCMD0            0x2979
#define VDEC2_DBLK_VLD_HCMD2             0x297a
#define VDEC2_DBLK_VLD_HCMD1             0x297b
#define VDEC2_DBLK_VLD_HCMD0             0x297c
#define VDEC2_DBLK_OST_YBASE             0x297d
#define VDEC2_DBLK_OST_CBCRDIFF          0x297e
#define VDEC2_DBLK_CTRL1                 0x297f
#define VDEC2_MCRCC_CTL1                 0x2980
#define VDEC2_MCRCC_CTL2                 0x2981
#define VDEC2_MCRCC_CTL3                 0x2982
#define VDEC2_GCLK_EN                    0x2983
#define VDEC2_MDEC_SW_RESET              0x2984

#define VDEC2_VLD_STATUS_CTRL            0x2c00
#define VDEC2_MPEG1_2_REG                0x2c01
#define VDEC2_F_CODE_REG                 0x2c02
#define VDEC2_PIC_HEAD_INFO              0x2c03
#define VDEC2_SLICE_VER_POS_PIC_TYPE     0x2c04
#define VDEC2_QP_VALUE_REG               0x2c05
#define VDEC2_MBA_INC                    0x2c06
#define VDEC2_MB_MOTION_MODE             0x2c07
#define VDEC2_POWER_CTL_VLD              0x2c08
#define VDEC2_MB_WIDTH                   0x2c09
#define VDEC2_SLICE_QP                   0x2c0a
#define VDEC2_PRE_START_CODE             0x2c0b
#define VDEC2_SLICE_START_BYTE_01        0x2c0c
#define VDEC2_SLICE_START_BYTE_23        0x2c0d
#define VDEC2_RESYNC_MARKER_LENGTH       0x2c0e
#define VDEC2_DECODER_BUFFER_INFO        0x2c0f
#define VDEC2_FST_FOR_MV_X               0x2c10
#define VDEC2_FST_FOR_MV_Y               0x2c11
#define VDEC2_SCD_FOR_MV_X               0x2c12
#define VDEC2_SCD_FOR_MV_Y               0x2c13
#define VDEC2_FST_BAK_MV_X               0x2c14
#define VDEC2_FST_BAK_MV_Y               0x2c15
#define VDEC2_SCD_BAK_MV_X               0x2c16
#define VDEC2_SCD_BAK_MV_Y               0x2c17
#define VDEC2_VLD_DECODE_CONTROL         0x2c18
#define VDEC2_VLD_REVERVED_19            0x2c19
#define VDEC2_VIFF_BIT_CNT               0x2c1a
#define VDEC2_BYTE_ALIGN_PEAK_HI         0x2c1b
#define VDEC2_BYTE_ALIGN_PEAK_LO         0x2c1c
#define VDEC2_NEXT_ALIGN_PEAK            0x2c1d
#define VDEC2_VC1_CONTROL_REG            0x2c1e
#define VDEC2_PMV1_X                     0x2c20
#define VDEC2_PMV1_Y                     0x2c21
#define VDEC2_PMV2_X                     0x2c22
#define VDEC2_PMV2_Y                     0x2c23
#define VDEC2_PMV3_X                     0x2c24
#define VDEC2_PMV3_Y                     0x2c25
#define VDEC2_PMV4_X                     0x2c26
#define VDEC2_PMV4_Y                     0x2c27
#define VDEC2_M4_TABLE_SELECT            0x2c28
#define VDEC2_M4_CONTROL_REG             0x2c29
#define VDEC2_BLOCK_NUM                  0x2c2a
#define VDEC2_PATTERN_CODE               0x2c2b
#define VDEC2_MB_INFO                    0x2c2c
#define VDEC2_VLD_DC_PRED                0x2c2d
#define VDEC2_VLD_ERROR_MASK             0x2c2e
#define VDEC2_VLD_DC_PRED_C              0x2c2f
#define VDEC2_LAST_SLICE_MV_ADDR         0x2c30
#define VDEC2_LAST_MVX                   0x2c31
#define VDEC2_LAST_MVY                   0x2c32
#define VDEC2_VLD_C38                    0x2c38
#define VDEC2_VLD_C39                    0x2c39
#define VDEC2_VLD_STATUS                 0x2c3a
#define VDEC2_VLD_SHIFT_STATUS           0x2c3b
#define VDEC2_VOFF_STATUS                0x2c3c
#define VDEC2_VLD_C3D                    0x2c3d
#define VDEC2_VLD_DBG_INDEX              0x2c3e
#define VDEC2_VLD_DBG_DATA               0x2c3f
#define VDEC2_VLD_MEM_VIFIFO_START_PTR   0x2c40
#define VDEC2_VLD_MEM_VIFIFO_CURR_PTR    0x2c41
#define VDEC2_VLD_MEM_VIFIFO_END_PTR     0x2c42
#define VDEC2_VLD_MEM_VIFIFO_BYTES_AVAIL 0x2c43
#define VDEC2_VLD_MEM_VIFIFO_CONTROL     0x2c44
#define VDEC2_VLD_MEM_VIFIFO_WP          0x2c45
#define VDEC2_VLD_MEM_VIFIFO_RP          0x2c46
#define VDEC2_VLD_MEM_VIFIFO_LEVEL       0x2c47
#define VDEC2_VLD_MEM_VIFIFO_BUF_CNTL    0x2c48
#define VDEC2_VLD_TIME_STAMP_CNTL        0x2c49
#define VDEC2_VLD_TIME_STAMP_SYNC_0      0x2c4a
#define VDEC2_VLD_TIME_STAMP_SYNC_1      0x2c4b
#define VDEC2_VLD_TIME_STAMP_0           0x2c4c
#define VDEC2_VLD_TIME_STAMP_1           0x2c4d
#define VDEC2_VLD_TIME_STAMP_2           0x2c4e
#define VDEC2_VLD_TIME_STAMP_3           0x2c4f
#define VDEC2_VLD_TIME_STAMP_LENGTH      0x2c50
#define VDEC2_VLD_MEM_VIFIFO_WRAP_COUNT  0x2c51
#define VDEC2_VLD_MEM_VIFIFO_MEM_CTL     0x2c52
#define VDEC2_VLD_MEM_VBUF_RD_PTR        0x2c53
#define VDEC2_VLD_MEM_VBUF2_RD_PTR       0x2c54
#define VDEC2_VLD_MEM_SWAP_ADDR          0x2c55
#define VDEC2_VLD_MEM_SWAP_CTL           0x2c56

#define VDEC2_VCOP_CTRL_REG              0x2e00
#define VDEC2_QP_CTRL_REG                0x2e01
#define VDEC2_INTRA_QUANT_MATRIX         0x2e02
#define VDEC2_NON_I_QUANT_MATRIX         0x2e03
#define VDEC2_DC_SCALER                  0x2e04
#define VDEC2_DC_AC_CTRL                 0x2e05
#define VDEC2_DC_AC_SCALE_MUL            0x2e06
#define VDEC2_DC_AC_SCALE_DIV            0x2e07
#define VDEC2_POWER_CTL_IQIDCT           0x2e08
#define VDEC2_RV_AI_Y_X                  0x2e09
#define VDEC2_RV_AI_U_X                  0x2e0a
#define VDEC2_RV_AI_V_X                  0x2e0b
#define VDEC2_RV_AI_MB_COUNT             0x2e0c
#define VDEC2_NEXT_INTRA_DMA_ADDRESS     0x2e0d
#define VDEC2_IQIDCT_CONTROL             0x2e0e
#define VDEC2_IQIDCT_DEBUG_INFO_0        0x2e0f
#define VDEC2_DEBLK_CMD                  0x2e10
#define VDEC2_IQIDCT_DEBUG_IDCT          0x2e11
#define VDEC2_DCAC_DMA_CTRL              0x2e12
#define VDEC2_DCAC_DMA_ADDRESS           0x2e13
#define VDEC2_DCAC_CPU_ADDRESS           0x2e14
#define VDEC2_DCAC_CPU_DATA              0x2e15
#define VDEC2_DCAC_MB_COUNT              0x2e16
#define VDEC2_IQ_QUANT                   0x2e17
#define VDEC2_VC1_BITPLANE_CTL           0x2e18
#endif


