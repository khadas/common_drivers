/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __AML_CARD_H_
#define __AML_CARD_H_

#include <sound/soc.h>
#include "card_utils.h"

struct aml_card_info {
	const char *name;
	const char *card;
	const char *codec;
	const char *platform;

	unsigned int daifmt;
	struct aml_dai cpu_dai;
	struct aml_dai codec_dai;
};

#endif /* __AML_CARD_H_ */
