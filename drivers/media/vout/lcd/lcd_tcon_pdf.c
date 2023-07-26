// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include "lcd_common.h"
#include "lcd_reg.h"
#include "lcd_tcon_pdf.h"

struct tcon_reg_map_s {
	/**
	 * for gpio, idx means which gpioh_x
	 * for p2p vpol, idx means which p2p_pol_x
	 */
	unsigned int index;

	unsigned int reg;
	unsigned int mask;

	unsigned int start_bit;
	unsigned int val_mask;
};

typedef unsigned int (*get_val_func_t)(struct tcon_reg_map_s *, unsigned int);

struct pdf_plat_info_s {
	enum lcd_chip_e chip;
	enum p2p_type_e type;

	struct tcon_reg_map_s *gpio_unit_tbl;
	unsigned int gpio_unit_tbl_len;
	get_val_func_t gpio_unit_get_val;

	struct tcon_reg_map_s *p2p_unit_tbl;
	unsigned int p2p_unit_tbl_len;
	get_val_func_t p2p_unit_get_val;

	struct tcon_reg_map_s *p2p_cmd_tbl;
	unsigned int p2p_cmd_tbl_len;
	get_val_func_t p2p_cmd_get_val;
};

static struct tcon_reg_map_s gpio_unit_map_tbl[] = {
	{0,  0x300, 0xf,         0, 0xf},  //GPIOH_0
	{1,  0x300, 0xf0,        4, 0xf},  //GPIOH_1
	{2,  0x300, 0xf00,       8, 0xf},  //GPIOH_2
	{3,  0x300, 0xf000,     12, 0xf},  //GPIOH_3
	{4,  0x300, 0xf0000,    16, 0xf},  //GPIOH_4
	{5,  0x300, 0xf00000,   20, 0xf},  //GPIOH_5
	{6,  0x300, 0xf000000,  24, 0xf},  //GPIOH_6
	{7,  0x300, 0xf0000000, 28, 0xf},  //GPIOH_7
	{8,  0x301, 0xf,         0, 0xf},  //GPIOH_8
	{9,  0x301, 0xf0,        4, 0xf},  //GPIOH_9
	{10, 0x301, 0xf00,       8, 0xf},  //GPIOH_10
	{11, 0x301, 0xf000,     12, 0xf},  //GPIOH_11
	{12, 0x301, 0xf0000,    16, 0xf},  //GPIOH_12
	{13, 0x301, 0xf00000,   20, 0xf},  //GPIOH_13
	{14, 0x301, 0xf000000,  24, 0xf},  //GPIOH_14
	{15, 0x301, 0xf0000000, 28, 0xf},  //GPIOH_15
};

static struct tcon_reg_map_s p2p_unit_map_tbl[] = {
	{0, 0x30d, 0x3c000,   14, 0xf},  //p2p_pol_0
	{1, 0x30d, 0x3c0000,  18, 0xf},  //p2p_pol_1
	{2, 0x30d, 0x3c00000, 22, 0xf},  //p2p_pol_2
	{3, 0x312, 0xf,       0,  0xf},  //p2p_pol_3
};

static struct tcon_reg_map_s cspi_cmd_map_tbl[] = {
	{1, 0x477, 0xff,        0, 0xff},  //byte1
	{2, 0x477, 0x7f800,    11, 0xff},  //byte2
	{3, 0x477, 0x3fc00000, 22, 0xff},  //byte3
	{4, 0x478, 0xff,        0, 0xff},  //byte4
	{5, 0x478, 0x7f800,    11, 0xff},  //byte5
	{6, 0x478, 0x3fc00000, 22, 0xff},  //byte6
	{7, 0x475, 0xff,        0, 0xff},  //byte7
	{8, 0x475, 0x7f800,    11, 0xff},  //byte8
	{9, 0x475, 0x3fc00000, 22, 0xff},  //byte9
};

static struct tcon_reg_map_s ceds_cmd_map_tbl[] = {
	{1, 0x478, 0x3fc7f8ff, 0, 0x3fc7f8ff},  //CTRS
	{2, 0x477, 0x3fc7f8ff, 0, 0x3fc7f8ff},  //CTRE
};

static struct tcon_reg_map_s cmpi_cmd_map_tbl[] = {
	{1, 0x477, 0x3fc7f8ff, 0, 0x3fc7f8ff},  //CMD1
	{2, 0x476, 0x3fc7f8ff, 0, 0x3fc7f8ff},  //CMD2
};

static unsigned int generic_get_val(struct tcon_reg_map_s *cur_map, unsigned int val)
{
	if (cur_map)
		return ((cur_map->val_mask & val) << cur_map->start_bit);
	return 0xffffffff;
}

static struct pdf_plat_info_s pdf_t3x_info = {
	.chip = LCD_CHIP_T3X,

	.gpio_unit_tbl = gpio_unit_map_tbl,
	.gpio_unit_tbl_len = ARRAY_SIZE(gpio_unit_map_tbl),
	.gpio_unit_get_val = generic_get_val,

	.p2p_unit_tbl = p2p_unit_map_tbl,
	.p2p_unit_tbl_len = ARRAY_SIZE(p2p_unit_map_tbl),
	.p2p_unit_get_val = generic_get_val,
};

static struct pdf_plat_info_s pdf_txhd2_info = {
	.chip = LCD_CHIP_TXHD2,

	.gpio_unit_tbl = gpio_unit_map_tbl,
	.gpio_unit_tbl_len = ARRAY_SIZE(gpio_unit_map_tbl),
	.gpio_unit_get_val = generic_get_val,
};

static struct tcon_pdf_s tcon_pdf;
static struct pdf_plat_info_s *cur_plat;

static unsigned int ceds_get_val(struct tcon_reg_map_s *cur_map, unsigned int val)
{
	int i = 0;
	unsigned int ret = 0;

	for (i = 2; i < 26; i++) {
		if (val & (1 << i)) {
			if (i <= 9)
				ret |= (1 << (i - 2));
			else if (i <= 17)
				ret |= (1 << (i + 1));
			else
				ret |= (1 << (i + 4));
		}
	}

	return ret;
}

static unsigned int cmpi_get_val(struct tcon_reg_map_s *cur_map, unsigned int val)
{
	int i = 0;
	unsigned int ret = 0;

	for (i = 0; i < 23; i++) {
		if (val & (1 << i)) {
			if (i <= 7)
				ret |= (1 << i);
			else if (i <= 15)
				ret |= (1 << (i + 3));
			else
				ret |= (1 << (i + 6));
		}
	}

	return ret;
}

static struct tcon_pdf_action_s *alloc_pdf_action(void)
{
	struct tcon_pdf_action_s *pdf_act = NULL;

	pdf_act = kzalloc(sizeof(*pdf_act), GFP_KERNEL);
	if (pdf_act) {
		INIT_LIST_HEAD(&pdf_act->dst_list);
		INIT_LIST_HEAD(&pdf_act->reg_list);
	}

	return pdf_act;
}

static struct tcon_pdf_dst_s *alloc_pdf_dst(struct aml_lcd_drv_s *pdrv,
		struct lcd_tcon_data_part_pdf_dst_s *dst)
{
	struct tcon_pdf_dst_s *pdf_dst = NULL;

	pdf_dst = kzalloc(sizeof(*pdf_dst), GFP_KERNEL);
	if (pdf_dst) {
		pdf_dst->mode = dst->mode;
		pdf_dst->index = dst->index;
		pdf_dst->mask = dst->data_mask;
		pdf_dst->val = dst->data_value;
	}

	return pdf_dst;
}

static void add_pdf_src(struct tcon_pdf_action_s *action,
		struct lcd_tcon_data_part_pdf_src_s *src)
{
	if (!action || !src)
		return;

	action->src.reg = src->reg_addr;
	action->src.mask = src->data_mask;
	action->src.val = src->data_value;
	action->src.mode = src->data_check_mode;
}

static int pdf_check_reg(struct tcon_pdf_reg_s *reg_item, struct list_head *head)
{
	struct tcon_pdf_reg_s *pos = NULL;

	if (!reg_item || !head)
		return -1;

	list_for_each_entry(pos, head, list) {
		if (pos == reg_item)
			return -1;
		if (pos->mode == reg_item->mode &&
				pos->reg == reg_item->reg) {
			pos->mask = pos->mask | reg_item->mask;
			pos->val  = pos->val  | reg_item->val;
			return 0;
		}
	}

	return 1;
}

static int pdf_fill_reg_by_dst(struct tcon_pdf_reg_s *pdf_reg,
		struct tcon_pdf_dst_s *dst, struct pdf_plat_info_s *cur_plat)
{
	unsigned int i = 0;
	struct tcon_reg_map_s *match_reg = NULL;
	struct tcon_reg_map_s *table = NULL;
	unsigned int table_len = 0;
	get_val_func_t get_val_func = NULL;

	if (!pdf_reg || !dst || !cur_plat)
		return -1;

	switch (dst->mode) {
	case PDF_DST_MODE_P2P_CMD:
		table = cur_plat->p2p_cmd_tbl;
		table_len = cur_plat->p2p_cmd_tbl_len;
		get_val_func = cur_plat->p2p_cmd_get_val;
		break;
	case PDF_DST_MODE_P2P_UNIT:
		table = cur_plat->p2p_unit_tbl;
		table_len = cur_plat->p2p_unit_tbl_len;
		get_val_func = cur_plat->p2p_unit_get_val;
		break;
	case PDF_DST_MODE_GPIO_UNIT:
		table = cur_plat->gpio_unit_tbl;
		table_len = cur_plat->gpio_unit_tbl_len;
		get_val_func = cur_plat->gpio_unit_get_val;
		break;
	default:
		break;
	}

	for (i = 0; i < table_len; i++) {
		if (table[i].index == dst->index) {
			match_reg = &table[i];
			break;
		}
	}

	if (match_reg) {
		pdf_reg->mode = PDF_REG_MODE_WR;
		pdf_reg->reg  = match_reg->reg;
		if (get_val_func) {
			pdf_reg->mask = get_val_func(match_reg, dst->mask);
			pdf_reg->val  = get_val_func(match_reg, dst->val);
		} else {
			pdf_reg->mask = dst->mask;
			pdf_reg->val  = dst->val;
		}
	} else {
		pdf_reg->mode = PDF_REG_MODE_WR_GPIO;
		pdf_reg->reg  = dst->index;
		pdf_reg->mask = dst->mask;
		pdf_reg->val  = dst->val;
	}

	return 0;
}

static int pdf_analyse_reg(struct tcon_pdf_s *pdf)
{
	struct tcon_pdf_action_s *action = NULL;
	struct tcon_pdf_dst_s *dst = NULL;
	struct tcon_pdf_reg_s *pdf_reg = NULL;
	unsigned long flags = 0;
	unsigned char success = 0;

	if (!pdf)
		return -1;

	list_for_each_entry(action, &pdf->action_list, list) {
		list_for_each_entry(dst, &action->dst_list, list) {
			success = 0;
			pdf_reg = kzalloc(sizeof(*pdf_reg), GFP_KERNEL);
			if (!pdf_reg)
				continue;
			if (!pdf_fill_reg_by_dst(pdf_reg, dst, cur_plat)) {
				if (pdf_check_reg(pdf_reg, &action->reg_list) > 0)
					success = 1;
			}
			if (success) {
				spin_lock_irqsave(&pdf->vs_lock, flags);
				list_add_tail(&pdf_reg->list, &action->reg_list);
				spin_unlock_irqrestore(&pdf->vs_lock, flags);
			} else {
				kfree(pdf_reg);  //invalid or combined reg
			}
		}
	}

	return 0;
}

static inline char *dst_mode_2_str(enum pdf_dst_mode_e mode)
{
	return ((mode) == PDF_DST_MODE_P2P_CMD ? "P2P CMD" :
		(mode) == PDF_DST_MODE_P2P_UNIT ? "P2P UNIT" :
		(mode) == PDF_DST_MODE_GPIO_UNIT ? "GPIO UNIT" :
		(mode) == PDF_DST_MODE_GPIO ? "GPIO" : "unknown");
}

static inline char *reg_mode_2_str(enum pdf_reg_mode_e mode)
{
	return ((mode) == PDF_REG_MODE_WR ? "WR REG" :
		(mode) == PDF_REG_MODE_WR_GPIO ? "WR GPIO" :
		(mode) == PDF_REG_MODE_RD_CHK_ALL ? "RD CHK ALL" :
		(mode) == PDF_REG_MODE_RD_CHK_ANY ? "RD CHK ANY" : "unknown");
}

static inline void _pdf_get_dft_reg(struct tcon_pdf_s *pdf,
		struct tcon_pdf_reg_s *reg)
{
	unsigned long flags = 0;

	if (!reg)
		return;

	spin_lock_irqsave(&pdf->vs_lock, flags);
	if (reg->mode == PDF_REG_MODE_WR)
		reg->dft_val = lcd_tcon_read(pdf->priv_data, reg->reg);
	else if (reg->mode == PDF_REG_MODE_WR_GPIO)
		reg->dft_val = !reg->val;
	spin_unlock_irqrestore(&pdf->vs_lock, flags);
}

static inline void pdf_get_dft_reg_by_action(struct tcon_pdf_s *pdf,
		struct tcon_pdf_action_s *action)
{
	struct tcon_pdf_reg_s *reg_item = NULL;

	if (!pdf || !action)
		return;

	list_for_each_entry(reg_item, &action->reg_list, list)
		_pdf_get_dft_reg(pdf, reg_item);
}

static void pdf_get_default_reg(struct tcon_pdf_s *pdf)
{
	struct tcon_pdf_action_s *action = NULL;

	if (!pdf)
		return;

	list_for_each_entry(action, &pdf->action_list, list)
		pdf_get_dft_reg_by_action(pdf, action);
}

static int pdf_fill_action(struct tcon_pdf_s *pdf, struct list_head *head,
		struct lcd_tcon_pdf_data_s *pos, struct tcon_pdf_action_s *action)
{
	struct lcd_tcon_pdf_data_s *pos2 = NULL;
	struct tcon_pdf_dst_s *pdf_dst = NULL;
	unsigned short part_id = 0xffff;
	int i = 0;

	if (!pdf || !head || !pos || !action)
		return -1;

	list_for_each_entry(pos2, head, list) {
		switch (pos2->data_type) {
		case LCD_TCON_DATA_PART_TYPE_PDF_ACTION_DST:
			part_id = pos2->data.pdf_dst->part_id;
			for (i = 0; i < pos->data.pdf_action->dst_cnt; i++) {
				if (part_id ==
						pos->data.pdf_action->dst_array[i]) {
					pdf_dst = alloc_pdf_dst(pdf->priv_data,
						pos2->data.pdf_dst);
					if (pdf_dst)
						list_add_tail(&pdf_dst->list, &action->dst_list);
				}
			}
			break;
		case LCD_TCON_DATA_PART_TYPE_PDF_ACTION_SRC:
			part_id = pos2->data.pdf_src->part_id;
			if (pos->data.pdf_action->src_id == part_id)
				add_pdf_src(action, pos2->data.pdf_src);
			break;
		default:
			break;
		}
	}

	return 0;
}

static int pdf_fill_by_data_config(struct list_head *data_list, struct tcon_pdf_s *pdf)
{
	struct lcd_tcon_pdf_data_s *pos = NULL;
	struct tcon_pdf_action_s *pdf_action = NULL;
	struct list_head *pdf_listhead = NULL;
	unsigned long flags = 0;
	int i = 0;

	if (!data_list || !pdf)
		return -1;

	pdf_listhead = data_list;
	if (list_empty(pdf_listhead)) {
		LCDERR("pdf data empty\n");
		return -1;
	}

	list_for_each_entry(pos, pdf_listhead, list) {
		if (pos && pos->data_type ==
				LCD_TCON_DATA_PART_TYPE_PDF_ACTION) {
			pdf_action = alloc_pdf_action();
			if (!pdf_action)
				continue;

			/* search src&dst inside this action */
			if (pdf_fill_action(pdf, pdf_listhead,
					pos, pdf_action) < 0) {
				kfree(pdf_action);
				continue;
			}

			pdf_action->group = i++;
			spin_lock_irqsave(&pdf->vs_lock, flags);
			pdf_action->enable = 1;
			list_add_tail(&pdf_action->list, &pdf->action_list);
			spin_unlock_irqrestore(&pdf->vs_lock, flags);
		}
	}

	if (!pdf_analyse_reg(pdf))
		pdf_get_default_reg(pdf);

	return 0;
}

static int tcon_pdf_is_enable(struct tcon_pdf_s *pdf)
{
	if (!pdf)
		return 0;
	return pdf->enable;
}

static ssize_t tcon_pdf_show_status(struct tcon_pdf_s *pdf, char *buf)
{
	struct tcon_pdf_action_s *action = NULL;
	struct tcon_pdf_dst_s *dst_item = NULL;
	struct tcon_pdf_reg_s *reg_item = NULL;
	int i = 0, j = 0;
	ssize_t size = 0;

	if (!pdf || !buf)
		return size;

	size += sprintf(buf + size, "PDF Enable = %d\n", pdf->enable);
	if (pdf->enable) {
		size += sprintf(buf + size, "PDF Detected = %d\n", pdf->detected);
		size += sprintf(buf + size, "PDF Action List:\n");
		list_for_each_entry(action, &pdf->action_list, list) {
			size += sprintf(buf + size,
				"  ACTION[%d]  enable=%d  detected=%d\n",
				action->group, action->enable, action->detected);
			size += sprintf(buf + size,
				"    SRC:  mode=%s, reg=%#x, mask=%#x, val=%#x\n",
				reg_mode_2_str(action->src.mode), action->src.reg,
				action->src.mask, action->src.val);
			j = 0;
			list_for_each_entry(dst_item, &action->dst_list, list) {
				size += sprintf(buf + size,
					"    DST[%d]  mode=%s, index=%#x",
					j, dst_mode_2_str(dst_item->mode), dst_item->index);
				size += sprintf(buf + size,
					", mask=%#x, val=%#x\n",
					dst_item->mask, dst_item->val);
				j++;
			}
			j = 0;
			list_for_each_entry(reg_item, &action->reg_list, list) {
				size += sprintf(buf + size,
					"    REG OPR[%d]  mode=%s, reg=%#x, mask=%#x",
					j, reg_mode_2_str(reg_item->mode),
					reg_item->reg, reg_item->mask);
				size += sprintf(buf + size,
					", val=%#x, dft_val=%#x\n",
					reg_item->val, reg_item->dft_val);
				j++;
			}
			i++;
		}
	}

	return size;
}

static int set_pdf_by_action_reg(struct tcon_pdf_s *pdf,
		struct tcon_pdf_reg_s *reg_item, unsigned char is_set_dft)
{
	if (!pdf || !reg_item)
		return -1;

	if (reg_item->mode == PDF_REG_MODE_WR) {
		lcd_tcon_update_bits(pdf->priv_data,
			reg_item->reg, reg_item->mask,
			is_set_dft ? reg_item->dft_val : reg_item->val);
	} else if (reg_item->mode == PDF_REG_MODE_WR_GPIO) {
		lcd_cpu_gpio_set(pdf->priv_data, reg_item->reg,
			is_set_dft ? !!reg_item->dft_val : !!reg_item->val);
	}

	return 0;
}

static int tcon_pdf_vs_handler(struct tcon_pdf_s *pdf)
{
	struct tcon_pdf_action_s *action = NULL;
	struct tcon_pdf_reg_s *reg_item = NULL;
	struct tcon_pdf_reg_s *src = NULL;
	unsigned int sreg_val = 0;
	unsigned char detected = 0;
	unsigned char detect_cnt = 0;
	unsigned long flags = 0;

	if (!tcon_pdf_is_enable(pdf))
		return -1;

	spin_lock_irqsave(&pdf->vs_lock, flags);
	list_for_each_entry(action, &pdf->action_list, list) {
		if (!action->enable)
			continue;
		src = &action->src;
		detected = 0;
		sreg_val = lcd_tcon_read(pdf->priv_data, src->reg);
		if (src->mode == PDF_REG_MODE_RD_CHK_ALL) {
			if ((sreg_val & src->mask) == src->val)
				detected = 1;
		} else {
			if (sreg_val & src->mask)
				detected = 1;
		}

		if (detected && !action->detected) {
			/* first detected */
			list_for_each_entry(reg_item, &action->reg_list, list)
				set_pdf_by_action_reg(pdf, reg_item, 0);
			action->detected = detected;
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
				LCDPR("group %d detected\n", action->group);
		} else if (!detected && action->detected) {
			/* leave detected */
			list_for_each_entry(reg_item, &action->reg_list, list)
				set_pdf_by_action_reg(pdf, reg_item, 1);
			action->detected = detected;
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
				LCDPR("group %d undetect\n", action->group);
		}

		if (detected)
			detect_cnt++;
	}
	spin_unlock_irqrestore(&pdf->vs_lock, flags);

	pdf->detected = !!detect_cnt;

	return 0;
}

static int tcon_pdf_group_enable(struct tcon_pdf_s *pdf,
		unsigned char group, int enable)
{
	struct tcon_pdf_action_s *action = NULL;
	unsigned long flags = 0;
	int ret = -1;

	if (!pdf)
		goto __group_enable_exit;

	if (group == 0xff) {
		pdf->enable = !!enable;
		ret = 0;
	} else {
		list_for_each_entry(action, &pdf->action_list, list) {
			if (action->group == group) {
				spin_lock_irqsave(&pdf->vs_lock, flags);
				action->enable = !!enable;
				spin_unlock_irqrestore(&pdf->vs_lock, flags);
				ret = 0;
				goto __group_enable_exit;
			}
		}
	}

__group_enable_exit:
	return ret;
}

static int tcon_pdf_new_group(struct tcon_pdf_s *pdf, unsigned char group)
{
	struct tcon_pdf_action_s *action = NULL;
	struct tcon_pdf_action_s *item = NULL;
	unsigned long flags = 0;

	if (!pdf)
		goto __new_group_fail;

	action = alloc_pdf_action();
	if (!action)
		goto __new_group_fail;

	list_for_each_entry(item, &pdf->action_list, list) {
		if (item->group == group) {
			LCDERR("Group %d exist!\n", group);
			goto __new_group_fail;
		}
	}

	action->group = group;

	spin_lock_irqsave(&pdf->vs_lock, flags);
	list_add_tail(&action->list, &pdf->action_list);
	spin_unlock_irqrestore(&pdf->vs_lock, flags);

	return action->group;

__new_group_fail:
	kfree(action);

	return -1;
}

static int tcon_pdf_group_add_src(struct tcon_pdf_s *pdf,
		unsigned char group, struct tcon_pdf_reg_s *src)
{
	struct tcon_pdf_action_s *action = NULL;
	unsigned long flags = 0;

	if (!pdf || !src)
		return -1;

	list_for_each_entry(action, &pdf->action_list, list) {
		if (action->group == group) {
			spin_lock_irqsave(&pdf->vs_lock, flags);
			action->src.mode = src->mode;
			action->src.reg  = src->reg;
			action->src.mask = src->mask;
			action->src.val  = src->val;
			spin_unlock_irqrestore(&pdf->vs_lock, flags);

			return 0;
		}
	}

	return -1;
}

static int tcon_pdf_group_add_dst(struct tcon_pdf_s *pdf,
		unsigned char group, struct tcon_pdf_dst_s *dst)
{
	struct tcon_pdf_action_s *action = NULL;
	struct tcon_pdf_dst_s *dst_item = NULL;
	struct tcon_pdf_reg_s *reg_item = NULL;
	unsigned char found = 0, success = 0;
	unsigned long flags = 0;
	int ret = -1;

	if (!pdf || !dst)
		return -1;

	list_for_each_entry(action, &pdf->action_list, list) {
		if (action->group == group) {
			found = 1;
			break;
		}
	}

	if (!found)
		return -1;

	dst_item = kzalloc(sizeof(*dst_item), GFP_KERNEL);
	if (!dst_item)
		return -1;

	dst_item->mode  = dst->mode;
	dst_item->index = dst->index;
	dst_item->mask  = dst->mask;
	dst_item->val   = dst->val;

	reg_item = kzalloc(sizeof(*reg_item), GFP_KERNEL);
	if (!pdf_fill_reg_by_dst(reg_item, dst_item, cur_plat)) {
		ret = pdf_check_reg(reg_item, &action->reg_list);
		if (ret < 0)
			goto __group_add_dst_fail;

		if (ret > 0) {
			_pdf_get_dft_reg(pdf, reg_item);
			success = 1;
		}
	}

	if (success) {
		spin_lock_irqsave(&pdf->vs_lock, flags);
		list_add_tail(&reg_item->list, &action->reg_list);
		spin_unlock_irqrestore(&pdf->vs_lock, flags);
	} else {
		kfree(reg_item);
	}

	spin_lock_irqsave(&pdf->vs_lock, flags);
	list_add_tail(&dst_item->list, &action->dst_list);
	spin_unlock_irqrestore(&pdf->vs_lock, flags);

	return 0;

__group_add_dst_fail:
	kfree(dst_item);
	kfree(reg_item);

	return -1;
}

static int tcon_pdf_del_group(struct tcon_pdf_s *pdf, unsigned char group)
{
	struct tcon_pdf_action_s *action = NULL;
	struct tcon_pdf_dst_s *dst_item = NULL, *dst_next_item = NULL;
	struct tcon_pdf_reg_s *reg_item = NULL, *reg_next_item = NULL;
	unsigned long flags = 0;

	if (!pdf)
		return -1;

	list_for_each_entry(action, &pdf->action_list, list) {
		if (action->group == group) {
			spin_lock_irqsave(&pdf->vs_lock, flags);
			list_for_each_entry_safe(dst_item, dst_next_item,
					&action->dst_list, list) {
				list_del(&dst_item->list);
				kfree(dst_item);
			}

			list_for_each_entry_safe(reg_item, reg_next_item,
					&action->reg_list, list) {
				/* recover default val */
				if (action->detected)
					set_pdf_by_action_reg(pdf, reg_item, 1);
				list_del(&reg_item->list);
				kfree(reg_item);
			}
			list_del(&action->list);
			kfree(action);
			spin_unlock_irqrestore(&pdf->vs_lock, flags);
			break;
		}
	}

	return 0;
}

static void tcon_pdf_remove_action_list(struct list_head *action_list)
{
	struct tcon_pdf_s *pdf = lcd_tcon_get_pdf();
	struct tcon_pdf_action_s *action = NULL, *action_next = NULL;
	struct tcon_pdf_dst_s *dst_item = NULL, *dst_next = NULL;
	struct tcon_pdf_reg_s *reg_item = NULL, *reg_next = NULL;
	unsigned long flags = 0;

	if (!action_list || !list_empty(action_list))
		return;

	list_for_each_entry_safe(action, action_next,
				action_list, list) {
		list_for_each_entry_safe(dst_item, dst_next,
				&action->dst_list, list) {
			list_del(&dst_item->list);
			kfree(dst_item);
		}

		spin_lock_irqsave(&pdf->vs_lock, flags);
		list_for_each_entry_safe(reg_item, reg_next,
				&action->reg_list, list) {
			/* recover default val */
			if (action->detected)
				set_pdf_by_action_reg(pdf, reg_item, 1);
			list_del(&reg_item->list);
			kfree(reg_item);
		}
		list_del(&action->list);
		kfree(action);
		spin_unlock_irqrestore(&pdf->vs_lock, flags);
	}
}

int lcd_tcon_pdf_init(struct aml_lcd_drv_s *pdrv)
{
	unsigned int p2p_type;

	switch (pdrv->data->chip_type) {
	case LCD_CHIP_TXHD2:
		cur_plat = &pdf_txhd2_info;
		break;
	case LCD_CHIP_T3X:
		cur_plat = &pdf_t3x_info;
		break;
	default:
		LCDERR("%s: not support pdf\n", __func__);
		goto __pdf_init_fail;
	}

	if (cur_plat && pdrv->config.basic.lcd_type == LCD_P2P) {
		// p2p needs to judge protocols and assign p2p cmd table
		p2p_type = pdrv->config.control.p2p_cfg.p2p_type & 0x1f;
		switch (p2p_type) {
		case P2P_ISP:
		case P2P_CSPI:
			cur_plat->p2p_cmd_tbl = cspi_cmd_map_tbl;
			cur_plat->p2p_cmd_tbl_len = ARRAY_SIZE(cspi_cmd_map_tbl);
			cur_plat->p2p_cmd_get_val = generic_get_val;
			break;
		case P2P_CEDS:
			cur_plat->p2p_cmd_tbl = ceds_cmd_map_tbl;
			cur_plat->p2p_cmd_tbl_len = ARRAY_SIZE(ceds_cmd_map_tbl);
			cur_plat->p2p_cmd_get_val = ceds_get_val;
			break;
		case P2P_CMPI:
			cur_plat->p2p_cmd_tbl = cmpi_cmd_map_tbl;
			cur_plat->p2p_cmd_tbl_len = ARRAY_SIZE(cmpi_cmd_map_tbl);
			cur_plat->p2p_cmd_get_val = cmpi_get_val;
			break;
		default:
			LCDERR("%s: pdf not support protocol:%d\n", __func__,
				p2p_type);
			goto __pdf_init_fail;
		}
	}

	INIT_LIST_HEAD(&tcon_pdf.action_list);
	spin_lock_init(&tcon_pdf.vs_lock);

	tcon_pdf.priv_data = pdrv;
	tcon_pdf.detected = 0;
	tcon_pdf.is_enable     = tcon_pdf_is_enable;
	tcon_pdf.show_status   = tcon_pdf_show_status;
	tcon_pdf.vs_handler    = tcon_pdf_vs_handler;
	tcon_pdf.group_enable  = tcon_pdf_group_enable;
	tcon_pdf.new_group     = tcon_pdf_new_group;
	tcon_pdf.del_group     = tcon_pdf_del_group;
	tcon_pdf.group_add_src = tcon_pdf_group_add_src;
	tcon_pdf.group_add_dst = tcon_pdf_group_add_dst;

	LCDPR("PDF is supported\n");

	return 0;

__pdf_init_fail:
	return -1;
}

int lcd_tcon_pdf_get_config(struct list_head *data_list)
{
	struct tcon_pdf_s *pdf = lcd_tcon_get_pdf();

	if (!data_list || list_empty(data_list))
		return -1;

	if (!pdf->enable) {
		/* clean exist list */
		if (!list_empty(&pdf->action_list)) {
			tcon_pdf_remove_action_list(&pdf->action_list);
			INIT_LIST_HEAD(&pdf->action_list);
		}

		if (pdf_fill_by_data_config(data_list, pdf) < 0) {
			LCDERR("PDF get config fail\n");
			return -1;
		}
		pdf->enable = 1;
		LCDPR("PDF Data ready\n");
	}

	return 0;
}

struct tcon_pdf_s *lcd_tcon_get_pdf(void)
{
	return &tcon_pdf;
}

int lcd_tcon_pdf_remove(struct aml_lcd_drv_s *pdrv)
{
	struct tcon_pdf_s *pdf = lcd_tcon_get_pdf();
	unsigned long flags = 0;

	if (!pdrv || !pdf)
		return 0;

	spin_lock_irqsave(&pdf->vs_lock, flags);
	tcon_pdf_remove_action_list(&pdf->action_list);
	pdf->enable = 0;
	pdf->detected = 0;
	spin_unlock_irqrestore(&pdf->vs_lock, flags);

	return 0;
}

