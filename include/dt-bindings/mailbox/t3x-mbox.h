/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __T3X_MBOX_H__
#define __T3X_MBOX_H__

#include "amlogic,mbox.h"

// MBOX DRIVER ID
#define T3X_DSPA2REE0     0
#define T3X_REE2DSPA0     1
#define T3X_REE2DSPA1     2
#define T3X_REE2DSPA2     3
#define T3X_AO2REE        4
#define T3X_REE2AO0       (T3X_AO2REE + 1)
#define T3X_REE2AO1       (T3X_AO2REE + 2)
#define T3X_REE2AO2       (T3X_AO2REE + 3)
#define T3X_REE2AO3       (T3X_AO2REE + 4)
#define T3X_REE2AO4       (T3X_AO2REE + 5)
#define T3X_REE2AO5       (T3X_AO2REE + 6)

#define T3X_REE2DSPA_DEV  T3X_REE2DSPA0
#define T3X_REE2DSPA_DSP  T3X_REE2DSPA1
#define T3X_DSPA2REE_DEV  T3X_DSPA2REE0
#define T3X_REE2AO_DEV    T3X_REE2AO0
#define T3X_REE2AO_VRTC   T3X_REE2AO1
#define T3X_REE2AO_KEYPAD T3X_REE2AO2
#define T3X_REE2AO_AOCEC  T3X_REE2AO3
#define T3X_REE2AO_LED    T3X_REE2AO4
#define T3X_REE2AO_ETH    T3X_REE2AO5

// MBOX CHANNEL ID
#define T3X_MBOX_DSPA2REE  0
#define T3X_MBOX_REE2DSPA  1
#define T3X_MBOX_AO2REE    2
#define T3X_MBOX_REE2AO    3
#define T3X_MBOX_NUMS      4

#endif /* __T3X_MBOX_H__ */
