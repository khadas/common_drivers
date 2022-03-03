/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __AML_SND_IOMAP_H__
#define __AML_SND_IOMAP_H__

#include "ddr_mngr.h"

enum{
	IO_PDM_BUS = 0,
	IO_AUDIO_BUS,
	IO_AUDIO_LOCKER,
	IO_EQDRC_BUS,
	IO_RESET,
	IO_VAD,
	IO_RESAMPLEA,
	IO_RESAMPLEB,
	IO_TOP_VAD,
	IO_EARCRX_CMDC = IO_RESAMPLEB + 1,
	IO_EARCRX_DMAC,
	IO_EARCRX_TOP,

	IO_MAX,
};

int aml_pdm_read(unsigned int reg);
void aml_pdm_write(unsigned int reg, unsigned int val);
void aml_pdm_update_bits(unsigned int reg, unsigned int mask,
			 unsigned int val);

int audiobus_read(unsigned int reg);
void audiobus_write(unsigned int reg, unsigned int val);
void audiobus_update_bits(unsigned int reg, unsigned int mask,
			  unsigned int val);

int audiolocker_read(unsigned int reg);
void audiolocker_write(unsigned int reg, unsigned int val);
void audiolocker_update_bits(unsigned int reg, unsigned int mask,
			     unsigned int val);

int eqdrc_read(unsigned int reg);
void eqdrc_write(unsigned int reg, unsigned int val);
void eqdrc_update_bits(unsigned int reg, unsigned int mask,
		       unsigned int val);

int audioreset_read(unsigned int reg);
void audioreset_write(unsigned int reg, unsigned int val);
void audioreset_update_bits(unsigned int reg, unsigned int mask,
			    unsigned int val);

int vad_read(unsigned int reg);
void vad_write(unsigned int reg, unsigned int val);
void vad_update_bits(unsigned int reg, unsigned int mask,
		     unsigned int val);

unsigned int new_resample_read(enum resample_idx id, unsigned int reg);
void new_resample_write(enum resample_idx id, unsigned int reg,
			unsigned int val);
void new_resample_update_bits(enum resample_idx id, unsigned int reg,
			      unsigned int mask, unsigned int val);

int vad_top_read(unsigned int reg);
void vad_top_write(unsigned int reg, unsigned int val);
void vad_top_update_bits(unsigned int reg,
			 unsigned int mask,
			 unsigned int val);

int earcrx_cmdc_read(unsigned int reg);
void earcrx_cmdc_write(unsigned int reg, unsigned int val);
void earcrx_cmdc_update_bits(unsigned int reg,
			     unsigned int mask, unsigned int val);
int earcrx_dmac_read(unsigned int reg);
void earcrx_dmac_write(unsigned int reg, unsigned int val);
void earcrx_dmac_update_bits(unsigned int reg,
			     unsigned int mask, unsigned int val);
int earcrx_top_read(unsigned int reg);
void earcrx_top_write(unsigned int reg, unsigned int val);
void earcrx_top_update_bits(unsigned int reg,
			    unsigned int mask, unsigned int val);
#endif
