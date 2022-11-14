/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __G12B_MBOX_H__
#define __G12B_MBOX_H__

#include "amlogic,mbox.h"

// MBOX DRIVER ID
#define G12B_REE2AO_L0       0
#define G12B_REE2AO_L1       1
#define G12B_REE2AO_L2       2
#define G12B_REE2AO_L3       3
#define G12B_REE2AO_L4       4
#define G12B_REE2AO_L5       5
#define G12B_REE2AO_L6       6
#define G12B_REE2AO_L7       7

#define G12B_REE2AO_DEV    G12B_REE2AO_L0
#define G12B_REE2AO_KEYPAD G12B_REE2AO_L2
#define G12B_REE2AO_AOCEC  G12B_REE2AO_L3
#define G12B_REE2AO_LED    G12B_REE2AO_L4
#define G12B_REE2AO_ETH    G12B_REE2AO_L5

#define G12B_REE2AO_VRTC   G12B_REE2AO_L0

// MBOX CHANNEL ID
#define G12B_MBOX_REE2AO        0
#define G12B_MBOX_AO_NUMS       1

#endif /* __G12B_MBOX_H__ */
