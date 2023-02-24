/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __AML_CARD_H_
#define __AML_CARD_H_

#include <sound/soc.h>
#include "card_utils.h"
#include "../common/iec_info.h"

enum hdmitx_src {
	HDMITX_SRC_SPDIF,
	HDMITX_SRC_SPDIF_B,
	HDMITX_SRC_TDM_A,
	HDMITX_SRC_TDM_B,
	HDMITX_SRC_TDM_C,
	HDMITX_SRC_NUM
};

struct aml_card_info {
	const char *name;
	const char *card;
	const char *codec;
	const char *platform;

	unsigned int daifmt;
	struct aml_dai cpu_dai;
	struct aml_dai codec_dai;
};

enum hdmitx_src get_hdmitx_audio_src(struct snd_soc_card *card);
enum aud_codec_types get_i2s2hdmitx_audio_format(struct snd_soc_card *card);
int get_hdmitx_i2s_mask(struct snd_soc_card *card);

#endif /* __AML_CARD_H_ */
