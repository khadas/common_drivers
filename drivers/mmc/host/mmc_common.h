/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _MMC_COMMON_H_
#define _MMC_COMMON_H_

int mmc_check_result(struct mmc_request *mrq);

int mmc_transfer(struct mmc_card *card, unsigned int dev_addr,
					unsigned int blocks, void *buf, int write);

int mmc_read_internal(struct mmc_card *card, unsigned int dev_addr,
						unsigned int blocks, void *buf);

int mmc_write_internal(struct mmc_card *card, unsigned int dev_addr,
						unsigned int blocks, void *buf);

void mmc_prepare_mrq(struct mmc_card *card,
						struct mmc_request *mrq, struct scatterlist *sg,
						unsigned int sg_len, unsigned int dev_addr,
						unsigned int blocks,
						unsigned int blksz, int write);

int get_reserve_partition_off_from_tbl(void);


#endif

