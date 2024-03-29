/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef VPP_S5_HDR_REGS_H
// s5 osd1 hdr
#define S5_OSD1_HDR2_CTRL                      0x6450
#define S5_OSD1_HDR2_CLK_GATE                  0x6451
#define S5_OSD1_HDR2_MATRIXI_COEF00_01         0x6452
#define S5_OSD1_HDR2_MATRIXI_COEF02_10         0x6453
#define S5_OSD1_HDR2_MATRIXI_COEF11_12         0x6454
#define S5_OSD1_HDR2_MATRIXI_COEF20_21         0x6455
#define S5_OSD1_HDR2_MATRIXI_COEF22            0x6456
#define S5_OSD1_HDR2_MATRIXI_COEF30_31         0x6457
#define S5_OSD1_HDR2_MATRIXI_COEF32_40         0x6458
#define S5_OSD1_HDR2_MATRIXI_COEF41_42         0x6459
#define S5_OSD1_HDR2_MATRIXI_OFFSET0_1         0x645a
#define S5_OSD1_HDR2_MATRIXI_OFFSET2           0x645b
#define S5_OSD1_HDR2_MATRIXI_PRE_OFFSET0_1     0x645c
#define S5_OSD1_HDR2_MATRIXI_PRE_OFFSET2       0x645d
#define S5_OSD1_HDR2_MATRIXO_COEF00_01         0x645e
#define S5_OSD1_HDR2_MATRIXO_COEF02_10         0x645f
#define S5_OSD1_HDR2_MATRIXO_COEF11_12         0x6460
#define S5_OSD1_HDR2_MATRIXO_COEF20_21         0x6461
#define S5_OSD1_HDR2_MATRIXO_COEF22            0x6462
#define S5_OSD1_HDR2_MATRIXO_COEF30_31         0x6463
#define S5_OSD1_HDR2_MATRIXO_COEF32_40         0x6464
#define S5_OSD1_HDR2_MATRIXO_COEF41_42         0x6465
#define S5_OSD1_HDR2_MATRIXO_OFFSET0_1         0x6466
#define S5_OSD1_HDR2_MATRIXO_OFFSET2           0x6467
#define S5_OSD1_HDR2_MATRIXO_PRE_OFFSET0_1     0x6468
#define S5_OSD1_HDR2_MATRIXO_PRE_OFFSET2       0x6469
#define S5_OSD1_HDR2_MATRIXI_CLIP              0x646a
#define S5_OSD1_HDR2_MATRIXO_CLIP              0x646b
#define S5_OSD1_HDR2_CGAIN_OFFT                0x646c
#define S5_OSD1_EOTF_LUT_ADDR_PORT             0x646e
#define S5_OSD1_EOTF_LUT_DATA_PORT             0x646f
#define S5_OSD1_OETF_LUT_ADDR_PORT             0x6470
#define S5_OSD1_OETF_LUT_DATA_PORT             0x6471
#define S5_OSD1_CGAIN_LUT_ADDR_PORT            0x6472
#define S5_OSD1_CGAIN_LUT_DATA_PORT            0x6473
#define S5_OSD1_HDR2_CGAIN_COEF0               0x6474
#define S5_OSD1_HDR2_CGAIN_COEF1               0x6475
#define S5_OSD1_OGAIN_LUT_ADDR_PORT            0x6476
#define S5_OSD1_OGAIN_LUT_DATA_PORT            0x6477
#define S5_OSD1_HDR2_ADPS_CTRL                 0x6478
#define S5_OSD1_HDR2_ADPS_ALPHA0               0x6479
#define S5_OSD1_HDR2_ADPS_ALPHA1               0x647a
#define S5_OSD1_HDR2_ADPS_BETA0                0x647b
#define S5_OSD1_HDR2_ADPS_BETA1                0x647c
#define S5_OSD1_HDR2_ADPS_BETA2                0x647d
#define S5_OSD1_HDR2_ADPS_COEF0                0x647e
#define S5_OSD1_HDR2_ADPS_COEF1                0x647f
#define S5_OSD1_HDR2_GMUT_CTRL                 0x6480
#define S5_OSD1_HDR2_GMUT_COEF0                0x6481
#define S5_OSD1_HDR2_GMUT_COEF1                0x6482
#define S5_OSD1_HDR2_GMUT_COEF2                0x6483
#define S5_OSD1_HDR2_GMUT_COEF3                0x6484
#define S5_OSD1_HDR2_GMUT_COEF4                0x6485
#define S5_OSD1_HDR2_PIPE_CTRL1                0x6486
#define S5_OSD1_HDR2_PIPE_CTRL2                0x6487
#define S5_OSD1_HDR2_PIPE_CTRL3                0x6488
#define S5_OSD1_HDR2_PROC_WIN1                 0x6489
#define S5_OSD1_HDR2_PROC_WIN2                 0x648a
#define S5_OSD1_HDR2_MATRIXI_EN_CTRL           0x648b
#define S5_OSD1_HDR2_MATRIXO_EN_CTRL           0x648c
#define S5_OSD1_HDR2_HIST_CTRL                 0x648d
#define S5_OSD1_HDR2_HIST_H_START_END          0x648e
#define S5_OSD1_HDR2_HIST_V_START_END          0x648f

// t3x osd2 hdr and hdr matrix
#define T3X_OSD2_HDR2_CTRL                      0x6850
#define T3X_OSD2_HDR2_CLK_GATE                  0x6851
#define T3X_OSD2_HDR2_MATRIXI_COEF00_01         0x6852
#define T3X_OSD2_HDR2_MATRIXI_COEF02_10         0x6853
#define T3X_OSD2_HDR2_MATRIXI_COEF11_12         0x6854
#define T3X_OSD2_HDR2_MATRIXI_COEF20_21         0x6855
#define T3X_OSD2_HDR2_MATRIXI_COEF22            0x6856
#define T3X_OSD2_HDR2_MATRIXI_COEF30_31         0x6857
#define T3X_OSD2_HDR2_MATRIXI_COEF32_40         0x6858
#define T3X_OSD2_HDR2_MATRIXI_COEF41_42         0x6859
#define T3X_OSD2_HDR2_MATRIXI_OFFSET0_1         0x685a
#define T3X_OSD2_HDR2_MATRIXI_OFFSET2           0x685b
#define T3X_OSD2_HDR2_MATRIXI_PRE_OFFSET0_1     0x685c
#define T3X_OSD2_HDR2_MATRIXI_PRE_OFFSET2       0x685d
#define T3X_OSD2_HDR2_MATRIXO_COEF00_01         0x685e
#define T3X_OSD2_HDR2_MATRIXO_COEF02_10         0x685f
#define T3X_OSD2_HDR2_MATRIXO_COEF11_12         0x6860
#define T3X_OSD2_HDR2_MATRIXO_COEF20_21         0x6861
#define T3X_OSD2_HDR2_MATRIXO_COEF22            0x6862
#define T3X_OSD2_HDR2_MATRIXO_COEF30_31         0x6863
#define T3X_OSD2_HDR2_MATRIXO_COEF32_40         0x6864
#define T3X_OSD2_HDR2_MATRIXO_COEF41_42         0x6865
#define T3X_OSD2_HDR2_MATRIXO_OFFSET0_1         0x6866
#define T3X_OSD2_HDR2_MATRIXO_OFFSET2           0x6867
#define T3X_OSD2_HDR2_MATRIXO_PRE_OFFSET0_1     0x6868
#define T3X_OSD2_HDR2_MATRIXO_PRE_OFFSET2       0x6869
#define T3X_OSD2_HDR2_MATRIXI_CLIP              0x686a
#define T3X_OSD2_HDR2_MATRIXO_CLIP              0x686b
#define T3X_OSD2_HDR2_CGAIN_OFFT                0x686c
#define T3X_OSD2_EOTF_LUT_ADDR_PORT             0x686e
#define T3X_OSD2_EOTF_LUT_DATA_PORT             0x686f
#define T3X_OSD2_OETF_LUT_ADDR_PORT             0x6870
#define T3X_OSD2_OETF_LUT_DATA_PORT             0x6871
#define T3X_OSD2_CGAIN_LUT_ADDR_PORT            0x6872
#define T3X_OSD2_CGAIN_LUT_DATA_PORT            0x6873
#define T3X_OSD2_HDR2_CGAIN_COEF0               0x6874
#define T3X_OSD2_HDR2_CGAIN_COEF1               0x6875
#define T3X_OSD2_OGAIN_LUT_ADDR_PORT            0x6876
#define T3X_OSD2_OGAIN_LUT_DATA_PORT            0x6877
#define T3X_OSD2_HDR2_ADPS_CTRL                 0x6878
#define T3X_OSD2_HDR2_ADPS_ALPHA0               0x6879
#define T3X_OSD2_HDR2_ADPS_ALPHA1               0x687a
#define T3X_OSD2_HDR2_ADPS_BETA0                0x687b
#define T3X_OSD2_HDR2_ADPS_BETA1                0x687c
#define T3X_OSD2_HDR2_ADPS_BETA2                0x687d
#define T3X_OSD2_HDR2_ADPS_COEF0                0x687e
#define T3X_OSD2_HDR2_ADPS_COEF1                0x687f
#define T3X_OSD2_HDR2_GMUT_CTRL                 0x6880
#define T3X_OSD2_HDR2_GMUT_COEF0                0x6881
#define T3X_OSD2_HDR2_GMUT_COEF1                0x6882
#define T3X_OSD2_HDR2_GMUT_COEF2                0x6883
#define T3X_OSD2_HDR2_GMUT_COEF3                0x6884
#define T3X_OSD2_HDR2_GMUT_COEF4                0x6885
#define T3X_OSD2_HDR2_PIPE_CTRL1                0x6886
#define T3X_OSD2_HDR2_PIPE_CTRL2                0x6887
#define T3X_OSD2_HDR2_PIPE_CTRL3                0x6888
#define T3X_OSD2_HDR2_PROC_WIN1                 0x6889
#define T3X_OSD2_HDR2_PROC_WIN2                 0x688a
#define T3X_OSD2_HDR2_MATRIXI_EN_CTRL           0x688b
#define T3X_OSD2_HDR2_MATRIXO_EN_CTRL           0x688c
#define T3X_OSD2_HDR2_HIST_CTRL                 0x688d
#define T3X_OSD2_HDR2_HIST_H_START_END          0x688e
#define T3X_OSD2_HDR2_HIST_V_START_END          0x688f

// s5 osd3 hdr
#define S5_OSD3_HDR2_CTRL                      0x6c50
#define S5_OSD3_HDR2_CLK_GATE                  0x6c51
#define S5_OSD3_HDR2_MATRIXI_COEF00_01         0x6c52
#define S5_OSD3_HDR2_MATRIXI_COEF02_10         0x6c53
#define S5_OSD3_HDR2_MATRIXI_COEF11_12         0x6c54
#define S5_OSD3_HDR2_MATRIXI_COEF20_21         0x6c55
#define S5_OSD3_HDR2_MATRIXI_COEF22            0x6c56
#define S5_OSD3_HDR2_MATRIXI_COEF30_31         0x6c57
#define S5_OSD3_HDR2_MATRIXI_COEF32_40         0x6c58
#define S5_OSD3_HDR2_MATRIXI_COEF41_42         0x6c59
#define S5_OSD3_HDR2_MATRIXI_OFFSET0_1         0x6c5a
#define S5_OSD3_HDR2_MATRIXI_OFFSET2           0x6c5b
#define S5_OSD3_HDR2_MATRIXI_PRE_OFFSET0_1     0x6c5c
#define S5_OSD3_HDR2_MATRIXI_PRE_OFFSET2       0x6c5d
#define S5_OSD3_HDR2_MATRIXO_COEF00_01         0x6c5e
#define S5_OSD3_HDR2_MATRIXO_COEF02_10         0x6c5f
#define S5_OSD3_HDR2_MATRIXO_COEF11_12         0x6c60
#define S5_OSD3_HDR2_MATRIXO_COEF20_21         0x6c61
#define S5_OSD3_HDR2_MATRIXO_COEF22            0x6c62
#define S5_OSD3_HDR2_MATRIXO_COEF30_31         0x6c63
#define S5_OSD3_HDR2_MATRIXO_COEF32_40         0x6c64
#define S5_OSD3_HDR2_MATRIXO_COEF41_42         0x6c65
#define S5_OSD3_HDR2_MATRIXO_OFFSET0_1         0x6c66
#define S5_OSD3_HDR2_MATRIXO_OFFSET2           0x6c67
#define S5_OSD3_HDR2_MATRIXO_PRE_OFFSET0_1     0x6c68
#define S5_OSD3_HDR2_MATRIXO_PRE_OFFSET2       0x6c69
#define S5_OSD3_HDR2_MATRIXI_CLIP              0x6c6a
#define S5_OSD3_HDR2_MATRIXO_CLIP              0x6c6b
#define S5_OSD3_HDR2_CGAIN_OFFT                0x6c6c
#define S5_OSD3_EOTF_LUT_ADDR_PORT             0x6c6e
#define S5_OSD3_EOTF_LUT_DATA_PORT             0x6c6f
#define S5_OSD3_OETF_LUT_ADDR_PORT             0x6c70
#define S5_OSD3_OETF_LUT_DATA_PORT             0x6c71
#define S5_OSD3_CGAIN_LUT_ADDR_PORT            0x6c72
#define S5_OSD3_CGAIN_LUT_DATA_PORT            0x6c73
#define S5_OSD3_HDR2_CGAIN_COEF0               0x6c74
#define S5_OSD3_HDR2_CGAIN_COEF1               0x6c75
#define S5_OSD3_OGAIN_LUT_ADDR_PORT            0x6c76
#define S5_OSD3_OGAIN_LUT_DATA_PORT            0x6c77
#define S5_OSD3_HDR2_ADPS_CTRL                 0x6c78
#define S5_OSD3_HDR2_ADPS_ALPHA0               0x6c79
#define S5_OSD3_HDR2_ADPS_ALPHA1               0x6c7a
#define S5_OSD3_HDR2_ADPS_BETA0                0x6c7b
#define S5_OSD3_HDR2_ADPS_BETA1                0x6c7c
#define S5_OSD3_HDR2_ADPS_BETA2                0x6c7d
#define S5_OSD3_HDR2_ADPS_COEF0                0x6c7e
#define S5_OSD3_HDR2_ADPS_COEF1                0x6c7f
#define S5_OSD3_HDR2_GMUT_CTRL                 0x6c80
#define S5_OSD3_HDR2_GMUT_COEF0                0x6c81
#define S5_OSD3_HDR2_GMUT_COEF1                0x6c82
#define S5_OSD3_HDR2_GMUT_COEF2                0x6c83
#define S5_OSD3_HDR2_GMUT_COEF3                0x6c84
#define S5_OSD3_HDR2_GMUT_COEF4                0x6c85
#define S5_OSD3_HDR2_PIPE_CTRL1                0x6c86
#define S5_OSD3_HDR2_PIPE_CTRL2                0x6c87
#define S5_OSD3_HDR2_PIPE_CTRL3                0x6c88
#define S5_OSD3_HDR2_PROC_WIN1                 0x6c89
#define S5_OSD3_HDR2_PROC_WIN2                 0x6c8a
#define S5_OSD3_HDR2_MATRIXI_EN_CTRL           0x6c8b
#define S5_OSD3_HDR2_MATRIXO_EN_CTRL           0x6c8c
#define S5_OSD3_HDR2_HIST_CTRL                 0x6c8d
#define S5_OSD3_HDR2_HIST_H_START_END          0x6c8e
#define S5_OSD3_HDR2_HIST_V_START_END          0x6c8f

// s5 vd1 hdr
#define S5_VD1_HDR2_CTRL                      0x25a0
#define S5_VD1_HDR2_CLK_GATE                  0x25a1
#define S5_VD1_HDR2_MATRIXI_COEF00_01         0x25a2
#define S5_VD1_HDR2_MATRIXI_COEF02_10         0x25a3
#define S5_VD1_HDR2_MATRIXI_COEF11_12         0x25a4
#define S5_VD1_HDR2_MATRIXI_COEF20_21         0x25a5
#define S5_VD1_HDR2_MATRIXI_COEF22            0x25a6
#define S5_VD1_HDR2_MATRIXI_COEF30_31         0x25a7
#define S5_VD1_HDR2_MATRIXI_COEF32_40         0x25a8
#define S5_VD1_HDR2_MATRIXI_COEF41_42         0x25a9
#define S5_VD1_HDR2_MATRIXI_OFFSET0_1         0x25aa
#define S5_VD1_HDR2_MATRIXI_OFFSET2           0x25ab
#define S5_VD1_HDR2_MATRIXI_PRE_OFFSET0_1     0x25ac
#define S5_VD1_HDR2_MATRIXI_PRE_OFFSET2       0x25ad
#define S5_VD1_HDR2_MATRIXO_COEF00_01         0x25ae
#define S5_VD1_HDR2_MATRIXO_COEF02_10         0x25af
#define S5_VD1_HDR2_MATRIXO_COEF11_12         0x25b0
#define S5_VD1_HDR2_MATRIXO_COEF20_21         0x25b1
#define S5_VD1_HDR2_MATRIXO_COEF22            0x25b2
#define S5_VD1_HDR2_MATRIXO_COEF30_31         0x25b3
#define S5_VD1_HDR2_MATRIXO_COEF32_40         0x25b4
#define S5_VD1_HDR2_MATRIXO_COEF41_42         0x25b5
#define S5_VD1_HDR2_MATRIXO_OFFSET0_1         0x25b6
#define S5_VD1_HDR2_MATRIXO_OFFSET2           0x25b7
#define S5_VD1_HDR2_MATRIXO_PRE_OFFSET0_1     0x25b8
#define S5_VD1_HDR2_MATRIXO_PRE_OFFSET2       0x25b9
#define S5_VD1_HDR2_MATRIXI_CLIP              0x25ba
#define S5_VD1_HDR2_MATRIXO_CLIP              0x25bb
#define S5_VD1_HDR2_CGAIN_OFFT                0x25bc
#define S5_VD1_EOTF_LUT_ADDR_PORT             0x25be
#define S5_VD1_EOTF_LUT_DATA_PORT             0x25bf
#define S5_VD1_OETF_LUT_ADDR_PORT             0x25c0
#define S5_VD1_OETF_LUT_DATA_PORT             0x25c1
#define S5_VD1_CGAIN_LUT_ADDR_PORT            0x25c2
#define S5_VD1_CGAIN_LUT_DATA_PORT            0x25c3
#define S5_VD1_HDR2_CGAIN_COEF0               0x25c4
#define S5_VD1_HDR2_CGAIN_COEF1               0x25c5
#define S5_VD1_OGAIN_LUT_ADDR_PORT            0x25c6
#define S5_VD1_OGAIN_LUT_DATA_PORT            0x25c7
#define S5_VD1_HDR2_ADPS_CTRL                 0x25c8
#define S5_VD1_HDR2_ADPS_ALPHA0               0x25c9
#define S5_VD1_HDR2_ADPS_ALPHA1               0x25ca
#define S5_VD1_HDR2_ADPS_BETA0                0x25cb
#define S5_VD1_HDR2_ADPS_BETA1                0x25cc
#define S5_VD1_HDR2_ADPS_BETA2                0x25cd
#define S5_VD1_HDR2_ADPS_COEF0                0x25ce
#define S5_VD1_HDR2_ADPS_COEF1                0x25cf
#define S5_VD1_HDR2_GMUT_CTRL                 0x25d0
#define S5_VD1_HDR2_GMUT_COEF0                0x25d1
#define S5_VD1_HDR2_GMUT_COEF1                0x25d2
#define S5_VD1_HDR2_GMUT_COEF2                0x25d3
#define S5_VD1_HDR2_GMUT_COEF3                0x25d4
#define S5_VD1_HDR2_GMUT_COEF4                0x25d5
#define S5_VD1_HDR2_PIPE_CTRL1                0x25d6
#define S5_VD1_HDR2_PIPE_CTRL2                0x25d7
#define S5_VD1_HDR2_PIPE_CTRL3                0x25d8
#define S5_VD1_HDR2_PROC_WIN1                 0x25d9
#define S5_VD1_HDR2_PROC_WIN2                 0x25da
#define S5_VD1_HDR2_MATRIXI_EN_CTRL           0x25db
#define S5_VD1_HDR2_MATRIXO_EN_CTRL           0x25dc
#define S5_VD1_HDR2_HIST_CTRL                 0x25dd
#define S5_VD1_HDR2_HIST_H_START_END          0x25de
#define S5_VD1_HDR2_HIST_V_START_END          0x25df
#define S5_VD1_HDR2_HIST_RD                   0x25e0

// s5 vd1 slice1 hdr
#define S5_VD1_SLICE1_HDR2_CTRL                      0x26a0
#define S5_VD1_SLICE1_HDR2_CLK_GATE                  0x26a1
#define S5_VD1_SLICE1_HDR2_MATRIXI_COEF00_01         0x26a2
#define S5_VD1_SLICE1_HDR2_MATRIXI_COEF02_10         0x26a3
#define S5_VD1_SLICE1_HDR2_MATRIXI_COEF11_12         0x26a4
#define S5_VD1_SLICE1_HDR2_MATRIXI_COEF20_21         0x26a5
#define S5_VD1_SLICE1_HDR2_MATRIXI_COEF22            0x26a6
#define S5_VD1_SLICE1_HDR2_MATRIXI_COEF30_31         0x26a7
#define S5_VD1_SLICE1_HDR2_MATRIXI_COEF32_40         0x26a8
#define S5_VD1_SLICE1_HDR2_MATRIXI_COEF41_42         0x26a9
#define S5_VD1_SLICE1_HDR2_MATRIXI_OFFSET0_1         0x26aa
#define S5_VD1_SLICE1_HDR2_MATRIXI_OFFSET2           0x26ab
#define S5_VD1_SLICE1_HDR2_MATRIXI_PRE_OFFSET0_1     0x26ac
#define S5_VD1_SLICE1_HDR2_MATRIXI_PRE_OFFSET2       0x26ad
#define S5_VD1_SLICE1_HDR2_MATRIXO_COEF00_01         0x26ae
#define S5_VD1_SLICE1_HDR2_MATRIXO_COEF02_10         0x26af
#define S5_VD1_SLICE1_HDR2_MATRIXO_COEF11_12         0x26b0
#define S5_VD1_SLICE1_HDR2_MATRIXO_COEF20_21         0x26b1
#define S5_VD1_SLICE1_HDR2_MATRIXO_COEF22            0x26b2
#define S5_VD1_SLICE1_HDR2_MATRIXO_COEF30_31         0x26b3
#define S5_VD1_SLICE1_HDR2_MATRIXO_COEF32_40         0x26b4
#define S5_VD1_SLICE1_HDR2_MATRIXO_COEF41_42         0x26b5
#define S5_VD1_SLICE1_HDR2_MATRIXO_OFFSET0_1         0x26b6
#define S5_VD1_SLICE1_HDR2_MATRIXO_OFFSET2           0x26b7
#define S5_VD1_SLICE1_HDR2_MATRIXO_PRE_OFFSET0_1     0x26b8
#define S5_VD1_SLICE1_HDR2_MATRIXO_PRE_OFFSET2       0x26b9
#define S5_VD1_SLICE1_HDR2_MATRIXI_CLIP              0x26ba
#define S5_VD1_SLICE1_HDR2_MATRIXO_CLIP              0x26bb
#define S5_VD1_SLICE1_HDR2_CGAIN_OFFT                0x26bc
#define S5_VD1_SLICE1_EOTF_LUT_ADDR_PORT             0x26be
#define S5_VD1_SLICE1_EOTF_LUT_DATA_PORT             0x26bf
#define S5_VD1_SLICE1_OETF_LUT_ADDR_PORT             0x26c0
#define S5_VD1_SLICE1_OETF_LUT_DATA_PORT             0x26c1
#define S5_VD1_SLICE1_CGAIN_LUT_ADDR_PORT            0x26c2
#define S5_VD1_SLICE1_CGAIN_LUT_DATA_PORT            0x26c3
#define S5_VD1_SLICE1_HDR2_CGAIN_COEF0               0x26c4
#define S5_VD1_SLICE1_HDR2_CGAIN_COEF1               0x26c5
#define S5_VD1_SLICE1_OGAIN_LUT_ADDR_PORT            0x26c6
#define S5_VD1_SLICE1_OGAIN_LUT_DATA_PORT            0x26c7
#define S5_VD1_SLICE1_HDR2_ADPS_CTRL                 0x26c8
#define S5_VD1_SLICE1_HDR2_ADPS_ALPHA0               0x26c9
#define S5_VD1_SLICE1_HDR2_ADPS_ALPHA1               0x26ca
#define S5_VD1_SLICE1_HDR2_ADPS_BETA0                0x26cb
#define S5_VD1_SLICE1_HDR2_ADPS_BETA1                0x26cc
#define S5_VD1_SLICE1_HDR2_ADPS_BETA2                0x26cd
#define S5_VD1_SLICE1_HDR2_ADPS_COEF0                0x26ce
#define S5_VD1_SLICE1_HDR2_ADPS_COEF1                0x26cf
#define S5_VD1_SLICE1_HDR2_GMUT_CTRL                 0x26d0
#define S5_VD1_SLICE1_HDR2_GMUT_COEF0                0x26d1
#define S5_VD1_SLICE1_HDR2_GMUT_COEF1                0x26d2
#define S5_VD1_SLICE1_HDR2_GMUT_COEF2                0x26d3
#define S5_VD1_SLICE1_HDR2_GMUT_COEF3                0x26d4
#define S5_VD1_SLICE1_HDR2_GMUT_COEF4                0x26d5
#define S5_VD1_SLICE1_HDR2_PIPE_CTRL1                0x26d6
#define S5_VD1_SLICE1_HDR2_PIPE_CTRL2                0x26d7
#define S5_VD1_SLICE1_HDR2_PIPE_CTRL3                0x26d8
#define S5_VD1_SLICE1_HDR2_PROC_WIN1                 0x26d9
#define S5_VD1_SLICE1_HDR2_PROC_WIN2                 0x26da
#define S5_VD1_SLICE1_HDR2_MATRIXI_EN_CTRL           0x26db
#define S5_VD1_SLICE1_HDR2_MATRIXO_EN_CTRL           0x26dc
#define S5_VD1_SLICE1_HDR2_HIST_CTRL                 0x26dd
#define S5_VD1_SLICE1_HDR2_HIST_H_START_END          0x26de
#define S5_VD1_SLICE1_HDR2_HIST_V_START_END          0x26df
#define S5_VD1_SLICE1_HDR2_HIST_RD                   0x26e0

// s5 vd1 slice2 hdr
#define S5_VD1_SLICE2_HDR2_CTRL                       0x2ca0
#define S5_VD1_SLICE2_HDR2_CLK_GATE                   0x2ca1
#define S5_VD1_SLICE2_HDR2_MATRIXI_COEF00_01          0x2ca2
#define S5_VD1_SLICE2_HDR2_MATRIXI_COEF02_10          0x2ca3
#define S5_VD1_SLICE2_HDR2_MATRIXI_COEF11_12          0x2ca4
#define S5_VD1_SLICE2_HDR2_MATRIXI_COEF20_21          0x2ca5
#define S5_VD1_SLICE2_HDR2_MATRIXI_COEF22             0x2ca6
#define S5_VD1_SLICE2_HDR2_MATRIXI_COEF30_31          0x2ca7
#define S5_VD1_SLICE2_HDR2_MATRIXI_COEF32_40          0x2ca8
#define S5_VD1_SLICE2_HDR2_MATRIXI_COEF41_42          0x2ca9
#define S5_VD1_SLICE2_HDR2_MATRIXI_OFFSET0_1          0x2caa
#define S5_VD1_SLICE2_HDR2_MATRIXI_OFFSET2            0x2cab
#define S5_VD1_SLICE2_HDR2_MATRIXI_PRE_OFFSET0_1      0x2cac
#define S5_VD1_SLICE2_HDR2_MATRIXI_PRE_OFFSET2        0x2cad
#define S5_VD1_SLICE2_HDR2_MATRIXO_COEF00_01          0x2cae
#define S5_VD1_SLICE2_HDR2_MATRIXO_COEF02_10          0x2caf
#define S5_VD1_SLICE2_HDR2_MATRIXO_COEF11_12          0x2cb0
#define S5_VD1_SLICE2_HDR2_MATRIXO_COEF20_21          0x2cb1
#define S5_VD1_SLICE2_HDR2_MATRIXO_COEF22             0x2cb2
#define S5_VD1_SLICE2_HDR2_MATRIXO_COEF30_31          0x2cb3
#define S5_VD1_SLICE2_HDR2_MATRIXO_COEF32_40          0x2cb4
#define S5_VD1_SLICE2_HDR2_MATRIXO_COEF41_42          0x2cb5
#define S5_VD1_SLICE2_HDR2_MATRIXO_OFFSET0_1          0x2cb6
#define S5_VD1_SLICE2_HDR2_MATRIXO_OFFSET2            0x2cb7
#define S5_VD1_SLICE2_HDR2_MATRIXO_PRE_OFFSET0_1      0x2cb8
#define S5_VD1_SLICE2_HDR2_MATRIXO_PRE_OFFSET2        0x2cb9
#define S5_VD1_SLICE2_HDR2_MATRIXI_CLIP               0x2cba
#define S5_VD1_SLICE2_HDR2_MATRIXO_CLIP               0x2cbb
#define S5_VD1_SLICE2_HDR2_CGAIN_OFFT                 0x2cbc
#define S5_VD1_SLICE2_EOTF_LUT_ADDR_PORT              0x2cbe
#define S5_VD1_SLICE2_EOTF_LUT_DATA_PORT              0x2cbf
#define S5_VD1_SLICE2_OETF_LUT_ADDR_PORT              0x2cc0
#define S5_VD1_SLICE2_OETF_LUT_DATA_PORT              0x2cc1
#define S5_VD1_SLICE2_CGAIN_LUT_ADDR_PORT             0x2cc2
#define S5_VD1_SLICE2_CGAIN_LUT_DATA_PORT             0x2cc3
#define S5_VD1_SLICE2_HDR2_CGAIN_COEF0                0x2cc4
#define S5_VD1_SLICE2_HDR2_CGAIN_COEF1                0x2cc5
#define S5_VD1_SLICE2_OGAIN_LUT_ADDR_PORT             0x2cc6
#define S5_VD1_SLICE2_OGAIN_LUT_DATA_PORT             0x2cc7
#define S5_VD1_SLICE2_HDR2_ADPS_CTRL                  0x2cc8
#define S5_VD1_SLICE2_HDR2_ADPS_ALPHA0                0x2cc9
#define S5_VD1_SLICE2_HDR2_ADPS_ALPHA1                0x2cca
#define S5_VD1_SLICE2_HDR2_ADPS_BETA0                 0x2ccb
#define S5_VD1_SLICE2_HDR2_ADPS_BETA1                 0x2ccc
#define S5_VD1_SLICE2_HDR2_ADPS_BETA2                 0x2ccd
#define S5_VD1_SLICE2_HDR2_ADPS_COEF0                 0x2cce
#define S5_VD1_SLICE2_HDR2_ADPS_COEF1                 0x2ccf
#define S5_VD1_SLICE2_HDR2_GMUT_CTRL                  0x2cd0
#define S5_VD1_SLICE2_HDR2_GMUT_COEF0                 0x2cd1
#define S5_VD1_SLICE2_HDR2_GMUT_COEF1                 0x2cd2
#define S5_VD1_SLICE2_HDR2_GMUT_COEF2                 0x2cd3
#define S5_VD1_SLICE2_HDR2_GMUT_COEF3                 0x2cd4
#define S5_VD1_SLICE2_HDR2_GMUT_COEF4                 0x2cd5
#define S5_VD1_SLICE2_HDR2_PIPE_CTRL1                 0x2cd6
#define S5_VD1_SLICE2_HDR2_PIPE_CTRL2                 0x2cd7
#define S5_VD1_SLICE2_HDR2_PIPE_CTRL3                 0x2cd8
#define S5_VD1_SLICE2_HDR2_PROC_WIN1                  0x2cd9
#define S5_VD1_SLICE2_HDR2_PROC_WIN2                  0x2cda
#define S5_VD1_SLICE2_HDR2_MATRIXI_EN_CTRL            0x2cdb
#define S5_VD1_SLICE2_HDR2_MATRIXO_EN_CTRL            0x2cdc
#define S5_VD1_SLICE2_HDR2_HIST_CTRL                  0x2cdd
#define S5_VD1_SLICE2_HDR2_HIST_H_START_END           0x2cde
#define S5_VD1_SLICE2_HDR2_HIST_V_START_END           0x2cdf

// s5 vd1 slice3 hdr
#define S5_VD1_SLICE3_HDR2_CTRL                       0x3ea0
#define S5_VD1_SLICE3_HDR2_CLK_GATE                   0x3ea1
#define S5_VD1_SLICE3_HDR2_MATRIXI_COEF00_01          0x3ea2
#define S5_VD1_SLICE3_HDR2_MATRIXI_COEF02_10          0x3ea3
#define S5_VD1_SLICE3_HDR2_MATRIXI_COEF11_12          0x3ea4
#define S5_VD1_SLICE3_HDR2_MATRIXI_COEF20_21          0x3ea5
#define S5_VD1_SLICE3_HDR2_MATRIXI_COEF22             0x3ea6
#define S5_VD1_SLICE3_HDR2_MATRIXI_COEF30_31          0x3ea7
#define S5_VD1_SLICE3_HDR2_MATRIXI_COEF32_40          0x3ea8
#define S5_VD1_SLICE3_HDR2_MATRIXI_COEF41_42          0x3ea9
#define S5_VD1_SLICE3_HDR2_MATRIXI_OFFSET0_1          0x3eaa
#define S5_VD1_SLICE3_HDR2_MATRIXI_OFFSET2            0x3eab
#define S5_VD1_SLICE3_HDR2_MATRIXI_PRE_OFFSET0_1      0x3eac
#define S5_VD1_SLICE3_HDR2_MATRIXI_PRE_OFFSET2        0x3ead
#define S5_VD1_SLICE3_HDR2_MATRIXO_COEF00_01          0x3eae
#define S5_VD1_SLICE3_HDR2_MATRIXO_COEF02_10          0x3eaf
#define S5_VD1_SLICE3_HDR2_MATRIXO_COEF11_12          0x3eb0
#define S5_VD1_SLICE3_HDR2_MATRIXO_COEF20_21          0x3eb1
#define S5_VD1_SLICE3_HDR2_MATRIXO_COEF22             0x3eb2
#define S5_VD1_SLICE3_HDR2_MATRIXO_COEF30_31          0x3eb3
#define S5_VD1_SLICE3_HDR2_MATRIXO_COEF32_40          0x3eb4
#define S5_VD1_SLICE3_HDR2_MATRIXO_COEF41_42          0x3eb5
#define S5_VD1_SLICE3_HDR2_MATRIXO_OFFSET0_1          0x3eb6
#define S5_VD1_SLICE3_HDR2_MATRIXO_OFFSET2            0x3eb7
#define S5_VD1_SLICE3_HDR2_MATRIXO_PRE_OFFSET0_1      0x3eb8
#define S5_VD1_SLICE3_HDR2_MATRIXO_PRE_OFFSET2        0x3eb9
#define S5_VD1_SLICE3_HDR2_MATRIXI_CLIP               0x3eba
#define S5_VD1_SLICE3_HDR2_MATRIXO_CLIP               0x3ebb
#define S5_VD1_SLICE3_HDR2_CGAIN_OFFT                 0x3ebc
#define S5_VD1_SLICE3_EOTF_LUT_ADDR_PORT              0x3ebe
#define S5_VD1_SLICE3_EOTF_LUT_DATA_PORT              0x3ebf
#define S5_VD1_SLICE3_OETF_LUT_ADDR_PORT              0x3ec0
#define S5_VD1_SLICE3_OETF_LUT_DATA_PORT              0x3ec1
#define S5_VD1_SLICE3_CGAIN_LUT_ADDR_PORT             0x3ec2
#define S5_VD1_SLICE3_CGAIN_LUT_DATA_PORT             0x3ec3
#define S5_VD1_SLICE3_HDR2_CGAIN_COEF0                0x3ec4
#define S5_VD1_SLICE3_HDR2_CGAIN_COEF1                0x3ec5
#define S5_VD1_SLICE3_OGAIN_LUT_ADDR_PORT             0x3ec6
#define S5_VD1_SLICE3_OGAIN_LUT_DATA_PORT             0x3ec7
#define S5_VD1_SLICE3_HDR2_ADPS_CTRL                  0x3ec8
#define S5_VD1_SLICE3_HDR2_ADPS_ALPHA0                0x3ec9
#define S5_VD1_SLICE3_HDR2_ADPS_ALPHA1                0x3eca
#define S5_VD1_SLICE3_HDR2_ADPS_BETA0                 0x3ecb
#define S5_VD1_SLICE3_HDR2_ADPS_BETA1                 0x3ecc
#define S5_VD1_SLICE3_HDR2_ADPS_BETA2                 0x3ecd
#define S5_VD1_SLICE3_HDR2_ADPS_COEF0                 0x3ece
#define S5_VD1_SLICE3_HDR2_ADPS_COEF1                 0x3ecf
#define S5_VD1_SLICE3_HDR2_GMUT_CTRL                  0x3ed0
#define S5_VD1_SLICE3_HDR2_GMUT_COEF0                 0x3ed1
#define S5_VD1_SLICE3_HDR2_GMUT_COEF1                 0x3ed2
#define S5_VD1_SLICE3_HDR2_GMUT_COEF2                 0x3ed3
#define S5_VD1_SLICE3_HDR2_GMUT_COEF3                 0x3ed4
#define S5_VD1_SLICE3_HDR2_GMUT_COEF4                 0x3ed5
#define S5_VD1_SLICE3_HDR2_PIPE_CTRL1                 0x3ed6
#define S5_VD1_SLICE3_HDR2_PIPE_CTRL2                 0x3ed7
#define S5_VD1_SLICE3_HDR2_PIPE_CTRL3                 0x3ed8
#define S5_VD1_SLICE3_HDR2_PROC_WIN1                  0x3ed9
#define S5_VD1_SLICE3_HDR2_PROC_WIN2                  0x3eda
#define S5_VD1_SLICE3_HDR2_MATRIXI_EN_CTRL            0x3edb
#define S5_VD1_SLICE3_HDR2_MATRIXO_EN_CTRL            0x3edc
#define S5_VD1_SLICE3_HDR2_HIST_CTRL                  0x3edd
#define S5_VD1_SLICE3_HDR2_HIST_H_START_END           0x3ede
#define S5_VD1_SLICE3_HDR2_HIST_V_START_END           0x3edf

// s5 vd2 hdr
// s5 osd1 hdr
#define S5_VD2_HDR2_CTRL                      0x3800
#define S5_VD2_HDR2_CLK_GATE                  0x3801
#define S5_VD2_HDR2_MATRIXI_COEF00_01         0x3802
#define S5_VD2_HDR2_MATRIXI_COEF02_10         0x3803
#define S5_VD2_HDR2_MATRIXI_COEF11_12         0x3804
#define S5_VD2_HDR2_MATRIXI_COEF20_21         0x3805
#define S5_VD2_HDR2_MATRIXI_COEF22            0x3806
#define S5_VD2_HDR2_MATRIXI_COEF30_31         0x3807
#define S5_VD2_HDR2_MATRIXI_COEF32_40         0x3808
#define S5_VD2_HDR2_MATRIXI_COEF41_42         0x3809
#define S5_VD2_HDR2_MATRIXI_OFFSET0_1         0x380a
#define S5_VD2_HDR2_MATRIXI_OFFSET2           0x380b
#define S5_VD2_HDR2_MATRIXI_PRE_OFFSET0_1     0x380c
#define S5_VD2_HDR2_MATRIXI_PRE_OFFSET2       0x380d
#define S5_VD2_HDR2_MATRIXO_COEF00_01         0x380e
#define S5_VD2_HDR2_MATRIXO_COEF02_10         0x380f
#define S5_VD2_HDR2_MATRIXO_COEF11_12         0x3810
#define S5_VD2_HDR2_MATRIXO_COEF20_21         0x3811
#define S5_VD2_HDR2_MATRIXO_COEF22            0x3812
#define S5_VD2_HDR2_MATRIXO_COEF30_31         0x3813
#define S5_VD2_HDR2_MATRIXO_COEF32_40         0x3814
#define S5_VD2_HDR2_MATRIXO_COEF41_42         0x3815
#define S5_VD2_HDR2_MATRIXO_OFFSET0_1         0x3816
#define S5_VD2_HDR2_MATRIXO_OFFSET2           0x3817
#define S5_VD2_HDR2_MATRIXO_PRE_OFFSET0_1     0x3818
#define S5_VD2_HDR2_MATRIXO_PRE_OFFSET2       0x3819
#define S5_VD2_HDR2_MATRIXI_CLIP              0x381a
#define S5_VD2_HDR2_MATRIXO_CLIP              0x381b
#define S5_VD2_HDR2_CGAIN_OFFT                0x381c
#define S5_VD2_EOTF_LUT_ADDR_PORT             0x381e
#define S5_VD2_EOTF_LUT_DATA_PORT             0x381f
#define S5_VD2_OETF_LUT_ADDR_PORT             0x3820
#define S5_VD2_OETF_LUT_DATA_PORT             0x3821
#define S5_VD2_CGAIN_LUT_ADDR_PORT            0x3822
#define S5_VD2_CGAIN_LUT_DATA_PORT            0x3823
#define S5_VD2_HDR2_CGAIN_COEF0               0x3824
#define S5_VD2_HDR2_CGAIN_COEF1               0x3825
#define S5_VD2_OGAIN_LUT_ADDR_PORT            0x3826
#define S5_VD2_OGAIN_LUT_DATA_PORT            0x3827
#define S5_VD2_HDR2_ADPS_CTRL                 0x3828
#define S5_VD2_HDR2_ADPS_ALPHA0               0x3829
#define S5_VD2_HDR2_ADPS_ALPHA1               0x382a
#define S5_VD2_HDR2_ADPS_BETA0                0x382b
#define S5_VD2_HDR2_ADPS_BETA1                0x382c
#define S5_VD2_HDR2_ADPS_BETA2                0x382d
#define S5_VD2_HDR2_ADPS_COEF0                0x382e
#define S5_VD2_HDR2_ADPS_COEF1                0x382f
#define S5_VD2_HDR2_GMUT_CTRL                 0x3830
#define S5_VD2_HDR2_GMUT_COEF0                0x3831
#define S5_VD2_HDR2_GMUT_COEF1                0x3832
#define S5_VD2_HDR2_GMUT_COEF2                0x3833
#define S5_VD2_HDR2_GMUT_COEF3                0x3834
#define S5_VD2_HDR2_GMUT_COEF4                0x3835
#define S5_VD2_HDR2_PIPE_CTRL1                0x3836
#define S5_VD2_HDR2_PIPE_CTRL2                0x3837
#define S5_VD2_HDR2_PIPE_CTRL3                0x3838
#define S5_VD2_HDR2_PROC_WIN1                 0x3839
#define S5_VD2_HDR2_PROC_WIN2                 0x383a
#define S5_VD2_HDR2_MATRIXI_EN_CTRL           0x383b
#define S5_VD2_HDR2_MATRIXO_EN_CTRL           0x383c
#define S5_VD2_HDR2_HIST_CTRL                 0x383d
#define S5_VD2_HDR2_HIST_H_START_END          0x383e
#define S5_VD2_HDR2_HIST_V_START_END          0x383f
#define S5_VD2_HDR2_HIST_RD                   0x3840

// t3x vdin0 hdr
#define T3X_VDIN0_HDR2_CTRL                            0x033d
#define T3X_VDIN0_HDR2_CLK_GATE                        0x033e
#define T3X_VDIN0_HDR2_MATRIXI_COEF00_01               0x033f
#define T3X_VDIN0_HDR2_MATRIXI_COEF02_10               0x0340
#define T3X_VDIN0_HDR2_MATRIXI_COEF11_12               0x0341
#define T3X_VDIN0_HDR2_MATRIXI_COEF20_21               0x0342
#define T3X_VDIN0_HDR2_MATRIXI_COEF22                  0x0343
#define T3X_VDIN0_HDR2_MATRIXI_COEF30_31               0x0344
#define T3X_VDIN0_HDR2_MATRIXI_COEF32_40               0x0345
#define T3X_VDIN0_HDR2_MATRIXI_COEF41_42               0x0346
#define T3X_VDIN0_HDR2_MATRIXI_OFFSET0_1               0x0347
#define T3X_VDIN0_HDR2_MATRIXI_OFFSET2                 0x0348
#define T3X_VDIN0_HDR2_MATRIXI_PRE_OFFSET0_1           0x0349
#define T3X_VDIN0_HDR2_MATRIXI_PRE_OFFSET2             0x034a
#define T3X_VDIN0_HDR2_MATRIXO_COEF00_01               0x034b
#define T3X_VDIN0_HDR2_MATRIXO_COEF02_10               0x034c
#define T3X_VDIN0_HDR2_MATRIXO_COEF11_12               0x034d
#define T3X_VDIN0_HDR2_MATRIXO_COEF20_21               0x034e
#define T3X_VDIN0_HDR2_MATRIXO_COEF22                  0x034f
#define T3X_VDIN0_HDR2_MATRIXO_COEF30_31               0x0350
#define T3X_VDIN0_HDR2_MATRIXO_COEF32_40               0x0351
#define T3X_VDIN0_HDR2_MATRIXO_COEF41_42               0x0352
#define T3X_VDIN0_HDR2_MATRIXO_OFFSET0_1               0x0353
#define T3X_VDIN0_HDR2_MATRIXO_OFFSET2                 0x0354
#define T3X_VDIN0_HDR2_MATRIXO_PRE_OFFSET0_1           0x0355
#define T3X_VDIN0_HDR2_MATRIXO_PRE_OFFSET2             0x0356
#define T3X_VDIN0_HDR2_MATRIXI_CLIP                    0x0357
#define T3X_VDIN0_HDR2_MATRIXO_CLIP                    0x0358
#define T3X_VDIN0_HDR2_CGAIN_OFFT                      0x0359
#define T3X_VDIN0_HDR2_CGAIN_COEF0                     0x0361
#define T3X_VDIN0_HDR2_CGAIN_COEF1                     0x0362
#define T3X_VDIN0_HDR2_ADPS_CTRL                       0x0365
#define T3X_VDIN0_HDR2_ADPS_ALPHA0                     0x0366
#define T3X_VDIN0_HDR2_ADPS_ALPHA1                     0x0367
#define T3X_VDIN0_HDR2_ADPS_BETA0                      0x0368
#define T3X_VDIN0_HDR2_ADPS_BETA1                      0x0369
#define T3X_VDIN0_HDR2_ADPS_BETA2                      0x036a
#define T3X_VDIN0_HDR2_ADPS_COEF0                      0x036b
#define T3X_VDIN0_HDR2_ADPS_COEF1                      0x036c
#define T3X_VDIN0_HDR2_GMUT_CTRL                       0x036d
#define T3X_VDIN0_HDR2_GMUT_COEF0                      0x036e
#define T3X_VDIN0_HDR2_GMUT_COEF1                      0x036f
#define T3X_VDIN0_HDR2_GMUT_COEF2                      0x0370
#define T3X_VDIN0_HDR2_GMUT_COEF3                      0x0371
#define T3X_VDIN0_HDR2_GMUT_COEF4                      0x0372
#define T3X_VDIN0_HDR2_PIPE_CTRL1                      0x0373
#define T3X_VDIN0_HDR2_PIPE_CTRL2                      0x0374
#define T3X_VDIN0_HDR2_PIPE_CTRL3                      0x0375
#define T3X_VDIN0_HDR2_PROC_WIN1                       0x0376
#define T3X_VDIN0_HDR2_PROC_WIN2                       0x0377
#define T3X_VDIN0_HDR2_MATRIXI_EN_CTRL                 0x0378
#define T3X_VDIN0_HDR2_MATRIXO_EN_CTRL                 0x0379
#define T3X_VDIN0_HDR2_HIST_CTRL                       0x037a
#define T3X_VDIN0_HDR2_HIST_H_START_END                0x037b
#define T3X_VDIN0_HDR2_HIST_V_START_END                0x037c
#define T3X_VDIN0_HDR2_HIST_RD                         0x035a
#define T3X_VDIN0_EOTF_LUT_ADDR_PORT                   0x035b
#define T3X_VDIN0_EOTF_LUT_DATA_PORT                   0x035c
#define T3X_VDIN0_OETF_LUT_ADDR_PORT                   0x035d
#define T3X_VDIN0_OETF_LUT_DATA_PORT                   0x035e
#define T3X_VDIN0_OGAIN_LUT_ADDR_PORT                  0x0363
#define T3X_VDIN0_OGAIN_LUT_DATA_PORT                  0x0364
#define T3X_VDIN0_CGAIN_LUT_ADDR_PORT                  0x035f
#define T3X_VDIN0_CGAIN_LUT_DATA_PORT                  0x0360

// t3x vdin1 hdr
#define T3X_VDIN1_HDR2_CTRL			      0x053d
#define T3X_VDIN1_HDR2_CLK_GATE			      0x053e
#define T3X_VDIN1_HDR2_MATRIXI_COEF00_01	      0x053f
#define T3X_VDIN1_HDR2_MATRIXI_COEF02_10	      0x0540
#define T3X_VDIN1_HDR2_MATRIXI_COEF11_12	      0x0541
#define T3X_VDIN1_HDR2_MATRIXI_COEF20_21	      0x0542
#define T3X_VDIN1_HDR2_MATRIXI_COEF22		      0x0543
#define T3X_VDIN1_HDR2_MATRIXI_COEF30_31	      0x0544
#define T3X_VDIN1_HDR2_MATRIXI_COEF32_40	      0x0545
#define T3X_VDIN1_HDR2_MATRIXI_COEF41_42	      0x0546
#define T3X_VDIN1_HDR2_MATRIXI_OFFSET0_1	      0x0547
#define T3X_VDIN1_HDR2_MATRIXI_OFFSET2		      0x0548
#define T3X_VDIN1_HDR2_MATRIXI_PRE_OFFSET0_1	      0x0549
#define T3X_VDIN1_HDR2_MATRIXI_PRE_OFFSET2	      0x054a
#define T3X_VDIN1_HDR2_MATRIXO_COEF00_01	      0x054b
#define T3X_VDIN1_HDR2_MATRIXO_COEF02_10	      0x054c
#define T3X_VDIN1_HDR2_MATRIXO_COEF11_12	      0x054d
#define T3X_VDIN1_HDR2_MATRIXO_COEF20_21	      0x054e
#define T3X_VDIN1_HDR2_MATRIXO_COEF22		      0x054f
#define T3X_VDIN1_HDR2_MATRIXO_COEF30_31	      0x0550
#define T3X_VDIN1_HDR2_MATRIXO_COEF32_40	      0x0551
#define T3X_VDIN1_HDR2_MATRIXO_COEF41_42	      0x0552
#define T3X_VDIN1_HDR2_MATRIXO_OFFSET0_1	      0x0553
#define T3X_VDIN1_HDR2_MATRIXO_OFFSET2		      0x0554
#define T3X_VDIN1_HDR2_MATRIXO_PRE_OFFSET0_1	      0x0555
#define T3X_VDIN1_HDR2_MATRIXO_PRE_OFFSET2	      0x0556
#define T3X_VDIN1_HDR2_MATRIXI_CLIP		      0x0557
#define T3X_VDIN1_HDR2_MATRIXO_CLIP		      0x0558
#define T3X_VDIN1_HDR2_CGAIN_OFFT		      0x0559
#define T3X_VDIN1_HDR2_CGAIN_COEF0		      0x0561
#define T3X_VDIN1_HDR2_CGAIN_COEF1		      0x0562
#define T3X_VDIN1_HDR2_ADPS_CTRL		      0x0565
#define T3X_VDIN1_HDR2_ADPS_ALPHA0		      0x0566
#define T3X_VDIN1_HDR2_ADPS_ALPHA1		      0x0567
#define T3X_VDIN1_HDR2_ADPS_BETA0		      0x0568
#define T3X_VDIN1_HDR2_ADPS_BETA1		      0x0569
#define T3X_VDIN1_HDR2_ADPS_BETA2		      0x056a
#define T3X_VDIN1_HDR2_ADPS_COEF0		      0x056b
#define T3X_VDIN1_HDR2_ADPS_COEF1		      0x056c
#define T3X_VDIN1_HDR2_GMUT_CTRL		      0x056d
#define T3X_VDIN1_HDR2_GMUT_COEF0		      0x056e
#define T3X_VDIN1_HDR2_GMUT_COEF1		      0x056f
#define T3X_VDIN1_HDR2_GMUT_COEF2		      0x0570
#define T3X_VDIN1_HDR2_GMUT_COEF3		      0x0571
#define T3X_VDIN1_HDR2_GMUT_COEF4		      0x0572
#define T3X_VDIN1_HDR2_PIPE_CTRL1		      0x0573
#define T3X_VDIN1_HDR2_PIPE_CTRL2		      0x0574
#define T3X_VDIN1_HDR2_PIPE_CTRL3		      0x0575
#define T3X_VDIN1_HDR2_PROC_WIN1		      0x0576
#define T3X_VDIN1_HDR2_PROC_WIN2		      0x0577
#define T3X_VDIN1_HDR2_MATRIXI_EN_CTRL		      0x0578
#define T3X_VDIN1_HDR2_MATRIXO_EN_CTRL		      0x0579
#define T3X_VDIN1_HDR2_HIST_CTRL		      0x057a
#define T3X_VDIN1_HDR2_HIST_H_START_END		      0x057b
#define T3X_VDIN1_HDR2_HIST_V_START_END		      0x057c
#define T3X_VDIN1_HDR2_HIST_RD			      0x055a
#define T3X_VDIN1_EOTF_LUT_ADDR_PORT	              0x055b
#define T3X_VDIN1_EOTF_LUT_DATA_PORT	              0x055c
#define T3X_VDIN1_OETF_LUT_ADDR_PORT	              0x055d
#define T3X_VDIN1_OETF_LUT_DATA_PORT	              0x055e
#define T3X_VDIN1_OGAIN_LUT_ADDR_PORT	              0x0563
#define T3X_VDIN1_OGAIN_LUT_DATA_PORT	              0x0564
#define T3X_VDIN1_CGAIN_LUT_ADDR_PORT	              0x055f
#define T3X_VDIN1_CGAIN_LUT_DATA_PORT	              0x0560

// t3x vdin2 hdr
#define T3X_VDIN2_HDR2_CTRL                            0x073d
#define T3X_VDIN2_HDR2_CLK_GATE                        0x073e
#define T3X_VDIN2_HDR2_MATRIXI_COEF00_01               0x073f
#define T3X_VDIN2_HDR2_MATRIXI_COEF02_10               0x0740
#define T3X_VDIN2_HDR2_MATRIXI_COEF11_12               0x0741
#define T3X_VDIN2_HDR2_MATRIXI_COEF20_21               0x0742
#define T3X_VDIN2_HDR2_MATRIXI_COEF22                  0x0743
#define T3X_VDIN2_HDR2_MATRIXI_COEF30_31               0x0744
#define T3X_VDIN2_HDR2_MATRIXI_COEF32_40               0x0745
#define T3X_VDIN2_HDR2_MATRIXI_COEF41_42               0x0746
#define T3X_VDIN2_HDR2_MATRIXI_OFFSET0_1               0x0747
#define T3X_VDIN2_HDR2_MATRIXI_OFFSET2                 0x0748
#define T3X_VDIN2_HDR2_MATRIXI_PRE_OFFSET0_1           0x0749
#define T3X_VDIN2_HDR2_MATRIXI_PRE_OFFSET2             0x074a
#define T3X_VDIN2_HDR2_MATRIXO_COEF00_01               0x074b
#define T3X_VDIN2_HDR2_MATRIXO_COEF02_10               0x074c
#define T3X_VDIN2_HDR2_MATRIXO_COEF11_12               0x074d
#define T3X_VDIN2_HDR2_MATRIXO_COEF20_21               0x074e
#define T3X_VDIN2_HDR2_MATRIXO_COEF22                  0x074f
#define T3X_VDIN2_HDR2_MATRIXO_COEF30_31               0x0750
#define T3X_VDIN2_HDR2_MATRIXO_COEF32_40               0x0751
#define T3X_VDIN2_HDR2_MATRIXO_COEF41_42               0x0752
#define T3X_VDIN2_HDR2_MATRIXO_OFFSET0_1               0x0753
#define T3X_VDIN2_HDR2_MATRIXO_OFFSET2                 0x0754
#define T3X_VDIN2_HDR2_MATRIXO_PRE_OFFSET0_1           0x0755
#define T3X_VDIN2_HDR2_MATRIXO_PRE_OFFSET2             0x0756
#define T3X_VDIN2_HDR2_MATRIXI_CLIP                    0x0757
#define T3X_VDIN2_HDR2_MATRIXO_CLIP                    0x0758
#define T3X_VDIN2_HDR2_CGAIN_OFFT                      0x0759
#define T3X_VDIN2_HDR2_CGAIN_COEF0                     0x0761
#define T3X_VDIN2_HDR2_CGAIN_COEF1                     0x0762
#define T3X_VDIN2_HDR2_ADPS_CTRL                       0x0765
#define T3X_VDIN2_HDR2_ADPS_ALPHA0                     0x0766
#define T3X_VDIN2_HDR2_ADPS_ALPHA1                     0x0767
#define T3X_VDIN2_HDR2_ADPS_BETA0                      0x0768
#define T3X_VDIN2_HDR2_ADPS_BETA1                      0x0769
#define T3X_VDIN2_HDR2_ADPS_BETA2                      0x076a
#define T3X_VDIN2_HDR2_ADPS_COEF0                      0x076b
#define T3X_VDIN2_HDR2_ADPS_COEF1                      0x076c
#define T3X_VDIN2_HDR2_GMUT_CTRL                       0x076d
#define T3X_VDIN2_HDR2_GMUT_COEF0                      0x076e
#define T3X_VDIN2_HDR2_GMUT_COEF1                      0x076f
#define T3X_VDIN2_HDR2_GMUT_COEF2                      0x0770
#define T3X_VDIN2_HDR2_GMUT_COEF3                      0x0771
#define T3X_VDIN2_HDR2_GMUT_COEF4                      0x0772
#define T3X_VDIN2_HDR2_PIPE_CTRL1                      0x0773
#define T3X_VDIN2_HDR2_PIPE_CTRL2                      0x0774
#define T3X_VDIN2_HDR2_PIPE_CTRL3                      0x0775
#define T3X_VDIN2_HDR2_PROC_WIN1                       0x0776
#define T3X_VDIN2_HDR2_PROC_WIN2                       0x0777
#define T3X_VDIN2_HDR2_MATRIXI_EN_CTRL                 0x0778
#define T3X_VDIN2_HDR2_MATRIXO_EN_CTRL                 0x0779
#define T3X_VDIN2_HDR2_HIST_CTRL                       0x077a
#define T3X_VDIN2_HDR2_HIST_H_START_END                0x077b
#define T3X_VDIN2_HDR2_HIST_V_START_END                0x077c
#define T3X_VDIN2_HDR2_HIST_RD                         0x075a
#define T3X_VDIN2_EOTF_LUT_ADDR_PORT                   0x075b
#define T3X_VDIN2_EOTF_LUT_DATA_PORT                   0x075c
#define T3X_VDIN2_OETF_LUT_ADDR_PORT                   0x075d
#define T3X_VDIN2_OETF_LUT_DATA_PORT                   0x075e
#define T3X_VDIN2_OGAIN_LUT_ADDR_PORT                  0x0763
#define T3X_VDIN2_OGAIN_LUT_DATA_PORT                  0x0764
#define T3X_VDIN2_CGAIN_LUT_ADDR_PORT                  0x075f
#define T3X_VDIN2_CGAIN_LUT_DATA_PORT                  0x0760

#define S5_VPP_VD_PREBLND_H_V_SIZE            0x3201

// s5 vdin1 hdr2
#define VDIN_PP_HDR2_CTRL                                      0x0291 // RW
#define VDIN_PP_HDR2_CLK_GATE                                  0x0292 // RW
#define VDIN_PP_HDR2_MATRIXI_COEF00_01                         0x0293 // RW
#define VDIN_PP_HDR2_MATRIXI_COEF02_10                         0x0294 // RW
#define VDIN_PP_HDR2_MATRIXI_COEF11_12                         0x0295 // RW
#define VDIN_PP_HDR2_MATRIXI_COEF20_21                         0x0296 // RW
#define VDIN_PP_HDR2_MATRIXI_COEF22                            0x0297 // RW
#define VDIN_PP_HDR2_MATRIXI_COEF30_31                         0x0298 // RW
#define VDIN_PP_HDR2_MATRIXI_COEF32_40                         0x0299 // RW
#define VDIN_PP_HDR2_MATRIXI_COEF41_42                         0x029a // RW
#define VDIN_PP_HDR2_MATRIXI_OFFSET0_1                         0x029b // RW
#define VDIN_PP_HDR2_MATRIXI_OFFSET2                           0x029c // RW
#define VDIN_PP_HDR2_MATRIXI_PRE_OFFSET0_1                     0x029d // RW
#define VDIN_PP_HDR2_MATRIXI_PRE_OFFSET2                       0x029e // RW
#define VDIN_PP_HDR2_MATRIXO_COEF00_01                         0x029f // RW
#define VDIN_PP_HDR2_MATRIXO_COEF02_10                         0x02a0 // RW
#define VDIN_PP_HDR2_MATRIXO_COEF11_12                         0x02a1 // RW
#define VDIN_PP_HDR2_MATRIXO_COEF20_21                         0x02a2 // RW
#define VDIN_PP_HDR2_MATRIXO_COEF22                            0x02a3 // RW
#define VDIN_PP_HDR2_MATRIXO_COEF30_31                         0x02a4 // RW
#define VDIN_PP_HDR2_MATRIXO_COEF32_40                         0x02a5 // RW
#define VDIN_PP_HDR2_MATRIXO_COEF41_42                         0x02a6 // RW
#define VDIN_PP_HDR2_MATRIXO_OFFSET0_1                         0x02a7 // RW
#define VDIN_PP_HDR2_MATRIXO_OFFSET2                           0x02a8 // RW
#define VDIN_PP_HDR2_MATRIXO_PRE_OFFSET0_1                     0x02a9 // RW
#define VDIN_PP_HDR2_MATRIXO_PRE_OFFSET2                       0x02aa // RW
#define VDIN_PP_HDR2_MATRIXI_CLIP                              0x02ab // RW
#define VDIN_PP_HDR2_MATRIXO_CLIP                              0x02ac // RW
#define VDIN_PP_HDR2_CGAIN_OFFT                                0x02ad // RW
#define VDIN_PP_HDR2_CGAIN_COEF0                               0x02b5 // RW
#define VDIN_PP_HDR2_CGAIN_COEF1                               0x02b6 // RW
#define VDIN_PP_HDR2_ADPS_CTRL                                 0x02b9 // RW
#define VDIN_PP_HDR2_ADPS_ALPHA0                               0x02ba // RW
#define VDIN_PP_HDR2_ADPS_ALPHA1                               0x02bb // RW
#define VDIN_PP_HDR2_ADPS_BETA0                                0x02bc // RW
#define VDIN_PP_HDR2_ADPS_BETA1                                0x02bd // RW
#define VDIN_PP_HDR2_ADPS_BETA2                                0x02be // RW
#define VDIN_PP_HDR2_ADPS_COEF0                                0x02bf // RW
#define VDIN_PP_HDR2_ADPS_COEF1                                0x02c0 // RW
#define VDIN_PP_HDR2_GMUT_CTRL                                 0x02c1 // RW
#define VDIN_PP_HDR2_GMUT_COEF0                                0x02c2 // RW
#define VDIN_PP_HDR2_GMUT_COEF1                                0x02c3 // RW
#define VDIN_PP_HDR2_GMUT_COEF2                                0x02c4 // RW
#define VDIN_PP_HDR2_GMUT_COEF3                                0x02c5 // RW
#define VDIN_PP_HDR2_GMUT_COEF4                                0x02c6 // RW
#define VDIN_PP_HDR2_PIPE_CTRL1                                0x02c7 // RW
#define VDIN_PP_HDR2_PIPE_CTRL2                                0x02c8 // RW
#define VDIN_PP_HDR2_PIPE_CTRL3                                0x02c9 // RW
#define VDIN_PP_HDR2_PROC_WIN1                                 0x02ca // RW
#define VDIN_PP_HDR2_PROC_WIN2                                 0x02cb // RW
#define VDIN_PP_HDR2_MATRIXI_EN_CTRL                           0x02cc // RW
#define VDIN_PP_HDR2_MATRIXO_EN_CTRL                           0x02cd // RW
#define VDIN_PP_HDR2_HIST_CTRL                                 0x02ce // RW
#define VDIN_PP_HDR2_HIST_H_START_END                          0x02cf // RW
#define VDIN_PP_HDR2_HIST_V_START_END                          0x02d0 // RW
#define VDIN_PP_HDR2_HIST_RD                                   0x02ae // RO
#define VDIN_PP_EOTF_LUT_ADDR_PORT                             0x02af // RW
#define VDIN_PP_EOTF_LUT_DATA_PORT                             0x02b0 // RW
#define VDIN_PP_OETF_LUT_ADDR_PORT                             0x02b1 // RW
#define VDIN_PP_OETF_LUT_DATA_PORT                             0x02b2 // RW
#define VDIN_PP_OGAIN_LUT_ADDR_PORT                            0x02b7 // RW
#define VDIN_PP_OGAIN_LUT_DATA_PORT                            0x02b8 // RW
#define VDIN_PP_CGAIN_LUT_ADDR_PORT                            0x02b3 // RW
#define VDIN_PP_CGAIN_LUT_DATA_PORT                            0x02b4 // RW

/* dma register*/
#define VPU_DMA_REG_BASE 0x2700
// 0x50~0x7f
//
// Reading file:  ./viu_dma_top.h
//
#define VPU_DMA_RDMIF0_CTRL					(VPU_DMA_REG_BASE + 0x50)
//Bit    31        reserved
//Bit    30        reg_rd0_clr_fcnt		// unsigned ,    RW , default = 0
//Bit 29:28        reserved
//Bit    27        reg_rd0_frm_ctrl     // unsigned ,    RW , default = 0
//Bit    26        reg_rd0_frm_force    // unsigned ,    RW , default = 0
//Bit 25:24        reg_rd0_frm_ini      // unsigned ,    RW , default = 0
//Bit 23:16        reg_rd0_enable_int   // unsigned ,    RW , default = 0
										// channel0 select
										// interrupt source
//Bit 15:13        reserved
//Bit 12:0         reg_rd0_stride       // unsigned ,    RW , default = 512
									// channel0 send number
#define VPU_DMA_RDMIF1_CTRL					(VPU_DMA_REG_BASE + 0x51)
//Bit    31        reserved
//Bit    30        reg_rd1_clr_fcnt     // unsigned ,    RW , default = 0
//Bit 29:28        reserved
//Bit    27        reg_rd1_frm_ctrl     // unsigned ,    RW , default = 0
//Bit    26        reg_rd1_frm_force    // unsigned ,    RW , default = 0
//Bit 25:24        reg_rd1_frm_ini		// unsigned ,    RW , default = 0
//Bit 23:16        reg_rd1_enable_int   // unsigned ,    RW , default = 0
						// channel1 select interrupt source
//Bit 15:13        reserved
//Bit 12:0         reg_rd1_stride       // unsigned ,    RW , default = 512
								// channel1 send number
#define VPU_DMA_RDMIF2_CTRL				(VPU_DMA_REG_BASE + 0x52)
//Bit    31        reserved
//Bit    30        reg_rd2_clr_fcnt     // unsigned ,    RW , default = 0
//Bit 29:28        reserved
//Bit    27        reg_rd2_frm_ctrl     // unsigned ,    RW , default = 0
//Bit    26        reg_rd2_frm_force    // unsigned ,    RW , default = 0
//Bit 25:24        reg_rd2_frm_ini      // unsigned ,    RW , default = 0
//Bit 23:16        reg_rd2_enable_int   // unsigned ,    RW , default = 0
								// channel2 select interrupt source
//Bit 15:13        reserved
//Bit 12:0         reg_rd2_stride       // unsigned ,    RW , default = 512
								// channel2 send number
#define VPU_DMA_RDMIF3_CTRL					(VPU_DMA_REG_BASE + 0x53)
//Bit    31        reserved
//Bit    30        reg_rd3_clr_fcnt             // unsigned ,    RW , default = 0
//Bit 29:28        reserved
//Bit    27        reg_rd3_frm_ctrl             // unsigned ,    RW , default = 0
//Bit    26        reg_rd3_frm_force            // unsigned ,    RW , default = 0
//Bit 25:24        reg_rd3_frm_ini              // unsigned ,    RW , default = 0
//Bit 23:16        reg_rd3_enable_int           // unsigned ,    RW , default = 0
								// channel3 select interrupt source
//Bit 15:13        reserved
//Bit 12:0         reg_rd3_stride               // unsigned ,    RW , default = 512
								// channel3 send number
#define VPU_DMA_RDMIF4_CTRL					(VPU_DMA_REG_BASE + 0x54)
//Bit    31        reserved
//Bit    30        reg_rd4_clr_fcnt             // unsigned ,    RW , default = 0
//Bit 29:28        reserved
//Bit    27        reg_rd4_frm_ctrl             // unsigned ,    RW , default = 0
//Bit    26        reg_rd4_frm_force            // unsigned ,    RW , default = 0
//Bit 25:24        reg_rd4_frm_ini              // unsigned ,    RW , default = 0
//Bit 23:16        reg_rd4_enable_int           // unsigned ,    RW , default = 0
								// channel4 select interrupt source
//Bit 15:13        reserved
//Bit 12:0         reg_rd4_stride               // unsigned ,    RW , default = 512
								// channel4 send number
#define VPU_DMA_RDMIF5_CTRL					(VPU_DMA_REG_BASE + 0x55)
//Bit    31        reserved
//Bit    30        reg_rd5_clr_fcnt             // unsigned ,    RW , default = 0
//Bit 29:28        reserved
//Bit    27        reg_rd5_frm_ctrl             // unsigned ,    RW , default = 0
//Bit    26        reg_rd5_frm_force            // unsigned ,    RW , default = 0
//Bit 25:24        reg_rd5_frm_ini              // unsigned ,    RW , default = 0
//Bit 23:16        reg_rd5_enable_int           // unsigned ,    RW , default = 0
								// channel5 select interrupt source
//Bit 15:13        reserved
//Bit 12:0         reg_rd5_stride               // unsigned ,    RW , default = 512
								// channel5 send number
#define VPU_DMA_RDMIF6_CTRL					(VPU_DMA_REG_BASE + 0x56)
//Bit    31        reserved
//Bit    30        reg_rd6_clr_fcnt             // unsigned ,    RW , default = 0
//Bit 29:28        reserved
//Bit    27        reg_rd6_frm_ctrl             // unsigned ,    RW , default = 0
//Bit    26        reg_rd6_frm_force            // unsigned ,    RW , default = 0
//Bit 25:24        reg_rd6_frm_ini              // unsigned ,    RW , default = 0
//Bit 23:16        reg_rd6_enable_int           // unsigned ,    RW , default = 0
								// channel6 select interrupt source
//Bit 15:13        reserved
//Bit 12:0         reg_rd6_stride               // unsigned ,    RW , default = 512
								// channel6 send number
#define VPU_DMA_RDMIF7_CTRL					(VPU_DMA_REG_BASE + 0x57)
//Bit    31        reserved
//Bit    30        reg_rd7_clr_fcnt             // unsigned ,    RW , default = 0
//Bit 29:28        reserved
//Bit    27        reg_rd7_frm_ctrl             // unsigned ,    RW , default = 0
//Bit    26        reg_rd7_frm_force            // unsigned ,    RW , default = 0
//Bit 25:24        reg_rd7_frm_ini              // unsigned ,    RW , default = 0
//Bit 23:16        reg_rd7_enable_int           // unsigned ,    RW , default = 0
								// channel7 select interrupt source
//Bit 15:13        reserved
//Bit 12:0         reg_rd7_stride       // unsigned ,    RW , default = 512
								// channel7 send number
#define VPU_DMA_RDMIF0_BADR0				(VPU_DMA_REG_BASE + 0x58)
//Bit 31:0  lut0_reg_baddr0
#define VPU_DMA_RDMIF0_BADR1				(VPU_DMA_REG_BASE + 0x59)
//Bit 31:0  lut0_reg_baddr1
#define VPU_DMA_RDMIF0_BADR2				(VPU_DMA_REG_BASE + 0x5a)
//Bit 31:0  lut0_reg_baddr2
#define VPU_DMA_RDMIF0_BADR3				(VPU_DMA_REG_BASE + 0x5b)
//Bit 31:0  lut0_reg_baddr3
#define VPU_DMA_RDMIF1_BADR0				(VPU_DMA_REG_BASE + 0x5c)
//Bit 31:0  lut1_reg_baddr0
#define VPU_DMA_RDMIF1_BADR1				(VPU_DMA_REG_BASE + 0x5d)
//Bit 31:0  lut1_reg_baddr1
#define VPU_DMA_RDMIF1_BADR2				(VPU_DMA_REG_BASE + 0x5e)
//Bit 31:0  lut1_reg_baddr2
#define VPU_DMA_RDMIF1_BADR3				(VPU_DMA_REG_BASE + 0x5f)
//Bit 31:0  lut1_reg_baddr3
#define VPU_DMA_RDMIF2_BADR0				(VPU_DMA_REG_BASE + 0x60)
//Bit 31:0  lut2_reg_baddr0
#define VPU_DMA_RDMIF2_BADR1				(VPU_DMA_REG_BASE + 0x61)
//Bit 31:0  lut2_reg_baddr1
#define VPU_DMA_RDMIF2_BADR2				(VPU_DMA_REG_BASE + 0x62)
//Bit 31:0  lut2_reg_baddr2
#define VPU_DMA_RDMIF2_BADR3				(VPU_DMA_REG_BASE + 0x63)
//Bit 31:0  lut2_reg_baddr3
#define VPU_DMA_RDMIF3_BADR0				(VPU_DMA_REG_BASE + 0x64)
//Bit 31:0  lut3_reg_baddr0
#define VPU_DMA_RDMIF3_BADR1				(VPU_DMA_REG_BASE + 0x65)
//Bit 31:0  lut3_reg_baddr1
#define VPU_DMA_RDMIF3_BADR2				(VPU_DMA_REG_BASE + 0x66)
//Bit 31:0  lut3_reg_baddr2
#define VPU_DMA_RDMIF3_BADR3				(VPU_DMA_REG_BASE + 0x67)
//Bit 31:0  lut3_reg_baddr3
#define VPU_DMA_RDMIF4_BADR0				(VPU_DMA_REG_BASE + 0x68)
//Bit 31:0  lut4_reg_baddr0
#define VPU_DMA_RDMIF4_BADR1				(VPU_DMA_REG_BASE + 0x69)
//Bit 31:0  lut4_reg_baddr1
#define VPU_DMA_RDMIF4_BADR2				(VPU_DMA_REG_BASE + 0x6a)
//Bit 31:0  lut4_reg_baddr2
#define VPU_DMA_RDMIF4_BADR3				(VPU_DMA_REG_BASE + 0x6b)
//Bit 31:0  lut4_reg_baddr3
#define VPU_DMA_RDMIF5_BADR0				(VPU_DMA_REG_BASE + 0x6c)
//Bit 31:0  lut5_reg_baddr0
#define VPU_DMA_RDMIF5_BADR1				(VPU_DMA_REG_BASE + 0x6d)
//Bit 31:0  lut5_reg_baddr1
#define VPU_DMA_RDMIF5_BADR2				(VPU_DMA_REG_BASE + 0x6e)
//Bit 31:0  lut5_reg_baddr2
#define VPU_DMA_RDMIF5_BADR3				(VPU_DMA_REG_BASE + 0x6f)
//Bit 31:0  lut5_reg_baddr3
#define VPU_DMA_RDMIF6_BADR0				(VPU_DMA_REG_BASE + 0x70)
//Bit 31:0  lut6_reg_baddr0
#define VPU_DMA_RDMIF6_BADR1				(VPU_DMA_REG_BASE + 0x71)
//Bit 31:0  lut6_reg_baddr1
#define VPU_DMA_RDMIF6_BADR2				(VPU_DMA_REG_BASE + 0x72)
//Bit 31:0  lut6_reg_baddr2
#define VPU_DMA_RDMIF6_BADR3				(VPU_DMA_REG_BASE + 0x73)
//Bit 31:0  lut6_reg_baddr3
#define VPU_DMA_RDMIF7_BADR0				(VPU_DMA_REG_BASE + 0x74)
//Bit 31:0  lut7_reg_baddr0
#define VPU_DMA_RDMIF7_BADR1				(VPU_DMA_REG_BASE + 0x75)
//Bit 31:0  lut7_reg_baddr1
#define VPU_DMA_RDMIF7_BADR2				(VPU_DMA_REG_BASE + 0x76)
//Bit 31:0  lut7_reg_baddr2
#define VPU_DMA_RDMIF7_BADR3				(VPU_DMA_REG_BASE + 0x77)
//Bit 31:0  lut7_reg_baddr3
#define VPU_DMA_RDMIF_SEL					(VPU_DMA_REG_BASE + 0x78)
//Bit 31:1  reserved
//Bit 0     lut_reg_sel				// unsigned ,    RW ,
									// default = 0, 0:
				// sel lut0,1,2,...,7 1:sel lut8,9,...,15
//
// Closing file:  ./viu_dma_top.h
//
#define VIU_DMA_CTRL0						0x1a28//32'hff8068a0
#define VIU_DMA_CTRL1						0x1a29//32'hff8068a4
#define VPU_INTF_CTRL                       0x270a//32'hff809c28

#define SA_CTRL                                 0x3f00
#endif
