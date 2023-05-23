/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __C1_MBOX_H__
#define __C1_MBOX_H__

#include "amlogic,mbox.h"

// MBOX DRIVER ID
#define C1_DSPA2REE0     0
#define C1_REE2DSPA0     1
#define C1_REE2DSPA1     2
#define C1_REE2DSPA2     3
#define C1_DSPB2REE0     (C1_REE2DSPA2 + 1)
#define C1_REE2DSPB0     (C1_REE2DSPA2 + 2)
#define C1_REE2DSPB1     (C1_REE2DSPA2 + 3)
#define C1_REE2DSPB2     (C1_REE2DSPA2 + 4)

#define C1_REE2DSPA_DEV  C1_REE2DSPA0
#define C1_REE2DSPA_DSP  C1_REE2DSPA1
#define C1_REE2DSPA_AUDIO C1_REE2DSPA2
#define C1_DSPA2REE_DEV  C1_DSPA2REE0
#define C1_REE2DSPB_DEV  C1_REE2DSPB0
#define C1_REE2DSPB_DSP  C1_REE2DSPB1
#define C1_DSPB2REE_DEV  C1_DSPB2REE0
#define C1_REE2DSPB_AUDIO C1_REE2DSPB2

// MBOX CHANNEL ID
#define C1_MBOX_DSPA2REE  0
#define C1_MBOX_REE2DSPA  1
#define C1_MBOX_DSPB2REE  2
#define C1_MBOX_REE2DSPB  3
#define C1_MBOX_NUMS      4

#endif /* __C1_MBOX_H__ */
