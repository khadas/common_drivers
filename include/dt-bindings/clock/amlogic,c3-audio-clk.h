/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __C3_AUDIO_CLK_H__
#define __C3_AUDIO_CLK_H__

/*
 * CLKID audio index values
 */

/* Audio CLK Gate En0 */
#define CLKID_AUDIO_GATE_DDR_ARB                0
#define CLKID_AUDIO_GATE_PDM                    1
#define CLKID_AUDIO_GATE_TDMINA                 2
#define CLKID_AUDIO_GATE_TDMINLB                3
#define CLKID_AUDIO_GATE_TDMOUTA                4
#define CLKID_AUDIO_GATE_TDMOUTB                5
#define CLKID_AUDIO_GATE_FRDDRA                 6
#define CLKID_AUDIO_GATE_FRDDRB                 7
#define CLKID_AUDIO_GATE_FRDDRC                 8
#define CLKID_AUDIO_GATE_TODDRA                 9
#define CLKID_AUDIO_GATE_TODDRB                 10
#define CLKID_AUDIO_GATE_LOOPBACKA              11
#define CLKID_AUDIO_GATE_RESAMPLEA              12
#define CLKID_AUDIO_GATE_EQDRC                  13
#define CLKID_AUDIO_GATE_TOVAD                  14
#define CLKID_AUDIO_GATE_AUDIOLOCKER            15

#define CLKID_AUDIO_GATE_MAX                    16

#define MCLK_BASE                               CLKID_AUDIO_GATE_MAX
#define CLKID_AUDIO_MCLK_A                      (MCLK_BASE + 0)
#define CLKID_AUDIO_MCLK_B                      (MCLK_BASE + 1)
#define CLKID_AUDIO_MCLK_C                      (MCLK_BASE + 2)
#define CLKID_AUDIO_MCLK_D                      (MCLK_BASE + 3)
#define CLKID_AUDIO_MCLK_E                      (MCLK_BASE + 4)
#define CLKID_AUDIO_MCLK_F                      (MCLK_BASE + 5)

#define CLKID_AUDIO_RESAMPLE_A                  (MCLK_BASE + 6)
#define CLKID_AUDIO_LOCKER_OUT                  (MCLK_BASE + 7)
#define CLKID_AUDIO_LOCKER_IN                   (MCLK_BASE + 8)
#define CLKID_AUDIO_PDMIN0                      (MCLK_BASE + 9)
#define CLKID_AUDIO_PDMIN1                      (MCLK_BASE + 10)
#define CLKID_AUDIO_EQDRC                       (MCLK_BASE + 11)
#define CLKID_AUDIO_VAD                         (MCLK_BASE + 12)

#define CLKID_AUDIO_MCLK_PAD0                   (MCLK_BASE + 13)
#define CLKID_AUDIO_MCLK_PAD1                   (MCLK_BASE + 14)
#define CLKID_AUDIO_MCLK_PAD2                   (MCLK_BASE + 15)
#define CLKID_AUDIO_MCLK_PAD3                   (MCLK_BASE + 16)

#define NUM_AUDIO_CLKS                          (MCLK_BASE + 17)
#endif /* __C3_AUDIO_CLK_H__ */
