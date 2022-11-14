/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __TXHD2_MBOX_H__
#define __TXHD2_MBOX_H__

#include "amlogic,mbox.h"

// MBOX DRIVER ID
#define TXHD2_AO2REE        0
#define TXHD2_REE2AO0       (TXHD2_AO2REE + 1)
#define TXHD2_REE2AO1       (TXHD2_AO2REE + 2)
#define TXHD2_REE2AO2       (TXHD2_AO2REE + 3)
#define TXHD2_REE2AO3       (TXHD2_AO2REE + 4)
#define TXHD2_REE2AO4       (TXHD2_AO2REE + 5)
#define TXHD2_REE2AO5       (TXHD2_AO2REE + 6)

#define TXHD2_REE2AO_DEV    TXHD2_REE2AO0
#define TXHD2_REE2AO_VRTC   TXHD2_REE2AO1
#define TXHD2_REE2AO_KEYPAD TXHD2_REE2AO2
#define TXHD2_REE2AO_AOCEC  TXHD2_REE2AO3
#define TXHD2_REE2AO_LED    TXHD2_REE2AO4
#define TXHD2_REE2AO_ETH    TXHD2_REE2AO5

// MBOX CHANNEL ID
#define TXHD2_MBOX_AO2REE    2
#define TXHD2_MBOX_REE2AO    3
#define TXHD2_MBOX_NUMS      2

#endif /* __TXHD2_MBOX_H__ */
