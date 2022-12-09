/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __AML_AUDIO_UTILS_H__
#define __AML_AUDIO_UTILS_H__

#include <sound/soc.h>
#include <sound/control.h>

int snd_card_add_kcontrols(struct snd_soc_card *card);
void audio_locker_set(int enable);
int audio_locker_get(void);
void fratv_enable(bool enable);
void cec_arc_enable(int src, bool enable);
void aml_audio_reset(int reg, int shift, bool use_vadtop);
bool is_force_mpll_clk(void);
#endif
