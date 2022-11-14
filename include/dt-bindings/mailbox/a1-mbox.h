/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __A1_MBOX_H__
#define __A1_MBOX_H__

#include "amlogic,mbox.h"

// MBOX DRIVER ID
#define A1_DSPA2REE0     0
#define A1_REE2DSPA0     1
#define A1_REE2DSPA1     2
#define A1_REE2DSPA2     3
#define A1_DSPB2REE0     (A1_REE2DSPA2 + 1)
#define A1_REE2DSPB0     (A1_REE2DSPA2 + 2)
#define A1_REE2DSPB1     (A1_REE2DSPA2 + 3)
#define A1_REE2DSPB2     (A1_REE2DSPA2 + 4)

#define A1_REE2DSPA_DEV  A1_REE2DSPA0
#define A1_REE2DSPA_DSP  A1_REE2DSPA1
#define A1_REE2DSPA_AUDIO A1_REE2DSPA2
#define A1_DSPA2REE_DEV  A1_DSPA2REE0
#define A1_REE2DSPB_DEV  A1_REE2DSPB0
#define A1_REE2DSPB_DSP  A1_REE2DSPB1
#define A1_DSPB2REE_DEV  A1_DSPB2REE0
#define A1_REE2DSPB_AUDIO A1_REE2DSPB2

// MBOX CHANNEL ID
#define A1_MBOX_DSPA2REE  0
#define A1_MBOX_REE2DSPA  1
#define A1_MBOX_DSPB2REE  2
#define A1_MBOX_REE2DSPB  3
#define A1_MBOX_NUMS      4

#endif /* __A1_MBOX_H__ */
