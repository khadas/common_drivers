/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef RDMA_REGS_HEADER_
#define RDMA_REGS_HEADER_

/* cbus reset ctrl */
#define RESETCTRL_RESET4  0x0004

#ifndef RDMA_AHB_START_ADDR_MAN
#define RDMA_AHB_START_ADDR_MAN     0x1100
#define RDMA_AHB_END_ADDR_MAN       0x1101
#define RDMA_AHB_START_ADDR_1       0x1102
#define RDMA_AHB_END_ADDR_1         0x1103
#define RDMA_AHB_START_ADDR_2       0x1104
#define RDMA_AHB_END_ADDR_2         0x1105
#define RDMA_AHB_START_ADDR_3       0x1106
#define RDMA_AHB_END_ADDR_3         0x1107

#define RDMA_AHB_START_ADDR_4       0x1108
#define RDMA_AHB_END_ADDR_4         0x1109
#define RDMA_AHB_START_ADDR_5       0x110a
#define RDMA_AHB_END_ADDR_5         0x110b
#define RDMA_AHB_START_ADDR_6       0x110c
#define RDMA_AHB_END_ADDR_6         0x110d
#define RDMA_AHB_START_ADDR_7       0x110e
#define RDMA_AHB_END_ADDR_7         0x110f

#define RDMA_ACCESS_AUTO            0x1110
#define RDMA_ACCESS_AUTO2           0x1111
#define RDMA_ACCESS_AUTO3           0x1112
#define RDMA_ACCESS_MAN             0x1113
#define RDMA_CTRL                   0x1114
#define RDMA_STATUS                 0x1115
#define RDMA_STATUS2                0x1116
#define RDMA_STATUS3                0x1117

#define RDMA_AUTO_SRC1_SEL          0x1123
#define RDMA_AUTO_SRC2_SEL          0x1124
#define RDMA_AUTO_SRC3_SEL          0x1125
#define RDMA_AUTO_SRC4_SEL          0x1126
#define RDMA_AUTO_SRC5_SEL          0x1127
#define RDMA_AUTO_SRC6_SEL          0x1128
#define RDMA_AUTO_SRC7_SEL          0x1129

#define RDMA_AHB_START_ADDR_MAN_MSB                0x1130
#define RDMA_AHB_END_ADDR_MAN_MSB                  0x1131
#define RDMA_AHB_START_ADDR_1_MSB                  0x1132
#define RDMA_AHB_END_ADDR_1_MSB                    0x1133
#define RDMA_AHB_START_ADDR_2_MSB                  0x1134
#define RDMA_AHB_END_ADDR_2_MSB                    0x1135
#define RDMA_AHB_START_ADDR_3_MSB                  0x1136
#define RDMA_AHB_END_ADDR_3_MSB                    0x1137
#define RDMA_AHB_START_ADDR_4_MSB                  0x1138
#define RDMA_AHB_END_ADDR_4_MSB                    0x1139
#define RDMA_AHB_START_ADDR_5_MSB                  0x113a
#define RDMA_AHB_END_ADDR_5_MSB                    0x113b
#define RDMA_AHB_START_ADDR_6_MSB                  0x113c
#define RDMA_AHB_END_ADDR_6_MSB                    0x113d
#define RDMA_AHB_START_ADDR_7_MSB                  0x113e
#define RDMA_AHB_END_ADDR_7_MSB                    0x113f

#define T3X_RDMA_CTRL                              0x1100
#define T3X_RDMA_STATUS                            0x1104
#define T3X_RDMA_STATUS1                           0x1105
#define T3X_RDMA_STATUS2                           0x1106
#define T3X_RDMA_STATUS3                           0x1107
#define T3X_RDMA_ACCESS_MAN                        0x1110
#define T3X_RDMA_AHB_START_ADDR_MAN_MSB            0x1111
#define T3X_RDMA_AHB_START_ADDR_MAN                0x1112
#define T3X_RDMA_AHB_END_ADDR_MAN_MSB              0x1113
#define T3X_RDMA_AHB_END_ADDR_MAN                  0x1114
#define T3X_RDMA_ACCESS_AUTO                       0x1120
#define T3X_RDMA_ACCESS_AUTO2                      0x1121
#define T3X_RDMA_ACCESS_AUTO3                      0x1122
#define T3X_RDMA_ACCESS_AUTO4                      0x1123
#define T3X_RDMA_AUTO_SRC1_SEL                     0x1124
#define T3X_RDMA_AHB_START_ADDR_1_MSB              0x1125
#define T3X_RDMA_AHB_START_ADDR_1                  0x1126
#define T3X_RDMA_AHB_END_ADDR_1_MSB                0x1127
#define T3X_RDMA_AHB_END_ADDR_1                    0x1128
#define T3X_RDMA_AUTO_SRC2_SEL                     0x1129
#define T3X_RDMA_AHB_START_ADDR_2_MSB              0x112a
#define T3X_RDMA_AHB_START_ADDR_2                  0x112b
#define T3X_RDMA_AHB_END_ADDR_2_MSB                0x112c
#define T3X_RDMA_AHB_END_ADDR_2                    0x112d
#define T3X_RDMA_AUTO_SRC3_SEL                     0x112e
#define T3X_RDMA_AHB_START_ADDR_3_MSB              0x112f
#define T3X_RDMA_AHB_START_ADDR_3                  0x1130
#define T3X_RDMA_AHB_END_ADDR_3_MSB                0x1131
#define T3X_RDMA_AHB_END_ADDR_3                    0x1132
#define T3X_RDMA_AUTO_SRC4_SEL                     0x1133
#define T3X_RDMA_AHB_START_ADDR_4_MSB              0x1134
#define T3X_RDMA_AHB_START_ADDR_4                  0x1135
#define T3X_RDMA_AHB_END_ADDR_4_MSB                0x1136
#define T3X_RDMA_AHB_END_ADDR_4                    0x1137
#define T3X_RDMA_AUTO_SRC5_SEL                     0x1138
#define T3X_RDMA_AHB_START_ADDR_5_MSB              0x1139
#define T3X_RDMA_AHB_START_ADDR_5                  0x113a
#define T3X_RDMA_AHB_END_ADDR_5_MSB                0x113b
#define T3X_RDMA_AHB_END_ADDR_5                    0x113c
#define T3X_RDMA_AUTO_SRC6_SEL                     0x113d
#define T3X_RDMA_AHB_START_ADDR_6_MSB              0x113e
#define T3X_RDMA_AHB_START_ADDR_6                  0x113f
#define T3X_RDMA_AHB_END_ADDR_6_MSB                0x1140
#define T3X_RDMA_AHB_END_ADDR_6                    0x1141
#define T3X_RDMA_AUTO_SRC7_SEL                     0x1142
#define T3X_RDMA_AHB_START_ADDR_7_MSB              0x1143
#define T3X_RDMA_AHB_START_ADDR_7                  0x1144
#define T3X_RDMA_AHB_END_ADDR_7_MSB                0x1145
#define T3X_RDMA_AHB_END_ADDR_7                    0x1146
#define T3X_RDMA_AUTO_SRC8_SEL                     0x1147
#define T3X_RDMA_AHB_START_ADDR_8_MSB              0x1148
#define T3X_RDMA_AHB_START_ADDR_8                  0x1149
#define T3X_RDMA_AHB_END_ADDR_8_MSB                0x114a
#define T3X_RDMA_AHB_END_ADDR_8                    0x114b
#define T3X_RDMA_AUTO_SRC9_SEL                     0x114c
#define T3X_RDMA_AHB_START_ADDR_9_MSB              0x114d
#define T3X_RDMA_AHB_START_ADDR_9                  0x114e
#define T3X_RDMA_AHB_END_ADDR_9_MSB                0x114f
#define T3X_RDMA_AHB_END_ADDR_9                    0x1150
#define T3X_RDMA_AUTO_SRC10_SEL                    0x1151
#define T3X_RDMA_AHB_START_ADDR_10_MSB             0x1152
#define T3X_RDMA_AHB_START_ADDR_10                 0x1153
#define T3X_RDMA_AHB_END_ADDR_10_MSB               0x1154
#define T3X_RDMA_AHB_END_ADDR_10                   0x1155
#define T3X_RDMA_AUTO_SRC11_SEL                    0x1156
#define T3X_RDMA_AHB_START_ADDR_11_MSB             0x1157
#define T3X_RDMA_AHB_START_ADDR_11                 0x1158
#define T3X_RDMA_AHB_END_ADDR_11_MSB               0x1159
#define T3X_RDMA_AHB_END_ADDR_11                   0x115a
#define T3X_RDMA_AUTO_SRC12_SEL                    0x115b
#define T3X_RDMA_AHB_START_ADDR_12_MSB             0x115c
#define T3X_RDMA_AHB_START_ADDR_12                 0x115d
#define T3X_RDMA_AHB_END_ADDR_12_MSB               0x115e
#define T3X_RDMA_AHB_END_ADDR_12                   0x115f
#define T3X_RDMA_AUTO_SRC13_SEL                    0x1160
#define T3X_RDMA_AHB_START_ADDR_13_MSB             0x1161
#define T3X_RDMA_AHB_START_ADDR_13                 0x1162
#define T3X_RDMA_AHB_END_ADDR_13_MSB               0x1163
#define T3X_RDMA_AHB_END_ADDR_13                   0x1164
#define T3X_RDMA_AUTO_SRC14_SEL                    0x1165
#define T3X_RDMA_AHB_START_ADDR_14_MSB             0x1166
#define T3X_RDMA_AHB_START_ADDR_14                 0x1167
#define T3X_RDMA_AHB_END_ADDR_14_MSB               0x1168
#define T3X_RDMA_AHB_END_ADDR_14                   0x1169
#define T3X_RDMA_AUTO_SRC15_SEL                    0x116a
#define T3X_RDMA_AHB_START_ADDR_15_MSB             0x116b
#define T3X_RDMA_AHB_START_ADDR_15                 0x116c
#define T3X_RDMA_AHB_END_ADDR_15_MSB               0x116d
#define T3X_RDMA_AHB_END_ADDR_15                   0x116e
#endif

#endif

