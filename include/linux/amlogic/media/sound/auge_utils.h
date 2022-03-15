/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __AUGE_UTILS_H__
#define __AUGE_UTILS_H__

#define DATA_SEL_SHIFT_VERSION0 (0)
#define DATA_SEL_SHIFT_VERSION1 (1)

void auge_acodec_reset(void);
void auge_toacodec_ctrl(int tdmout_id);
void auge_toacodec_ctrl_ext(int tdmout_id, int ch0_sel, int ch1_sel,
	bool separate_toacodec_en, int data_sel_shift);
#endif
