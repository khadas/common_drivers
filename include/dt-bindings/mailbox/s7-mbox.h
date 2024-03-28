/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __S7_MBOX_H__
#define __S7_MBOX_H__

#include "amlogic,mbox.h"

// MBOX DRIVER ID
#define S7_AO2REE        0
#define S7_REE2AO0       (S7_AO2REE + 1)
#define S7_REE2AO1       (S7_AO2REE + 2)
#define S7_REE2AO2       (S7_AO2REE + 3)
#define S7_REE2AO3       (S7_AO2REE + 4)
#define S7_REE2AO4       (S7_AO2REE + 5)
#define S7_REE2AO5       (S7_AO2REE + 6)

#define S7_REE2AO_DEV    S7_REE2AO0
#define S7_REE2AO_VRTC   S7_REE2AO1
#define S7_REE2AO_KEYPAD S7_REE2AO2
#define S7_REE2AO_AOCEC  S7_REE2AO3
#define S7_REE2AO_LED    S7_REE2AO4
#define S7_REE2AO_ETH    S7_REE2AO5

// MBOX CHANNEL ID
#define S7_MBOX_AO2REE    2
#define S7_MBOX_REE2AO    3
#define S7_MBOX_NUMS      2

// AOCPU STATUS MBOX CHANNEL ID
#define S7_MBOX_AO2TEE    4

#endif /* __S7_MBOX_H__ */
