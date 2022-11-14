/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __T7_MBOX_H__
#define __T7_MBOX_H__

#include "amlogic,mbox.h"

// MBOX DRIVER ID
#define T7_DSPA2REE0     0
#define T7_REE2DSPA0     1
#define T7_REE2DSPA1     2
#define T7_REE2DSPA2     3
#define T7_AO2REE        4
#define T7_REE2AO0       (T7_AO2REE + 1)
#define T7_REE2AO1       (T7_AO2REE + 2)
#define T7_REE2AO2       (T7_AO2REE + 3)
#define T7_REE2AO3       (T7_AO2REE + 4)
#define T7_REE2AO4       (T7_AO2REE + 5)
#define T7_REE2AO5       (T7_AO2REE + 6)
#define T7_DSPB2REE0     (T7_REE2AO5 + 1)
#define T7_REE2DSPB0     (T7_REE2AO5 + 2)
#define T7_REE2DSPB1     (T7_REE2AO5 + 3)
#define T7_REE2DSPB2     (T7_REE2AO5 + 4)

#define T7_REE2DSPA_DEV  T7_REE2DSPA0
#define T7_REE2DSPA_DSP  T7_REE2DSPA1
#define T7_DSPA2REE_DEV  T7_DSPA2REE0
#define T7_REE2AO_DEV    T7_REE2AO0
#define T7_REE2AO_VRTC   T7_REE2AO1
#define T7_REE2AO_KEYPAD T7_REE2AO2
#define T7_REE2AO_AOCEC  T7_REE2AO3
#define T7_REE2AO_LED    T7_REE2AO4
#define T7_REE2AO_ETH    T7_REE2AO5
#define T7_REE2DSPB_DEV  T7_REE2DSPB0
#define T7_REE2DSPB_DSP  T7_REE2DSPB1
#define T7_DSPB2REE_DEV  T7_DSPB2REE0

// MBOX CHANNEL ID
#define T7_MBOX_DSPA2REE  0
#define T7_MBOX_REE2DSPA  1
#define T7_MBOX_AO2REE    2
#define T7_MBOX_REE2AO    3
#define T7_MBOX_DSPB2REE  6
#define T7_MBOX_REE2DSPB  7
#define T7_MBOX_NUMS      6

#endif /* __T7_MBOX_H__ */
