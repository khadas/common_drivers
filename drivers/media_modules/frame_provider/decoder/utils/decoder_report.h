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
#ifndef __DEC_REPORT_H__
#define __DEC_REPORT_H__
#include "../../../amvdec_ports/aml_vcodec_drv.h"

int report_module_init(void);
void report_module_exit(void);
void register_dump_v4ldec_state_func(struct aml_vcodec_dev *dev, dump_v4ldec_state_func func);

typedef ssize_t (*dump_amstream_bufs_func)(char *);
void register_dump_amstream_bufs_func(dump_amstream_bufs_func func);

#endif

