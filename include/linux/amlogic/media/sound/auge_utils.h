/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __AUGE_UTILS_H__
#define __AUGE_UTILS_H__

void auge_acodec_reset(void);
void auge_toacodec_ctrl(int tdmout_id);
void auge_toacodec_ctrl_ext(int tdmout_id, int ch0_sel, int ch1_sel);
#endif
