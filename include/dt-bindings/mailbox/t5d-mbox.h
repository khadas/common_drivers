/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __T5D_MBOX_H__
#define __T5D_MBOX_H__

#include "amlogic,mbox.h"

// MBOX DRIVER ID
#define T5D_AO2REE        0
#define T5D_REE2AO0       (T5D_AO2REE + 1)
#define T5D_REE2AO1       (T5D_AO2REE + 2)
#define T5D_REE2AO2       (T5D_AO2REE + 3)
#define T5D_REE2AO3       (T5D_AO2REE + 4)
#define T5D_REE2AO4       (T5D_AO2REE + 5)
#define T5D_REE2AO5       (T5D_AO2REE + 6)

#define T5D_REE2AO_DEV    T5D_REE2AO0
#define T5D_REE2AO_VRTC   T5D_REE2AO1
#define T5D_REE2AO_KEYPAD T5D_REE2AO2
#define T5D_REE2AO_AOCEC  T5D_REE2AO3
#define T5D_REE2AO_LED    T5D_REE2AO4
#define T5D_REE2AO_ETH    T5D_REE2AO5

// MBOX CHANNEL ID
#define T5D_MBOX_AO2REE    2
#define T5D_MBOX_REE2AO    3
#define T5D_MBOX_NUMS      2

#endif /* __T5D_MBOX_H__ */
