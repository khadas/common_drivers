/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __S4_MBOX_H__
#define __S4_MBOX_H__

#include "amlogic,mbox.h"

// MBOX DRIVER ID
#define S4_AO2REE        0
#define S4_REE2AO0       (S4_AO2REE + 1)
#define S4_REE2AO1       (S4_AO2REE + 2)
#define S4_REE2AO2       (S4_AO2REE + 3)
#define S4_REE2AO3       (S4_AO2REE + 4)
#define S4_REE2AO4       (S4_AO2REE + 5)
#define S4_REE2AO5       (S4_AO2REE + 6)

#define S4_REE2AO_DEV    S4_REE2AO0
#define S4_REE2AO_VRTC   S4_REE2AO1
#define S4_REE2AO_KEYPAD S4_REE2AO2
#define S4_REE2AO_AOCEC  S4_REE2AO3
#define S4_REE2AO_LED    S4_REE2AO4
#define S4_REE2AO_ETH    S4_REE2AO5

// MBOX CHANNEL ID
#define S4_MBOX_AO2REE    2
#define S4_MBOX_REE2AO    3
#define S4_MBOX_NUMS      2

#endif /* __S4_MBOX_H__ */
