/*
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Description:
 */
#ifndef VAV1_H
#define VAV1_H
void adapt_coef_probs(int pic_count, int prev_kf, int cur_kf, int pre_fc,
unsigned int *prev_prob, unsigned int *cur_prob, unsigned int *count);

#define PXP_CODE
#define PXP_DEBUG_CODE

#define PRINT_BUF_SIZE 1024

#define PRINT_STRCAT(buf, len, fmt, args...)  do {	\
		if (buf) {									\
			len += snprintf(buf + len,				\
				PRINT_BUF_SIZE - len, fmt, ##args);	\
		} else										\
			printk(fmt, ##args);					\
	} while (0)

#endif
