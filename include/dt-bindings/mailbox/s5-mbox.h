/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __S5_MBOX_H__
#define __S5_MBOX_H__

#include "amlogic,mbox.h"

// MBOX DRIVER ID
#define S5_AO2REE        0
#define S5_REE2AO0       (S5_AO2REE + 1)
#define S5_REE2AO1       (S5_AO2REE + 2)
#define S5_REE2AO2       (S5_AO2REE + 3)
#define S5_REE2AO3       (S5_AO2REE + 4)
#define S5_REE2AO4       (S5_AO2REE + 5)
#define S5_REE2AO5       (S5_AO2REE + 6)

#define S5_REE2AO_DEV    S5_REE2AO0
#define S5_REE2AO_VRTC   S5_REE2AO1
#define S5_REE2AO_KEYPAD S5_REE2AO2
#define S5_REE2AO_AOCEC  S5_REE2AO3
#define S5_REE2AO_LED    S5_REE2AO4
#define S5_REE2AO_ETH    S5_REE2AO5

// MBOX CHANNEL ID
#define S5_MBOX_AO2REE    2
#define S5_MBOX_REE2AO    3
#define S5_MBOX_NUMS      2

#endif /* __S5_MBOX_H__ */
