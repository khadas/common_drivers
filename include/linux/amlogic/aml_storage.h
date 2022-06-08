/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AML_STORAGE_H_
#define __AML_STORAGE_H_

#define BL2E_STORAGE_PARAM_SIZE		(0x80)
#define BOOT_FIRST_BLOB_SIZE		(254 * 1024)
#define BOOT_FILLER_SIZE	(4 * 1024)
#define BOOT_RESERVED_SIZE	(4 * 1024)
#define BOOT_RANDOM_NONCE	(16)
#define BOOT_BL2E_SIZE		(66672) //74864-8K
#define BOOT_EBL2E_SIZE		\
	(BOOT_FILLER_SIZE + BOOT_RESERVED_SIZE + BOOT_BL2E_SIZE)
#define BOOT_BL2X_SIZE		(66672)
#define MAX_BOOT_AREA_ENTRIES	(8)
#define BL2_CORE_BASE_OFFSET_EMMC	(0x200000)
#define BOOT_AREA_BB1ST             (0)
#define BOOT_AREA_BL2E              (1)
#define BOOT_AREA_BL2X              (2)
#define BOOT_AREA_DDRFIP            (3)
#define BOOT_AREA_DEVFIP            (4)
#define BOOT_AREA_INVALID           (MAX_BOOT_AREA_ENTRIES)
#define BAE_BB1ST                   "1STBLOB"
#define BAE_BL2E                    "BL2E"
#define BAE_BL2X                    "BL2X"
#define BAE_DDRFIP                  "DDRFIP"
#define BAE_DEVFIP                  "DEVFIP"

struct boot_area_entry {
	char name[11];
	unsigned char idx;
	u64 offset;
	u64 size;
};

struct boot_layout {
	struct boot_area_entry *boot_entry;
};

struct storage_boot_entry {
	unsigned int offset;
	unsigned int size;
};

struct nand_startup_parameter {
	int page_size;
	int block_size;
	int layout_reserve_size;
	int pages_per_block;
	int setup_data;
	int page0_disable;
};

union storage_independent_parameter {
	struct nand_startup_parameter nsp;
};

struct storage_startup_parameter {
	unsigned char boot_device;
	unsigned char	boot_seq;
	unsigned char	boot_backups;
	unsigned char reserved;
	struct storage_boot_entry boot_entry[MAX_BOOT_AREA_ENTRIES];
	union storage_independent_parameter sip;
};

extern struct storage_startup_parameter g_ssp;
int aml_nand_param_check_and_layout_init(struct mtd_info *mtd);
/**sc2 new layout**/

#endif  /* __AML_STORAGE_H_ */
