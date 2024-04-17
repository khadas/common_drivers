// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/amlogic/media/video_sink/video.h>
#include <linux/amlogic/media/vfm/vframe.h>
#include <linux/amlogic/media/registers/cpu_version.h>
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM
#include <linux/amlogic/media/amvecm/amvecm.h>
#endif
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/amlogic/media/vfm/vframe_provider.h>
#include <linux/amlogic/media/vfm/vframe_receiver.h>
#include "amdv_regs_hw5.h"
#include "../amvecm/arch/vpp_regs.h"
#include "../amvecm/arch/vpp_hdr_regs.h"
#include "../amvecm/arch/vpp_dolbyvision_regs.h"
#include "../amvecm/amcsc.h"
#include "../amvecm/reg_helper.h"
#include <linux/amlogic/media/registers/regs/viu_regs.h>
#include <linux/amlogic/media/amdolbyvision/dolby_vision.h>
#include <linux/amlogic/media/vpu/vpu.h>
#include <linux/dma-map-ops.h>
#include <linux/amlogic/iomap.h>
#include "md_config.h"
#include <linux/of.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/amlogic/media/canvas/canvas.h>
#include "amdv.h"
#ifdef CONFIG_AMLOGIC_MEDIA_CODEC_MM
#include <linux/amlogic/media/codec_mm/codec_mm.h>
#endif

static struct dv_atsc p_atsc_md;

bool enable_top1_scale = true;
u32 hw5_reg_from_file;
module_param(hw5_reg_from_file, uint, 0664);
MODULE_PARM_DESC(hw5_reg_from_file, "\n hw5_reg_from_file\n");

u32 force_update_top2 = true;
module_param(force_update_top2, uint, 0664);
MODULE_PARM_DESC(force_update_top2, "\n force_update_top2\n");

struct dv5_top1_vd_info top1_vd_info;
struct vframe_s *vf_tmp;
static u32 last_vf_valid_crc_top1;

#define signal_cuva ((vf->signal_type >> 31) & 1)
#define signal_color_primaries ((vf->signal_type >> 16) & 0xff)
#define signal_transfer_characteristic ((vf->signal_type >> 8) & 0xff)

struct dynamic_cfg_s dynamic_config_new;
struct dynamic_cfg_s dynamic_darkdetail = {16, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0};

const char level_str[4][10] = {
	"6",
	"7",
	"0",
	"invalid"
};

bool update_top2_cfg;

void dump_top1_frame(int force_w, int force_h)
{
#ifdef CONFIG_AMLOGIC_ENABLE_VIDEO_PIPELINE_DUMP_DATA
	struct file *fp = NULL;
	char name_buf[32];
	loff_t pos;
#endif
	int data_size_y, data_size_uv;
	u8 *data_y;
	u8 *data_uv;
	u16 *data_y_16;
	u16 *data_uv_16;
	int i;
	struct vframe_s *vf = vf_tmp;
	int data_w = 1;

#ifdef CONFIG_AMLOGIC_ENABLE_VIDEO_PIPELINE_DUMP_DATA
	snprintf(name_buf, sizeof(name_buf), "/data/src_vframe.yuv");
	fp = filp_open(name_buf, O_CREAT | O_RDWR, 0644);
	if (IS_ERR(fp))
		return;
#endif

	if (vf && vf->canvas0_config[0].phy_addr) {
		pr_dv_dbg("vf %px: size %d %d %d %d %d %d\n", vf, force_w, force_h,
			vf->canvas0_config[0].width, vf->canvas0_config[0].height,
			vf->canvas0_config[1].width, vf->canvas0_config[1].height);
		if (vf->canvas0_config[0].block_mode)
			pr_dv_dbg("data in block mode, maybe disaplay error\n");

		data_w = vf->canvas0_config[0].bit_depth ? 2 : 1;/*420-10bit or 8bit*/

		if (force_w > 0 && force_w <= vf->canvas0_config[0].width &&
			force_h > 0 && force_h <= vf->canvas0_config[0].height)
			data_size_y = force_w * force_h * data_w;
		else
			data_size_y = vf->canvas0_config[0].width *
				vf->canvas0_config[0].height * data_w;

		data_size_uv = data_size_y / 2;

		data_y = codec_mm_vmap(vf->canvas0_config[0].phy_addr, data_size_y);
		data_uv = codec_mm_vmap(vf->canvas0_config[1].phy_addr, data_size_uv);
		data_y_16 = (u16 *)data_y;
		data_uv_16 = (u16 *)data_uv;
		if (!data_y || !data_uv) {
			pr_dv_error("error map data\n");
			return;
		}
#ifdef CONFIG_AMLOGIC_ENABLE_VIDEO_PIPELINE_DUMP_DATA
		pos = fp->f_pos;
		kernel_write(fp, data_y, data_size_y, &pos);
		fp->f_pos = pos;
		pos = fp->f_pos;
		kernel_write(fp, data_uv, data_size_uv, &pos);
		fp->f_pos = pos;
		filp_close(fp, NULL);
#endif
		if (debug_dolby & 0x80000) {
			pr_dv_dbg("y8===>size %d\n", data_size_y);
			for (i = 0; i < 1200; i += 8)
				pr_dv_dbg("%02x %02x %02x %02x %02x %02x %02x %02x\n",
				data_y[i],
				data_y[i + 1],
				data_y[i + 2],
				data_y[i + 3],
				data_y[i + 4],
				data_y[i + 5],
				data_y[i + 6],
				data_y[i + 7]);
			pr_dv_dbg("y16===>size %d\n", data_size_y);
			for (i = 0; i < 600; i += 8)
				pr_info("%02x %02x %02x %02x %02x %02x %02x %02x\n",
				data_y_16[i],
				data_y_16[i + 1],
				data_y_16[i + 2],
				data_y_16[i + 3],
				data_y_16[i + 4],
				data_y_16[i + 5],
				data_y_16[i + 6],
				data_y_16[i + 7]);
			pr_dv_dbg("uv16===>size %d\n", data_size_uv);
			for (i = 0; i < 600; i += 8)
				pr_dv_dbg("%02x %02x %02x %02x %02x %02x %02x %02x\n",
				data_uv_16[i],
				data_uv_16[i + 1],
				data_uv_16[i + 2],
				data_uv_16[i + 3],
				data_uv_16[i + 4],
				data_uv_16[i + 5],
				data_uv_16[i + 6],
				data_uv_16[i + 7]);
		}
		codec_mm_unmap_phyaddr(data_y);
		codec_mm_unmap_phyaddr(data_uv);
	}
}

static void get_vf_size(struct vframe_s *vf, u32 *h_size, u32 *v_size)
{
	if (vf && h_size && v_size) {
		if (vf->type & VIDTYPE_COMPRESS) {
			if (is_src_crop_valid(vf->src_crop)) {
				*h_size = vf->compWidth -
					vf->src_crop.left - vf->src_crop.right;
				*v_size = vf->compHeight -
					vf->src_crop.top - vf->src_crop.bottom;
			} else {
				*h_size = vf->compWidth;
				*v_size = vf->compHeight;
			}
		} else {
			*h_size = vf->width;
			*v_size = vf->height;
		}
	}
}

static void get_top1_vd_info(struct vframe_s *vf,
	struct dv5_top1_vd_info *top1_vd_info)
{
	u32 burst_len;
	u32 canvas_w;

	if (!vf)
		return;

	top1_vd_info->width = vf->width;
	top1_vd_info->height = vf->height;
	if (is_src_crop_valid(vf->src_crop)) {
		top1_vd_info->compWidth = vf->compWidth -
			vf->src_crop.left - vf->src_crop.right;
		top1_vd_info->compHeight = vf->compHeight -
			vf->src_crop.top - vf->src_crop.bottom;
	} else {
		top1_vd_info->compWidth = vf->compWidth;
		top1_vd_info->compHeight = vf->compHeight;
	}
	top1_vd_info->type = vf->type;
	top1_vd_info->bitdepth = vf->canvas0_config[0].bit_depth == 1 ? 10 : 8;
	top1_vd_info->plane = vf->plane_num;
	top1_vd_info->canvasaddr[0] = vf->canvas0_config[0].phy_addr;
	top1_vd_info->canvasaddr[1] = vf->canvas0_config[1].phy_addr;
	top1_vd_info->canvasaddr[2] = vf->canvas0_config[2].phy_addr;
	top1_vd_info->blk_mode = vf->canvas0_config[0].block_mode;
	top1_vd_info->buf_width = vf->canvas0_config[0].width;
	top1_vd_info->buf_height = vf->canvas0_config[0].height;
	top1_vd_info->linear_mode = (vf->flag & VFRAME_FLAG_VIDEO_LINEAR) ? 1 : 0;

	/* vd mif burst len is 2 as default */
	burst_len = 2;
	if (vf->canvas0Addr != (u32)-1)
		canvas_w = canvas_get_width(vf->canvas0Addr & 0xff);
	else
		canvas_w = vf->canvas0_config[0].width;

	if (canvas_w % 32)
		burst_len = 0;
	else if (canvas_w % 64)
		burst_len = 1;
	if (vf->canvas0_config[0].block_mode)
		burst_len = vf->canvas0_config[0].block_mode;
	top1_vd_info->burst_len = burst_len;

	/*all hdmi idk cases are 444-10bit */
	if (vf->source_type == VFRAME_SOURCE_TYPE_HDMI) {
		top1_vd_info->buf_width = top1_vd_info->width;/*no canvas0_config for hdmi*/
		top1_vd_info->buf_height = top1_vd_info->height;
		top1_vd_info->bitdepth = 10;
		top1_vd_info->blk_mode = 0;
		top1_vd_info->type &= (~VIDTYPE_VIU_NV21);
		top1_vd_info->type &= (~VIDTYPE_VIU_NV12);
		top1_vd_info->type &= (~VIDTYPE_VIU_422);
		top1_vd_info->type |= VIDTYPE_VIU_444;
	}

	if (debug_dolby & 0x80000) {
		pr_info("vf %px,w,h:%d,%d,cw,ch:%d,%d,%d,%d,%d,%d,%d,%d,type:0x%x,%x,flag %x,bdp:%d,p:%d,addr:%lx,%lx,%lx,b %d\n",
			vf,
			top1_vd_info->width,
			top1_vd_info->height,
			top1_vd_info->compWidth,
			top1_vd_info->compHeight,
			vf->canvas0_config[0].width,
			vf->canvas0_config[0].height,
			vf->canvas0_config[1].width,
			vf->canvas0_config[1].height,
			vf->canvas0_config[2].width,
			vf->canvas0_config[2].height,
			vf->type,
			top1_vd_info->type,
			vf->flag,
			top1_vd_info->bitdepth,
			top1_vd_info->plane,
			top1_vd_info->canvasaddr[0],
			top1_vd_info->canvasaddr[1],
			top1_vd_info->canvasaddr[2],
			top1_vd_info->blk_mode);
	}
}

void update_num_downsamplers(u32 w, u32 h)
{
	if (enable_top1) {/*todo, update according decoder dw*/
		if (w <= 512 && h <= 288)
			num_downsamplers = 1;
		else if (w <= 512 && h <= 576)
			num_downsamplers = 1;
		else if (w <= 1024 && h <= 288)
			num_downsamplers = 1;
		else
			num_downsamplers = 2;
	} else {
		num_downsamplers = 2;
	}

	if ((w == top1_vd_info.width * 4 && h == top1_vd_info.height * 4) ||
		((((w + 7) >> 3) << 3) == top1_vd_info.width * 4 &&
		(((h + 7) >> 3) << 3) == top1_vd_info.height * 4)) {
		num_downsamplers = 2;/*number of times video was downscaled before top1*/
		if (debug_dolby & 0x80000)
			pr_info("1/4 * 1/4 dw, set num_downsamplers=2\n");
	} else if ((w == top1_vd_info.width * 2 && h == top1_vd_info.height * 2) ||
		((((w + 7) >> 3) << 3) == top1_vd_info.width * 2 &&
		(((h + 7) >> 3) << 3) == top1_vd_info.height * 2)) {
		num_downsamplers = 1;
		if (debug_dolby & 0x80000)
			pr_info("1/2 * 1/2 dw, set num_downsamplers=1\n");
	} else if ((w == top1_vd_info.width && h == top1_vd_info.height) ||
		((((w + 7) >> 3) << 3) == top1_vd_info.width &&
		(((h + 7) >> 3) << 3) == top1_vd_info.height)) {
		num_downsamplers = 0;
		if (debug_dolby & 0x80000)
			pr_info("no dw, set num_downsamplers=0\n");
	}
	if (top1_scale) {
		if (num_downsamplers == 0) {
			num_downsamplers = 1;
			if (debug_dolby & 0x80000)
				pr_info("1:1 dw + top1 scaler, update num_downsamplers=1\n");
		} else if (num_downsamplers == 1) {
			num_downsamplers = 2;
			if (debug_dolby & 0x80000)
				pr_info("1/2 * 1/2 dw + top1 scaler, update num_downsamplers=2\n");
		} else if (num_downsamplers == 2) {
			num_downsamplers = 3;
			if (debug_dolby & 0x80000)
				pr_info("1/4 * 1/4 dw + top1 scaler, update num_downsamplers=3\n");
		}
	}
}

/*return true: need top1 rdmif scale, false: no scale*/
bool check_top1_scale(u32 w, u32 h)
{
	u32 decoder_scale = 0;
	bool scale = false;

	if (!enable_top1_scale)
		return scale;

	if ((w == top1_vd_info.width * 4 && h == top1_vd_info.height * 4) ||
		((((w + 7) >> 3) << 3) == top1_vd_info.width * 4 &&
		(((h + 7) >> 3) << 3) == top1_vd_info.height * 4)) {
		decoder_scale = 2;
	} else if ((w == top1_vd_info.width * 2 && h == top1_vd_info.height * 2) ||
		((((w + 7) >> 3) << 3) == top1_vd_info.width * 2 &&
		(((h + 7) >> 3) << 3) == top1_vd_info.height * 2)) {
		decoder_scale = 1;
	} else if ((w == top1_vd_info.width && h == top1_vd_info.height) ||
		((((w + 7) >> 3) << 3) == top1_vd_info.width &&
		(((h + 7) >> 3) << 3) == top1_vd_info.height)) {
		decoder_scale = 0;
	}
	if (decoder_scale > 0 &&
		top1_vd_info.width < 960 && top1_vd_info.height < 540 &&
		top1_vd_info.width > 512 && top1_vd_info.height > 288) {
		/*1. Some special res, pyramid start line too large top2 stuck at level7*/
		/*need work with level6, so need downscaler once more*/
		scale = true;
	} else if (decoder_scale == 0 && top1_vd_info.width > 512) {
		/*2. For 1024 >= width > 512, ko force pyramid setting with level 6*/
		/*if dw is 1:1,need top1 scale*/
		scale = true;
	} else {
		scale = false;
	}
	return scale;
}

void update_top1_onoff(struct vframe_s *vf)
{
	u32 w;
	u32 h;
	u32 ori_w;
	u32 ori_h;
	static bool last_enable_top1 = true;
	static u32 last_py_level = PY_LEVEL_INVALID;
	u32 cfg_enable_top1 = 0;
	bool bypass_pr = false;

	if (vf) {
		if (vf->type & VIDTYPE_COMPRESS) {
			if (is_src_crop_valid(vf->src_crop)) {
				w = vf->compWidth -
					vf->src_crop.left - vf->src_crop.right;
				h = vf->compHeight -
					vf->src_crop.top - vf->src_crop.bottom;
			} else {
				w = vf->compWidth;
				h = vf->compHeight;
			}
			ori_w = vf->compWidth;
			ori_h = vf->compHeight;
		} else {
			w = vf->width;
			h = vf->height;
			ori_w = vf->width;
			ori_h = vf->height;
		}
		cfg_enable_top1 = check_cfg_enabled_top1();
		if (cfg_enable_top1) {
			get_top1_vd_info(vf, &top1_vd_info);
			if (check_top1_scale(w, h)) {
				if (debug_dolby & 0x80000)
					pr_dv_dbg("need top1 mif scale, %dx%d\n",
						top1_vd_info.width, top1_vd_info.height);
				top1_scale = 1;
			} else {
				top1_scale = 0;
			}
			update_num_downsamplers(w, h);
			vf->src_fmt.downsamplers = num_downsamplers;
			enable_top1 = true;/*cur cfg enable pyramid*/
		} else {
			enable_top1 = false;
		}

		if (force_top1_enable == 1) {
			if (cfg_enable_top1 == CFG_NONE) {
				if (debug_dolby & 1)
					pr_dv_dbg("warning: cfg not enable top1, not allow force enable!\n");
			} else {
				enable_top1 = true;
			}
		} else if (force_top1_enable == 2) {
			enable_top1 = false;
		}

		if (enable_top1) {
			if (top1_vd_info.width > 512 && top1_vd_info.height > 288 && !bypass_pr) {
				vf->src_fmt.py_level = PY_SEVEN_LEVEL;
				if (top1_vd_info.width >> top1_scale >= 512 &&
					top1_vd_info.height >> top1_scale > 288)
					vf->src_fmt.py_level = PY_SEVEN_LEVEL;
				else
					vf->src_fmt.py_level = PY_SIX_LEVEL;
			} else if (top1_vd_info.width > 256 && top1_vd_info.height > 144 &&
				!bypass_pr) {
				vf->src_fmt.py_level = PY_SIX_LEVEL;
			} else {
				vf->src_fmt.py_level = PY_NO_LEVEL;
			}

			/*make sure not odd res for top1*/
			if (((top1_vd_info.width >> top1_scale) % 2) ||
				((top1_vd_info.height >> top1_scale) % 2)) {
				vf->src_fmt.py_level = PY_NO_LEVEL;
				if (debug_dolby & 0x80000)
					pr_dv_dbg("top1 size %dx%d not align, bypass precision\n",
					top1_vd_info.width, top1_vd_info.height);
			} else if (w <= h || ori_w <= ori_h) {/*hw bug, need disable top1*/
				vf->src_fmt.py_level = PY_NO_LEVEL;
			}
		} else {
			vf->src_fmt.py_level = PY_NO_LEVEL;
		}
		top1_info.py_level = vf->src_fmt.py_level;

		if (debug_dolby & 0x80000)
			pr_dv_dbg("update enable_top1 %d %d,level %s,last %s,wxh: %dx%d %dx%d,ds %d\n",
				cfg_enable_top1, enable_top1,
				level_str[vf->src_fmt.py_level],
				level_str[last_py_level],
				w, h, ori_w, ori_h, num_downsamplers);

		if (last_enable_top1 != enable_top1) {
			if (debug_dolby & 0x80000)
				pr_dv_dbg("enable_top1 status changed %d=>%d\n",
					last_enable_top1, enable_top1);
			last_enable_top1 = enable_top1;

		}

		if (cfg_enable_top1) {
			if (!enable_top1 ||
				(last_py_level != vf->src_fmt.py_level &&
				vf->src_fmt.py_level == PY_NO_LEVEL)) {
				if (debug_dolby & 0x80000)
					pr_dv_dbg("top1 py_level changed off %s->%s\n",
					level_str[last_py_level],
					level_str[vf->src_fmt.py_level]);
			} else if (enable_top1 && last_py_level != vf->src_fmt.py_level &&
				last_py_level == PY_NO_LEVEL) {
				if (debug_dolby & 0x80000)
					pr_dv_dbg("top1 py_level changed on %s->%s\n",
					level_str[last_py_level],
					level_str[vf->src_fmt.py_level]);
			}
		}
		last_py_level = vf->src_fmt.py_level;
	}
}

/*val=0: top1 disabled*/
/*val=1: top1 enabled but no need get one more frame in advance*/
/*val=3: top1 enabled and need get one more frame in advance*/
u32 get_top1_onoff(void)
{
	u32 val = 0;

	/*todo for precision disabled but L1L4 enable*/

	if (is_amdv_on() && is_aml_hw5() && enable_top1) {
		val = 3;
		if (cfg_info[cur_pic_mode].bypass_pd_from_user)
			val = 1;/*remove bit1*/
	}
	return val;
}

static bool prepare_parser(int reset_flag, struct video_inst_s *v_inst_info)
{
	bool parser_ready = false;

	if (!v_inst_info->metadata_parser) {
		if (p_funcs_tv && p_funcs_tv->multi_mp_init) {
			v_inst_info->metadata_parser =
			p_funcs_tv->multi_mp_init
				(dolby_vision_flags
				 & FLAG_CHANGE_SEQ_HEAD
				 ? 1 : 0);
		p_funcs_tv->multi_mp_reset(v_inst_info->metadata_parser, 1);
		} else {
			pr_dv_dbg("p_funcs is null\n");
		}
		if (v_inst_info->metadata_parser) {
			parser_ready = true;
			if (debug_dolby & 1)
				pr_dv_dbg("metadata parser init OK\n");
		}
	} else {
		//} else if (p_funcs_tv && p_funcs_tv->multi_mp_reset) {
		//	if (p_funcs_tv->multi_mp_reset
		//	    (v_inst_info->metadata_parser,
		//		reset_flag | metadata_parser_reset_flag) == 0)
		//		metadata_parser_reset_flag = 0;*/ //no need
		parser_ready = true;
	}
	return parser_ready;
}

void update_src_format_hw5(enum signal_format_enum src_format, struct vframe_s *vf)
{
	enum signal_format_enum cur_format = top2_v_info.amdv_src_format;

	if (src_format == FORMAT_DOVI ||
		src_format == FORMAT_DOVI_LL) {
		top2_v_info.amdv_src_format = 3;
	} else {
		if (vf) {
			if (is_cuva_frame(vf)) {
				if ((signal_transfer_characteristic == 14 ||
				     signal_transfer_characteristic == 18) &&
				    signal_color_primaries == 9)
					top2_v_info.amdv_src_format = 9;
				else if (signal_transfer_characteristic == 16)
					top2_v_info.amdv_src_format = 8;
			} else if (is_primesl_frame(vf)) {
				/* need check prime_sl before hdr and sdr */
				top2_v_info.amdv_src_format = 4;
			} else if (vf_is_hdr10_plus(vf)) {
				top2_v_info.amdv_src_format = 2;
			} else if (vf_is_hdr10(vf)) {
				top2_v_info.amdv_src_format = 1;
			} else if (vf_is_hlg(vf)) {
				top2_v_info.amdv_src_format = 5;
			} else if (is_mvc_frame(vf)) {
				top2_v_info.amdv_src_format = 7;
			} else {
				top2_v_info.amdv_src_format = 6;
			}
		}
	}
	if (cur_format != top2_v_info.amdv_src_format) {
		update_pwm_control();
		if ((debug_dolby & 1) || (debug_dolby & 0x100))
			pr_dv_dbg
			("update src fmt: %s => %s,signal_type 0x%x,src fmt %d\n",
			input_str[cur_format],
			input_str[top2_v_info.amdv_src_format],
			vf ? vf->signal_type : 0,
			src_format);
		cur_format = top2_v_info.amdv_src_format;
	}
}

/*id=0=>top1 instance paser, id=1=>top2 instance parser*/
int parse_sei_and_meta_ext_hw5(struct vframe_s *vf,
					 char *aux_buf,
					 int aux_size,
					 int *total_comp_size,
					 int *total_md_size,
					 void *fmt,
					 int *ret_flags,
					 char *md_buf,
					 char *comp_buf,
					 int id)
{
	int i;
	char *p;
	unsigned int size = 0u;
	unsigned int type = 0;
	int md_size = 0;
	int comp_size = 0;
	int ret = 2;
	bool parser_overflow = false;
	int rpu_ret = 0;
	u32 reset_flag = 0;
	unsigned int rpu_size = 0;
	enum signal_format_enum *src_format = (enum signal_format_enum *)fmt;
	static int parse_process_count;
	char meta_buf[1024];
	struct video_inst_s *v_inst_info;

	if (id == 0) /*top1*/
		v_inst_info = &top1_v_info;
	else
		v_inst_info = &top2_v_info;

	if (!aux_buf || aux_size == 0 || !fmt || !md_buf || !comp_buf ||
	    !total_comp_size || !total_md_size || !ret_flags)
		return 1;

	parse_process_count++;
	if (parse_process_count > 1) {
		pr_err("parser not support multi instance\n");
		ret = 1;
		goto parse_err;
	}
	if (!p_funcs_tv) {
		ret = 1;
		goto parse_err;
	}

	/* release metadata_parser when new playing */
	if (vf && vf->src_fmt.play_id != v_inst_info->last_play_id) {
		if (top1_v_info.metadata_parser) {
			//if (p_funcs_tv && p_funcs_tv->multi_mp_release)/*idk5.1*/
			//	p_funcs_tv->multi_mp_release(&top1_v_info.metadata_parser);
			//top1_v_info.metadata_parser = NULL;

			if (p_funcs_tv && p_funcs_tv->multi_mp_reset)/*idk5.1*/
				p_funcs_tv->multi_mp_reset(top1_v_info.metadata_parser, 1);
		}
		if (top2_v_info.metadata_parser) {
			//if (p_funcs_tv && p_funcs_tv->multi_mp_release)/*idk5.1*/
			//	p_funcs_tv->multi_mp_release(&top2_v_info.metadata_parser);
			//top2_v_info.metadata_parser = NULL;

			if (p_funcs_tv && p_funcs_tv->multi_mp_reset)/*idk5.1*/
				p_funcs_tv->multi_mp_reset(top2_v_info.metadata_parser, 1);
			amdv_clear_buf(0);
			if (debug_dolby & 0x100)
				pr_dv_dbg("new play, release parser\n");
		}
		v_inst_info->last_play_id = vf->src_fmt.play_id;
		if (debug_dolby & 2)
			pr_dv_dbg("update play id=%d:\n", v_inst_info->last_play_id);
	}
	p = aux_buf;
	while (p < aux_buf + aux_size - 8) {
		size = *p++;
		size = (size << 8) | *p++;
		size = (size << 8) | *p++;
		size = (size << 8) | *p++;
		type = *p++;
		type = (type << 8) | *p++;
		type = (type << 8) | *p++;
		type = (type << 8) | *p++;
		if (debug_dolby & 4)
			pr_dv_dbg("metadata type=%08x, size=%d:\n",
				     type, size);
		if (size == 0 || size > aux_size) {
			pr_dv_dbg("invalid aux size %d\n", size);
			ret = 1;
			goto parse_err;
		}
		if (type == DV_SEI || /* hevc t35 sei */
			(type & 0xffff0000) == AV1_SEI) { /* av1 t35 obu */
			*total_comp_size = 0;
			*total_md_size = 0;

			if ((type & 0xffff0000) == AV1_SEI &&
			    p[0] == 0xb5 &&
			    p[1] == 0x00 &&
			    p[2] == 0x3b &&
			    p[3] == 0x00 &&
			    p[4] == 0x00 &&
			    p[5] == 0x08 &&
			    p[6] == 0x00 &&
			    p[7] == 0x37 &&
			    p[8] == 0xcd &&
			    p[9] == 0x08) {
				/* AV1 dv meta in obu */
				*src_format = FORMAT_DOVI;
				meta_buf[0] = 0;
				meta_buf[1] = 0;
				meta_buf[2] = 0;
				meta_buf[3] = 0x01;
				meta_buf[4] = 0x19;
				if (p[11] & 0x10) {
					rpu_size = 0x100;
					rpu_size |= (p[11] & 0x0f) << 4;
					rpu_size |= (p[12] >> 4) & 0x0f;
					if (p[12] & 0x08) {
						pr_dv_error
							("rpu in obu exceed 512 bytes\n");
						break;
					}
					for (i = 0; i < rpu_size; i++) {
						meta_buf[5 + i] =
							(p[12 + i] & 0x07) << 5;
						meta_buf[5 + i] |=
							(p[13 + i] >> 3) & 0x1f;
					}
					rpu_size += 5;
				} else {
					rpu_size = (p[10] & 0x1f) << 3;
					rpu_size |= (p[11] >> 5) & 0x07;
					for (i = 0; i < rpu_size; i++) {
						meta_buf[5 + i] =
							(p[11 + i] & 0x0f) << 4;
						meta_buf[5 + i] |=
							(p[12 + i] >> 4) & 0x0f;
					}
					rpu_size += 5;
				}
			} else {
				/* HEVC dv meta in sei */
				*src_format = FORMAT_DOVI;
				if (size > (sizeof(meta_buf) - 3))
					size = (sizeof(meta_buf) - 3);
				meta_buf[0] = 0;
				meta_buf[1] = 0;
				meta_buf[2] = 0;
				memcpy(&meta_buf[3], p + 1, size - 1);
				rpu_size = size + 2;
			}
			if ((debug_dolby & 4) && dump_enable_f(0)) {
				pr_dv_dbg("metadata(%d):\n", rpu_size);
				for (i = 0; i < size; i += 16)
					pr_info("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
						meta_buf[i],
						meta_buf[i + 1],
						meta_buf[i + 2],
						meta_buf[i + 3],
						meta_buf[i + 4],
						meta_buf[i + 5],
						meta_buf[i + 6],
						meta_buf[i + 7],
						meta_buf[i + 8],
						meta_buf[i + 9],
						meta_buf[i + 10],
						meta_buf[i + 11],
						meta_buf[i + 12],
						meta_buf[i + 13],
						meta_buf[i + 14],
						meta_buf[i + 15]);
			}

			/* prepare metadata parser */
			if (!prepare_parser(reset_flag, v_inst_info)) {
				pr_dv_error
				("meta(%d), pts(%lld) -> parser init fail\n",
					rpu_size, vf->pts_us64);
				ret = 1;
				goto parse_err;
			}

			md_size = 0;
			comp_size = 0;

			if (p_funcs_tv && p_funcs_tv->multi_mp_process)
				rpu_ret =
				p_funcs_tv->multi_mp_process
				(v_inst_info->metadata_parser, meta_buf, rpu_size,
				 comp_buf + *total_comp_size,
				 &comp_size, md_buf + *total_md_size,
				 &md_size, true, DV_TYPE_DOVI);
			if (rpu_ret < 0) {
				if (vf)
					pr_dv_error
					("meta(%d), pts(%lld) -> metadata parser process fail\n",
					 rpu_size, vf->pts_us64);
				ret = 3;
			} else {
				if (*total_comp_size + comp_size
					< COMP_BUF_SIZE)
					*total_comp_size += comp_size;
				else
					parser_overflow = true;

				if (*total_md_size + md_size
					< MD_BUF_SIZE)
					*total_md_size += md_size;
				else
					parser_overflow = true;
				if (rpu_ret == 1)
					*ret_flags = 1;
				ret = 0;
			}
			if (parser_overflow) {
				ret = 2;
				break;
			}
			/*dv type just appears once in metadata
			 *after parsing dv type,breaking the
			 *circulation directly
			 */
			break;
		}
		p += size;
	}

	/*continue to check atsc/dvb dv */
	if (*src_format != FORMAT_DOVI) {
		struct dv_vui_parameters vui_param = {0};
		static u32 len_2086_sei;
		u32 len_2094_sei = 0;
		static u8 payload_2086_sei[MAX_LEN_2086_SEI];
		u8 payload_2094_sei[MAX_LEN_2094_SEI];
		unsigned char nal_type;
		unsigned char sei_payload_type = 0;
		unsigned char sei_payload_size = 0;

		if (vf) {
			vui_param.video_fmt_i = (vf->signal_type >> 26) & 7;
			vui_param.video_fullrange_b = (vf->signal_type >> 25) & 1;
			vui_param.color_description_b = (vf->signal_type >> 24) & 1;
			vui_param.color_primaries_i = (vf->signal_type >> 16) & 0xff;
			vui_param.trans_characteristic_i =
							(vf->signal_type >> 8) & 0xff;
			vui_param.matrix_coeff_i = (vf->signal_type) & 0xff;
			if (debug_dolby & 2)
				pr_dv_dbg("vui_param %d, %d, %d, %d, %d, %d\n",
					vui_param.video_fmt_i,
					vui_param.video_fullrange_b,
					vui_param.color_description_b,
					vui_param.color_primaries_i,
					vui_param.trans_characteristic_i,
					vui_param.matrix_coeff_i);
		}
		p = aux_buf;
		if ((debug_dolby & 0x200) && dump_enable_f(0)) {
			pr_dv_dbg("aux_buf(%d):\n", aux_size);
			for (i = 0; i < aux_size; i += 8)
				pr_info("\t%02x %02x %02x %02x %02x %02x %02x %02x\n",
					p[i],
					p[i + 1],
					p[i + 2],
					p[i + 3],
					p[i + 4],
					p[i + 5],
					p[i + 6],
					p[i + 7]);
		}
		while (p < aux_buf + aux_size - 8) {
			size = *p++;
			size = (size << 8) | *p++;
			size = (size << 8) | *p++;
			size = (size << 8) | *p++;
			type = *p++;
			type = (type << 8) | *p++;
			type = (type << 8) | *p++;
			type = (type << 8) | *p++;
			if (debug_dolby & 2)
				pr_dv_dbg("type: 0x%x\n", type);

			/*4 byte size + 4 byte type*/
			/*1 byte nal_type + 1 byte (layer_id+temporal_id)*/
			/*1 byte payload type + 1 byte size + payload data*/
			if (type == 0x02000000) {
				nal_type = ((*p) & 0x7E) >> 1; /*nal unit type*/
				if (debug_dolby & 2)
					pr_dv_dbg("nal_type: %d\n",
						     nal_type);

				if (nal_type == PREFIX_SEI_NUT_NAL ||
					nal_type == SUFFIX_SEI_NUT_NAL) {
					sei_payload_type = *(p + 2);
					sei_payload_size = *(p + 3);
					if (debug_dolby & 2)
						pr_dv_dbg("type %d, size %d\n",
							     sei_payload_type,
							     sei_payload_size);
					if (sei_payload_type ==
					SEI_TYPE_MASTERING_DISP_COLOUR_VOLUME) {
						len_2086_sei =
							sei_payload_size;
						memcpy(payload_2086_sei, p + 4,
						       len_2086_sei);
					} else if (sei_payload_type == SEI_ITU_T_T35 &&
						sei_payload_size >= 8 && check_atsc_dvb(p)) {
						len_2094_sei = sei_payload_size;
						memcpy(payload_2094_sei, p + 4,
						       len_2094_sei);
					}
					if (len_2086_sei > 0 &&
					    len_2094_sei > 0)
						break;
				}
			}
			p += size;
		}
		if (len_2094_sei > 0) {
			/* source is VS10 */
			*total_comp_size = 0;
			*total_md_size = 0;
			*src_format = FORMAT_DOVI;
			p_atsc_md.vui_param = vui_param;
			p_atsc_md.len_2086_sei = len_2086_sei;
			memcpy(p_atsc_md.sei_2086, payload_2086_sei,
			       len_2086_sei);
			p_atsc_md.len_2094_sei = len_2094_sei;
			memcpy(p_atsc_md.sei_2094, payload_2094_sei,
			       len_2094_sei);
			size = sizeof(struct dv_atsc);
			if (size > sizeof(meta_buf))
				size = sizeof(meta_buf);
			memcpy(meta_buf, (unsigned char *)(&p_atsc_md), size);
			if ((debug_dolby & 4) && dump_enable_f(0)) {
				pr_dv_dbg("metadata(%d):\n", size);
				for (i = 0; i < size; i += 8)
					pr_info("\t%02x %02x %02x %02x %02x %02x %02x %02x\n",
						meta_buf[i],
						meta_buf[i + 1],
						meta_buf[i + 2],
						meta_buf[i + 3],
						meta_buf[i + 4],
						meta_buf[i + 5],
						meta_buf[i + 6],
						meta_buf[i + 7]);
			}
			/* prepare metadata parser */
			reset_flag = 2; /*flag: bit0 flag, bit1 0->dv, 1->atsc*/
			if (!prepare_parser(reset_flag, v_inst_info)) {
				if (vf)
					pr_dv_error
					("meta(%d), pts(%lld) -> metadata parser init fail\n",
					 size, vf->pts_us64);
				ret = 1;
				goto parse_err;
			}

			md_size = 0;
			comp_size = 0;

			if (p_funcs_tv && p_funcs_tv->multi_mp_process)
				rpu_ret =
				p_funcs_tv->multi_mp_process
				(v_inst_info->metadata_parser, meta_buf, size,
				comp_buf + *total_comp_size,
				&comp_size, md_buf + *total_md_size,
				&md_size, true, DV_TYPE_ATSC);

			if (rpu_ret < 0) {
				if (vf)
					pr_dv_error
					("meta(%d), pts(%lld) -> metadata parser process fail\n",
					size, vf->pts_us64);
				ret = 3;
			} else {
				if (*total_comp_size + comp_size
					< COMP_BUF_SIZE)
					*total_comp_size += comp_size;
				else
					parser_overflow = true;

				if (*total_md_size + md_size
					< MD_BUF_SIZE)
					*total_md_size += md_size;
				else
					parser_overflow = true;
				if (rpu_ret == 1)
					*ret_flags = 1;
				ret = 0;
			}
			if (parser_overflow)
				ret = 2;
		} else {
			len_2086_sei = 2;
		}
	}

	if (*total_md_size) {
		if ((debug_dolby & 1) && vf)
			pr_dv_dbg
			("meta(%d), pts(%lld) -> md(%d), comp(%d)\n",
			 size, vf->pts_us64,
			 *total_md_size, *total_comp_size);
		if ((debug_dolby & 4) && dump_enable_f(0))  {
			pr_dv_dbg("parsed md(%d):\n", *total_md_size);
			for (i = 0; i < *total_md_size + 7; i += 8) {
				pr_info("%02x %02x %02x %02x %02x %02x %02x %02x\n",
					md_buf[i],
					md_buf[i + 1],
					md_buf[i + 2],
					md_buf[i + 3],
					md_buf[i + 4],
					md_buf[i + 5],
					md_buf[i + 6],
					md_buf[i + 7]);
			}
		}
	}
parse_err:
	parse_process_count--;
	return ret;
}

int parse_sei_and_meta_hw5(struct vframe_s *vf,
				      void *frame_req,
				      int *total_comp_size,
				      int *total_md_size,
				      enum signal_format_enum *src_format,
				      int *ret_flags, bool drop_flag, u32 inst_id)
{
	int ret = 1;
	char *p_md_buf;
	char *p_comp_buf;
	int next_id;
	struct provider_aux_req_s *req = (struct provider_aux_req_s *)frame_req;
	struct video_inst_s *v_inst_info;

	if (inst_id == 0) /*top1*/
		v_inst_info = &top1_v_info;
	else
		v_inst_info = &top2_v_info;

	if (!req->aux_buf || req->aux_size == 0 || !is_aml_hw5())
		return 1;

	next_id = v_inst_info->current_id ^ 1;
	p_md_buf = v_inst_info->md_buf[next_id];
	p_comp_buf = v_inst_info->comp_buf[next_id];

	ret = parse_sei_and_meta_ext_hw5(vf,
				     req->aux_buf,
				     req->aux_size,
				     total_comp_size,
				     total_md_size,
				     src_format,
				     ret_flags,
				     p_md_buf,
				     p_comp_buf,
				     inst_id);

	if (ret == 0) {
		v_inst_info->current_id = next_id;
		v_inst_info->last_total_comp_size = *total_comp_size;
		v_inst_info->last_total_md_size = *total_md_size;
	} else { /*parser err, use backup md and comp*/
		*total_comp_size = v_inst_info->last_total_comp_size;
		*total_md_size = v_inst_info->last_total_md_size;
	}

	return ret;
}

void set_vf_crc_valid_top1(int val)
{
	last_vf_valid_crc_top1 = val;
}

/*for hdmi cert: not run cp when vf not change*/
/*sometimes  even if frame is same but  frame crc changed, maybe ucd or rx not stable,*/
/*so need to check  few more frames*/
bool check_vf_changed_top1(struct vframe_s *vf)
{
//#define MAX_VF_CRC_CHECK_COUNT_TOP1 0

	static u32 new_vf_crc;
	static u32 vf_crc_repeat_cnt;
	bool changed = false;

	if (!vf)
		return true;

	if (debug_dolby & 1)
		pr_dv_dbg("top1 vf %px, crc %x, last valid crc %x, rpt %d\n",
				vf, vf->crc, last_vf_valid_crc_top1,
				vf_crc_repeat_cnt);

	//vf->src_fmt.hdmi_new_frame = false;

	if (vf->crc == 0) {
		/*invalid crc, maybe vdin dropped last frame*/
		return changed;
	}
	if (last_vf_valid_crc_top1 == 0) {
		last_vf_valid_crc_top1 = vf->crc;
		changed = true;
		pr_dv_dbg("top1 first new frame\n");
	} else if (vf->crc != last_vf_valid_crc_top1) {
		if (vf->crc != new_vf_crc)
			vf_crc_repeat_cnt = 0;
		else
			++vf_crc_repeat_cnt;

		//if (vf_crc_repeat_cnt >= MAX_VF_CRC_CHECK_COUNT_TOP1) {
		vf_crc_repeat_cnt = 0;
		last_vf_valid_crc_top1 = vf->crc;
		changed = true;
		pr_dv_dbg("top1 new frame\n");
		//}
	} else {
		vf_crc_repeat_cnt = 0;
	}
	new_vf_crc = vf->crc;
	return changed;
}

/*for top1 parser + controlpath*/
int amdv_parse_metadata_hw5_top1(struct vframe_s *vf)
{
	const struct vinfo_s *vinfo = get_current_vinfo();
	struct provider_aux_req_s req;
	struct provider_aux_req_s el_req;
	int flag = 0;
	enum signal_format_enum src_format = FORMAT_SDR;
	int total_md_size = 0;
	int total_comp_size = 0;
	u32 w = 0xffff;
	u32 h = 0xffff;
	int meta_flag_bl = 1;
	int src_chroma_format = 0;
	int src_bdp = 12;
	bool video_frame = false;
	int i;
	struct vframe_master_display_colour_s *p_mdc;
	unsigned int current_mode = dolby_vision_mode;
	enum input_mode_enum input_mode = IN_MODE_OTT;
	int ret_flags = 0;
	int ret = -1;
	bool mel_flag = false;
	int vsem_size = 0;
	int vsem_if_size = 0;
	bool dump_emp = false;
	bool dv_vsem = false;
	bool hdr10_src_primary_changed = false;
	unsigned long time_use = 0;
	struct timeval start;
	struct timeval end;
	char *pic_mode;
	bool run_control_path = true;
	bool vf_changed = true;
	struct dynamic_cfg_s *p_ambient = NULL;
	u32 cur_id = 0;
	struct video_inst_s *v_inst_info = &top1_v_info;
	u32 new_w;
	u32 new_h;

	if (!p_funcs_tv || !p_funcs_tv->tv_hw5_control_path_analyzer || !tv_hw5_setting)
		return -1;

	if (vf) {
		update_top1_onoff(vf);
		vf_tmp = vf;
	}

	if (!enable_top1)
		return -1;

	memset(&req, 0, (sizeof(struct provider_aux_req_s)));
	memset(&el_req, 0, (sizeof(struct provider_aux_req_s)));

	if (vf) {
		video_frame = true;
		if (vf->type & VIDTYPE_COMPRESS) {
			if (is_src_crop_valid(vf->src_crop)) {
				w = vf->compWidth -
					vf->src_crop.left - vf->src_crop.right;
				h = vf->compHeight -
					vf->src_crop.top - vf->src_crop.bottom;
			} else {
				w = vf->compWidth;
				h = vf->compHeight;
			}
		} else {
			w = vf->width;
			h = vf->height;
		}
	}

	if (is_aml_tvmode() && vf &&
	    vf->source_type == VFRAME_SOURCE_TYPE_HDMI) {
		req.vf = vf;
		req.bot_flag = 0;
		req.aux_buf = NULL;
		req.aux_size = 0;
		req.dv_enhance_exist = 0;
		req.low_latency = 0;
		vf_notify_provider_by_name("dv_vdin",
					   VFRAME_EVENT_RECEIVER_GET_AUX_DATA,
					   (void *)&req);
		input_mode = IN_MODE_HDMI;

		if ((dolby_vision_flags & FLAG_CERTIFICATION) && enable_vf_check)
			vf_changed = check_vf_changed_top1(vf);

		/* meta */
		if ((dolby_vision_flags & FLAG_RX_EMP_VSEM) &&
			vf->emp.size > 0) {
			vsem_size = vf->emp.size * VSEM_PKT_SIZE;
			memcpy(vsem_if_buf, vf->emp.addr, vsem_size);
			if (vsem_if_buf[0] == 0x7f &&
			    vsem_if_buf[10] == 0x46 &&
			    vsem_if_buf[11] == 0xd0) {
				dv_vsem = true;
				if (!vsem_check(vsem_if_buf, vsem_md_buf)) {
					vsem_if_size = vsem_size;
					if (!vsem_md_buf[10]) {
						req.aux_buf =
							&vsem_md_buf[13];
						req.aux_size =
							(vsem_md_buf[5] << 8)
							+ vsem_md_buf[6]
							- 6 - 4;
						/* cancel vsem, use md */
						/* vsem_if_size = 0; */
					} else {
						req.low_latency = 1;
					}
				} else {
					/* emp error, use previous md */
					pr_dv_dbg("EMP packet error %d\n",
						vf->emp.size);
					dump_emp = true;
					vsem_if_size = 0;
					req.aux_buf = NULL;
					req.aux_size = v_inst_info->last_total_md_size;
				}
			} else if (debug_dolby & 4) {
				pr_dv_dbg("EMP packet not DV vsem %d\n",
					vf->emp.size);
				dump_emp = true;
			}
			if ((debug_dolby & 4) || dump_emp) {
				pr_info("vsem pkt count = %d\n", vf->emp.size);
				for (i = 0; i < vsem_size; i += 8) {
					pr_info("%02x %02x %02x %02x %02x %02x %02x %02x\n",
						vsem_if_buf[i],	vsem_if_buf[i + 1],
						vsem_if_buf[i + 2],	vsem_if_buf[i + 3],
						vsem_if_buf[i + 4],	vsem_if_buf[i + 5],
						vsem_if_buf[i + 6],	vsem_if_buf[i + 7]);
				}
			}
		}

		/* w/t vsif and no dv_vsem */
		if (vf->vsif.size && !dv_vsem) {
			memset(vsem_if_buf, 0, VSEM_IF_BUF_SIZE);
			memcpy(vsem_if_buf, vf->vsif.addr, vf->vsif.size);
			vsem_if_size = vf->vsif.size;
			if (debug_dolby & 4) {
				pr_info("vsif size = %d\n", vf->vsif.size);
				for (i = 0; i < vsem_if_size; i += 8) {
					pr_info("%02x %02x %02x %02x %02x %02x %02x %02x\n",
						vsem_if_buf[i],	vsem_if_buf[i + 1],
						vsem_if_buf[i + 2],	vsem_if_buf[i + 3],
						vsem_if_buf[i + 4],	vsem_if_buf[i + 5],
						vsem_if_buf[i + 6],	vsem_if_buf[i + 7]);
				}
			}
		}
		if ((debug_dolby & 1) && (dv_vsem || vsem_if_size))
			pr_dv_dbg("vdin get %s:%d, md:%p %d,ll:%d,bit %x,type %x\n",
				dv_vsem ? "vsem" : "vsif",
				dv_vsem ? vsem_size : vsem_if_size,
				req.aux_buf, req.aux_size,
				req.low_latency,
				vf->bitdepth, vf->source_type);

		/*check vsem_if_buf */
		if (vsem_if_size &&
			vsem_if_buf[0] != 0x81) {
			/*not vsif, continue to check vsem*/
			if (!(vsem_if_buf[0] == 0x7F &&
				vsem_if_buf[1] == 0x80 &&
				vsem_if_buf[10] == 0x46 &&
				vsem_if_buf[11] == 0xd0 &&
				vsem_if_buf[12] == 0x00)) {
				vsem_if_size = 0;
				pr_dv_dbg("vsem_if_buf is invalid!\n");
				pr_dv_dbg("%x %x %x %x %x %x %x %x %x %x %x %x\n",
					vsem_if_buf[0], vsem_if_buf[1], vsem_if_buf[2],
					vsem_if_buf[3],	vsem_if_buf[4],	vsem_if_buf[5],
					vsem_if_buf[6],	vsem_if_buf[7],	vsem_if_buf[8],
					vsem_if_buf[9],	vsem_if_buf[10], vsem_if_buf[11]);
			}
		}
		src_chroma_format = 2;/*fixed CF_P444 for hdmi dw data*/;
		if ((dolby_vision_flags & FLAG_FORCE_DV_LL) ||
		    req.low_latency == 1) {
			src_format = FORMAT_DOVI_LL;
			for (i = 0; i < 2; i++) {
				if (v_inst_info->md_buf[i])
					memset(v_inst_info->md_buf[i], 0, MD_BUF_SIZE);
				if (v_inst_info->comp_buf[i])
					memset(v_inst_info->comp_buf[i], 0, COMP_BUF_SIZE);
			}
			req.aux_size = 0;
			req.aux_buf = NULL;
		} else if (req.aux_size) {
			if (req.aux_buf) {
				v_inst_info->current_id = v_inst_info->current_id ^ 1;
				memcpy(v_inst_info->md_buf[v_inst_info->current_id],
				       req.aux_buf, req.aux_size);
			}
			src_format = FORMAT_DOVI;
		} else {
			src_format =  tv_hw5_setting->top1.src_format;
			p_mdc =	&vf->prop.master_display_colour;
			if (is_hdr10_frame(vf) || force_hdmin_fmt == 1) {
				src_format = FORMAT_HDR10;
				/* prepare parameter from hdmi for hdr10 */
				p_mdc->luminance[0] *= 10000;
				prepare_hdr10_param
					(p_mdc, &v_inst_info->hdr10_param);
				req.dv_enhance_exist = 0;
				src_bdp = 10;
			}
			if (src_format != FORMAT_DOVI &&
				(is_hlg_frame(vf) || force_hdmin_fmt == 2)) {
				src_format = FORMAT_HLG;
				src_bdp = 10;
			}
			if (src_format == FORMAT_SDR && force_sdr10 == 1)
				src_format = FORMAT_SDR10;

			if (src_format == FORMAT_SDR10)
				src_bdp = 10;
		}
		if ((debug_dolby & 4) && req.aux_size) {
			pr_dv_dbg("metadata(%d):\n", req.aux_size);
			for (i = 0; i < req.aux_size + 8; i += 8)
				pr_info("\t%02x %02x %02x %02x %02x %02x %02x %02x\n",
					v_inst_info->md_buf[v_inst_info->current_id][i],
					v_inst_info->md_buf[v_inst_info->current_id][i + 1],
					v_inst_info->md_buf[v_inst_info->current_id][i + 2],
					v_inst_info->md_buf[v_inst_info->current_id][i + 3],
					v_inst_info->md_buf[v_inst_info->current_id][i + 4],
					v_inst_info->md_buf[v_inst_info->current_id][i + 5],
					v_inst_info->md_buf[v_inst_info->current_id][i + 6],
					v_inst_info->md_buf[v_inst_info->current_id][i + 7]);
		}
		total_md_size = req.aux_size;
		total_comp_size = 0;
		meta_flag_bl = 0;
		if (req.aux_buf && req.aux_size) {
			v_inst_info->last_total_md_size = total_md_size;
			v_inst_info->last_total_comp_size = total_comp_size;
		}
		if (debug_dolby & 1)
			pr_dv_dbg("top1 frame %d, %p, pts %lld, format: %s, type %x\n",
			v_inst_info->frame_count, vf, vf->pts_us64,
			(src_format == FORMAT_HDR10) ? "HDR10" :
			((src_format == FORMAT_DOVI) ? "DOVI" :
			((src_format == FORMAT_DOVI_LL) ? "DOVI_LL" :
			((src_format == FORMAT_HLG) ? "HLG" : "SDR"))),
			vf->type);

	} else if (vf && (vf->source_type == VFRAME_SOURCE_TYPE_OTHERS)) {
		enum vframe_signal_fmt_e fmt;

		input_mode = IN_MODE_OTT;
		req.vf = vf;
		req.bot_flag = 0;
		req.aux_buf = NULL;
		req.aux_size = 0;
		req.dv_enhance_exist = 0;

		/* check source format */
		fmt = get_vframe_src_fmt(vf);
		if ((fmt == VFRAME_SIGNAL_FMT_DOVI ||
		    fmt == VFRAME_SIGNAL_FMT_INVALID ||
		    vf->src_fmt.sei_magic_code != SEI_MAGIC_CODE) &&
		    !vf->discard_dv_data) {
			vf_notify_provider_by_name
				(dv_provider[0],
				 VFRAME_EVENT_RECEIVER_GET_AUX_DATA,
				  (void *)&req);
		}
		/* use callback aux date first, if invalid, use sei_ptr */
		if ((!req.aux_buf || !req.aux_size) &&
		    fmt == VFRAME_SIGNAL_FMT_DOVI) {
			u32 sei_size = 0;
			char *sei;

			if (debug_dolby & 2)
				pr_dv_dbg("top1:no aux %p %x, el %d from %s, use sei_ptr\n",
					     req.aux_buf, req.aux_size,
					     req.dv_enhance_exist, dv_provider[0]);

			sei = (char *)get_sei_from_src_fmt(vf, &sei_size);
			if (sei && sei_size) {
				req.aux_buf = sei;
				req.aux_size = sei_size;
				req.dv_enhance_exist =
					vf->src_fmt.dual_layer;
			}
		}
		if (debug_dolby & 1)
			pr_dv_dbg("top1:%s get vf %px(%d), fmt %d, aux %p %x, el %d, type %x\n",
				     dv_provider[0], vf, vf->discard_dv_data, fmt,
				     req.aux_buf, req.aux_size, req.dv_enhance_exist, vf->type);
		/* parse meta in base layer */
		ret = get_md_from_src_fmt(vf);
		if (ret == 1) { /*parse succeeded*/
			meta_flag_bl = 0;
			src_format = FORMAT_DOVI;
			v_inst_info->current_id = v_inst_info->current_id ^ 1;
			cur_id = v_inst_info->current_id;
			memcpy(v_inst_info->md_buf[cur_id],
			       vf->src_fmt.md_buf,
			       vf->src_fmt.md_size);
			memcpy(v_inst_info->comp_buf[cur_id],
			       vf->src_fmt.comp_buf,
			       vf->src_fmt.comp_size);
			total_md_size =  vf->src_fmt.md_size;
			total_comp_size =  vf->src_fmt.comp_size;
			ret_flags = vf->src_fmt.parse_ret_flags;

			v_inst_info->last_total_md_size = total_md_size;
			v_inst_info->last_total_comp_size = total_comp_size;

			if ((debug_dolby & 4) && dump_enable_f(0)) {
				pr_dv_dbg("get md_buf %p, size(%d):\n",
					vf->src_fmt.md_buf, vf->src_fmt.md_size);
				for (i = 0; i < total_md_size; i += 8)
					pr_info("%02x %02x %02x %02x %02x %02x %02x %02x\n",
						v_inst_info->md_buf[cur_id][i],
						v_inst_info->md_buf[cur_id][i + 1],
						v_inst_info->md_buf[cur_id][i + 2],
						v_inst_info->md_buf[cur_id][i + 3],
						v_inst_info->md_buf[cur_id][i + 4],
						v_inst_info->md_buf[cur_id][i + 5],
						v_inst_info->md_buf[cur_id][i + 6],
						v_inst_info->md_buf[cur_id][i + 7]);
			}
		} else {  /*no parse in advance or parse failed*/
			if (get_vframe_src_fmt(vf) ==
			    VFRAME_SIGNAL_FMT_HDR10PRIME)
				src_format = FORMAT_PRIMESL;
			else
				meta_flag_bl =
				parse_sei_and_meta_hw5
					(vf, &req,
					 &total_comp_size,
					 &total_md_size,
					 &src_format,
					 &ret_flags, false, 0);
		}
		if (ret_flags && req.dv_enhance_exist) {
			if (!strcmp(dv_provider[0], "dvbldec"))
				vf_notify_provider_by_name
				(dv_provider[0],
				 VFRAME_EVENT_RECEIVER_DOLBY_BYPASS_EL,
				 (void *)&req);
			amdv_el_disable = 1;
			pr_dv_dbg("bypass mel\n");
		}
		if (ret_flags == 1)
			mel_flag = true;
		if (!is_dv_standard_es(req.dv_enhance_exist,
				       ret_flags, w)) {
			src_format = FORMAT_SDR;
			/* dovi_setting.src_format = src_format; */
			total_comp_size = 0;
			total_md_size = 0;
			src_bdp = 10;
		}
		if (is_aml_tvmode() &&
			(req.dv_enhance_exist && !mel_flag)) {
			src_format = FORMAT_SDR;
			/* dovi_setting.src_format = src_format; */
			total_comp_size = 0;
			total_md_size = 0;
			src_bdp = 10;
			if (debug_dolby & 1)
				pr_dv_dbg("tv: bypass fel\n");
		}

		if (src_format != FORMAT_DOVI && is_primesl_frame(vf)) {
			src_format = FORMAT_PRIMESL;
			src_bdp = 10;
		}
		if (src_format != FORMAT_DOVI && is_hdr10_frame(vf)) {
			src_format = FORMAT_HDR10;
			/* prepare parameter from SEI for hdr10 */
			p_mdc =	&vf->prop.master_display_colour;
			prepare_hdr10_param(p_mdc, &v_inst_info->hdr10_param);
			/* for 962x with v1.4 or stb with v2.3 may use 12 bit */
			src_bdp = 10;
			req.dv_enhance_exist = 0;
		}

		if (src_format != FORMAT_DOVI && is_hlg_frame(vf)) {
			src_format = FORMAT_HLG;
			src_bdp = 10;
		}
		if (src_format != FORMAT_DOVI && is_hdr10plus_frame(vf))
			src_format = FORMAT_HDR10PLUS;

		if (src_format != FORMAT_DOVI && is_mvc_frame(vf))
			src_format = FORMAT_MVC;

		if (src_format != FORMAT_DOVI && is_cuva_frame(vf))
			src_format = FORMAT_CUVA;

		if (src_format == FORMAT_SDR && force_sdr10 == 1)
			src_format = FORMAT_SDR10;

		if (src_format == FORMAT_SDR10 && !req.dv_enhance_exist)
			src_bdp = 10;

		if (src_format == FORMAT_SDR &&	!req.dv_enhance_exist)
			src_bdp = 8;

		if (debug_dolby & 1)
			pr_info
			("top1:[%d,%lld,%d,%s,%d,%d]\n",
			 v_inst_info->frame_count, vf->pts_us64, src_bdp,
			 (src_format == FORMAT_HDR10) ? "HDR10" :
			 (src_format == FORMAT_DOVI ? "DOVI" :
			 (src_format == FORMAT_HLG ? "HLG" :
			 (src_format == FORMAT_HDR10PLUS ? "HDR10+" :
			 (src_format == FORMAT_CUVA ? "CUVA" :
			 (src_format == FORMAT_PRIMESL ? "PRIMESL" :
			 (req.dv_enhance_exist ? "DOVI (el meta)" : "SDR")))))),
			 req.aux_size, req.dv_enhance_exist);
		if (src_format != FORMAT_DOVI && !req.dv_enhance_exist)
			memset(&req, 0, sizeof(req));

		if (meta_flag_bl) {
			total_md_size = v_inst_info->last_total_md_size;
			total_comp_size = v_inst_info->last_total_comp_size;
			mel_flag = v_inst_info->last_mel_mode;
			if (debug_dolby & 2)
				pr_dv_dbg("update melFlag %d\n", mel_flag);
			meta_flag_bl = 0;
		}

		if (src_format != FORMAT_DOVI)
			mel_flag = 0;
	}
	if (src_format == FORMAT_DOVI && meta_flag_bl) {
		/* dovi frame no meta or meta error */
		/* use old setting for this frame   */
		pr_dv_dbg("no meta or meta err!\n");
		return -1;
	}

	current_mode = dolby_vision_mode;
	if (amdv_policy_process
		(vf, &current_mode, src_format)) {
		amdv_set_toggle_flag(1);
		if (debug_dolby & 1)
			pr_dv_dbg("[%s]output change from %d to %d(%px, %d)\n",
			     __func__, dolby_vision_mode, current_mode,
			     vf, src_format);
		amdv_target_mode = current_mode;
		dolby_vision_mode = current_mode;
	}
	if (vf && (debug_dolby & 8))
		pr_dv_dbg("top1 parse_metadata: vf %px(index %d), mode %d\n",
			      vf, vf->omx_index, dolby_vision_mode);

	if (dolby_vision_mode == AMDV_OUTPUT_MODE_BYPASS) {
		if (amdv_target_mode == AMDV_OUTPUT_MODE_BYPASS)
			amdv_wait_on = false;
		if (debug_dolby & 8)
			pr_dv_dbg("now bypass mode, target %d, wait %d\n",
				  amdv_target_mode,
				  amdv_wait_on);
		if (get_hdr_module_status(VD1_PATH, VPP_TOP0) == HDR_MODULE_BYPASS)
			return 1;
		return -1;
	}

	if (src_format != tv_hw5_setting->top1.src_format ||
		(dolby_vision_flags & FLAG_CERTIFICATION)) {
		pq_config_set_flag = false;
		best_pq_config_set_flag = false;
	}
	if (!pq_config_set_flag) {
		if (debug_dolby & 2)
			pr_dv_dbg("update def_tgt_display_cfg\n");
		if (!get_load_config_status()) {
			memcpy(&(((struct pq_config_dvp *)pq_config_dvp_fake_top1)->tdc),
			       &def_tgt_dvp_cfg,
			       sizeof(def_tgt_dvp_cfg));//todo
		}
		pq_config_set_flag = true;
	}
	if (force_best_pq && !best_pq_config_set_flag) {
		pr_dv_dbg("update best def_tgt_display_cfg\n");
		//memcpy(&(((struct pq_config_dvp *)
		//	pq_config_dvp_fake)->tdc),
		//	&def_tgt_display_cfg_bestpq,
		//	sizeof(def_tgt_display_cfg_bestpq));//todo
		best_pq_config_set_flag = true;

		p_funcs_tv->tv_hw5_control_path_analyzer(invalid_hw5_setting);
	}
	calculate_panel_max_pq(src_format, vinfo,
			       &(((struct pq_config_dvp *)
			       pq_config_dvp_fake_top1)->tdc));

	((struct pq_config_dvp *)
		pq_config_dvp_fake_top1)->tdc.tuning_mode =
		amdv_tuning_mode;
	if (dolby_vision_flags & FLAG_DISABLE_COMPOSER) {
		((struct pq_config_dvp *)pq_config_dvp_fake_top1)
			->tdc.tuning_mode |=
			TUNING_MODE_EL_FORCE_DISABLE;
	} else {
		((struct pq_config_dvp *)pq_config_dvp_fake_top1)
			->tdc.tuning_mode &=
			(~TUNING_MODE_EL_FORCE_DISABLE);
	}
	if ((dolby_vision_flags & FLAG_CERTIFICATION) && sdr_ref_mode) {
		((struct pq_config_dvp *)
		pq_config_dvp_fake_top1)->tdc.ambient_config.ambient =
		0;
		((struct pq_config_dvp *)pq_config_dvp_fake_top1)
			->tdc.ref_mode_dark_id = 0;
	}
	if (is_hdr10_src_primary_changed()) {
		hdr10_src_primary_changed = true;
		pr_dv_dbg("hdr10 src primary changed!\n");
	}
	new_w = (top1_vd_info.width << num_downsamplers) >> top1_scale;
	new_h = (top1_vd_info.height << num_downsamplers) >> top1_scale;
	if (force_top1_vskip)
		new_h = new_h >> 1;
	if (src_format != tv_hw5_setting->top1.src_format ||
		tv_hw5_setting->top1.video_width != new_w ||
		tv_hw5_setting->top1.video_height != new_h ||
		hdr10_src_primary_changed) {
		if (debug_dolby & 0x100)
			pr_dv_dbg("reset control_path_analyze fmt %d->%d, w %d->%d, h %d->%d\n",
				tv_hw5_setting->top1.src_format, src_format,
				tv_hw5_setting->top1.video_width, new_w,
				tv_hw5_setting->top1.video_height, new_h);
		/*for hdmi in cert*/
		if (dolby_vision_flags & FLAG_CERTIFICATION)
			vf_changed = true;
		p_funcs_tv->tv_hw5_control_path_analyzer(invalid_hw5_setting);
	}
	pic_mode = get_cur_pic_mode_name();
	if (!(dolby_vision_flags & FLAG_CERTIFICATION) && pic_mode &&
	    (strstr(pic_mode, "dark") ||
	    strstr(pic_mode, "Dark") ||
	    strstr(pic_mode, "DARK"))) {
		memcpy(tv_input_info,
		       brightness_off,
		       sizeof(brightness_off));
		/*for HDR10/HLG, only has DM4, ko only use value from tv_input_info[3][1]*/
		/*and tv_input_info[4][1]. To avoid ko code changed, we reuse these*/
		/*parameter for both HDMI and OTT mode, that means need copy HDR10 to */
		/*tv_input_info[3][1] and copy HLG to tv_input_info[4][1] for HDMI mode*/
		if (input_mode == IN_MODE_HDMI) {
			tv_input_info->brightness_off[3][1] = brightness_off[3][0];
			tv_input_info->brightness_off[4][1] = brightness_off[4][0];
		}
	} else {
		memset(tv_input_info, 0, sizeof(brightness_off));
	}
	/*config source fps and gd_rf_adjust, dmcfg_id*/
	tv_input_info->content_fps = 24 * (1 << 16);
	tv_input_info->gd_rf_adjust = gd_rf_adjust;
	tv_input_info->tid = get_pic_mode();
	if (debug_dolby & 0x400)
		do_gettimeofday(&start);
	/*for hdmi in cert, only run control_path for different frame*/
	if ((dolby_vision_flags & FLAG_CERTIFICATION) &&
	    !vf_changed && input_mode == IN_MODE_HDMI) {
		run_control_path = false;
	}

	if (ambient_update) {
		/*only if cfg enables darkdetail we allow the API to set values*/
		if (((struct pq_config_dvp *)pq_config_dvp_fake_top1)->
			tdc.ambient_config.dark_detail) {
			dynamic_config_new.dark_detail =
			cfg_info[cur_pic_mode].dark_detail;
		}
		p_ambient = &dynamic_config_new;
	} else {
		if (ambient_test_mode == 1 &&
		    v_inst_info->frame_count < AMBIENT_CFG_FRAMES) {
			p_ambient = &dynamic_test_cfg[v_inst_info->frame_count];
		} else if (ambient_test_mode == 2 &&
			   v_inst_info->frame_count < AMBIENT_CFG_FRAMES) {
			p_ambient = &dynamic_test_cfg_2[v_inst_info->frame_count];
		} else if (ambient_test_mode == 3 &&
			   hdmi_frame_count < AMBIENT_CFG_FRAMES) {
			p_ambient = &dynamic_test_cfg_3[hdmi_frame_count];
		} else if (ambient_test_mode == 4 &&
			   v_inst_info->frame_count < AMBIENT_CFG_FRAMES_2) {
			p_ambient = &dynamic_test_cfg_4[v_inst_info->frame_count];
		} else if (((struct pq_config_dvp *)pq_config_dvp_fake_top1)->
			tdc.ambient_config.dark_detail) {
			/*only if cfg enables darkdetail we allow the API to set*/
			dynamic_darkdetail.dark_detail =
				cfg_info[cur_pic_mode].dark_detail;
			p_ambient = &dynamic_darkdetail;
		}
	}

	if (debug_dolby & 0x200)
		pr_dv_dbg("[count %d %d]dark_detail from cfg:%d,from api:%d\n",
			     hdmi_frame_count, v_inst_info->frame_count,
			     ((struct pq_config_dvp *)pq_config_dvp_fake_top1)->
			     tdc.ambient_config.dark_detail,
			     cfg_info[cur_pic_mode].dark_detail);


	v_inst_info->src_format = src_format;
	v_inst_info->input_mode = input_mode;
	v_inst_info->video_width = new_w;
	v_inst_info->video_height = new_h;

	tv_hw5_setting->top1.src_format = src_format;
	tv_hw5_setting->top1.video_width = new_w;
	tv_hw5_setting->top1.video_height = new_h;

	tv_hw5_setting->top1.input_mode = input_mode;
	tv_hw5_setting->top1.in_md = v_inst_info->md_buf[v_inst_info->current_id];
	tv_hw5_setting->top1.in_md_size = (src_format == FORMAT_DOVI) ? total_md_size : 0;
	tv_hw5_setting->top1.in_comp = v_inst_info->comp_buf[v_inst_info->current_id];
	tv_hw5_setting->top1.in_comp_size = (src_format == FORMAT_DOVI) ? total_comp_size : 0;
	tv_hw5_setting->top1.set_bit_depth = src_bdp;
	tv_hw5_setting->top1.set_chroma_format = src_chroma_format;
	tv_hw5_setting->top1.set_yuv_range = SIGNAL_RANGE_SMPTE;
	tv_hw5_setting->top1.color_format = (vf && (vf->type & VIDTYPE_RGB_444)) ? CP_RGB : CP_YUV;
	tv_hw5_setting->top1.vsem_if = vsem_if_buf;
	tv_hw5_setting->top1.vsem_if_size = vsem_if_size;
	tv_hw5_setting->hdr10_param = &v_inst_info->hdr10_param;
	tv_hw5_setting->pq_config = (struct pq_config_dvp *)pq_config_dvp_fake_top1;
	tv_hw5_setting->menu_param = &menu_param;
	tv_hw5_setting->dynamic_cfg = p_ambient;
	tv_hw5_setting->input_info = tv_input_info;
	tv_hw5_setting->enable_debug = debug_ko;
	tv_hw5_setting->dither_bdp = 0;//dither bitdepth,0=>no dither
	tv_hw5_setting->L1L4_distance = -1;
	tv_hw5_setting->num_ext_downsamplers = num_downsamplers;//todo
	tv_hw5_setting->frame_rate = content_fps;//24000

	if (run_control_path) {
		/*step1: top1 frame N*/
		tv_hw5_setting->analyzer = 1;
		flag = p_funcs_tv->tv_hw5_control_path_analyzer(tv_hw5_setting);

		if (debug_dolby & 0x400) {
			do_gettimeofday(&end);
			time_use = (end.tv_sec - start.tv_sec) * 1000000 +
				(end.tv_usec - start.tv_usec);

			pr_info("controlpath time: %5ld us\n", time_use);
		}
		if (flag >= 0) {
			top1_info.amdv_setting_video_flag = video_frame;
			v_inst_info->tv_dovi_setting_change_flag = true;

			if (debug_dolby & 1) {
				pr_dv_dbg
				("top1 tv setting %s-%d:flag=%x,md=%d,comp=%d\n",
					 input_mode == IN_MODE_HDMI ?
					 "hdmi" : "ott",
					 src_format,
					 flag,
					 total_md_size,
					 total_comp_size);
			}
			dump_tv_setting(tv_hw5_setting,
				v_inst_info->frame_count, debug_dolby);
			ret = 0; /* setting updated */
		} else {
			pr_dv_error("tv_hw5_control_path_analyzer() failed\n");
		}
	} else { /*for cert: vf no change, not run cp*/
		v_inst_info->tv_dovi_setting_change_flag = true;
		ret = 0;
	}
	(top1_v_info.frame_count)++;
	if (debug_dolby & 0x20000)
		pr_dv_dbg("update top1 frame_count %d,ret %d\n",
		top1_v_info.frame_count, ret);
	return ret;
}

/*for top2 parser + controlpath*/
int amdv_parse_metadata_hw5(struct vframe_s *vf,
					      u8 toggle_mode,
					      bool bypass_release,
					      bool drop_flag)
{
	const struct vinfo_s *vinfo = get_current_vinfo();
	struct provider_aux_req_s req;
	struct provider_aux_req_s el_req;
	enum signal_format_enum src_format = FORMAT_SDR;
	enum signal_format_enum check_format;
	int total_md_size = 0;
	int total_comp_size = 0;
	bool el_flag = 0;
	u32 w = 0xffff;
	u32 h = 0xffff;
	int meta_flag_bl = 1;
	int src_chroma_format = 0;
	int src_bdp = 12;
	bool video_frame = false;
	int i;
	struct vframe_master_display_colour_s *p_mdc;
	unsigned int current_mode = dolby_vision_mode;
	enum input_mode_enum input_mode = IN_MODE_OTT;
	int ret_flags = 0;
	int ret = -1;
	bool mel_flag = false;
	int vsem_size = 0;
	int vsem_if_size = 0;
	bool dump_emp = false;
	bool dv_vsem = false;
	bool hdr10_src_primary_changed = false;
	char *pic_mode;
	bool run_control_path = true;
	bool vf_changed = true;
	struct dynamic_cfg_s *p_ambient = NULL;
	u32 cur_id = 0;
	struct video_inst_s *v_inst_info = &top2_v_info;
	u32 test_count = 0;
	static u32 last_top2_level = PY_NO_LEVEL;

	if (!p_funcs_tv || !p_funcs_tv->tv_hw5_control_path || !tv_hw5_setting)
		return -1;

	memset(&req, 0, (sizeof(struct provider_aux_req_s)));
	memset(&el_req, 0, (sizeof(struct provider_aux_req_s)));

	if (vf) {
		video_frame = true;
		if (vf->type & VIDTYPE_COMPRESS) {
			if (is_src_crop_valid(vf->src_crop)) {
				w = vf->compWidth -
					vf->src_crop.left - vf->src_crop.right;
				h = vf->compHeight -
					vf->src_crop.top - vf->src_crop.bottom;
			} else {
				w = vf->compWidth;
				h = vf->compHeight;
			}
		} else {
			w = vf->width;
			h = vf->height;
		}
		if (enable_top1) {
			if (vf->src_fmt.py_level != last_top2_level) {
				if (debug_dolby & 0x80000)
					pr_dv_dbg("top2 py_level changed %s->%s\n",
					level_str[last_top2_level],
					level_str[vf->src_fmt.py_level]);
			}
			if (vf->src_fmt.py_level == PY_NO_LEVEL)
				force_bypass_pd_level0 = true;
			else
				force_bypass_pd_level0 = false;

			/*cfg pd off->on: top2 cfg update after enable_top1*/
			if (update_top2_cfg) {
				update_cp_cfg_hw5(false, false, false);
				update_top2_cfg = false;
			}
		}
		last_top2_level = vf->src_fmt.py_level;
	}

	if (is_aml_tvmode() && vf &&
	    vf->source_type == VFRAME_SOURCE_TYPE_HDMI) {
		req.vf = vf;
		req.bot_flag = 0;
		req.aux_buf = NULL;
		req.aux_size = 0;
		req.dv_enhance_exist = 0;
		req.low_latency = 0;
		vf_notify_provider_by_name("dv_vdin",
					   VFRAME_EVENT_RECEIVER_GET_AUX_DATA,
					   (void *)&req);
		input_mode = IN_MODE_HDMI;

		if ((dolby_vision_flags & FLAG_CERTIFICATION) && enable_vf_check)
			vf_changed = check_vf_changed(vf);

		/* meta */
		if ((dolby_vision_flags & FLAG_RX_EMP_VSEM) &&
			vf->emp.size > 0) {
			vsem_size = vf->emp.size * VSEM_PKT_SIZE;
			memcpy(vsem_if_buf, vf->emp.addr, vsem_size);
			if (vsem_if_buf[0] == 0x7f &&
			    vsem_if_buf[10] == 0x46 &&
			    vsem_if_buf[11] == 0xd0) {
				dv_vsem = true;
				if (!vsem_check(vsem_if_buf, vsem_md_buf)) {
					vsem_if_size = vsem_size;
					if (!vsem_md_buf[10]) {
						req.aux_buf =
							&vsem_md_buf[13];
						req.aux_size =
							(vsem_md_buf[5] << 8)
							+ vsem_md_buf[6]
							- 6 - 4;
						/* cancel vsem, use md */
						/* vsem_if_size = 0; */
					} else {
						req.low_latency = 1;
					}
				} else {
					/* emp error, use previous md */
					pr_dv_dbg("EMP packet error %d\n",
						vf->emp.size);
					dump_emp = true;
					vsem_if_size = 0;
					req.aux_buf = NULL;
					req.aux_size = v_inst_info->last_total_md_size;
				}
			} else if (debug_dolby & 4) {
				pr_dv_dbg("EMP packet not DV vsem %d\n",
					vf->emp.size);
				dump_emp = true;
			}
			if ((debug_dolby & 4) || dump_emp) {
				pr_info("vsem pkt count = %d\n", vf->emp.size);
				for (i = 0; i < vsem_size; i += 8) {
					pr_info("%02x %02x %02x %02x %02x %02x %02x %02x\n",
						vsem_if_buf[i],	vsem_if_buf[i + 1],
						vsem_if_buf[i + 2],	vsem_if_buf[i + 3],
						vsem_if_buf[i + 4],	vsem_if_buf[i + 5],
						vsem_if_buf[i + 6],	vsem_if_buf[i + 7]);
				}
			}
		}
		dv_unique_drm = is_dv_unique_drm(vf);

		/* w/t vsif and no dv_vsem */
		if (vf->vsif.size && !dv_vsem) {
			memset(vsem_if_buf, 0, VSEM_IF_BUF_SIZE);
			memcpy(vsem_if_buf, vf->vsif.addr, vf->vsif.size);
			vsem_if_size = vf->vsif.size;
			if (debug_dolby & 4) {
				pr_info("vsif size = %d\n", vf->vsif.size);
				for (i = 0; i < vsem_if_size; i += 8) {
					pr_info("%02x %02x %02x %02x %02x %02x %02x %02x\n",
						vsem_if_buf[i],	vsem_if_buf[i + 1],
						vsem_if_buf[i + 2],	vsem_if_buf[i + 3],
						vsem_if_buf[i + 4],	vsem_if_buf[i + 5],
						vsem_if_buf[i + 6],	vsem_if_buf[i + 7]);
				}
			}
		} else if (dv_unique_drm && !dv_vsem) { /* dv unique drm and no dv_vsem */
			memset(vsem_if_buf, 0, VSEM_IF_BUF_SIZE);
			if (force_hdmin_fmt >= 3 && force_hdmin_fmt <= 9) {
				memcpy(vsem_if_buf, force_drm, 32);
				vsem_if_size = 32;
			} else if (vf->drm_if.size > 0 && vf->drm_if.addr) {
				memcpy(vsem_if_buf, vf->drm_if.addr, vf->drm_if.size);
				vsem_if_size = vf->drm_if.size;
			}
			if (debug_dolby & 4) {
				pr_info("drm size = %d\n", vf->drm_if.size);
				for (i = 0; i < vsem_if_size; i += 8) {
					pr_info("%02x %02x %02x %02x %02x %02x %02x %02x\n",
						vsem_if_buf[i],
						vsem_if_buf[i + 1],
						vsem_if_buf[i + 2],
						vsem_if_buf[i + 3],
						vsem_if_buf[i + 4],
						vsem_if_buf[i + 5],
						vsem_if_buf[i + 6],
						vsem_if_buf[i + 7]);
				}
			}
		}
		if ((debug_dolby & 1) && (dv_vsem || vsem_if_size))
			pr_dv_dbg("vdin get %s:%d, md:%p %d,ll:%d,bit %x,type %x %x\n",
				dv_vsem ? "vsem" : "vsif",
				dv_vsem ? vsem_size : vsem_if_size,
				req.aux_buf, req.aux_size,
				req.low_latency,
				vf->bitdepth, vf->source_type, vf->type);

		/*check vsem_if_buf */
		if (!dv_unique_drm && vsem_if_size &&
			vsem_if_buf[0] != 0x81) {
			/*not vsif, continue to check vsem*/
			if (!(vsem_if_buf[0] == 0x7F &&
				vsem_if_buf[1] == 0x80 &&
				vsem_if_buf[10] == 0x46 &&
				vsem_if_buf[11] == 0xd0 &&
				vsem_if_buf[12] == 0x00)) {
				vsem_if_size = 0;
				pr_dv_dbg("vsem_if_buf is invalid!\n");
				pr_dv_dbg("%x %x %x %x %x %x %x %x %x %x %x %x\n",
					vsem_if_buf[0], vsem_if_buf[1], vsem_if_buf[2],
					vsem_if_buf[3],	vsem_if_buf[4],	vsem_if_buf[5],
					vsem_if_buf[6],	vsem_if_buf[7],	vsem_if_buf[8],
					vsem_if_buf[9],	vsem_if_buf[10], vsem_if_buf[11]);
			}
		}
		if ((dolby_vision_flags & FLAG_FORCE_DV_LL) ||
		    req.low_latency == 1) {
			src_format = FORMAT_DOVI_LL;
			src_chroma_format = 1;
			for (i = 0; i < 2; i++) {
				if (v_inst_info->md_buf[i])
					memset(v_inst_info->md_buf[i], 0, MD_BUF_SIZE);
				if (v_inst_info->comp_buf[i])
					memset(v_inst_info->comp_buf[i], 0, COMP_BUF_SIZE);
			}
			req.aux_size = 0;
			req.aux_buf = NULL;
		} else if (req.aux_size) {
			if (req.aux_buf) {
				v_inst_info->current_id = v_inst_info->current_id ^ 1;
				memcpy(v_inst_info->md_buf[v_inst_info->current_id],
				       req.aux_buf, req.aux_size);
			}
			src_format = FORMAT_DOVI;
		} else {
			if (toggle_mode == 2)
				src_format =  tv_hw5_setting->top2.src_format;
			if (vf->type & VIDTYPE_VIU_422)
				src_chroma_format = 1;/*CF_UYVY*/
			if (dv_unique_drm) {
				src_format = FORMAT_DOVI_LL;
				input_mode = IN_MODE_HDMI;
				src_chroma_format = 3;/*CF_I444*/
				src_bdp = 8;
				req.aux_size = 0;
				req.aux_buf = NULL;
			} else {
				p_mdc =	&vf->prop.master_display_colour;
				if (is_hdr10_frame(vf) || force_hdmin_fmt == 1) {
					src_format = FORMAT_HDR10;
					/* prepare parameter from hdmi for hdr10 */
					p_mdc->luminance[0] *= 10000;
					prepare_hdr10_param
						(p_mdc, &v_inst_info->hdr10_param);
					req.dv_enhance_exist = 0;
					src_bdp = 10;
				}
				if (src_format != FORMAT_DOVI &&
					(is_hlg_frame(vf) || force_hdmin_fmt == 2)) {
					src_format = FORMAT_HLG;
					src_bdp = 10;
				}
				if (src_format == FORMAT_SDR && force_sdr10 == 1)
					src_format = FORMAT_SDR10;

				if (src_format == FORMAT_SDR10)
					src_bdp = 10;
			}
		}
		if ((debug_dolby & 4) && req.aux_size) {
			pr_dv_dbg("metadata(%d):\n", req.aux_size);
			for (i = 0; i < req.aux_size + 8; i += 8)
				pr_info("%02x %02x %02x %02x %02x %02x %02x %02x\n",
					v_inst_info->md_buf[v_inst_info->current_id][i],
					v_inst_info->md_buf[v_inst_info->current_id][i + 1],
					v_inst_info->md_buf[v_inst_info->current_id][i + 2],
					v_inst_info->md_buf[v_inst_info->current_id][i + 3],
					v_inst_info->md_buf[v_inst_info->current_id][i + 4],
					v_inst_info->md_buf[v_inst_info->current_id][i + 5],
					v_inst_info->md_buf[v_inst_info->current_id][i + 6],
					v_inst_info->md_buf[v_inst_info->current_id][i + 7]);
		}
		total_md_size = req.aux_size;
		total_comp_size = 0;
		meta_flag_bl = 0;
		if (req.aux_buf && req.aux_size) {
			v_inst_info->last_total_md_size = total_md_size;
			v_inst_info->last_total_comp_size = total_comp_size;
		} else if (toggle_mode == 2) {
			total_md_size = v_inst_info->last_total_md_size;
			total_comp_size = v_inst_info->last_total_comp_size;
		}
		if (debug_dolby & 1)
			pr_dv_dbg("top2 frame %d, %px,pts %lld,format:%s,type %x\n",
			v_inst_info->frame_count, vf, vf->pts_us64,
			(src_format == FORMAT_HDR10) ? "HDR10" :
			((src_format == FORMAT_DOVI) ? "DOVI" :
			((src_format == FORMAT_DOVI_LL) ? "DOVI_LL" :
			((src_format == FORMAT_HLG) ? "HLG" : "SDR"))),
			vf->type);

		if (toggle_mode == 1) {
			if (debug_dolby & 2)
				pr_dv_dbg("+++ get bl(%p-%lld) +++\n",
						  vf, vf->pts_us64);
			amdvdolby_vision_vf_add(vf, NULL);
		}
	} else if (vf && (vf->source_type == VFRAME_SOURCE_TYPE_OTHERS)) {
		enum vframe_signal_fmt_e fmt;

		input_mode = IN_MODE_OTT;

		req.vf = vf;
		req.bot_flag = 0;
		req.aux_buf = NULL;
		req.aux_size = 0;
		req.dv_enhance_exist = 0;

		/* check source format */
		fmt = get_vframe_src_fmt(vf);
		if ((fmt == VFRAME_SIGNAL_FMT_DOVI ||
		    fmt == VFRAME_SIGNAL_FMT_INVALID ||
		    vf->src_fmt.sei_magic_code != SEI_MAGIC_CODE) &&
		    !vf->discard_dv_data) {
			vf_notify_provider_by_name
				(dv_provider[0],
				 VFRAME_EVENT_RECEIVER_GET_AUX_DATA,
				  (void *)&req);
		}
		/* use callback aux date first, if invalid, use sei_ptr */
		if ((!req.aux_buf || !req.aux_size) &&
		    fmt == VFRAME_SIGNAL_FMT_DOVI) {
			u32 sei_size = 0;
			char *sei;

			if (debug_dolby & 2)
				pr_dv_dbg("top2:no aux %px %x, el %d from %s, use sei_ptr\n",
					     req.aux_buf, req.aux_size,
					     req.dv_enhance_exist, dv_provider[0]);

			sei = (char *)get_sei_from_src_fmt(vf, &sei_size);
			if (sei && sei_size) {
				req.aux_buf = sei;
				req.aux_size = sei_size;
				req.dv_enhance_exist =
					vf->src_fmt.dual_layer;
			}
		}
		if (debug_dolby & 1)
			pr_dv_dbg("top2:%s get %px(%d,index %d),fmt %d,aux %px %x,el %d,type %x\n",
				     dv_provider[0], vf, vf->discard_dv_data, vf->omx_index, fmt,
				     req.aux_buf, req.aux_size, req.dv_enhance_exist, vf->type);
		/* parse meta in base layer */
		if (toggle_mode != 2) {
			ret = get_md_from_src_fmt(vf);
			if (ret == 1) { /*parse succeeded*/
				meta_flag_bl = 0;
				src_format = FORMAT_DOVI;
				v_inst_info->current_id = v_inst_info->current_id ^ 1;
				cur_id = v_inst_info->current_id;
				memcpy(v_inst_info->md_buf[cur_id],
				       vf->src_fmt.md_buf,
				       vf->src_fmt.md_size);
				memcpy(v_inst_info->comp_buf[cur_id],
				       vf->src_fmt.comp_buf,
				       vf->src_fmt.comp_size);
				total_md_size =  vf->src_fmt.md_size;
				total_comp_size =  vf->src_fmt.comp_size;
				ret_flags = vf->src_fmt.parse_ret_flags;

				v_inst_info->last_total_md_size = total_md_size;
				v_inst_info->last_total_comp_size = total_comp_size;

				if ((debug_dolby & 4) && dump_enable_f(0)) {
					pr_dv_dbg("top2:get md_buf %px, size(%d):\n",
						vf->src_fmt.md_buf, vf->src_fmt.md_size);
					for (i = 0; i < total_md_size; i += 8)
						pr_info("%02x %02x %02x %02x %02x %02x %02x %02x\n",
							v_inst_info->md_buf[cur_id][i],
							v_inst_info->md_buf[cur_id][i + 1],
							v_inst_info->md_buf[cur_id][i + 2],
							v_inst_info->md_buf[cur_id][i + 3],
							v_inst_info->md_buf[cur_id][i + 4],
							v_inst_info->md_buf[cur_id][i + 5],
							v_inst_info->md_buf[cur_id][i + 6],
							v_inst_info->md_buf[cur_id][i + 7]);
				}
			} else {  /*no parse in advance or parse failed*/
				if (get_vframe_src_fmt(vf) ==
				    VFRAME_SIGNAL_FMT_HDR10PRIME)
					src_format = FORMAT_PRIMESL;
				else
					meta_flag_bl =
					parse_sei_and_meta_hw5
						(vf, &req,
						 &total_comp_size,
						 &total_md_size,
						 &src_format,
						 &ret_flags, drop_flag, 1);
			}
			if (force_mel)
				ret_flags = 1;

			if (ret_flags && req.dv_enhance_exist) {
				if (!strcmp(dv_provider[0], "dvbldec"))
					vf_notify_provider_by_name
					(dv_provider[0],
					 VFRAME_EVENT_RECEIVER_DOLBY_BYPASS_EL,
					 (void *)&req);
				amdv_el_disable = 1;
				pr_dv_dbg("bypass mel\n");
			}
			if (ret_flags == 1)
				mel_flag = true;
			if (!is_dv_standard_es(req.dv_enhance_exist,
					       ret_flags, w)) {
				src_format = FORMAT_SDR;
				/* dovi_setting.src_format = src_format; */
				total_comp_size = 0;
				total_md_size = 0;
				src_bdp = 10;
				bypass_release = true;
			}
			if (is_aml_tvmode() &&
				(req.dv_enhance_exist && !mel_flag)) {
				src_format = FORMAT_SDR;
				/* dovi_setting.src_format = src_format; */
				total_comp_size = 0;
				total_md_size = 0;
				src_bdp = 10;
				bypass_release = true;
				if (debug_dolby & 1)
					pr_dv_dbg("tv: bypass fel\n");
			}
		} else {
			src_format = tv_hw5_setting->top2.src_format;
		}

		if (src_format != FORMAT_DOVI && is_primesl_frame(vf)) {
			src_format = FORMAT_PRIMESL;
			src_bdp = 10;
		}
		if (src_format != FORMAT_DOVI && is_hdr10_frame(vf)) {
			src_format = FORMAT_HDR10;
			/* prepare parameter from SEI for hdr10 */
			p_mdc =	&vf->prop.master_display_colour;
			prepare_hdr10_param(p_mdc, &v_inst_info->hdr10_param);
			/* for 962x with v1.4 or stb with v2.3 may use 12 bit */
			src_bdp = 10;
			req.dv_enhance_exist = 0;
		}

		if (src_format != FORMAT_DOVI && is_hlg_frame(vf)) {
			src_format = FORMAT_HLG;
			src_bdp = 10;
		}
		if (src_format != FORMAT_DOVI && is_hdr10plus_frame(vf))
			src_format = FORMAT_HDR10PLUS;

		if (src_format != FORMAT_DOVI && is_mvc_frame(vf))
			src_format = FORMAT_MVC;

		if (src_format != FORMAT_DOVI && is_cuva_frame(vf))
			src_format = FORMAT_CUVA;

		if (src_format == FORMAT_SDR && force_sdr10 == 1)
			src_format = FORMAT_SDR10;

		if (src_format == FORMAT_SDR10 && !req.dv_enhance_exist)
			src_bdp = 10;

		if (src_format == FORMAT_SDR &&	!req.dv_enhance_exist)
			src_bdp = 8;

		if (((debug_dolby & 1) || ((debug_dolby & 0x100) &&
			v_inst_info->frame_count == 0)) && toggle_mode == 1)
			pr_info
			("top2:[%d,%lld,%d,%s,%d,%d]\n",
			 v_inst_info->frame_count, vf->pts_us64, src_bdp,
			 (src_format == FORMAT_HDR10) ? "HDR10" :
			 (src_format == FORMAT_DOVI ? "DOVI" :
			 (src_format == FORMAT_HLG ? "HLG" :
			 (src_format == FORMAT_HDR10PLUS ? "HDR10+" :
			 (src_format == FORMAT_CUVA ? "CUVA" :
			 (src_format == FORMAT_PRIMESL ? "PRIMESL" :
			 (req.dv_enhance_exist ? "DOVI (el meta)" : "SDR")))))),
			 req.aux_size, req.dv_enhance_exist);
		if (src_format != FORMAT_DOVI && !req.dv_enhance_exist)
			memset(&req, 0, sizeof(req));

		if (toggle_mode == 1) {
			if (debug_dolby & 2)
				pr_dv_dbg("+++ get bl(%p-%lld) +++\n",
					     vf, vf->pts_us64);
			amdvdolby_vision_vf_add(vf, NULL);
		}

		if (req.dv_enhance_exist)
			el_flag = 1;

		if (toggle_mode != 2) {
			v_inst_info->last_mel_mode = mel_flag;
		} else if (meta_flag_bl) {
			total_md_size = v_inst_info->last_total_md_size;
			total_comp_size = v_inst_info->last_total_comp_size;
			el_flag = tv_hw5_setting->top2.el_flag;
			mel_flag = v_inst_info->last_mel_mode;
			if (debug_dolby & 2)
				pr_dv_dbg("update el_flag %d, melFlag %d\n",
					     el_flag, mel_flag);
			meta_flag_bl = 0;
		}

		if (el_flag && !mel_flag &&
		    ((dolby_vision_flags & FLAG_CERTIFICATION) == 0)) {
			el_flag = 0;
			amdv_el_disable = 1;
		}
		if (src_format != FORMAT_DOVI) {
			el_flag = 0;
			mel_flag = 0;
		}
	}
	if (src_format == FORMAT_DOVI && meta_flag_bl) {
		/* dovi frame no meta or meta error */
		/* use old setting for this frame   */
		pr_dv_dbg("no meta or meta err!\n");
		return -1;
	}

	/* if not DOVI, release metadata_parser */
	//if (vf && src_format != FORMAT_DOVI &&
	    //metadata_parser && !bypass_release) {
		//p_funcs_tv->multi_mp_release(&metadata_parser);
		//metadata_parser = NULL;
		//pr_dv_dbg("parser release\n");
	//}

	if (drop_flag) {
		pr_dv_dbg("drop frame_count %d\n", v_inst_info->frame_count);
		return 2;
	}

	check_format = src_format;
	if (vf)
		update_src_format(check_format, vf);

	if (dolby_vision_request_mode != 0xff) {
		dolby_vision_mode = dolby_vision_request_mode;
		dolby_vision_request_mode = 0xff;
	}
	current_mode = dolby_vision_mode;
	if (amdv_policy_process
		(vf, &current_mode, check_format)) {
		if (!v_inst_info->amdv_wait_init)
			amdv_set_toggle_flag(1);
		if (debug_dolby & 1)
			pr_dv_dbg("[%s]output change from %d to %d(%d, %px, %d)\n",
			     __func__, dolby_vision_mode, current_mode,
			     toggle_mode, vf, src_format);
		amdv_target_mode = current_mode;
		dolby_vision_mode = current_mode;
		amdv_wait_on = true;
	} else {
		/*not clear target mode when:*/
		/*no mode change && no vf && target is not bypass */
		if ((!vf && amdv_target_mode != dolby_vision_mode &&
		    amdv_target_mode !=
		    AMDV_OUTPUT_MODE_BYPASS)) {
			if (debug_dolby & 8)
				pr_dv_dbg("not update target mode %d\n",
					     amdv_target_mode);
		} else if (amdv_target_mode != dolby_vision_mode) {
			amdv_target_mode = dolby_vision_mode;
			if (debug_dolby & 8)
				pr_dv_dbg("update target mode %d\n",
					     amdv_target_mode);
		}
	}

	if (vf && (debug_dolby & 8))
		pr_dv_dbg("parse_metadata: vf %px(index %d), mode %d\n",
			      vf, vf->omx_index, dolby_vision_mode);

	if (dolby_vision_mode == AMDV_OUTPUT_MODE_BYPASS) {
		if (amdv_target_mode == AMDV_OUTPUT_MODE_BYPASS)
			amdv_wait_on = false;
		if (debug_dolby & 8)
			pr_dv_dbg("now bypass mode, target %d, wait %d\n",
				  amdv_target_mode,
				  amdv_wait_on);
		if (get_hdr_module_status(VD1_PATH, VPP_TOP0) == HDR_MODULE_BYPASS)
			return 1;
		return -1;
	}

	if (src_format != tv_hw5_setting->top2.src_format ||
		(dolby_vision_flags & FLAG_CERTIFICATION)) {
		pq_config_set_flag = false;
		best_pq_config_set_flag = false;
	}
	if (!pq_config_set_flag) {
		if (debug_dolby & 2)
			pr_dv_dbg("update def_tgt_display_cfg\n");
		if (!get_load_config_status()) {
			memcpy(&(((struct pq_config_dvp *)pq_config_dvp_fake)->tdc),
			       &def_tgt_dvp_cfg,
			       sizeof(def_tgt_dvp_cfg));//todo
		}
		pq_config_set_flag = true;
	}
	if (force_best_pq && !best_pq_config_set_flag) {
		pr_dv_dbg("update best def_tgt_display_cfg\n");
		//memcpy(&(((struct pq_config_dvp *)
		//	pq_config_dvp_fake)->tdc),
		//	&def_tgt_display_cfg_bestpq,
		//	sizeof(def_tgt_display_cfg_bestpq));//todo
		best_pq_config_set_flag = true;

		p_funcs_tv->tv_hw5_control_path(invalid_hw5_setting);
	}
	calculate_panel_max_pq(src_format, vinfo,
			       &(((struct pq_config_dvp *)
			       pq_config_dvp_fake)->tdc));

	((struct pq_config_dvp *)
		pq_config_dvp_fake)->tdc.tuning_mode =
		amdv_tuning_mode;
	if (dolby_vision_flags & FLAG_DISABLE_COMPOSER) {
		((struct pq_config_dvp *)pq_config_dvp_fake)
			->tdc.tuning_mode |=
			TUNING_MODE_EL_FORCE_DISABLE;
	} else {
		((struct pq_config_dvp *)pq_config_dvp_fake)
			->tdc.tuning_mode &=
			(~TUNING_MODE_EL_FORCE_DISABLE);
	}
	if ((dolby_vision_flags & FLAG_CERTIFICATION) && sdr_ref_mode) {
		((struct pq_config_dvp *)
		pq_config_dvp_fake)->tdc.ambient_config.ambient =
		0;
		((struct pq_config_dvp *)pq_config_dvp_fake)
			->tdc.ref_mode_dark_id = 0;
	}
	if (is_hdr10_src_primary_changed()) {
		hdr10_src_primary_changed = true;
		pr_dv_dbg("hdr10 src primary changed!\n");
	}
	if (src_format != tv_hw5_setting->top2.src_format ||
		hdr10_src_primary_changed) {
		if (debug_dolby & 0x100)
			pr_dv_dbg("reset control_path fmt %d->%d\n",
				tv_hw5_setting->top2.src_format, src_format);
		/*for hdmi in cert*/
		if (dolby_vision_flags & FLAG_CERTIFICATION)
			vf_changed = true;
		p_funcs_tv->tv_hw5_control_path(invalid_hw5_setting);
	}
	pic_mode = get_cur_pic_mode_name();
	if (!(dolby_vision_flags & FLAG_CERTIFICATION) && pic_mode &&
	    (strstr(pic_mode, "dark") ||
	    strstr(pic_mode, "Dark") ||
	    strstr(pic_mode, "DARK"))) {
		memcpy(tv_input_info,
		       brightness_off,
		       sizeof(brightness_off));
		/*for HDR10/HLG, only has DM4, ko only use value from tv_input_info[3][1]*/
		/*and tv_input_info[4][1]. To avoid ko code changed, we reuse these*/
		/*parameter for both HDMI and OTT mode, that means need copy HDR10 to */
		/*tv_input_info[3][1] and copy HLG to tv_input_info[4][1] for HDMI mode*/
		if (input_mode == IN_MODE_HDMI) {
			tv_input_info->brightness_off[3][1] = brightness_off[3][0];
			tv_input_info->brightness_off[4][1] = brightness_off[4][0];
		}
	} else {
		memset(tv_input_info, 0, sizeof(brightness_off));
	}
	/*config source fps and gd_rf_adjust, dmcfg_id*/
	tv_input_info->content_fps = 24 * (1 << 16);
	tv_input_info->gd_rf_adjust = gd_rf_adjust;
	tv_input_info->tid = get_pic_mode();

	if (dolby_vision_flags & FLAG_CERTIFICATION) {
		/*for hdmi in cert, only run control_path for different frame*/
		if (!vf_changed && input_mode == IN_MODE_HDMI)
			run_control_path = false;
		else if (toggle_mode != 1)
			run_control_path = false;
	}

	if (ambient_update) {
		/*only if cfg enables darkdetail we allow the API to set values*/
		if (((struct pq_config_dvp *)pq_config_dvp_fake)->
			tdc.ambient_config.dark_detail) {
			dynamic_config_new.dark_detail =
			cfg_info[cur_pic_mode].dark_detail;
		}
		p_ambient = &dynamic_config_new;
	} else {
		if (vf && vf->source_type == VFRAME_SOURCE_TYPE_HDMI)
			test_count = hdmi_frame_count;
		else if (vf && vf->source_type == VFRAME_SOURCE_TYPE_OTHERS &&
			v_inst_info)
			test_count = v_inst_info->frame_count;

		if (ambient_test_mode == 1 && toggle_mode == 1 &&
		    test_count < AMBIENT_CFG_FRAMES) {
			p_ambient = &dynamic_test_cfg[test_count];
		} else if (ambient_test_mode == 2 && toggle_mode == 1 &&
			   test_count < AMBIENT_CFG_FRAMES) {
			p_ambient = &dynamic_test_cfg_2[test_count];
		} else if (ambient_test_mode == 3 && toggle_mode == 1 &&
			   test_count < AMBIENT_CFG_FRAMES) {
			p_ambient = &dynamic_test_cfg_3[test_count];
		} else if (ambient_test_mode == 4 && toggle_mode == 1 &&
			   test_count < AMBIENT_CFG_FRAMES_2) {
			p_ambient = &dynamic_test_cfg_4[test_count];
		} else if (((struct pq_config_dvp *)pq_config_dvp_fake)->
			tdc.ambient_config.dark_detail) {
			/*only if cfg enables darkdetail we allow the API to set*/
			dynamic_darkdetail.dark_detail =
				cfg_info[cur_pic_mode].dark_detail;
			p_ambient = &dynamic_darkdetail;
		}
	}
	if (variable_fps_mode == 1 && toggle_mode == 1 &&
		vf && vf->source_type == VFRAME_SOURCE_TYPE_HDMI &&
		hdmi_frame_count < VARIABLE_FPS_COUNT) {
		content_fps = variable_fps[hdmi_frame_count];
		if (debug_dolby & 1)
			pr_dv_dbg("variable_fps %d\n", content_fps);
	} else if ((dolby_vision_flags & FLAG_CERTIFICATION) && (test_dv & FORCE_ONE_SLICE)) {
		content_fps = 60000;
	}
	if (debug_dolby & 0x200)
		pr_dv_dbg("[count %d %d]dark_detail from cfg:%d,from api:%d\n",
			     hdmi_frame_count, v_inst_info->frame_count,
			     ((struct pq_config_dvp *)pq_config_dvp_fake)->
			     tdc.ambient_config.dark_detail,
			     cfg_info[cur_pic_mode].dark_detail);

	if (vf && enable_top1) {
		if ((dolby_vision_flags & FLAG_CERTIFICATION) &&
			(test_dv & HDMI_ONLY_UPDATE_HIST_FOR_NEW_FRAME) &&
			vf && vf->source_type == VFRAME_SOURCE_TYPE_HDMI) {
			if (debug_dolby & 1)
				pr_dv_dbg("hdmi in, hdmi_new_frame %d\n",
				vf->src_fmt.hdmi_new_frame);
			if (vf->src_fmt.hdmi_new_frame)
				get_l1l4_hist();
		} else {
			get_l1l4_hist();
		}
	} else {
		tv_hw5_setting->top1_stats.enable = false;
	}

	v_inst_info->src_format = src_format;
	v_inst_info->input_mode = input_mode;
	v_inst_info->video_width = w;
	v_inst_info->video_height = h;

	tv_hw5_setting->top2.src_format = src_format;
	tv_hw5_setting->top2.video_width = w;
	tv_hw5_setting->top2.video_height = h;

	tv_hw5_setting->top2.input_mode = input_mode;
	tv_hw5_setting->top2.in_md = v_inst_info->md_buf[v_inst_info->current_id];
	tv_hw5_setting->top2.in_md_size = (src_format == FORMAT_DOVI) ? total_md_size : 0;
	tv_hw5_setting->top2.in_comp = v_inst_info->comp_buf[v_inst_info->current_id];
	tv_hw5_setting->top2.in_comp_size = (src_format == FORMAT_DOVI) ? total_comp_size : 0;
	tv_hw5_setting->top2.set_bit_depth = src_bdp;
	tv_hw5_setting->top2.set_chroma_format = src_chroma_format;
	tv_hw5_setting->top2.set_yuv_range = SIGNAL_RANGE_SMPTE;
	tv_hw5_setting->top2.color_format = (vf && (vf->type & VIDTYPE_RGB_444)) ? CP_RGB : CP_YUV;
	tv_hw5_setting->top2.vsem_if = vsem_if_buf;
	tv_hw5_setting->top2.vsem_if_size = vsem_if_size;
	tv_hw5_setting->top2.el_flag = el_flag;
	tv_hw5_setting->hdr10_param = &v_inst_info->hdr10_param;
	tv_hw5_setting->pq_config = (struct pq_config_dvp *)pq_config_dvp_fake;
	tv_hw5_setting->menu_param = &menu_param;
	tv_hw5_setting->dynamic_cfg = p_ambient;
	tv_hw5_setting->input_info = tv_input_info;
	tv_hw5_setting->enable_debug = debug_ko;
	tv_hw5_setting->dither_bdp = 0;//dither bitdepth,0=>no dither
	tv_hw5_setting->L1L4_distance = -1;
	tv_hw5_setting->num_ext_downsamplers = vf ?
		vf->src_fmt.downsamplers : num_downsamplers;//todo
	tv_hw5_setting->frame_rate = content_fps;

	top2_info.amdv_setting_video_flag = video_frame;
	v_inst_info->tv_dovi_setting_change_flag = true;
	v_inst_info->last_mel_mode = mel_flag;

	if (run_control_path)
		ret = 0;
	else /*for cert: vf no change, not run cp*/
		ret = 2;

	return ret;
}

int amdv_wait_metadata_hw5(struct vframe_s *vf)
{
	int ret = 0;
	int tmp = 0;
	unsigned int mode = dolby_vision_mode;
	enum signal_format_enum check_format;
	bool vd1_on = false;
	bool temp_done = false;
	static struct timeval start_wait_time;
	struct timeval cur_time;
	unsigned long wait_us = 0;
	static bool is_waiting;
	static u32 last_hsize;
	static u32 last_vsize;
	u32 w;
	u32 h;
	bool size_changed = false;

	get_vf_size(vf, &w, &h);
	if (w != last_hsize || h != last_vsize) {
		size_changed = true;
		last_hsize = w;
		last_vsize = h;
	}

	/*only for first frame. next frame will update in parser_metadata_hw5_top1*/
	if (!top2_info.core_on || pic_mode_changed || size_changed) {
		update_top1_onoff(vf);
		pic_mode_changed = false;
	}

	if (single_step_enable_v2(0, VD1_PATH)) {
		if (dolby_vision_flags & FLAG_SINGLE_STEP)
			/* wait fake el for "step" */
			return 1;
		if (dolby_vision_flags & FLAG_CERTIFICATION)
			temp_done = true;
		dolby_vision_flags |= FLAG_SINGLE_STEP;
	}

	if (dolby_vision_flags & FLAG_CERTIFICATION) {
		bool ott_mode = true;

		if (is_aml_tvmode() && tv_hw5_setting)
			ott_mode = tv_hw5_setting->top2.input_mode !=
				IN_MODE_HDMI;
		if (debug_dolby & 0x1000)
			pr_dv_dbg("setting_update_count %d,crc_count %d,flag %x,top1_on %d %d\n",
				setting_update_count, crc_count, dolby_vision_flags,
				top1_info.core_on, top1_done);
		if (setting_update_count > crc_count &&
			!(dolby_vision_flags & FLAG_DISABLE_CRC)) {
			if (ott_mode)
				return 1;
		}
	}

	if (!top2_v_info.amdv_wait_init && !top2_info.core_on) {
		ret = is_amdv_frame(vf);
		if (ret) {
			check_format = FORMAT_DOVI;
			ret = 0;
		} else if (is_primesl_frame(vf)) {
			check_format = FORMAT_PRIMESL;
		} else if (is_hdr10_frame(vf)) {
			if (is_dv_unique_drm(vf))
				check_format = FORMAT_DOVI_LL;
			else
				check_format = FORMAT_HDR10;
		} else if (is_hlg_frame(vf)) {
			check_format = FORMAT_HLG;
		} else if (is_hdr10plus_frame(vf)) {
			check_format = FORMAT_HDR10PLUS;
		} else if (is_mvc_frame(vf)) {
			check_format = FORMAT_MVC;
		} else if (is_cuva_frame(vf)) {
			check_format = FORMAT_CUVA;
		} else {
			check_format = FORMAT_SDR;
		}
		/*wait the first frame top1 only for cert*/
		if (enable_top1 && !hw5_reg_from_file && wait_first_frame_top1) {
			if (dolby_vision_mode == AMDV_OUTPUT_MODE_BYPASS)
				tmp = amdv_policy_process(vf, &mode, check_format);

			/*wait top1 only when dv will on*/
			if (dolby_vision_mode != AMDV_OUTPUT_MODE_BYPASS ||
				(tmp == 1 && mode != AMDV_OUTPUT_MODE_BYPASS)) {
				if (!top1_info.core_on) {
					if (vf && (debug_dolby & 8))
						pr_dv_dbg("wait top1 and need to do top1\n");

					do_gettimeofday(&cur_time);
					is_waiting = true;
					start_wait_time = cur_time;
					return 4;
				}
				if (top1_info.core_on &&
					(!top1_done && !ignore_top1_result &&
					!force_ignore_top1_result) &&
					top1_info.py_level != PY_NO_LEVEL) {
					if (vf && (debug_dolby & 8))
						pr_dv_dbg("wait top1\n");

					do_gettimeofday(&cur_time);

					wait_us = (cur_time.tv_sec - start_wait_time.tv_sec) *
						1000000 +
						(cur_time.tv_usec - start_wait_time.tv_usec);

					if (!is_waiting)
						start_wait_time = cur_time;

					if (wait_us > 2 * 1000 * 1000) {
						pr_dv_dbg("top1 timeout after %5ld us,no longer wait\n",
						wait_us);
					} else {
						is_waiting = true;
						return 5;
					}
				}
				is_waiting = false;
			}
		}

		if (vf)
			update_src_format(check_format, vf);

		if (amdv_policy_process(vf, &mode, check_format)) {
			if (mode != AMDV_OUTPUT_MODE_BYPASS &&
			    dolby_vision_mode ==
			    AMDV_OUTPUT_MODE_BYPASS) {
				top2_v_info.amdv_wait_init = true;
				amdv_target_mode = mode;
				amdv_wait_on = true;
				if (debug_dolby & 1)
					pr_dv_dbg("dolby_vision_need_wait src=%d mode=%d\n",
						check_format, mode);
			}
		}
		if (is_aml_t3x()) {
			if (READ_VPP_DV_REG(T3X_VD1_BLEND_SRC_CTRL) & (0xf << 0))
				vd1_on = true;
		}
		/* don't use run mode when sdr -> dv and vd1 not disable */
		if (/*top2_v_info.amdv_wait_init && */vd1_on && !force_runmode)
			top2_info.run_mode_count = amdv_run_mode_delay + 1;
		if (debug_dolby & 8)
			pr_dv_dbg("amdv_on_count %d, vd1_on %d\n",
				      top2_info.run_mode_count, vd1_on);
	}

	if (top2_info.core_on &&
		top2_info.run_mode_count <= amdv_run_mode_delay)
		ret = 1;

	if (vf && (debug_dolby & 8))
		pr_dv_dbg("wait return %d, vf %px(index %d), runcount %d\n",
			      ret, vf, vf->omx_index, top2_info.run_mode_count);

	return ret;
}

int amdv_update_src_format_hw5(struct vframe_s *vf, u8 toggle_mode)
{
	unsigned int mode = dolby_vision_mode;
	enum signal_format_enum check_format;
	int ret = 0;

	if (!is_amdv_enable() || !vf)
		return -1;

	/* src_format is valid, need not re-init */
	if (top2_v_info.amdv_src_format != 0)
		return 0;

	/* vf is not in the dv queue, new frame case */
	if (amdv_vf_check(vf))
		return 0;

	ret = is_amdv_frame(vf);
	if (ret)
		check_format = FORMAT_DOVI;
	else if (is_primesl_frame(vf))
		check_format = FORMAT_PRIMESL;
	else if (is_hdr10_frame(vf))
		check_format = FORMAT_HDR10;
	else if (is_hlg_frame(vf))
		check_format = FORMAT_HLG;
	else if (is_hdr10plus_frame(vf))
		check_format = FORMAT_HDR10PLUS;
	else if (is_mvc_frame(vf))
		check_format = FORMAT_MVC;
	else if (is_cuva_frame(vf))
		check_format = FORMAT_CUVA;
	else
		check_format = FORMAT_SDR;

	if (vf)
		update_src_format(check_format, vf);

	if (!top2_v_info.amdv_wait_init &&
	    !top2_info.core_on &&
	    top2_v_info.amdv_src_format != 0) {
		if (amdv_policy_process
			(vf, &mode, check_format)) {
			if (mode != AMDV_OUTPUT_MODE_BYPASS &&
			    dolby_vision_mode ==
			    AMDV_OUTPUT_MODE_BYPASS) {
				top2_v_info.amdv_wait_init = true;
				amdv_target_mode = mode;
				amdv_wait_on = true;
				pr_dv_dbg
					("dolby_vision_need_wait src=%d mode=%d\n",
					 check_format, mode);
			}
		}
	}
	pr_dv_dbg
		("%s done vf:%p, src=%d, toggle mode:%d\n",
		__func__, vf, top2_v_info.amdv_src_format, toggle_mode);
	return 1;
}

/*vf: display on vd1*/
/* ret 0: setting generated for this frame */
/* ret -1: do nothing */
int amdv_hw5_control_path(struct vframe_s *vf, struct vd_proc_info_t *vd_proc_info)
{
	int flag;
	u32 w = 0xffff;
	u32 h = 0xffff;
	int ret = -1;
	unsigned long time_use = 0;
	struct timeval start;
	struct timeval end;
	struct video_inst_s *v_inst_info = &top2_v_info;

	if (!is_amdv_enable() || !module_installed || !tv_hw5_setting)
		return -1;

	if (vf) {
		if (vf->type & VIDTYPE_COMPRESS) {
			if (is_src_crop_valid(vf->src_crop)) {
				w = vf->compWidth -
					vf->src_crop.left - vf->src_crop.right;
				h = vf->compHeight -
					vf->src_crop.top - vf->src_crop.bottom;

				if (debug_dolby & 0x80000)
					pr_dv_dbg("vf[%px]comp size: %dx%d,crop info: %d %d %d %d\n",
						vf,
						vf->compWidth,
						vf->compHeight,
						vf->src_crop.left,
						vf->src_crop.right,
						vf->src_crop.top,
						vf->src_crop.bottom);
			} else {
				w = vf->compWidth;
				h = vf->compHeight;
			}
		} else {
			w = vf->width;
			h = vf->height;
		}
		if (vd_proc_info) {
			if (debug_dolby & 0x8)
				pr_dv_dbg("controlpath vf[%px], wxh: %dx%d, vd1 size: %dx%d\n",
					vf, w, h, vd_proc_info->vd1_in_hsize,
					vd_proc_info->vd1_in_vsize);
			/*1.if size not align to 2 or 4(2slice), vpp will align*/
			/*2.if play 4k video with 4k240hz output, vpp will vskip*/
			/*need set real vd1 size for controlpath*/
			if (w != vd_proc_info->vd1_in_hsize || h != vd_proc_info->vd1_in_vsize) {
				w = vd_proc_info->vd1_in_hsize;
				h = vd_proc_info->vd1_in_vsize;
			}
			if (!(dolby_vision_flags & FLAG_CERTIFICATION)) {
				if (vd_proc_info->slice_num == 2)
					tv_hw5_setting->force_num_slices = 2;
				else
					tv_hw5_setting->force_num_slices = 1;
			} else {
				tv_hw5_setting->force_num_slices = 0;
			}
		} else {
			tv_hw5_setting->force_num_slices = 0;
		}

		tv_hw5_setting->top2.video_width = w;
		tv_hw5_setting->top2.video_height = h;
		v_inst_info->video_width = w;
		v_inst_info->video_height = h;

		if (debug_cp_res > 0) {
			tv_hw5_setting->top2.video_width = (debug_cp_res & 0xffff0000) >> 16;
			tv_hw5_setting->top2.video_height = debug_cp_res & 0xffff;
			v_inst_info->video_width = (debug_cp_res & 0xffff0000) >> 16;
			v_inst_info->video_height = debug_cp_res & 0xffff;
		}
	}
	/*top2,no analyzer*/
	tv_hw5_setting->analyzer = 0;
	if ((dolby_vision_flags & FLAG_CERTIFICATION) &&
		vf && vf->source_type == VFRAME_SOURCE_TYPE_HDMI &&
		hdmi_frame_count == 0)
		p_funcs_tv->tv_hw5_control_path(invalid_hw5_setting);

	if (debug_dolby & 0x400)
		do_gettimeofday(&start);

	flag = p_funcs_tv->tv_hw5_control_path(tv_hw5_setting);

	if (debug_dolby & 0x400) {
		do_gettimeofday(&end);
		time_use = (end.tv_sec - start.tv_sec) * 1000000 +
			(end.tv_usec - start.tv_usec);

		pr_info("controlpath time: %5ld us\n", time_use);
	}
	if (flag >= 0) {
		if (debug_dolby & 1) {
			pr_dv_dbg
			("tv setting %s-%d:flag=%x,md=%d,comp=%d,force num=%d\n",
				 tv_hw5_setting->top2.input_mode == IN_MODE_HDMI ?
				 "hdmi" : "ott",
				 tv_hw5_setting->top2.src_format,
				 flag,
				 tv_hw5_setting->top2.in_md_size,
				 tv_hw5_setting->top2.in_comp_size,
				 tv_hw5_setting->force_num_slices);
		}
		if (debug_dolby & 1)
			pr_dv_dbg("ko get backlight %d\n", tv_hw5_setting->backlight);
		dump_tv_setting(tv_hw5_setting,
			v_inst_info->frame_count, debug_dolby);
		ret = 0; /* setting updated */
		top2_v_info.tv_dovi_setting_change_flag = true;
	} else {
		tv_hw5_setting->top2.video_width = 0;
		tv_hw5_setting->top2.video_height = 0;
		pr_dv_error("tv_hw5_control_path() failed\n");
	}

	return ret;
}

/*only process top1, no policy*/
int amdolby_vision_process_hw5_top1(struct vframe_s *vf_top1,
		u32 display_size)
{
	int src_chroma_format = 0;
	u32 h_size = (display_size >> 16) & 0xffff;
	u32 v_size = display_size & 0xffff;
	bool reset_flag = false;
	bool src_is_42210bit = false;
	u32 level;

	struct vframe_s *vf;

	if (!is_aml_hw5())
		return -1;

	if (!module_installed && !hw5_reg_from_file)
		return -1;

	vf = vf_top1;

	if (dolby_vision_flags & FLAG_CERTIFICATION) {
		if (vf && (vf->type & VIDTYPE_COMPRESS)) {
			if (is_src_crop_valid(vf->src_crop)) {
				h_size = vf->compWidth -
					vf->src_crop.left - vf->src_crop.right;
				v_size = vf->compHeight -
					vf->src_crop.top - vf->src_crop.bottom;
			} else {
				h_size = vf->compWidth;
				v_size = vf->compHeight;
			}
		} else if (vf) {
			h_size = vf->width;
			v_size = vf->height;
		}
	}

	if (vf && (debug_dolby & 0x8))
		pr_dv_dbg("top1_proc: vf %px(index %d),mode %d,core_on %d %d,%d %d,flag %x\n",
				  vf, vf->omx_index, dolby_vision_mode,
				  top1_info.core_on, top2_info.core_on,
				  h_size, v_size, dolby_vision_flags);

	if (dolby_vision_mode == AMDV_OUTPUT_MODE_BYPASS)
		return 0;
	reset_flag =
		(amdv_reset & 1) &&
		(!top1_info.core_on) &&
		(top1_info.core_on_cnt == 0);

	if (vf && (vf->type & VIDTYPE_VIU_422))
		src_chroma_format = 2;
	else if (vf)
		src_chroma_format = 1;
	if (tv_hw5_setting &&
		(tv_hw5_setting->top1.src_format ==
		FORMAT_HDR10 ||
		tv_hw5_setting->top1.src_format ==
		FORMAT_HLG ||
		tv_hw5_setting->top1.src_format ==
		FORMAT_SDR ||
		tv_hw5_setting->top2.src_format ==
		FORMAT_SDR10))
		src_is_42210bit = true;

	if (vf)
		level = vf->src_fmt.py_level;
	else
		level = top1_info.py_level;

	if (tv_hw5_setting)
		tv_top_set
		(tv_hw5_setting->top1_reg,
		 tv_hw5_setting->top1b_reg,
		 tv_hw5_setting->top2_reg,
		 h_size, v_size,
		 top1_info.amdv_setting_video_flag, /* video enable */
		 src_chroma_format,
		 tv_hw5_setting->top1.input_mode == IN_MODE_HDMI,
		 src_is_42210bit, reset_flag, true, false, level);
	else if (hw5_reg_from_file)
		tv_top_set
		(NULL, NULL, NULL,
		 h_size, v_size,
		 top1_info.amdv_setting_video_flag, /* video enable */
		 src_chroma_format,
		 false,
		 src_is_42210bit, reset_flag, true, false, level);

	enable_amdv(1);
	if (tv_hw5_setting && last_tv_hw5_setting)
		memcpy(last_tv_hw5_setting, tv_hw5_setting,
			sizeof(*tv_hw5_setting));
	vf_top1->src_fmt.pr_done = true;
	return 0;
}

int amdolby_vision_process_hw5(struct vframe_s *vf_top1,
		struct vframe_s *vf_top2,
		u32 display_size,
		u8 toggle_mode, u8 pps_state)
{
	int src_chroma_format = 0;
	u32 h_size = (display_size >> 16) & 0xffff;
	u32 v_size = display_size & 0xffff;
	bool reset_flag = false;
	bool force_set = false;
	unsigned int mode = dolby_vision_mode;
	static bool video_turn_off = true;
	static bool video_on[VD_PATH_MAX];
	int video_status = 0;
	int policy_changed = 0;
	int format_changed = 0;
	bool src_is_42210bit = false;
	static bool reverse_status;
	bool reverse = false;
	bool reverse_changed = false;
	static u8 last_toggle_mode;
	static struct vframe_s *last_vf;
	struct vframe_s *vf;
	bool pr_done = false;
	u32 level;
	u32 h_ori;
	u32 v_ori;
	struct vd_proc_info_t *vd_proc_info;

	if (!is_aml_tvmode())
		return -1;

	if (!module_installed && !hw5_reg_from_file)
		return -1;

	vf = vf_top2;

	if (vf && (debug_dolby & 0x8))
		pr_dv_dbg("proc:vf %px %px(index %d),mode %d,on %d %d,size %d %d,type %x,flag %x\n",
			     vf_top1, vf_top2, vf->omx_index, dolby_vision_mode,
			     top1_info.core_on, top2_info.core_on,
			     h_size, v_size, vf->type, vf->flag);
	else if ((debug_dolby & 0x20000))
		pr_dv_dbg("proc: mode %d,on %d %d,size %d %d\n",
			     dolby_vision_mode,
			     top1_info.core_on, top2_info.core_on,
			     h_size, v_size);

	if (vf_top1 && !vf_top2) {
		/*only_top1*/
		return amdolby_vision_process_hw5_top1(vf_top1, display_size);
	}

	if (vf && vf->type & VIDTYPE_COMPRESS) {
		if (is_src_crop_valid(vf->src_crop)) {
			h_ori = vf->compWidth -
				vf->src_crop.left - vf->src_crop.right;
			v_ori = vf->compHeight -
				vf->src_crop.top - vf->src_crop.bottom;
			if (debug_dolby & 0x8)
				pr_dv_dbg("size %d %d, crop %d %d %d %d\n",
						  vf->compWidth, vf->compHeight,
						  vf->src_crop.left, vf->src_crop.right,
						  vf->src_crop.top, vf->src_crop.bottom);
		} else {
			h_ori = vf->compWidth;
			v_ori = vf->compHeight;
		}
	} else if (vf) {
		h_ori = vf->width;
		v_ori = vf->height;
	}
	vd_proc_info = get_vd_proc_amdv_info();
	if (vd_proc_info && tv_hw5_setting &&
		is_amdv_on() && vf &&
		top2_info.amdv_setting_video_flag) {/*check if slice changed*/
		if (vd_proc_info->slice_num != tv_hw5_setting->force_num_slices) {
			if (debug_dolby & 0x400000)
				pr_dv_dbg("slice_num change %d->%d\n",
					tv_hw5_setting->force_num_slices, vd_proc_info->slice_num);
			dolby_vision_flags |= FLAG_TOGGLE_FRAME;
			update_top2_control_path_flag = true;
			toggle_mode = true;
		}
		if (debug_dolby & 0x400000)
			pr_dv_dbg("no_compress %d, flag %x, type %x\n",
					  vd_proc_info->no_compress, vf->flag, vf->type);
		/*afbc+dw two data path,dw is after detunnel,so no need dv detunnel*/
		if (vd_proc_info->no_compress && (vf->type & VIDTYPE_COMPRESS))
			/*afbc on but vpp use dw*/
			disable_detunnel = true;
		/*else if	(!(vf->type & VIDTYPE_COMPRESS))*/
			/*only yuv data path,dw is after detunnel,so no need dv detunnel*/
			/*disable_detunnel = true;*/
		else
			disable_detunnel = false;
	}
	//if (update_top2_control_path_flag) {
	//	amdv_hw5_control_path(vf, vd_proc_info);
	//	update_top2_control_path_flag = false;
	//}

	if (dolby_vision_flags & FLAG_CERTIFICATION) {
		h_size = h_ori;
		v_size = v_ori;
		top2_info.run_mode_count = 1 +	amdv_run_mode_delay;
	} else {
		//if (vf && vf != last_vf && tv_hw5_setting)
			//update_aoi_flag(vf, display_size);
		last_vf = vf;
	}

	if (dolby_vision_flags & FLAG_TOGGLE_FRAME) {
		/* tv control path case */
		if (top2_info.core_disp_hsize != h_size ||
			top2_info.core_disp_vsize != v_size) {
			/* tvcore need force config for res change */
			force_set = true;
			if (debug_dolby & 8)
				pr_dv_dbg
				("tv update disp size %d %d -> %d %d, ori %d %d\n",
				 top2_info.core_disp_hsize,
				 top2_info.core_disp_vsize, h_size, v_size, h_ori, v_ori);
			top2_info.core_disp_hsize = h_size;
			top2_info.core_disp_vsize = v_size;
		}
		/*check if vd1 data same as vframe original size*/
		/*1.if there is a big diff,the precision effect may be misplaced,bypassing pr*/
		/*2.debug mode for force_top1_vskip, not bypassing pr, set vskip for top1 rdmif*/
		if (enable_top1 && dolby_vision_mode != AMDV_OUTPUT_MODE_BYPASS &&
			vf && !force_top1_vskip) {
			if (h_ori >= 1920) {
				if ((h_ori >  h_size && (h_ori - h_size) > 88) ||
					(v_ori > v_size && (v_ori - v_size) > 44))
					force_bypass_precision = true;
				else
					force_bypass_precision = false;
			} else if (h_ori >= 1280) {
				if ((h_ori >  h_size && (h_ori - h_size) > 64) ||
					(v_ori > v_size && (v_ori - v_size) > 36))
					force_bypass_precision = true;
				else
					force_bypass_precision = false;
			} else if (h_ori >= 720) {
				if ((h_ori >  h_size && (h_ori - h_size) > 44) ||
					(v_ori > v_size && (v_ori - v_size) > 32))
					force_bypass_precision = true;
				else
					force_bypass_precision = false;
			} else {
				if ((h_ori >  h_size && (h_ori - h_size) > 16) ||
					(v_ori > v_size && (v_ori - v_size) > 10))
					force_bypass_precision = true;
				else
					force_bypass_precision = false;
			}
			if (force_bypass_precision) {
				if (debug_dolby & 1)
					pr_dv_dbg("vd1 size big diff with ori, bypass precision! %dx%d %dx%d\n",
						h_size, v_size, h_ori, v_ori);
			}
		}

		if (!vf || toggle_mode != 1) {
			/* log to monitor if has dv toggles not needed */
			if (debug_dolby & 0x100)
				pr_dv_dbg("NULL/RPT frame %p, hdr module %s, video %s\n",
				     vf,
				     get_hdr_module_status(VD1_PATH, VPP_TOP0)
				     == HDR_MODULE_ON ? "on" : "off",
				     get_video_enabled(VD1_PATH) ? "on" : "off");
		}
	}
	last_toggle_mode = toggle_mode;

	calculate_crc();

	video_status = is_video_turn_on(video_on, VD1_PATH);
	if (video_status == -1) {
		video_turn_off = true;
		if (debug_dolby & 0x100)
			pr_dv_dbg("VD1 video off, video_status -1\n");
	} else if (video_status == 1) {
		if (debug_dolby & 0x100)
			pr_dv_dbg("VD1 video on, video_status 1\n");
		video_turn_off = false;
	}

	if (dolby_vision_mode != amdv_target_mode)
		format_changed = 1;

	/* monitor policy changes */
	policy_changed = is_policy_changed();

	if (policy_changed || format_changed)
		amdv_set_toggle_flag(1);

	if (is_aml_tvmode()) {
		reverse = get_video_reverse();
		if (reverse != reverse_status) {
			reverse_status = reverse;
			reverse_changed = true;
		}
		if (policy_changed || format_changed ||
			video_status == 1 || reverse_changed) {
			if (debug_dolby & 100)
				pr_dv_dbg("tv %s %s %s %s\n",
					policy_changed ? "policy changed" : "",
					video_status ? "video_status changed" : "",
					format_changed ? "format_changed" : "",
					reverse_changed ? "reverse_changed" : "");
			}
	}

	if (policy_changed || format_changed ||
	    (video_status == 1 && !(dolby_vision_flags & FLAG_CERTIFICATION)) ||
	    need_update_cfg || reverse_changed) {
		if (debug_dolby & 1)
			pr_dv_dbg("video %s,osd %s,vf %p,toggle %d\n",
				     video_turn_off ? "off" : "on",
				     is_graphics_output_off() ? "off" : "on",
				     vf, toggle_mode);
		/* do not toggle a new el vf */
		if (toggle_mode == 1)
			toggle_mode = 0;
		if (vf &&
		    !amdv_parse_metadata
		    (vf, VD1_PATH, toggle_mode, false, false)) {
			amdv_set_toggle_flag(1);
		}
		need_update_cfg = false;
	}

	if (debug_dolby & 2)
		pr_dv_dbg("vf %p,turn_off %d,video_status %d,toggle %d,flag %x,size %d %d,st %d\n",
			vf, video_turn_off, video_status,
			toggle_mode, dolby_vision_flags,
			h_size, v_size, dolby_vision_status);

	if ((!vf && video_turn_off) ||
	    (video_status == -1)) {
		if (amdv_policy_process(vf, &mode, FORMAT_SDR)) {
			if (debug_dolby & 0x100)
				pr_dv_dbg("Fake SDR, mode->%d\n", mode);
			if (dolby_vision_policy == AMDV_FOLLOW_SOURCE &&
			    mode == AMDV_OUTPUT_MODE_BYPASS) {
				amdv_target_mode =
					AMDV_OUTPUT_MODE_BYPASS;
				dolby_vision_mode =
					AMDV_OUTPUT_MODE_BYPASS;
				amdv_set_toggle_flag(0);
				amdv_wait_on = false;
				top2_v_info.amdv_wait_init = false;
			} else {
				amdv_set_toggle_flag(1);
			}
		}
		if ((dolby_vision_flags & FLAG_TOGGLE_FRAME) ||
		((video_status == -1) && top2_info.core_on)) {
			if (debug_dolby & 0x1)
				pr_dv_dbg("update when video off\n");
			amdv_parse_metadata
				(NULL, VD1_PATH, 1, false, false);
			amdv_set_toggle_flag(1);
		}
		if (!vf && video_turn_off &&
			!top2_info.core_on &&
			top2_v_info.amdv_src_format != 0) {
			if (debug_dolby & 0x1)
				pr_dv_dbg("update src_fmt when video off\n");
			top2_v_info.amdv_src_format = 0;
		}
	}
	if (dolby_vision_mode == AMDV_OUTPUT_MODE_BYPASS) {
		if (dolby_vision_status != BYPASS_PROCESS)
			enable_amdv(0);
		dolby_vision_flags &= ~FLAG_TOGGLE_FRAME;
		return 0;
	}
	if ((dolby_vision_flags & FLAG_CERTIFICATION) ||
	    (dolby_vision_flags & FLAG_BYPASS_VPP))
		video_effect_bypass(1);

	if (!p_funcs_stb && (!p_funcs_tv && !hw5_reg_from_file)) {
		dolby_vision_flags &= ~FLAG_TOGGLE_FRAME;
		top2_v_info.tv_dovi_setting_change_flag = false;
		return 0;
	}
	if (update_top2_control_path_flag) {
		amdv_hw5_control_path(vf, vd_proc_info);
		update_top2_control_path_flag = false;
	}

	pr_done = vf ?  vf->src_fmt.pr_done : false;
	if (vf)
		level = vf->src_fmt.py_level;
	else
		level = top2_info.py_level;

	if (dolby_vision_flags & FLAG_TOGGLE_FRAME) {
		if (!(dolby_vision_flags & FLAG_CERTIFICATION)) {
			if (enable_top1) {
				if ((amdv_reset & 1) && !top1_info.core_on &&
					top1_info.core_on_cnt == 0 &&
					!top2_info.core_on)
					reset_flag = true;
			} else {
				if ((amdv_reset & 1) && !top2_info.core_on &&
					top2_info.core_on_cnt == 0)
					reset_flag = true;
			}
		}

		if (top2_v_info.tv_dovi_setting_change_flag || force_set) {
			if (vf && (vf->type & VIDTYPE_VIU_422))
				src_chroma_format = 2;
			else if (vf)
				src_chroma_format = 1;
			if (tv_hw5_setting &&
				(tv_hw5_setting->top2.src_format ==
				FORMAT_HDR10 ||
				tv_hw5_setting->top2.src_format ==
				FORMAT_HLG ||
				tv_hw5_setting->top2.src_format ==
				FORMAT_SDR ||
				tv_hw5_setting->top2.src_format ==
				FORMAT_SDR10))
				src_is_42210bit = true;

			if (tv_hw5_setting)
				tv_top_set
				(tv_hw5_setting->top1_reg,
				 tv_hw5_setting->top1b_reg,
				 tv_hw5_setting->top2_reg,
				 h_size, v_size,
				 top2_info.amdv_setting_video_flag, /* video enable */
				 src_chroma_format,
				 tv_hw5_setting->top2.input_mode == IN_MODE_HDMI,
				 src_is_42210bit, reset_flag, true, pr_done, level);
			else if (hw5_reg_from_file)
				tv_top_set
				(NULL, NULL, NULL,
				 h_size, v_size,
				 top2_info.amdv_setting_video_flag, /* video enable */
				 src_chroma_format,
				 false,
				 src_is_42210bit, reset_flag, true, pr_done, level);

			if (!h_size || !v_size)
				top2_info.amdv_setting_video_flag = false;
			if (top2_info.amdv_setting_video_flag &&
			    top2_info.core_on_cnt == 0) {
				if (debug_dolby & 0x100)
					pr_dv_dbg("first frame reset %d\n",
					     reset_flag);
			}
			enable_amdv(1);
			if (vf_top1) {
				if (debug_dolby & 0x2)
					pr_dv_dbg("update %px(index %d) pr_done %d=>1\n",
						vf_top1, vf_top1->omx_index,
						vf_top1->src_fmt.pr_done);
				vf_top1->src_fmt.pr_done = true;
			}
			if (tv_hw5_setting)
				update_amdv_status(tv_hw5_setting->top2.src_format);
			top2_v_info.tv_dovi_setting_change_flag = false;
			if (tv_hw5_setting && last_tv_hw5_setting)
				memcpy(last_tv_hw5_setting, tv_hw5_setting,
				sizeof(*tv_hw5_setting));
		}
		dolby_vision_flags &= ~FLAG_TOGGLE_FRAME;
	} else if (top2_info.core_on) {
		if (force_set || force_update_top2) {
			if (force_set)
				reset_flag = true;
			if (tv_hw5_setting &&
				(tv_hw5_setting->top2.src_format ==
				FORMAT_HDR10 ||
				tv_hw5_setting->top2.src_format ==
				FORMAT_HLG ||
				tv_hw5_setting->top2.src_format ==
				FORMAT_SDR ||
				tv_hw5_setting->top2.src_format ==
				FORMAT_SDR10))
				src_is_42210bit = true;

			if (tv_hw5_setting)
				tv_top_set
				(tv_hw5_setting->top1_reg,
				 tv_hw5_setting->top1b_reg,
				 tv_hw5_setting->top2_reg,
				 h_size, v_size,
				 top2_info.amdv_setting_video_flag, /* BL enable */
				 src_chroma_format,
				 tv_hw5_setting->top2.input_mode == IN_MODE_HDMI,
				 src_is_42210bit, reset_flag, toggle_mode, pr_done, level);
			else if (hw5_reg_from_file)
				tv_top_set
				(NULL, NULL, NULL,
				 h_size, v_size,
				 top2_info.amdv_setting_video_flag, /* BL enable */
				 src_chroma_format,
				 false,
				 src_is_42210bit, reset_flag, toggle_mode, pr_done, level);
		}
	}
	if (top2_info.core_on) {
		if (top2_info.run_mode_count <= amdv_run_mode_delay + 1)
			top2_info.run_mode_count++;
	} else {
		top2_info.run_mode_count = 0;
	}

	//if (debug_dolby & 8)
	//	pr_dv_dbg("%s: run_mode_count %d\n",
	//	     __func__, top2_info.run_mode_count);
	return 0;
}

