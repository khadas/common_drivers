/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _MMC_DTB_H_
#define _MMC_DTB_H_

#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>

void amlmmc_dtb_init(struct mmc_card *card_dtbkey, int *retp);

#endif
