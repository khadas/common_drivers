/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __SC2_MBOX_H__
#define __SC2_MBOX_H__

#include "amlogic,mbox.h"

// MBOX DRIVER ID
#define SC2_DSPA2REE0     0
#define SC2_REE2DSPA0     1
#define SC2_REE2DSPA1     2
#define SC2_REE2DSPA2     3
#define SC2_AO2REE        4
#define SC2_REE2AO0       (SC2_AO2REE + 1)
#define SC2_REE2AO1       (SC2_AO2REE + 2)
#define SC2_REE2AO2       (SC2_AO2REE + 3)
#define SC2_REE2AO3       (SC2_AO2REE + 4)
#define SC2_REE2AO4       (SC2_AO2REE + 5)
#define SC2_REE2AO5       (SC2_AO2REE + 6)

// MBOX CHANNEL ID
#define SC2_MBOX_DSPA2REE  0
#define SC2_MBOX_REE2DSPA  1
#define SC2_MBOX_AO2REE    2
#define SC2_MBOX_REE2AO    3
#define SC2_MBOX_NUMS      4

// MBOX SEC CHANNEL ID
#define SC2_MBOX_REE2SCPU  0
#define SC2_MBOX_SCPU2REE  1

// DEVICE TREE ID
#define SC2_REE2DSPA_DEV  SC2_REE2DSPA0
#define SC2_REE2DSPA_DSP  SC2_REE2DSPA1
#define SC2_DSPA2REE_DEV  SC2_DSPA2REE0
#define SC2_REE2AO_DEV    SC2_REE2AO0
#define SC2_REE2AO_VRTC   SC2_REE2AO1
#define SC2_REE2AO_KEYPAD SC2_REE2AO2
#define SC2_REE2AO_AOCEC  SC2_REE2AO3
#define SC2_REE2AO_ETH    SC2_REE2AO4
#define SC2_REE2SCPU_DEV  SC2_MBOX_REE2SCPU

#endif /* __SC2_MBOX_H__ */
