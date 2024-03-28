/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __G12A_MBOX_H__
#define __G12A_MBOX_H__

#include "amlogic,mbox.h"

// MBOX DRIVER ID
#define G12A_REE2AO0       1
#define G12A_REE2AO1       2
#define G12A_REE2AO2       3
#define G12A_REE2AO3       4
#define G12A_REE2AO4       5
#define G12A_REE2AO5       6
#define G12A_REE2AO6       7
#define G12A_REE2AO7       8

#define G12A_REE2MF0       0
#define G12A_REE2MF1       1
#define G12A_REE2MF2       2
#define G12A_REE2MF3       3
#define G12A_REE2MF4       4
#define G12A_MF2REE0       (G12A_REE2MF4 + 1)

// MBOX DEVICE TREE ID
#define G12A_REE2AO_DEV    G12A_REE2AO0
#define G12A_REE2AO_VRTC   G12A_REE2AO1
#define G12A_REE2AO_MF     G12A_REE2AO2
#define G12A_REE2AO_AOCEC  G12A_REE2AO3
#define G12A_REE2AO_LED    G12A_REE2AO4
#define G12A_REE2AO_ETH    G12A_REE2AO5

#define G12A_REE2MF_MF     G12A_REE2MF0
#define G12A_MF2REE_DEV    G12A_MF2REE0

// MBOX CHANNEL ID
#define G12A_MBOX_MF2REE    0
#define G12A_MBOX_REE2MF    1
#define G12A_MBOX_MF_NUMS   2

#define G12A_MBOX_REE2AO    0
#define G12A_MBOX_AO_NUMS   1

#endif /* __G12A_MBOX_H__ */
