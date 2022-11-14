/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __SM1_MBOX_H__
#define __SM1_MBOX_H__

#include "amlogic,mbox.h"

// MBOX DRIVER ID
#define SM1_REE2AO0       1
#define SM1_REE2AO1       2
#define SM1_REE2AO2       3
#define SM1_REE2AO3       4
#define SM1_REE2AO4       5
#define SM1_REE2AO5       6
#define SM1_REE2AO6       7
#define SM1_REE2AO7       8

#define SM1_REE2MF0       0
#define SM1_REE2MF1       1
#define SM1_REE2MF2       2
#define SM1_REE2MF3       3
#define SM1_REE2MF4       4

#define SM1_REE2AO_DEV    SM1_REE2AO0
#define SM1_REE2AO_VRTC   SM1_REE2AO1
#define SM1_REE2AO_MF     SM1_REE2AO2
#define SM1_REE2AO_AOCEC  SM1_REE2AO3
#define SM1_REE2AO_LED    SM1_REE2AO4
#define SM1_REE2AO_ETH    SM1_REE2AO5

#define SM1_REE2MF_MF     SM1_REE2MF0
// MBOX CHANNEL ID
#define SM1_MBOX_MF2REE    0
#define SM1_MBOX_REE2MF    1
#define SM1_MBOX_MF_NUMS   2

#define SM1_MBOX_REE2AO    0
#define SM1_MBOX_AO_NUMS   1

#endif /* __SM1_MBOX_H__ */
