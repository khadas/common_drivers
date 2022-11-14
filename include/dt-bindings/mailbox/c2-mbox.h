/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __C2_MBOX_H__
#define __C2_MBOX_H__

#include "amlogic,mbox.h"

// MBOX DRIVER ID
#define C2_DSPA2REE0     0
#define C2_REE2DSPA0     1
#define C2_REE2DSPA1     2
#define C2_REE2DSPA2     3
#define C2_AO2REE        4
#define C2_REE2AO0       (C2_AO2REE + 1)
#define C2_REE2AO1       (C2_AO2REE + 2)
#define C2_REE2AO2       (C2_AO2REE + 3)
#define C2_REE2AO3       (C2_AO2REE + 4)
#define C2_REE2AO4       (C2_AO2REE + 5)
#define C2_REE2AO5       (C2_AO2REE + 6)

#define C2_REE2DSPA_DEV  C2_REE2DSPA0
#define C2_REE2DSPA_DSP  C2_REE2DSPA1
#define C2_DSPA2REE_DEV  C2_DSPA2REE0
#define C2_REE2AO_DEV    C2_REE2AO0
#define C2_REE2AO_VRTC   C2_REE2AO1
#define C2_REE2AO_KEYPAD C2_REE2AO2
#define C2_REE2AO_AOCEC  C2_REE2AO3

// MBOX CHANNEL ID
#define C2_MBOX_REE2DSPA  0
#define C2_MBOX_DSPA2REE  1
#define C2_MBOX_REE2AO    2
#define C2_MBOX_AO2REE    3
#define C2_MBOX_NUMS      4

#endif /* __C2_MBOX_H__ */
