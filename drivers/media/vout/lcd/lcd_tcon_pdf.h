/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AML_LCD_TCON_PDF_H__
#define __AML_LCD_TCON_PDF_H__
#include <linux/amlogic/media/vout/lcd/lcd_vout.h>

struct lcd_tcon_pdf_data_s {
	unsigned int data_type;  //pdf_src, pdf_dst or pdf_action
	union lcd_tcon_data_part_u data;
	struct list_head list;
};

enum pdf_reg_mode_e {
	PDF_REG_MODE_RD_CHK_ALL,
	PDF_REG_MODE_RD_CHK_ANY,
	PDF_REG_MODE_WR_GPIO,
	PDF_REG_MODE_WR,
};

enum pdf_dst_mode_e {
	PDF_DST_MODE_P2P_CMD,
	PDF_DST_MODE_P2P_UNIT,
	PDF_DST_MODE_GPIO_UNIT,
	PDF_DST_MODE_GPIO,
};

struct tcon_pdf_dst_s {
	enum pdf_dst_mode_e mode;

	unsigned int index;
	unsigned int mask;
	unsigned int val;

	struct list_head list;
};

struct tcon_pdf_reg_s {
	enum pdf_reg_mode_e mode;

	unsigned int reg;
	unsigned int mask;
	unsigned int val;
	unsigned int dft_val;

	struct list_head list;
};

struct tcon_pdf_action_s {
	unsigned char enable;
	unsigned char detected;
	unsigned char group;

	struct tcon_pdf_reg_s src;
	struct list_head dst_list;  //for tcon_pdf_dst_s

	struct list_head reg_list;  //for tcon_pdf_reg_s

	struct list_head list;
};

struct tcon_pdf_s {
	unsigned char enable;
	unsigned char detected;
	void *priv_data;

	struct list_head action_list;  //for tcon_pdf_action_s

	spinlock_t vs_lock;  //for vsync isr

	/* interface */
	int (*vs_handler)(struct tcon_pdf_s *pdf);
	int (*is_enable)(struct tcon_pdf_s *pdf);
	ssize_t (*show_status)(struct tcon_pdf_s *pdf, char *buf);

	/*
	 * group: action group number
	 *        0xff means just control pdf function enable
	 * enable: 0-disable, 1-enable
	 */
	int (*group_enable)(struct tcon_pdf_s *pdf,
			unsigned char group, int enable);
	int (*new_group)(struct tcon_pdf_s *pdf, unsigned char group);
	int (*group_add_src)(struct tcon_pdf_s *pdf, unsigned char group,
			struct tcon_pdf_reg_s *src);
	int (*group_add_dst)(struct tcon_pdf_s *pdf, unsigned char group,
			struct tcon_pdf_dst_s *dst);
	int (*del_group)(struct tcon_pdf_s *pdf, unsigned char group);
};

int lcd_tcon_pdf_init(struct aml_lcd_drv_s *pdrv);
int lcd_tcon_pdf_get_config(struct list_head *data_list);
struct tcon_pdf_s *lcd_tcon_get_pdf(void);
int lcd_tcon_pdf_remove(struct aml_lcd_drv_s *pdrv);

#endif  //__AML_LCD_TCON_PDF_H__

