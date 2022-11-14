/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __T3_MBOX_H__
#define __T3_MBOX_H__

#include "amlogic,mbox.h"

// MBOX DRIVER ID
#define T3_DSPA2REE0     0
#define T3_REE2DSPA0     1
#define T3_REE2DSPA1     2
#define T3_REE2DSPA2     3
#define T3_AO2REE        4
#define T3_REE2AO0       (T3_AO2REE + 1)
#define T3_REE2AO1       (T3_AO2REE + 2)
#define T3_REE2AO2       (T3_AO2REE + 3)
#define T3_REE2AO3       (T3_AO2REE + 4)
#define T3_REE2AO4       (T3_AO2REE + 5)
#define T3_REE2AO5       (T3_AO2REE + 6)

#define T3_REE2DSPA_DEV  T3_REE2DSPA0
#define T3_REE2DSPA_DSP  T3_REE2DSPA1
#define T3_DSPA2REE_DEV  T3_DSPA2REE0
#define T3_REE2AO_DEV    T3_REE2AO0
#define T3_REE2AO_VRTC   T3_REE2AO1
#define T3_REE2AO_KEYPAD T3_REE2AO2
#define T3_REE2AO_AOCEC  T3_REE2AO3
#define T3_REE2AO_LED    T3_REE2AO4
#define T3_REE2AO_ETH    T3_REE2AO5

// MBOX CHANNEL ID
#define T3_MBOX_DSPA2REE  0
#define T3_MBOX_REE2DSPA  1
#define T3_MBOX_AO2REE    2
#define T3_MBOX_REE2AO    3
#define T3_MBOX_NUMS      4

#endif /* __T3_MBOX_H__ */
