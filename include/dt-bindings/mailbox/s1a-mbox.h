/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __S1A_MBOX_H__
#define __S1A_MBOX_H__

#include "amlogic,mbox.h"

// MBOX DRIVER ID
#define S1A_AO2REE        0
#define S1A_REE2AO0       (S1A_AO2REE + 1)
#define S1A_REE2AO1       (S1A_AO2REE + 2)
#define S1A_REE2AO2       (S1A_AO2REE + 3)
#define S1A_REE2AO3       (S1A_AO2REE + 4)
#define S1A_REE2AO4       (S1A_AO2REE + 5)
#define S1A_REE2AO5       (S1A_AO2REE + 6)
#define S1A_REE2AO6       (S1A_AO2REE + 7)

// MBOX CHANNEL ID
#define S1A_MBOX_AO2REE    2
#define S1A_MBOX_REE2AO    3
#define S1A_MBOX_NUMS      2

// MBOX SEC CHANNEL ID
#define S1A_MBOX_REE2SCPU  0
#define S1A_MBOX_SCPU2REE  1

// DEVICE TREE ID
#define S1A_REE2AO_DEV    S1A_REE2AO0
#define S1A_REE2AO_VRTC   S1A_REE2AO1
#define S1A_REE2AO_KEYPAD S1A_REE2AO2
#define S1A_REE2AO_AOCEC  S1A_REE2AO3
#define S1A_REE2AO_ETH    S1A_REE2AO4
#define S1A_REE2AO_LED    S1A_REE2AO5
#define S1A_REE2AO_FD650    S1A_REE2AO6
#define S1A_REE2SCPU_DEV  S1A_MBOX_REE2SCPU
#endif /* __S1A_MBOX_H__ */
