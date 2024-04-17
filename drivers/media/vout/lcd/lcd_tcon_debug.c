// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/compat.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/amlogic/media/vout/lcd/aml_bl.h>
#include <linux/amlogic/media/vout/lcd/lcd_vout.h>
#include <linux/amlogic/media/vout/lcd/lcd_tcon_data.h>
#include <linux/amlogic/media/vout/lcd/lcd_notify.h>
#include <linux/amlogic/media/vout/lcd/lcd_unifykey.h>
#include <linux/amlogic/media/vout/lcd/lcd_tcon_fw.h>
#include "lcd_reg.h"
#include "lcd_common.h"
#include "lcd_tcon.h"
#include "lcd_tcon_pdf.h"

//1: unlocked, 0: locked, negative: locked, possible waiters
struct mutex lcd_tcon_dbg_mutex;

/*tcon adb port use */
#define LCD_ADB_TCON_REG_RW_MODE_NULL              0
#define LCD_ADB_TCON_REG_RW_MODE_RN                1
#define LCD_ADB_TCON_REG_RW_MODE_WM                2
#define LCD_ADB_TCON_REG_RW_MODE_WN                3
#define LCD_ADB_TCON_REG_RW_MODE_WS                4
#define LCD_ADB_TCON_REG_RW_MODE_ERR               5

#define ADB_TCON_REG_8_bit                         0
#define ADB_TCON_REG_32_bit                        1

struct lcd_tcon_adb_reg_s {
	unsigned int rw_mode;
	unsigned int bit_width;
	unsigned int addr;
	unsigned int len;
};

/*for tconless reg adb use*/
static struct lcd_tcon_adb_reg_s adb_reg = {
	.rw_mode = LCD_ADB_TCON_REG_RW_MODE_NULL,
	.bit_width = ADB_TCON_REG_8_bit,
	.addr = 0,
	.len = 0,
};

static void lcd_tcon_multi_lut_print(void)
{
	struct tcon_mem_map_table_s *mm_table = get_lcd_tcon_mm_table();
	struct tcon_data_multi_s *data_multi;
	struct tcon_data_list_s *data_list;
	int i, j;

	if (!mm_table->data_multi || mm_table->data_multi_cnt == 0) {
		LCDERR("tcon data_multi is null\n");
		return;
	}

	LCDPR("tcon multi_update: %d\n", mm_table->multi_lut_update);
	for (i = 0; i < mm_table->data_multi_cnt; i++) {
		data_multi = &mm_table->data_multi[i];
		LCDPR("data_multi[%d]:\n"
			"  type:        0x%x\n"
			"  list_cnt:    %d\n"
			"  bypass_flag: %d\n",
			i, data_multi->block_type, data_multi->list_cnt,
			data_multi->bypass_flag);
		if (data_multi->list_cur) {
			data_list = data_multi->list_cur;
			pr_info("data_multi[%d] current:\n"
				"  sel_id:        %d\n"
				"  sel_name:      %s\n"
				"  range:         %d, %d\n"
				"  ctrl_data_cnt: %d\n",
				i, data_list->id,
				data_list->block_name,
				data_list->min,
				data_list->max,
				data_list->ctrl_data_cnt);
			if (data_list->ctrl_data_cnt) {
				if (data_list->ctrl_data) {
					pr_info("  ctrl_data:\n");
					for (j = 0; j < data_list->ctrl_data_cnt; j++) {
						pr_info("    [%d]: %d\n",
							j, data_list->ctrl_data[j]);
					}
				} else {
					pr_info("  ctrl_data is NULL\n");
				}
			}
		} else {
			pr_info("data_multi[%d] current: NULL\n", i);
		}
		pr_info("data_multi[%d] list:\n", i);
		data_list = data_multi->list_header;
		while (data_list) {
			pr_info("  block[%d]: %s, range: %d,%d, ctrl_data_cnt:%d, vaddr=0x%px\n",
				data_list->id,
				data_list->block_name,
				data_list->min,
				data_list->max,
				data_list->ctrl_data_cnt,
				data_list->block_vaddr);
			data_list = data_list->next;
		}
		pr_info("\n");
	}
}

int lcd_tcon_axi_mem_print(struct tcon_rmem_s *tcon_rmem, char *buf, int offset)
{
	int len = 0, n, i;

	if (!tcon_rmem || !tcon_rmem->axi_rmem)
		return 0;

	if (tcon_rmem->secure_axi_rmem.sec_protect) {
		n = lcd_debug_info_len(len + offset);
		len += snprintf((buf + len), n, "axi secure mem paddr: 0x%lx, size:0x%x\n",
			(unsigned long)tcon_rmem->secure_axi_rmem.mem_paddr,
			tcon_rmem->secure_axi_rmem.mem_size);
		}
	for (i = 0; i < tcon_rmem->axi_bank; i++) {
		n = lcd_debug_info_len(len + offset);
		len += snprintf((buf + len), n,
			"axi_mem[%d] paddr: 0x%lx\n"
			"axi_mem[%d] size:  0x%x\n",
			i, (unsigned long)tcon_rmem->axi_rmem[i].mem_paddr,
			i, tcon_rmem->axi_rmem[i].mem_size);
	}

	return len;
}

static int lcd_tcon_mm_table_v0_print(struct tcon_mem_map_table_s *mm_table,
		struct tcon_rmem_s *tcon_rmem, char *buf, int offset)
{
	int len = 0, n;

	n = lcd_debug_info_len(len + offset);
	len += snprintf((buf + len), n,
		"vac_mem\n"
		"  paddr: 0x%lx\n"
		"  vaddr: 0x%p\n"
		"  size:  0x%x\n"
		"demura_set_mem\n"
		"  paddr: 0x%lx\n"
		"  vaddr: 0x%p\n"
		"  size:  0x%x\n"
		"demura_lut_mem\n"
		"  paddr: 0x%lx\n"
		"  vaddr: 0x%p\n"
		"  size:  0x%x\n"
		"acc_lut_mem\n"
		"  paddr: 0x%lx\n"
		"  vaddr: 0x%p\n"
		"  size:  0x%x\n\n",
		(unsigned long)tcon_rmem->vac_rmem.mem_paddr,
		tcon_rmem->vac_rmem.mem_vaddr,
		tcon_rmem->vac_rmem.mem_size,
		(unsigned long)tcon_rmem->demura_set_rmem.mem_paddr,
		tcon_rmem->demura_set_rmem.mem_vaddr,
		tcon_rmem->demura_set_rmem.mem_size,
		(unsigned long)tcon_rmem->demura_lut_rmem.mem_paddr,
		tcon_rmem->demura_lut_rmem.mem_vaddr,
		tcon_rmem->demura_lut_rmem.mem_size,
		(unsigned long)tcon_rmem->acc_lut_rmem.mem_paddr,
		tcon_rmem->acc_lut_rmem.mem_vaddr,
		tcon_rmem->acc_lut_rmem.mem_size);

	return len;
}

static int lcd_tcon_mm_table_v1_print(struct tcon_mem_map_table_s *mm_table,
		struct tcon_rmem_s *tcon_rmem, char *buf, int offset)
{
	unsigned char *mem_vaddr;
	struct lcd_tcon_data_block_header_s *bin_header;
	struct tcon_data_list_s *cur_list;
	int len = 0, n, i;

	if (mm_table->data_mem_vaddr && mm_table->data_size) {
		n = lcd_debug_info_len(len + offset);
		len += snprintf((buf + len), n,
			"data_mem block_cnt: %d\n"
			"data_mem list:\n",
			mm_table->block_cnt);
		for (i = 0; i < mm_table->block_cnt; i++) {
			mem_vaddr = mm_table->data_mem_vaddr[i];
			n = lcd_debug_info_len(len + offset);
			if (mem_vaddr) {
				bin_header = (struct lcd_tcon_data_block_header_s *)mem_vaddr;
				len += snprintf((buf + len), n,
					"  [%d]: vaddr: 0x%px, size: 0x%x, %s(0x%x)\n",
					i, mem_vaddr,
					mm_table->data_size[i],
					bin_header->name, bin_header->block_type);
			} else {
				len += snprintf((buf + len), n,
					"  [%d]: vaddr: NULL, size:  0\n", i);
			}
		}
	}

	if (mm_table->data_multi_cnt > 0 && mm_table->data_multi) {
		n = lcd_debug_info_len(len + offset);
		len += snprintf((buf + len), n,
			"tcon multi_update: %d\n",
			mm_table->multi_lut_update);
		for (i = 0; i < mm_table->data_multi_cnt; i++) {
			n = lcd_debug_info_len(len + offset);
			len += snprintf((buf + len), n,
				"data_multi[%d] current (bypass:%d):\n",
				i, mm_table->data_multi[i].bypass_flag);
			if (!mm_table->data_multi[i].list_cur) {
				n = lcd_debug_info_len(len + offset);
				len += snprintf((buf + len), n, "  NULL\n");
			} else {
				cur_list = mm_table->data_multi[i].list_cur;
				n = lcd_debug_info_len(len + offset);
				len += snprintf((buf + len), n,
					"  type:      0x%x\n"
					"  list_cnt:  %d\n"
					"  sel_id:    %d\n"
					"  sel_name:  %s\n"
					"  sel_range: %d,%d\n",
					mm_table->data_multi[i].block_type,
					mm_table->data_multi[i].list_cnt,
					cur_list->id, cur_list->block_name,
					cur_list->min, cur_list->max);
			}
		}
	}

	return len;
}

int lcd_tcon_info_print(struct aml_lcd_drv_s *pdrv, char *buf, int offset)
{
	struct tcon_mem_map_table_s *mm_table = get_lcd_tcon_mm_table();
	struct tcon_rmem_s *tcon_rmem = get_lcd_tcon_rmem();
	struct lcd_tcon_local_cfg_s *local_cfg = get_lcd_tcon_local_cfg();
	struct lcd_tcon_config_s *tcon_conf = get_lcd_tcon_config();
	unsigned int size, file_size, cnt, m, sec_protect, sec_handle, *data;
	unsigned char *mem_vaddr;
	char *str;
	int len = 0, n, i, ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return len;
	if (!mm_table || !tcon_rmem || !local_cfg || !tcon_conf)
		return len;

	lcd_tcon_init_setting_check(pdrv, &pdrv->config.timing.act_timing,
			local_cfg->cur_core_reg_table);

	n = lcd_debug_info_len(len + offset);
	len += snprintf((buf + len), n,
			"\ntcon info:\n"
			"core_reg_width:  %d\n"
			"reg_table_len:   %d\n"
			"tcon_bin_ver:    %s\n"
			"tcon_rmem_flag:  %d\n"
			"rsv_mem paddr:   0x%lx\n"
			"rsv_mem size:    0x%x\n",
			tcon_conf->core_reg_width,
			tcon_conf->reg_table_len,
			local_cfg->bin_ver,
			tcon_rmem->flag,
			(unsigned long)tcon_rmem->rsv_mem_paddr,
			tcon_rmem->rsv_mem_size);
	if (mm_table->bin_path_valid) {
		n = lcd_debug_info_len(len + offset);
		len += snprintf((buf + len), n,
			"bin_path_mem\n"
			"  paddr: 0x%lx\n"
			"  vaddr: 0x%px\n"
			"  size:  0x%x\n",
			(unsigned long)tcon_rmem->bin_path_rmem.mem_paddr,
			tcon_rmem->bin_path_rmem.mem_vaddr,
			tcon_rmem->bin_path_rmem.mem_size);
	}
	n = lcd_debug_info_len(len + offset);
	len += snprintf((buf + len), n,	"\n");

	if (tcon_rmem->flag) {
		len += lcd_tcon_axi_mem_print(tcon_rmem, (buf + len), (len + offset));
		if (mm_table->version == 0) {
			len += lcd_tcon_mm_table_v0_print(mm_table, tcon_rmem,
					(buf + len), (len + offset));
		} else if (mm_table->version < 0xff) {
			len += lcd_tcon_mm_table_v1_print(mm_table, tcon_rmem,
					(buf + len), (len + offset));
		}

		if (tcon_rmem->bin_path_rmem.mem_vaddr) {
			mem_vaddr = tcon_rmem->bin_path_rmem.mem_vaddr;
			size = *(unsigned int *)&mem_vaddr[4];
			cnt = *(unsigned int *)&mem_vaddr[16];
			if (size < (32 + 256 * cnt))
				return len;
			if (cnt > 32)
				return len;
			n = lcd_debug_info_len(len + offset);
			len += snprintf((buf + len), n,
				"\nbin_path rsv_mem\n"
				"bin cnt:        %d\n"
				"data_complete:  %d\n"
				"bin_path_valid: %d\n",
				cnt, mm_table->data_complete, mm_table->bin_path_valid);
			for (i = 0; i < cnt; i++) {
				m = 32 + 256 * i;
				file_size = *(unsigned int *)&mem_vaddr[m];
				str = (char *)&mem_vaddr[m + 4];
				n = lcd_debug_info_len(len + offset);
				len += snprintf((buf + len), n,
					"bin[%d]: size: 0x%x, %s\n", i, file_size, str);
			}
		}

		if (tcon_rmem->secure_cfg_rmem.mem_vaddr) {
			data = (unsigned int *)tcon_rmem->secure_cfg_rmem.mem_vaddr;
			cnt = tcon_rmem->axi_bank;
			n = lcd_debug_info_len(len + offset);
			len += snprintf((buf + len), n,
				"\nsecure_cfg rsv_mem:\n"
				"  paddr: 0x%lx\n"
				"  vaddr: 0x%px\n"
				"  size:  0x%x\n",
				(unsigned long)tcon_rmem->secure_cfg_rmem.mem_paddr,
				tcon_rmem->secure_cfg_rmem.mem_vaddr,
				tcon_rmem->secure_cfg_rmem.mem_size);
			for (i = 0; i < cnt; i++) {
				m = 2 * i;
				sec_protect = *(data + m);
				sec_handle = *(data + m + 1);
				n = lcd_debug_info_len(len + offset);
				len += snprintf((buf + len), n,
					"  [%d]: protect: %d, handle: %d\n",
					i,  sec_protect, sec_handle);
			}
		}
	}

	n = lcd_debug_info_len(len + offset);
	len += snprintf((buf + len), n,
		"\nlut_valid:\n"
		"acc:    %d\n"
		"od:     %d\n"
		"lod:    %d\n"
		"vac:    %d\n"
		"demura: %d\n"
		"dither: %d\n",
		!!(mm_table->lut_valid_flag & LCD_TCON_DATA_VALID_ACC),
		!!(mm_table->lut_valid_flag & LCD_TCON_DATA_VALID_OD),
		!!(mm_table->lut_valid_flag & LCD_TCON_DATA_VALID_LOD),
		!!(mm_table->lut_valid_flag & LCD_TCON_DATA_VALID_VAC),
		!!(mm_table->lut_valid_flag & LCD_TCON_DATA_VALID_DEMURA),
		!!(mm_table->lut_valid_flag & LCD_TCON_DATA_VALID_DITHER));

	return len;
}

ssize_t lcd_tcon_debug_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct aml_lcd_drv_s *pdrv = dev_get_drvdata(dev);

	return lcd_tcon_info_print(pdrv, buf, 0);
}

ssize_t lcd_tcon_debug_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
#define __MAX_PARAM 520
	struct aml_lcd_drv_s *pdrv = dev_get_drvdata(dev);
	char *buf_orig;
	char **parm = NULL;
	unsigned int temp = 0, val, back_val, i, n, size = 0;
	struct tcon_mem_map_table_s *mm_table = get_lcd_tcon_mm_table();
	unsigned char data;
	unsigned char *table = NULL;
	int ret = -1;

	if (!buf)
		return count;
	buf_orig = kstrdup(buf, GFP_KERNEL);
	if (!buf_orig)
		return count;

	parm = kcalloc(__MAX_PARAM, sizeof(char *), GFP_KERNEL);
	if (!parm) {
		kfree(buf_orig);
		return count;
	}

	lcd_debug_parse_param(buf_orig, parm, __MAX_PARAM);

	if (strcmp(parm[0], "reg") == 0) {
		if (!parm[1]) {
			lcd_tcon_reg_readback_print(pdrv);
			goto lcd_tcon_debug_store_end;
		}
		if (strcmp(parm[1], "dump") == 0) {
			lcd_tcon_reg_readback_print(pdrv);
			goto lcd_tcon_debug_store_end;
		} else if (strcmp(parm[1], "rb") == 0) {
			if (!parm[2])
				goto lcd_tcon_debug_store_err;
			ret = kstrtouint(parm[2], 16, &temp);
			if (ret)
				goto lcd_tcon_debug_store_err;
			pr_info("read tcon byte [0x%04x] = 0x%02x\n",
				temp, lcd_tcon_read_byte(pdrv, temp));
		} else if (strcmp(parm[1], "wb") == 0) {
			if (!parm[3])
				goto lcd_tcon_debug_store_err;
			ret = kstrtouint(parm[2], 16, &temp);
			if (ret)
				goto lcd_tcon_debug_store_err;
			ret = kstrtouint(parm[3], 16, &val);
			if (ret)
				goto lcd_tcon_debug_store_err;
			data = (unsigned char)val;
			lcd_tcon_write_byte(pdrv, temp, data);
			pr_info("write tcon byte [0x%04x] = 0x%02x\n",
				temp, data);
		} else if (strcmp(parm[1], "wlb") == 0) { /*long write byte*/
			if (!parm[3])
				goto lcd_tcon_debug_store_err;
			ret = kstrtouint(parm[2], 16, &temp);
			if (ret)
				goto lcd_tcon_debug_store_err;
			ret = kstrtouint(parm[3], 16, &size);
			if (ret)
				goto lcd_tcon_debug_store_err;

			if (!parm[3 + size]) {
				pr_info("size and data is not match\n");
				goto lcd_tcon_debug_store_err;
			}

			for (i = 0; i < size; i++) {
				ret = kstrtouint(parm[4 + i], 16, &val);
				if (ret)
					goto lcd_tcon_debug_store_err;
				data = (unsigned char)val;
				lcd_tcon_write_byte(pdrv, (temp + i), data);
				pr_info("write tcon byte [0x%04x] = 0x%02x\n",
					(temp + i), data);
			}
		} else if (strcmp(parm[1], "db") == 0) {
			if (!parm[3])
				goto lcd_tcon_debug_store_err;
			ret = kstrtouint(parm[2], 16, &temp);
			if (ret)
				goto lcd_tcon_debug_store_err;
			ret = kstrtouint(parm[3], 10, &size);
			if (ret)
				goto lcd_tcon_debug_store_err;
			pr_info("dump tcon byte:\n");
			for (i = 0; i < size; i++) {
				pr_info("  [0x%04x] = 0x%02x\n",
					(temp + i),
					lcd_tcon_read_byte(pdrv, temp + i));
			}
		} else if (strcmp(parm[1], "r") == 0) {
			if (!parm[2])
				goto lcd_tcon_debug_store_err;
			ret = kstrtouint(parm[2], 16, &temp);
			if (ret)
				goto lcd_tcon_debug_store_err;
			pr_info("read tcon [0x%04x] = 0x%08x\n",
				temp, lcd_tcon_read(pdrv, temp));
		} else if (strcmp(parm[1], "w") == 0) {
			if (!parm[3])
				goto lcd_tcon_debug_store_err;
			ret = kstrtouint(parm[2], 16, &temp);
			if (ret)
				goto lcd_tcon_debug_store_err;
			ret = kstrtouint(parm[3], 16, &val);
			if (ret)
				goto lcd_tcon_debug_store_err;
			lcd_tcon_write(pdrv, temp, val);
			pr_info("write tcon [0x%04x] = 0x%08x\n",
				temp, val);
		} else if (strcmp(parm[1], "wl") == 0) { /*long write*/
			if (!parm[3])
				goto lcd_tcon_debug_store_err;
			ret = kstrtouint(parm[2], 16, &temp);
			if (ret)
				goto lcd_tcon_debug_store_err;
			ret = kstrtouint(parm[3], 16, &size);
			if (ret)
				goto lcd_tcon_debug_store_err;

			if (!parm[3 + size]) {
				pr_info("size and data is not match\n");
				goto lcd_tcon_debug_store_err;
			}

			for (i = 0; i < size; i++) {
				ret = kstrtouint(parm[4 + i], 16, &val);
				if (ret)
					goto lcd_tcon_debug_store_err;
				lcd_tcon_write(pdrv, temp + i, val);
				pr_info("write tcon [0x%04x] = 0x%08x\n",
					(temp + i), val);
			}
		} else if (strcmp(parm[1], "d") == 0) {
			if (!parm[3])
				goto lcd_tcon_debug_store_err;
			ret = kstrtouint(parm[2], 16, &temp);
			if (ret)
				goto lcd_tcon_debug_store_err;
			ret = kstrtouint(parm[3], 10, &size);
			if (ret)
				goto lcd_tcon_debug_store_err;
			pr_info("dump tcon:\n");
			for (i = 0; i < size; i++) {
				pr_info("  [0x%04x] = 0x%08x\n",
					(temp + i),
					lcd_tcon_read(pdrv, temp + i));
			}
		}
	} else if (strcmp(parm[0], "table") == 0) {
		if (!mm_table)
			goto lcd_tcon_debug_store_end;
		table = mm_table->core_reg_table;
		size = mm_table->core_reg_table_size;
		if (!table)
			goto lcd_tcon_debug_store_end;
		if (size == 0)
			goto lcd_tcon_debug_store_end;
		if (!parm[1]) {
			lcd_tcon_reg_table_print();
			goto lcd_tcon_debug_store_end;
		}
		if (strcmp(parm[1], "dump") == 0) {
			lcd_tcon_reg_table_print();
			goto lcd_tcon_debug_store_end;
		} else if (strcmp(parm[1], "r") == 0) {
			if (!parm[2])
				goto lcd_tcon_debug_store_err;
			ret = kstrtouint(parm[2], 16, &temp);
			if (ret)
				goto lcd_tcon_debug_store_err;
			val = lcd_tcon_table_read(temp);
			pr_info("read table 0x%x = 0x%x\n", temp, val);
		} else if (strcmp(parm[1], "w") == 0) {
			if (!parm[3])
				goto lcd_tcon_debug_store_err;
			ret = kstrtouint(parm[2], 16, &temp);
			if (ret)
				goto lcd_tcon_debug_store_err;
			ret = kstrtouint(parm[3], 16, &val);
			if (ret)
				goto lcd_tcon_debug_store_err;
			back_val = lcd_tcon_table_write(temp, val);
			pr_info("write table 0x%x = 0x%x, readback 0x%x\n",
				temp, val, back_val);
		} else if (strcmp(parm[1], "d") == 0) {
			if (!parm[3])
				goto lcd_tcon_debug_store_err;
			ret = kstrtouint(parm[2], 16, &temp);
			if (ret)
				goto lcd_tcon_debug_store_err;
			ret = kstrtouint(parm[3], 16, &n);
			if (ret)
				goto lcd_tcon_debug_store_err;
			pr_info("dump tcon table:\n");
			for (i = temp; i < (temp + n); i++) {
				if (i > size)
					break;
				data = table[i];
				pr_info("  [0x%04x]=0x%02x\n", i, data);
			}
		} else if (strcmp(parm[1], "update") == 0) {
			lcd_tcon_core_update(pdrv);
		} else {
			goto lcd_tcon_debug_store_err;
		}
	} else if (strcmp(parm[0], "od") == 0) { /* over drive */
		if (!parm[1]) {
			temp = lcd_tcon_od_get(pdrv);
			LCDPR("tcon od is %s: %d\n", temp ? "enabled" : "disabled", temp);
		} else {
			ret = kstrtouint(parm[1], 10, &temp);
			if (ret)
				goto lcd_tcon_debug_store_err;
			if (temp)
				lcd_tcon_od_set(pdrv, 1);
			else
				lcd_tcon_od_set(pdrv, 0);
		}
	} else if (strcmp(parm[0], "multi_lut") == 0) {
		lcd_tcon_multi_lut_print();
	} else if (strcmp(parm[0], "multi_update") == 0) {
		if (!mm_table)
			goto lcd_tcon_debug_store_end;
		if (!parm[1])
			goto lcd_tcon_debug_store_err;
		ret = kstrtouint(parm[1], 10, &temp);
		if (ret)
			goto lcd_tcon_debug_store_err;
		mm_table->multi_lut_update = temp;
		LCDPR("tcon multi_update: %d\n", temp);
	} else if (strcmp(parm[0], "multi_bypass") == 0) {
		if (!mm_table)
			goto lcd_tcon_debug_store_end;
		if (!parm[2])
			goto lcd_tcon_debug_store_err;
		ret = kstrtouint(parm[2], 16, &temp);
		if (ret)
			goto lcd_tcon_debug_store_err;
		if (strcmp(parm[1], "set") == 0)
			lcd_tcon_data_multi_bypass_set(mm_table, temp, 1);
		else //clr
			lcd_tcon_data_multi_bypass_set(mm_table, temp, 0);
	} else if (strcmp(parm[0], "tee") == 0) {
		if (!parm[1])
			goto lcd_tcon_debug_store_err;
#if IS_ENABLED(CONFIG_AMLOGIC_TEE)
		if (strcmp(parm[1], "status") == 0)
			lcd_tcon_mem_tee_get_status();
		else if (strcmp(parm[1], "off") == 0)
			ret = lcd_tcon_mem_tee_protect(0);
		else if (strcmp(parm[1], "on") == 0)
			ret = lcd_tcon_mem_tee_protect(1);
		else
			goto lcd_tcon_debug_store_err;
#endif
#ifdef TCON_DBG_TIME
	} else if (strcmp(parm[0], "isr") == 0) {
		if (parm[1]) {
			if (strcmp(parm[1], "dbg") == 0) {
				if (parm[2]) {
					ret = kstrtouint(parm[2], 16, &temp);
					if (ret)
						goto lcd_tcon_debug_store_err;
					if (temp)
						lcd_debug_print_flag |= LCD_DBG_PR_TEST;
					else
						lcd_debug_print_flag &= ~LCD_DBG_PR_TEST;
					pr_err("tcon isr dbg_log_en: %d\n", temp);
					goto lcd_tcon_debug_store_end;
				}
			} else if (strcmp(parm[1], "clr") == 0) {
				pdrv->tcon_isr_bypass = 1;
				lcd_delay_ms(100);
				lcd_tcon_dbg_trace_clear();
				pdrv->tcon_isr_bypass = 0;
				goto lcd_tcon_debug_store_end;
			} else if (strcmp(parm[1], "log") == 0) {
				if (parm[2]) {
					ret = kstrtouint(parm[2], 16, &temp);
					if (ret)
						goto lcd_tcon_debug_store_err;
				} else {
					temp = 0x1;
				}
				pdrv->tcon_isr_bypass = 1;
				lcd_delay_ms(100);
				lcd_tcon_dbg_trace_print(temp);
				pdrv->tcon_isr_bypass = 0;
				goto lcd_tcon_debug_store_end;
			}
		}
		pr_err("tcon dbg_log_en: %d\n",
			(lcd_debug_print_flag & LCD_DBG_PR_TEST) ? 1 : 0);
		if (lcd_debug_print_flag & LCD_DBG_PR_TEST)
			lcd_tcon_dbg_trace_print(0);
#endif
	} else {
		goto lcd_tcon_debug_store_err;
	}

lcd_tcon_debug_store_end:
	kfree(parm);
	kfree(buf_orig);
	return count;

lcd_tcon_debug_store_err:
	pr_info("invalid parameters\n");
	kfree(parm);
	kfree(buf_orig);
	return count;
#undef __MAX_PARAM
}

ssize_t lcd_tcon_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct aml_lcd_drv_s *pdrv = dev_get_drvdata(dev);

	return sprintf(buf, "0x%x\n", pdrv->tcon_status);
}

ssize_t lcd_tcon_reg_debug_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct aml_lcd_drv_s *pdrv = dev_get_drvdata(dev);
	int len = 0;
	unsigned int i, addr;

	mutex_lock(&lcd_tcon_dbg_mutex);

	len += sprintf(buf + len, "for_tool:");
	if ((pdrv->status & LCD_STATUS_IF_ON) == 0) {
		len += sprintf(buf + len, "ERROR\n");
		mutex_unlock(&lcd_tcon_dbg_mutex);
		return len;
	}
	switch (adb_reg.rw_mode) {
	case LCD_ADB_TCON_REG_RW_MODE_NULL:
		len += sprintf(buf + len, "NULL");
		break;
	case LCD_ADB_TCON_REG_RW_MODE_RN:
		if (adb_reg.bit_width == ADB_TCON_REG_32_bit) {
			for (i = 0; i < adb_reg.len; i++) {
				addr = adb_reg.addr + i;
				len += sprintf(buf + len, "%04x=%08x ",
					       addr, lcd_tcon_read(pdrv, addr));
			}
		} else {
			for (i = 0; i < adb_reg.len; i++) {
				addr = adb_reg.addr + i;
				len += sprintf(buf + len, "%04x=%02x ",
					       addr, lcd_tcon_read_byte(pdrv, addr));
			}
		}
		break;
	case LCD_ADB_TCON_REG_RW_MODE_WM:
		if (adb_reg.bit_width == ADB_TCON_REG_32_bit) {
			addr = adb_reg.addr;
			len += sprintf(buf + len, "%04x=%08x ",
				       addr, lcd_tcon_read(pdrv, addr));
		} else {
			addr = adb_reg.addr;
			len += sprintf(buf + len, "%04x=%02x ",
				       addr, lcd_tcon_read_byte(pdrv, addr));
		}
		break;
	case LCD_ADB_TCON_REG_RW_MODE_WN:
		if (adb_reg.bit_width == ADB_TCON_REG_32_bit) {
			for (i = 0; i < adb_reg.len; i++) {
				addr = adb_reg.addr + i;
				len += sprintf(buf + len, "%04x=%08x ",
					       addr, lcd_tcon_read(pdrv, addr));
			}
		} else {
			for (i = 0; i < adb_reg.len; i++) {
				addr = adb_reg.addr + i;
				len += sprintf(buf + len, "%04x=%02x ",
					       addr, lcd_tcon_read_byte(pdrv, addr));
			}
		}
		break;
	case LCD_ADB_TCON_REG_RW_MODE_WS:
		if (adb_reg.bit_width == ADB_TCON_REG_32_bit) {
			addr = adb_reg.addr;
			for (i = 0; i < adb_reg.len; i++) {
				len += sprintf(buf + len, "%04x=%08x ",
					       addr, lcd_tcon_read(pdrv, addr));
			}
		} else {
			addr = adb_reg.addr;
			for (i = 0; i < adb_reg.len; i++) {
				len += sprintf(buf + len, "%04x=%02x ",
					       addr, lcd_tcon_read_byte(pdrv, addr));
			}
		}
		break;
	case LCD_ADB_TCON_REG_RW_MODE_ERR:
		len += sprintf(buf + len, "ERROR");
		break;
	default:
		len += sprintf(buf + len, "ERROR");
		adb_reg.rw_mode = LCD_ADB_TCON_REG_RW_MODE_NULL;
		adb_reg.addr = 0;
		adb_reg.len = 0;
		break;
	}
	len += sprintf(buf + len, "\n");
	mutex_unlock(&lcd_tcon_dbg_mutex);
	return len;
}

ssize_t lcd_tcon_reg_debug_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
#define __MAX_PARAM 1500
	struct aml_lcd_drv_s *pdrv = dev_get_drvdata(dev);
	char *buf_orig;
	char **parm = NULL;
	unsigned int  temp32 = 0, temp_reg = 0;
	unsigned int  temp_len = 0, temp_mask = 0, temp_val = 0;
	unsigned char temp8 = 0;
	int ret = -1, i;

	if ((pdrv->status & LCD_STATUS_IF_ON) == 0)
		return count;

	if (!buf)
		return count;

	mutex_lock(&lcd_tcon_dbg_mutex);

	buf_orig = kstrdup(buf, GFP_KERNEL);
	if (!buf_orig) {
		mutex_unlock(&lcd_tcon_dbg_mutex);
		return count;
	}

	parm = kcalloc(__MAX_PARAM, sizeof(char *), GFP_KERNEL);
	if (!parm) {
		kfree(buf_orig);
		mutex_unlock(&lcd_tcon_dbg_mutex);
		return count;
	}

	lcd_debug_parse_param(buf_orig, parm, __MAX_PARAM);

	if (strcmp(parm[0], "wn") == 0) {
		if (!parm[3])
			goto lcd_tcon_adb_debug_store_err;
		if (strcmp(parm[1], "8") == 0)
			adb_reg.bit_width = ADB_TCON_REG_8_bit;
		else if (strcmp(parm[1], "32") == 0)
			adb_reg.bit_width = ADB_TCON_REG_32_bit;
		else
			goto lcd_tcon_adb_debug_store_err;
		ret = kstrtouint(parm[2], 16, &temp_reg);
		if (ret)
			goto lcd_tcon_adb_debug_store_err;
		ret = kstrtouint(parm[3], 10, &temp_len);
		if (ret)
			goto lcd_tcon_adb_debug_store_err;
		if (temp_len <= 0)
			goto lcd_tcon_adb_debug_store_err;
		if (!parm[4 + temp_len - 1])
			goto lcd_tcon_adb_debug_store_err;
		if (adb_reg.bit_width == ADB_TCON_REG_32_bit) {
			/*(4k - 9)/(8+1) ~=454*/
			if (temp_len > 454)
				goto lcd_tcon_adb_debug_store_err;
		} else {
			/*(4k - 9)/(2+1) ~=1362*/
			if (temp_len > 1362)
				goto lcd_tcon_adb_debug_store_err;
		}
		adb_reg.len = temp_len; /* for cat use */
		adb_reg.addr = temp_reg;
		adb_reg.rw_mode = LCD_ADB_TCON_REG_RW_MODE_WN;
		for (i = 0; i < temp_len; i++) {
			ret = kstrtouint(parm[i + 4], 16, &temp_val);
			if (ret)
				goto lcd_tcon_adb_debug_store_err;
			if (adb_reg.bit_width == ADB_TCON_REG_32_bit)
				lcd_tcon_write(pdrv, temp_reg, temp_val);
			else
				lcd_tcon_write_byte(pdrv, temp_reg, temp_val);
			temp_reg++;
		}
	} else if (strcmp(parm[0], "wm") == 0) {
		if (!parm[4])
			goto lcd_tcon_adb_debug_store_err;
		if (strcmp(parm[1], "8") == 0)
			adb_reg.bit_width = ADB_TCON_REG_8_bit;
		else if (strcmp(parm[1], "32") == 0)
			adb_reg.bit_width = ADB_TCON_REG_32_bit;
		else
			goto lcd_tcon_adb_debug_store_err;
		ret = kstrtouint(parm[2], 16, &temp_reg);
		if (ret)
			goto lcd_tcon_adb_debug_store_err;
		ret = kstrtouint(parm[3], 16, &temp_mask);
		if (ret)
			goto lcd_tcon_adb_debug_store_err;
		ret = kstrtouint(parm[4], 16, &temp_val);
		if (ret)
			goto lcd_tcon_adb_debug_store_err;
		adb_reg.len = 1; /* for cat use */
		adb_reg.addr = temp_reg;
		adb_reg.rw_mode = LCD_ADB_TCON_REG_RW_MODE_WM;
		if (adb_reg.bit_width == ADB_TCON_REG_32_bit) {
			temp32 = lcd_tcon_read(pdrv, temp_reg);
			temp32 &= ~temp_mask;
			temp32 |= temp_val & temp_mask;
			lcd_tcon_write(pdrv, temp_reg, temp32);
		} else {
			temp8 = lcd_tcon_read_byte(pdrv, temp_reg);
			temp8 &= ~temp_mask;
			temp8 |= temp_val & temp_mask;
			lcd_tcon_write_byte(pdrv, temp_reg, temp8);
		}
	} else if (strcmp(parm[0], "ws") == 0) {
		if (!parm[3])
			goto lcd_tcon_adb_debug_store_err;
		if (strcmp(parm[1], "8") == 0)
			adb_reg.bit_width = ADB_TCON_REG_8_bit;
		else if (strcmp(parm[1], "32") == 0)
			adb_reg.bit_width = ADB_TCON_REG_32_bit;
		else
			goto lcd_tcon_adb_debug_store_err;
		ret = kstrtouint(parm[2], 16, &temp_reg);
		if (ret)
			goto lcd_tcon_adb_debug_store_err;
		ret = kstrtouint(parm[3], 10, &temp_len);
		if (ret)
			goto lcd_tcon_adb_debug_store_err;
		if (temp_len <= 0)
			goto lcd_tcon_adb_debug_store_err;
		if (!parm[4 + temp_len - 1])
			goto lcd_tcon_adb_debug_store_err;
		if (adb_reg.bit_width == ADB_TCON_REG_32_bit) {
			/*(4k - 9)/(8+1) ~=454*/
			if (temp_len > 454)
				goto lcd_tcon_adb_debug_store_err;
		} else {
			/*(4k - 9)/(2+1) ~=1362*/
			if (temp_len > 1362)
				goto lcd_tcon_adb_debug_store_err;
		}
		adb_reg.len = temp_len; /* for cat use */
		adb_reg.addr = temp_reg;
		adb_reg.rw_mode = LCD_ADB_TCON_REG_RW_MODE_WS;
		for (i = 0; i < temp_len; i++) {
			ret = kstrtouint(parm[i + 4], 16, &temp_val);
			if (ret)
				goto lcd_tcon_adb_debug_store_err;
			if (adb_reg.bit_width == ADB_TCON_REG_32_bit)
				lcd_tcon_write(pdrv, temp_reg, temp_val);
			else
				lcd_tcon_write_byte(pdrv, temp_reg, temp_val);
		}
	} else if (strcmp(parm[0], "rn") == 0) {
		if (!parm[2])
			goto lcd_tcon_adb_debug_store_err;
		if (strcmp(parm[1], "8") == 0)
			adb_reg.bit_width = ADB_TCON_REG_8_bit;
		else if (strcmp(parm[1], "32") == 0)
			adb_reg.bit_width = ADB_TCON_REG_32_bit;
		else
			goto lcd_tcon_adb_debug_store_err;
		ret = kstrtouint(parm[2], 16, &temp_reg);
		if (ret)
			goto lcd_tcon_adb_debug_store_err;
		if (parm[3]) {
			ret = kstrtouint(parm[3], 10, &temp_len);
			if (ret)
				goto lcd_tcon_adb_debug_store_err;
			if (adb_reg.bit_width == ADB_TCON_REG_32_bit) {
				/*(4k - 9)/(8+1) ~=454*/
				if (temp_len > 454)
					goto lcd_tcon_adb_debug_store_err;
			} else {
				/*(4k - 9)/(2+1) ~=1362*/
				if (temp_len > 1362)
					goto lcd_tcon_adb_debug_store_err;
			}
			adb_reg.len = temp_len; /* for cat use */
			adb_reg.addr = temp_reg;
			adb_reg.rw_mode = LCD_ADB_TCON_REG_RW_MODE_RN;
		} else {
			adb_reg.len = 1; /* for cat use */
			adb_reg.addr = temp_reg;
			adb_reg.rw_mode = LCD_ADB_TCON_REG_RW_MODE_RN;
		}
	} else {
		goto lcd_tcon_adb_debug_store_err;
	}

	kfree(parm);
	kfree(buf_orig);
	mutex_unlock(&lcd_tcon_dbg_mutex);
	return count;

lcd_tcon_adb_debug_store_err:
	adb_reg.rw_mode = LCD_ADB_TCON_REG_RW_MODE_ERR;

	kfree(parm);
	kfree(buf_orig);
	mutex_unlock(&lcd_tcon_dbg_mutex);
	return count;
#undef __MAX_PARAM
}

ssize_t lcd_tcon_fw_dbg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct lcd_tcon_fw_s *tcon_fw = aml_lcd_tcon_get_fw();
	ssize_t ret = 0;

	if (!tcon_fw)
		return ret;

	if (tcon_fw->debug_show)
		ret = tcon_fw->debug_show(tcon_fw, buf);
	else
		LCDERR("%s: tcon_fw is not installed\n", __func__);

	return ret;
}

ssize_t lcd_tcon_fw_dbg_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct lcd_tcon_fw_s *tcon_fw = aml_lcd_tcon_get_fw();

	if (!buf)
		return count;
	if (!tcon_fw)
		return count;

	mutex_lock(&lcd_tcon_dbg_mutex);

	if (tcon_fw->debug_store)
		tcon_fw->debug_store(tcon_fw, buf, count);
	else
		LCDERR("%s: tcon_fw is not installed\n", __func__);

	mutex_unlock(&lcd_tcon_dbg_mutex);
	return count;
}

static const char *lcd_debug_tcon_pdf_usage_str = {
	"Usage:\n"
	"  echo help > tcon_pdf\n"
	"  echo ctrl <grp_num> <enable> > tcon_pdf\n"
	"     pdf function(group=0xff) or group enable\n"
	"  echo add group <grp_num> > tcon_pdf\n"
	"     add pdf action group\n"
	"  echo add src <grp_num> <src_mode> <reg> <mask> <val> > tcon_pdf\n"
	"     add src register for group\n"
	"  echo add dst <grp_num> <dst_mode> <index> <mask> <val> > tcon_pdf\n"
	"     add dst action for group\n"
	"  echo del group <grp_num>\n"
	"     delete action group\n"
	"  cat tcon_pdf\n"
	"     show pdf current status\n"
};

ssize_t lcd_tcon_pdf_dbg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct tcon_pdf_s *tcon_pdf = lcd_tcon_get_pdf();

	if (tcon_pdf->show_status)
		return tcon_pdf->show_status(tcon_pdf, buf);

	return 0;
}

ssize_t lcd_tcon_pdf_dbg_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
#define __MAX_PARAM 1500
	char *buf_orig;
	char **parm = NULL;
	int ret = -1;
	unsigned int val_arr[10] = { 0 };
	struct tcon_pdf_reg_s src;
	struct tcon_pdf_dst_s dst;
	struct tcon_pdf_s *tcon_pdf = lcd_tcon_get_pdf();

	if (!buf || !tcon_pdf)
		return count;

	mutex_lock(&lcd_tcon_dbg_mutex);

	buf_orig = kstrdup(buf, GFP_KERNEL);
	if (!buf_orig)
		goto __lcd_tcon_pdf_dbg_store_exit;

	parm = kcalloc(__MAX_PARAM, sizeof(char *), GFP_KERNEL);
	if (!parm)
		goto __lcd_tcon_pdf_dbg_store_exit;

	lcd_debug_parse_param(buf_orig, parm, __MAX_PARAM);
	if (!strcmp(parm[0], "help")) {
		LCDPR("%s", lcd_debug_tcon_pdf_usage_str);
	} else if (!strcmp(parm[0], "ctrl")) {
		ret = kstrtouint(parm[1], 0, &val_arr[0]);
		if (ret)
			goto __lcd_tcon_pdf_dbg_store_exit;
		ret = kstrtouint(parm[2], 0, &val_arr[1]);
		if (ret)
			goto __lcd_tcon_pdf_dbg_store_exit;
		if (tcon_pdf->group_enable) {
			ret = tcon_pdf->group_enable(tcon_pdf,
				val_arr[0], val_arr[1]);
			if (ret < 0) {
				LCDERR("Ctrl group %d fail\n", val_arr[0]);
			} else {
				LCDPR("Group %d %s\n", val_arr[0],
					val_arr[1] ? "enable" : "disable");
			}
		}
	} else if (!strcmp(parm[0], "add")) {
		if (!strcmp(parm[1], "group")) {
			if (tcon_pdf->new_group) {
				ret = kstrtouint(parm[2], 0, &val_arr[0]);
				if (ret)
					goto __lcd_tcon_pdf_dbg_store_exit;
				ret = tcon_pdf->new_group(tcon_pdf, val_arr[0]);
				if (ret < 0)
					LCDERR("Add group %d fail\n", val_arr[0]);
				else
					LCDPR("Add group %d\n", ret);
			}
		} else if (!strcmp(parm[1], "src")) {
			ret = kstrtouint(parm[2], 0, &val_arr[0]);  //group
			if (ret)
				goto __lcd_tcon_pdf_dbg_store_exit;
			ret = kstrtouint(parm[3], 0, &val_arr[1]);  //mode
			if (ret)
				goto __lcd_tcon_pdf_dbg_store_exit;
			ret = kstrtouint(parm[4], 0, &val_arr[2]);  //reg
			if (ret)
				goto __lcd_tcon_pdf_dbg_store_exit;
			ret = kstrtouint(parm[5], 0, &val_arr[3]);  //mask
			if (ret)
				goto __lcd_tcon_pdf_dbg_store_exit;
			ret = kstrtouint(parm[6], 0, &val_arr[4]);  //val
			if (ret)
				goto __lcd_tcon_pdf_dbg_store_exit;
			src.mode = val_arr[1];
			src.reg  = val_arr[2];
			src.mask = val_arr[3];
			src.val  = val_arr[4];
			LCDPR("src mode=%d, reg=%#x, mask=%#x, val=%#x\n",
				src.mode, src.reg, src.mask, src.val);
			if (tcon_pdf->group_add_src) {
				ret = tcon_pdf->group_add_src(tcon_pdf,
					val_arr[0], &src);
				if (ret < 0) {
					LCDERR("Add group src fail\n");
				} else {
					LCDPR("Group %d add src success\n",
						val_arr[0]);
				}
			}
		} else if (!strcmp(parm[1], "dst")) {
			ret = kstrtouint(parm[2], 0, &val_arr[0]);  //group
			if (ret)
				goto __lcd_tcon_pdf_dbg_store_exit;
			ret = kstrtouint(parm[3], 0, &val_arr[1]);  //mode
			if (ret)
				goto __lcd_tcon_pdf_dbg_store_exit;
			ret = kstrtouint(parm[4], 0, &val_arr[2]);  //index
			if (ret)
				goto __lcd_tcon_pdf_dbg_store_exit;
			ret = kstrtouint(parm[5], 0, &val_arr[3]);  //mask
			if (ret)
				goto __lcd_tcon_pdf_dbg_store_exit;
			ret = kstrtouint(parm[6], 0, &val_arr[4]);  //val
			if (ret)
				goto __lcd_tcon_pdf_dbg_store_exit;
			dst.mode  = val_arr[1];
			dst.index = val_arr[2];
			dst.mask  = val_arr[3];
			dst.val   = val_arr[4];
			LCDPR("dst mode=%d, index=%#x, mask=%#x, val=%#x\n",
				dst.mode, dst.index, dst.mask, dst.val);
			if (tcon_pdf->group_add_dst) {
				ret = tcon_pdf->group_add_dst(tcon_pdf,
					val_arr[0], &dst);
				if (ret < 0) {
					LCDERR("Add group dst fail\n");
				} else {
					LCDPR("Group %d add dst success\n",
						val_arr[0]);
				}
			}
		}
	} else if (!strcmp(parm[0], "del")) {
		if (!strcmp(parm[1], "group")) {
			ret = kstrtouint(parm[2], 0, &val_arr[0]);  //group
			if (ret)
				goto __lcd_tcon_pdf_dbg_store_exit;
			if (tcon_pdf->del_group) {
				ret = tcon_pdf->del_group(tcon_pdf, val_arr[0]);
				if (ret < 0) {
					LCDERR("Delete group %d fail\n",
						val_arr[0]);
				} else {
					LCDPR("Delete group %d success\n",
						val_arr[0]);
				}
			}
		}
	}

__lcd_tcon_pdf_dbg_store_exit:
	mutex_unlock(&lcd_tcon_dbg_mutex);
	kfree(parm);
	kfree(buf_orig);

	return count;
#undef __MAX_PARAM
}

static struct device_attribute lcd_tcon_debug_attrs[] = {
	__ATTR(debug,     0644, lcd_tcon_debug_show, lcd_tcon_debug_store),
	__ATTR(status,    0444, lcd_tcon_status_show, NULL),
	__ATTR(reg,       0644, lcd_tcon_reg_debug_show, lcd_tcon_reg_debug_store),
	__ATTR(tcon_fw,   0644, lcd_tcon_fw_dbg_show, lcd_tcon_fw_dbg_store),
	__ATTR(tcon_pdf,  0644, lcd_tcon_pdf_dbg_show, lcd_tcon_pdf_dbg_store),
};

/* **********************************
 * tcon IOCTL
 * **********************************
 */
static unsigned int lcd_tcon_bin_path_index;

long lcd_tcon_ioctl_handler(struct aml_lcd_drv_s *pdrv, int mcd_nr, unsigned long arg)
{
	void __user *argp;
	struct aml_lcd_tcon_bin_s lcd_tcon_buff;
	struct tcon_rmem_s *tcon_rmem = get_lcd_tcon_rmem();
	struct tcon_mem_map_table_s *mm_table = get_lcd_tcon_mm_table();
	struct lcd_tcon_data_block_header_s block_header, block_header_old;
	unsigned int size = 0, old_size, temp, m = 0, header_size = 0;
	struct device *dev;
	phys_addr_t paddr = 0, paddr_old = 0;
	unsigned char *mem_vaddr = NULL, *vaddr = NULL, *vaddr_old = NULL;
	char *str = NULL;
	int index = 0;
	int ret = 0;

	if (!pdrv)
		return -EFAULT;

	argp = (void __user *)arg;
	switch (mcd_nr) {
	case LCD_IOC_GET_TCON_BIN_MAX_CNT_INFO:
		if (!mm_table) {
			ret = -EFAULT;
			break;
		}
		if (copy_to_user(argp, &mm_table->block_cnt, sizeof(unsigned int)))
			ret = -EFAULT;
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("tcon: get bin max_cnt: %d\n", mm_table->block_cnt);
		break;
	case LCD_IOC_SET_TCON_DATA_INDEX_INFO:
		if (copy_from_user(&lcd_tcon_bin_path_index, argp, sizeof(unsigned int)))
			ret = -EFAULT;
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("tcon: set bin index: %d\n", lcd_tcon_bin_path_index);
		break;
	case LCD_IOC_GET_TCON_BIN_PATH_INFO:
		if (!mm_table || !tcon_rmem) {
			ret = -EFAULT;
			break;
		}

		mem_vaddr = tcon_rmem->bin_path_rmem.mem_vaddr;
		if (!mem_vaddr) {
			LCDERR("%s: no tcon bin path rmem\n", __func__);
			ret = -EFAULT;
			break;
		}
		if (lcd_tcon_bin_path_index >= mm_table->block_cnt) {
			ret = -EFAULT;
			break;
		}
		m = 32 + 256 * lcd_tcon_bin_path_index;
		str = (char *)&mem_vaddr[m + 4];
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("tcon: get bin_path[%d]: %s\n", lcd_tcon_bin_path_index, str);

		if (copy_to_user(argp, str, 256))
			ret = -EFAULT;
		break;
	case LCD_IOC_SET_TCON_BIN_DATA_INFO:
		if (!mm_table || !tcon_rmem) {
			ret = -EFAULT;
			break;
		}

		mem_vaddr = tcon_rmem->bin_path_rmem.mem_vaddr;
		if (!mem_vaddr) {
			LCDERR("%s: no tcon bin path rmem\n", __func__);
			ret = -EFAULT;
			break;
		}

		memset(&lcd_tcon_buff, 0, sizeof(struct aml_lcd_tcon_bin_s));
		if (copy_from_user(&lcd_tcon_buff, argp, sizeof(struct aml_lcd_tcon_bin_s))) {
			ret = -EFAULT;
			break;
		}
		if (lcd_tcon_buff.size == 0) {
			LCDERR("%s: invalid data size %d\n", __func__, size);
			ret = -EFAULT;
			break;
		}
		index = lcd_tcon_buff.index;
		if (index >= mm_table->block_cnt) {
			LCDERR("%s: invalid index %d\n", __func__, index);
			ret = -EFAULT;
			break;
		}
		m = 32 + 256 * index;
		str = (char *)&mem_vaddr[m + 4];
		temp = *(unsigned int *)&mem_vaddr[m];

		header_size = sizeof(struct lcd_tcon_data_block_header_s);
		memset(&block_header, 0, header_size);
		memset(&block_header_old, 0, header_size);
		vaddr_old = mm_table->data_mem_vaddr[index];
		paddr_old = mm_table->data_mem_paddr[index];
		if (vaddr_old)
			memcpy(&block_header_old, vaddr_old, header_size);

		argp = (void __user *)lcd_tcon_buff.ptr;
		if (copy_from_user(&block_header, argp, header_size)) {
			ret = -EFAULT;
			break;
		}

		dev = &pdrv->pdev->dev;
		old_size = block_header_old.block_size;
		size = block_header.block_size;
		if (size > lcd_tcon_buff.size || size < header_size) {
			LCDERR("%s: block[%d] size 0x%x error\n", __func__, index, size);
			ret = -EFAULT;
			break;
		}

		if (is_block_ctrl_dma(block_header.block_ctrl)) {
			vaddr = lcd_alloc_dma_buffer(pdrv, size, &paddr);
			if (lcd_debug_print_flag)
				LCDPR("alloc for tcon mm_table[%d] form lcd_cma_pool\n"
					"pa:0x%llx, va:%p, size:0x%x\n",
					index, (unsigned long long)paddr, vaddr, size);
		} else {
			vaddr = kcalloc(size, sizeof(unsigned char), GFP_KERNEL);
			paddr = virt_to_phys(vaddr);
		}

		if (unlikely(!vaddr))
			goto set_tcon_bin_error_break;

		if (unlikely(copy_from_user(vaddr, argp, size)))
			goto set_tcon_bin_error_break;

		LCDPR("tcon: load bin_path[%d]: %s, size: 0x%x -> 0x%x\n", index, str, temp, size);

		mm_table->data_mem_vaddr[index] = vaddr;
		mm_table->data_mem_paddr[index] = paddr;
		ret = lcd_tcon_data_load(pdrv, vaddr, index);
		if (unlikely(ret))
			goto set_tcon_bin_error_break;

		break;
set_tcon_bin_error_break:
		mm_table->data_mem_vaddr[index] = vaddr_old;
		mm_table->data_mem_paddr[index] = paddr_old;
		if (is_block_ctrl_dma(block_header.block_ctrl))
			dma_free_coherent(dev, size, vaddr, paddr);
		else
			kfree(vaddr);
		ret = -EFAULT;
		break;
	default:
		LCDERR("tcon: don't support ioctl cmd_nr: 0x%x\n", mcd_nr);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int lcd_tcon_open(struct inode *inode, struct file *file)
{
	struct lcd_tcon_local_cfg_s *tcon_cfg;

	/* Get the per-device structure that contains this cdev */
	tcon_cfg = container_of(inode->i_cdev, struct lcd_tcon_local_cfg_s, cdev);
	file->private_data = tcon_cfg;
	return 0;
}

static int lcd_tcon_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static long lcd_tcon_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct lcd_tcon_local_cfg_s *local_cfg = (struct lcd_tcon_local_cfg_s *)file->private_data;
	struct aml_lcd_drv_s *pdrv;
	void __user *argp;
	int mcd_nr = -1;
	int ret = 0;

	if (!local_cfg)
		return -EFAULT;
	pdrv = dev_get_drvdata(local_cfg->dev);
	if (!pdrv)
		return -EFAULT;

	mcd_nr = _IOC_NR(cmd);
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("tcon: %s: cmd_dir = 0x%x, cmd_nr = 0x%x\n",
			__func__, _IOC_DIR(cmd), mcd_nr);
	}

	argp = (void __user *)arg;
	switch (mcd_nr) {
	case LCD_IOC_GET_TCON_BIN_MAX_CNT_INFO:
	case LCD_IOC_SET_TCON_DATA_INDEX_INFO:
	case LCD_IOC_GET_TCON_BIN_PATH_INFO:
	case LCD_IOC_SET_TCON_BIN_DATA_INFO:
		lcd_tcon_ioctl_handler(pdrv, mcd_nr, arg);
		break;
	default:
		LCDERR("tcon: don't support ioctl cmd_nr: 0x%x\n", mcd_nr);
		ret = -EINVAL;
		break;
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long lcd_tcon_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned long ret;

	arg = (unsigned long)compat_ptr(arg);
	ret = lcd_tcon_ioctl(file, cmd, arg);
	return ret;
}
#endif

static const struct file_operations lcd_tcon_fops = {
	.owner          = THIS_MODULE,
	.open           = lcd_tcon_open,
	.release        = lcd_tcon_release,
	.unlocked_ioctl = lcd_tcon_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = lcd_tcon_compat_ioctl,
#endif
};

/* **********************************
 * tcon debug api
 * **********************************
 */
void lcd_tcon_debug_file_add(struct aml_lcd_drv_s *pdrv, struct lcd_tcon_local_cfg_s *local_cfg)
{
	int i, ret;

	if (!local_cfg)
		return;

	local_cfg->clsp = class_create(THIS_MODULE, AML_TCON_CLASS_NAME);
	if (IS_ERR(local_cfg->clsp)) {
		LCDERR("tcon: failed to create class\n");
		return;
	}

	ret = alloc_chrdev_region(&local_cfg->devno, 0, 1, AML_TCON_DEVICE_NAME);
	if (ret < 0) {
		LCDERR("tcon: failed to alloc major number\n");
		goto err1;
	}

	local_cfg->dev = device_create(local_cfg->clsp, NULL,
		local_cfg->devno, (void *)pdrv, AML_TCON_DEVICE_NAME);
	if (IS_ERR(local_cfg->dev)) {
		LCDERR("tcon: failed to create device\n");
		goto err2;
	}

	for (i = 0; i < ARRAY_SIZE(lcd_tcon_debug_attrs); i++) {
		if (device_create_file(local_cfg->dev, &lcd_tcon_debug_attrs[i])) {
			LCDERR("tcon: create attribute %s fail\n",
			       lcd_tcon_debug_attrs[i].attr.name);
			goto err3;
		}
	}

	/* connect the file operations with cdev */
	cdev_init(&local_cfg->cdev, &lcd_tcon_fops);
	local_cfg->cdev.owner = THIS_MODULE;

	/* connect the major/minor number to the cdev */
	ret = cdev_add(&local_cfg->cdev, local_cfg->devno, 1);
	if (ret) {
		LCDERR("tcon: failed to add device\n");
		goto err4;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("tcon: %s: OK\n", __func__);
	return;

err4:
	for (i = 0; i < ARRAY_SIZE(lcd_tcon_debug_attrs); i++)
		device_remove_file(local_cfg->dev, &lcd_tcon_debug_attrs[i]);
err3:
	device_destroy(local_cfg->clsp, local_cfg->devno);
	local_cfg->dev = NULL;
err2:
	unregister_chrdev_region(local_cfg->devno, 1);
err1:
	class_destroy(local_cfg->clsp);
	local_cfg->clsp = NULL;
	LCDERR("tcon: %s error\n", __func__);
}

void lcd_tcon_debug_file_remove(struct lcd_tcon_local_cfg_s *local_cfg)
{
	int i;

	if (!local_cfg)
		return;

	for (i = 0; i < ARRAY_SIZE(lcd_tcon_debug_attrs); i++)
		device_remove_file(local_cfg->dev, &lcd_tcon_debug_attrs[i]);

	cdev_del(&local_cfg->cdev);
	device_destroy(local_cfg->clsp, local_cfg->devno);
	local_cfg->dev = NULL;
	class_destroy(local_cfg->clsp);
	local_cfg->clsp = NULL;
	unregister_chrdev_region(local_cfg->devno, 1);
}
