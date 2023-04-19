// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * drivers/amlogic/media/video_processor/di_process/di_proc_file.c
 *
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
 */

#include "di_proc_file.h"
#include "di_proc_buf_mgr_internal.h"

static int di_proc_file_print(int debug_flag, const char *fmt, ...)
{
	if ((dp_buf_mgr_print_flag & debug_flag) ||
	    debug_flag == PRINT_ERROR) {
		unsigned char buf[256];
		int len = 0;
		va_list args;

		va_start(args, fmt);
		len = sprintf(buf, "dp_buf: file:");
		vsnprintf(buf + len, 256 - len, fmt, args);
		pr_info("%s", buf);
		va_end(args);
	}
	return 0;
}

static void di_proc_vf_free(struct file_private_data *file_private_data)
{
	struct vframe_s *vf;
	u32 flag;
	struct di_buffer *buf;

	vf = &file_private_data->vf;
	di_proc_file_print(PRINT_OTHER, "vf_free: type=%x\n", vf->type);

	flag = file_private_data->flag;

	if (flag & V4LVIDEO_FLAG_DI_V3) {
		buf = (struct di_buffer *)file_private_data->private2;
		/*di vf has dec vf, need put dec file*/
		if (buf->vf->flag & VFRAME_FLAG_DOUBLE_FRAM) {
			vf = buf->vf->vf_ext;
			if (vf && file_private_data->file)
				dp_put_file_ext(file_private_data->file);
			else
				di_proc_file_print(PRINT_ERROR,
					"free: di has dec vf, but dec vf/file is null vf=%px, file=%px\n",
					vf, file_private_data->file);
		}
		di_release_keep_buf(buf);
		total_fill_count++;
		di_proc_file_print(PRINT_OTHER, "vf_free: release di v3 buf\n");
	}  else if (flag & V4LVIDEO_FLAG_DI_BYPASS) {
		/*di bypass, need put dec file*/
		vf = (struct vframe_s *)(file_private_data->vf_p);
		if (vf && file_private_data->file)
			dp_put_file_ext(file_private_data->file);
		else
			di_proc_file_print(PRINT_ERROR,
				"free: di bypass, but dec vf/file is null vf=%px, file=%px\n",
				vf, file_private_data->file);
	} else {
		di_proc_file_print(PRINT_ERROR, "vf_free: is not v3 buf\n");
	}
}

static void di_proc_private_data_release(struct file_private_data *data)
{
	if (!data)
		return;

	di_proc_file_print(PRINT_OTHER, "is_keep =%d\n", data->is_keep);
	if (data->is_keep)
		di_proc_vf_free(data);

	di_proc_file_print(PRINT_OTHER, "private data release\n");

	memset(data, 0, sizeof(struct file_private_data));
}

static void di_proc_free_fd_private(void *arg)
{
	if (arg) {
		di_proc_file_print(PRINT_OTHER, "free_fd_private\n");
		di_proc_private_data_release(arg);
		vfree((u8 *)arg);
	} else {
		di_proc_file_print(PRINT_ERROR, "free: arg is NULL\n");
	}
}

struct file_private_data *di_proc_get_file_private_data(struct file *file_vf,
							 bool alloc_if_null)
{
	struct file_private_data *file_private_data;
	struct uvm_hook_mod *uhmod;
	struct uvm_hook_mod_info info;
	int ret;

	if (!file_vf) {
		di_proc_file_print(PRINT_ERROR, "get_file_private_data fail\n");
		return NULL;
	}

	uhmod = uvm_get_hook_mod((struct dma_buf *)(file_vf->private_data),
				 VF_PROCESS_V4LVIDEO);
	if (uhmod && uhmod->arg) {
		file_private_data = uhmod->arg;
		uvm_put_hook_mod((struct dma_buf *)(file_vf->private_data),
				 VF_PROCESS_V4LVIDEO);
		return file_private_data;
	} else if (!alloc_if_null) {
		return NULL;
	}

	file_private_data = vmalloc(sizeof(*file_private_data));
	if (!file_private_data)
		return NULL;
	memset(file_private_data, 0, sizeof(struct file_private_data));

	memset(&info, 0, sizeof(struct uvm_hook_mod_info));
	di_proc_file_print(PRINT_OTHER, "uvm attch\n");
	info.type = VF_PROCESS_V4LVIDEO;
	info.arg = file_private_data;
	info.free = di_proc_free_fd_private;
	info.acquire_fence = NULL;
	ret = uvm_attach_hook_mod((struct dma_buf *)(file_vf->private_data),
				   &info);
	return file_private_data;
}

