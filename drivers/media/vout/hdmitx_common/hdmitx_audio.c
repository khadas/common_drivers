// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/soundcard.h>
#include <linux/mutex.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/control.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_common.h>
#include "hdmitx_log.h"

/* Recommended N and Expected CTS for 32kHz */
static const struct hdmi_audio_fs_ncts aud_32k_para = {
	.array[0] = {
		.tmds_clk = 25175,
		.n = 4576,
		.cts = 28125,
		.n_30bit = 9152,
		.cts_30bit = 70312,
		.n_36bit = 9152,
		.cts_36bit = 84375,
		.n_48bit = 4576,
		.cts_48bit = 56250,
	},
	.array[1] = {
		.tmds_clk = 25200,
		.n = 4096,
		.cts = 25200,
		.n_30bit = 4096,
		.cts_30bit = 31500,
		.n_36bit = 4096,
		.cts_36bit = 37800,
		.n_48bit = 4096,
		.cts_48bit = 50400,
	},
	.array[2] = {
		.tmds_clk = 27000,
		.n = 4096,
		.cts = 27000,
		.n_30bit = 4096,
		.cts_30bit = 33750,
		.n_36bit = 4096,
		.cts_36bit = 40500,
		.n_48bit = 4096,
		.cts_48bit = 54000,
	},
	.array[3] = {
		.tmds_clk = 27027,
		.n = 4096,
		.cts = 27027,
		.n_30bit = 8192,
		.cts_30bit = 67567,
		.n_36bit = 8192,
		.cts_36bit = 81081,
		.n_48bit = 4096,
		.cts_48bit = 54054,
	},
	.array[4] = {
		.tmds_clk = 54000,
		.n = 4096,
		.cts = 54000,
		.n_30bit = 4096,
		.cts_30bit = 67500,
		.n_36bit = 4096,
		.cts_36bit = 81000,
		.n_48bit = 4096,
		.cts_48bit = 108000,
	},
	.array[5] = {
		.tmds_clk = 54054,
		.n = 4096,
		.cts = 54054,
		.n_30bit = 8192,
		.cts_30bit = 135135,
		.n_36bit = 4096,
		.cts_36bit = 81081,
		.n_48bit = 4096,
		.cts_48bit = 108108,
	},
	.array[6] = {
		.tmds_clk = 74176,
		.n = 11648,
		.cts = 210937,
		.n_30bit = 11648,
		.cts_30bit = 263671,
		.n_36bit = 11648,
		.cts_36bit = 316406,
		.n_48bit = 11648,
		.cts_48bit = 421875,
	},
	.array[7] = {
		.tmds_clk = 74250,
		.n = 4096,
		.cts = 74250,
		.n_30bit = 8192,
		.cts_30bit = 185625,
		.n_36bit = 4096,
		.cts_36bit = 111375,
		.n_48bit = 4096,
		.cts_48bit = 148500,
	},
	.array[8] = {
		.tmds_clk = 148352,
		.n = 11648,
		.cts = 421875,
		.n_30bit = 11648,
		.cts_30bit = 527343,
		.n_36bit = 11648,
		.cts_36bit = 632812,
		.n_48bit = 11648,
		.cts_48bit = 843750,
	},
	.array[9] = {
		.tmds_clk = 148500,
		.n = 4096,
		.cts = 148500,
		.n_30bit = 4096,
		.cts_30bit = 185625,
		.n_36bit = 4096,
		.cts_36bit = 222750,
		.n_48bit = 4096,
		.cts_48bit = 297000,
	},
	.array[10] = {
		.tmds_clk = 296703,
		.n = 5824,
		.cts = 421875,
		.n_30bit = 5824,
		.cts_30bit = 527343,
		.n_36bit = 5824,
		.cts_36bit = 632812,
		.n_48bit = 5824,
		.cts_48bit = 843750,
	},
	.array[11] = {
		.tmds_clk = 297000,
		.n = 3072,
		.cts = 222750,
		.n_30bit = 6144,
		.cts_30bit = 556875,
		.n_36bit = 4096,
		.cts_36bit = 445500,
		.n_48bit = 3072,
		.cts_48bit = 445500,
	},
	.array[12] = {
		.tmds_clk = 593407,
		.n = 5824,
		.cts = 843750,
	},
	.array[13] = {
		.tmds_clk = 594000,
		.n = 3072,
		.cts = 445500,
	},
	.def_n = 4096,
};

/* Recommended N and Expected CTS for 44.1kHz and Multiples */
static const struct hdmi_audio_fs_ncts aud_44k1_para = {
	.array[0] = {
		.tmds_clk = 25175,
		.n = 7007,
		.cts = 31250,
		.n_30bit = 14014,
		.cts_30bit = 78125,
		.n_36bit = 7007,
		.cts_36bit = 46875,
		.n_48bit = 7007,
		.cts_48bit = 62500,
	},
	.array[1] = {
		.tmds_clk = 25200,
		.n = 6272,
		.cts = 28000,
		.n_30bit = 6272,
		.cts_30bit = 35000,
		.n_36bit = 6272,
		.cts_36bit = 42000,
		.n_48bit = 6272,
		.cts_48bit = 56000,
	},
	.array[2] = {
		.tmds_clk = 27000,
		.n = 6272,
		.cts = 30000,
		.n_30bit = 6272,
		.cts_30bit = 37500,
		.n_36bit = 6272,
		.cts_36bit = 45000,
		.n_48bit = 6272,
		.cts_48bit = 60000,
	},
	.array[3] = {
		.tmds_clk = 27027,
		.n = 6272,
		.cts = 30030,
		.n_30bit = 12544,
		.cts_30bit = 75075,
		.n_36bit = 6272,
		.cts_36bit = 45045,
		.n_48bit = 6272,
		.cts_48bit = 60060,
	},
	.array[4] = {
		.tmds_clk = 54000,
		.n = 6272,
		.cts = 60000,
		.n_30bit = 6272,
		.cts_30bit = 75000,
		.n_36bit = 6272,
		.cts_36bit = 90000,
		.n_48bit = 6272,
		.cts_48bit = 120000,
	},
	.array[5] = {
		.tmds_clk = 54054,
		.n = 6272,
		.cts = 60060,
		.n_30bit = 6272,
		.cts_30bit = 75075,
		.n_36bit = 6272,
		.cts_36bit = 90090,
		.n_48bit = 6272,
		.cts_48bit = 120120,
	},
	.array[6] = {
		.tmds_clk = 74176,
		.n = 17836,
		.cts = 234375,
		.n_30bit = 17836,
		.cts_30bit = 292968,
		.n_36bit = 17836,
		.cts_36bit = 351562,
		.n_48bit = 17836,
		.cts_48bit = 468750,
	},
	.array[7] = {
		.tmds_clk = 74250,
		.n = 6272,
		.cts = 82500,
		.n_30bit = 6272,
		.cts_30bit = 103125,
		.n_36bit = 6272,
		.cts_36bit = 123750,
		.n_48bit = 6272,
		.cts_48bit = 165000,
	},
	.array[8] = {
		.tmds_clk = 148352,
		.n = 8918,
		.cts = 234375,
		.n_30bit = 17836,
		.cts_30bit = 585937,
		.n_36bit = 17836,
		.cts_36bit = 703125,
		.n_48bit = 8918,
		.cts_48bit = 468750,
	},
	.array[9] = {
		.tmds_clk = 148500,
		.n = 6272,
		.cts = 165000,
		.n_30bit = 6272,
		.cts_30bit = 206250,
		.n_36bit = 6272,
		.cts_36bit = 247500,
		.n_48bit = 6272,
		.cts_48bit = 330000,
	},
	.array[10] = {
		.tmds_clk = 296703,
		.n = 4459,
		.cts = 234375,
		.n_30bit = 8918,
		.cts_30bit = 585937,
		.n_36bit = 8918,
		.cts_36bit = 703125,
		.n_48bit = 4459,
		.cts_48bit = 468750,
	},
	.array[11] = {
		.tmds_clk = 297000,
		.n = 4707,
		.cts = 247500,
		.n_30bit = 4704,
		.cts_30bit = 309375,
		.n_36bit = 4704,
		.cts_36bit = 371250,
		.n_48bit = 4704,
		.cts_48bit = 495000,
	},
	.array[12] = {
		.tmds_clk = 593407,
		.n = 8918,
		.cts = 937500,
	},
	.array[13] = {
		.tmds_clk = 594000,
		.n = 9408,
		.cts = 990000,
	},
	.def_n = 6272,
};

/* Recommended N and Expected CTS for 48kHz and Multiples */
static const struct hdmi_audio_fs_ncts aud_48k_para = {
	.array[0] = {
		.tmds_clk = 25175,
		.n = 6864,
		.cts = 28125,
		.n_30bit = 9152,
		.cts_30bit = 46875,
		.n_36bit = 9152,
		.cts_36bit = 58250,
		.n_48bit = 6864,
		.cts_48bit = 56250,
	},
	.array[1] = {
		.tmds_clk = 25200,
		.n = 6144,
		.cts = 25200,
		.n_30bit = 6144,
		.cts_30bit = 31500,
		.n_36bit = 6144,
		.cts_36bit = 37800,
		.n_48bit = 6144,
		.cts_48bit = 50400,
	},
	.array[2] = {
		.tmds_clk = 27000,
		.n = 6144,
		.cts = 27000,
		.n_30bit = 6144,
		.cts_30bit = 33750,
		.n_36bit = 6144,
		.cts_36bit = 40500,
		.n_48bit = 6144,
		.cts_48bit = 54000,
	},
	.array[3] = {
		.tmds_clk = 27027,
		.n = 6144,
		.cts = 27027,
		.n_30bit = 8192,
		.cts_30bit = 45045,
		.n_36bit = 8192,
		.cts_36bit = 54054,
		.n_48bit = 6144,
		.cts_48bit = 54054,
	},
	.array[4] = {
		.tmds_clk = 54000,
		.n = 6144,
		.cts = 54000,
		.n_30bit = 6144,
		.cts_30bit = 67500,
		.n_36bit = 6144,
		.cts_36bit = 81000,
		.n_48bit = 6144,
		.cts_48bit = 108000,
	},
	.array[5] = {
		.tmds_clk = 54054,
		.n = 6144,
		.cts = 54054,
		.n_30bit = 8192,
		.cts_30bit = 90090,
		.n_36bit = 6144,
		.cts_36bit = 81081,
		.n_48bit = 6144,
		.cts_48bit = 108108,
	},
	.array[6] = {
		.tmds_clk = 74176,
		.n = 11648,
		.cts = 140625,
		.n_30bit = 11648,
		.cts_30bit = 175781,
		.n_36bit = 11648,
		.cts_36bit = 210937,
		.n_48bit = 11648,
		.cts_48bit = 281250,
	},
	.array[7] = {
		.tmds_clk = 74250,
		.n = 6144,
		.cts = 74250,
		.n_30bit = 12288,
		.cts_30bit = 185625,
		.n_36bit = 6144,
		.cts_36bit = 111375,
		.n_48bit = 6144,
		.cts_48bit = 148500,
	},
	.array[8] = {
		.tmds_clk = 148352,
		.n = 5824,
		.cts = 140625,
		.n_30bit = 11648,
		.cts_30bit = 351562,
		.n_36bit = 11648,
		.cts_36bit = 421875,
		.n_48bit = 5824,
		.cts_48bit = 281250,
	},
	.array[9] = {
		.tmds_clk = 148500,
		.n = 6144,
		.cts = 148500,
		.n_30bit = 6144,
		.cts_30bit = 185625,
		.n_36bit = 6144,
		.cts_36bit = 222750,
		.n_48bit = 6144,
		.cts_48bit = 297000,
	},
	.array[10] = {
		.tmds_clk = 296703,
		.n = 5824,
		.cts = 281250,
		.n_30bit = 11648,
		.cts_30bit = 703125,
		.n_36bit = 5824,
		.cts_36bit = 421875,
		.n_48bit = 5824,
		.cts_48bit = 562500,
	},
	.array[11] = {
		.tmds_clk = 297000,
		.n = 5120,
		.cts = 247500,
		.n_30bit = 5120,
		.cts_30bit = 309375,
		.n_36bit = 5120,
		.cts_36bit = 371250,
		.n_48bit = 5120,
		.cts_48bit = 495000,
	},
	.array[12] = {
		.tmds_clk = 593407,
		.n = 5824,
		.cts = 562500,
	},
	.array[13] = {
		.tmds_clk = 594000,
		.n = 6144,
		.cts = 594000,
	},
	.def_n = 6144,
};

static const struct hdmi_audio_fs_ncts *all_aud_paras[] = {
	&aud_32k_para,
	&aud_44k1_para,
	&aud_48k_para,
	NULL,
};

/* note: param tmds_clk is actually pixel_clk
 * for 8bit mode, use param cd for ACR_N of
 * deep color mode
 */
u32 hdmitx_hw_get_audio_n_paras(enum hdmi_audio_fs fs,
				  enum hdmi_color_depth cd,
				  u32 tmds_clk)
{
	const struct hdmi_audio_fs_ncts *p = NULL;
	u32 i, n;
	u32 N_multiples = 1;

	HDMITX_INFO("fs = %d, cd = %d, tmds_clk = %d\n", fs, cd, tmds_clk);
	switch (fs) {
	case FS_32K:
		p = all_aud_paras[0];
		N_multiples = 1;
		break;
	case FS_44K1:
		p = all_aud_paras[1];
		N_multiples = 1;
		break;
	case FS_88K2:
		p = all_aud_paras[1];
		N_multiples = 2;
		break;
	case FS_176K4:
		p = all_aud_paras[1];
		N_multiples = 4;
		break;
	case FS_48K:
		p = all_aud_paras[2];
		N_multiples = 1;
		break;
	case FS_96K:
		p = all_aud_paras[2];
		N_multiples = 2;
		break;
	case FS_192K:
		p = all_aud_paras[2];
		N_multiples = 4;
		break;
	default: /* Default as FS_48K */
		p = all_aud_paras[2];
		N_multiples = 1;
		break;
	}
	for (i = 0; i < AUDIO_PARA_MAX_NUM; i++) {
		if (tmds_clk == p->array[i].tmds_clk ||
		    (tmds_clk + 1) == p->array[i].tmds_clk ||
		    (tmds_clk - 1) == p->array[i].tmds_clk)
			break;
	}

	if (i < AUDIO_PARA_MAX_NUM)
		if (cd == COLORDEPTH_24B)
			n = p->array[i].n ? p->array[i].n : p->def_n;
		else if (cd == COLORDEPTH_30B)
			n = p->array[i].n_30bit ?
				p->array[i].n_30bit : p->def_n;
		else if (cd == COLORDEPTH_36B)
			n = p->array[i].n_36bit ?
				p->array[i].n_36bit : p->def_n;
		else if (cd == COLORDEPTH_48B)
			n = p->array[i].n_48bit ?
				p->array[i].n_48bit : p->def_n;
		else
			n = p->array[i].n ? p->array[i].n : p->def_n;
	else
		n = p->def_n;
	return n * N_multiples;
}

int hdmitx_audio_para_print(struct aud_para *audio_para, char *log_buf)
{
	char buf[256];
	const char *conf;
	ssize_t pos = 0;
	int len = sizeof(buf);

	switch (audio_para->aud_src_if) {
	case AUD_SRC_IF_SPDIF:
		conf = "SPDIF";
		break;
	case AUD_SRC_IF_I2S:
		conf = "I2S";
		break;
	case AUD_SRC_IF_TDM:
		conf = "TDM";
		break;
	default:
		conf = "none";
	}
	pos += snprintf(buf + pos, len - pos, "audio source: %s\n", conf);

	switch (audio_para->type) {
	case CT_REFER_TO_STREAM:
		conf = "refer to stream header";
		break;
	case CT_PCM:
		conf = "L-PCM";
		break;
	case CT_AC_3:
		conf = "AC-3";
		break;
	case CT_MPEG1:
		conf = "MPEG1";
		break;
	case CT_MP3:
		conf = "MP3";
		break;
	case CT_MPEG2:
		conf = "MPEG2";
		break;
	case CT_AAC:
		conf = "AAC";
		break;
	case CT_DTS:
		conf = "DTS";
		break;
	case CT_ATRAC:
		conf = "ATRAC";
		break;
	case CT_ONE_BIT_AUDIO:
		conf = "One Bit Audio";
		break;
	case CT_DD_P:
		conf = "Dobly Digital+";
		break;
	case CT_DTS_HD:
		conf = "DTS_HD";
		break;
	case CT_MAT:
		conf = "MAT";
		break;
	case CT_DST:
		conf = "DST";
		break;
	case CT_WMA:
		conf = "WMA";
		break;
	default:
		conf = "MAX";
	}
	pos += snprintf(buf + pos, len - pos, "audio type: %s\n", conf);

	switch (audio_para->chs) {
	case CC_REFER_TO_STREAM:
		conf = "refer to stream header";
		break;
	case CC_2CH:
		conf = "2 channels";
		break;
	case CC_3CH:
		conf = "3 channels";
		break;
	case CC_4CH:
		conf = "4 channels";
		break;
	case CC_5CH:
		conf = "5 channels";
		break;
	case CC_6CH:
		conf = "6 channels";
		break;
	case CC_7CH:
		conf = "7 channels";
		break;
	case CC_8CH:
		conf = "8 channels";
		break;
	default:
		conf = "MAX";
	}
	pos += snprintf(buf + pos, len - pos, "audio channel num: %s\n", conf);

	switch (audio_para->rate) {
	case FS_REFER_TO_STREAM:
		conf = "refer to stream header";
		break;
	case FS_32K:
		conf = "32kHz";
		break;
	case FS_44K1:
		conf = "44.1kHz";
		break;
	case FS_48K:
		conf = "48kHz";
		break;
	case FS_88K2:
		conf = "88.2kHz";
		break;
	case FS_96K:
		conf = "96kHz";
		break;
	case FS_176K4:
		conf = "176.4kHz";
		break;
	case FS_192K:
		conf = "192kHz";
		break;
	case FS_768K:
		conf = "768kHz";
		break;
	default:
		conf = "MAX";
	}
	pos += snprintf(buf + pos, len - pos, "audio sample rate: %s\n", conf);

	switch (audio_para->size) {
	case SS_REFER_TO_STREAM:
		conf = "refer to stream header";
		break;
	case SS_16BITS:
		conf = "16bit";
		break;
	case SS_20BITS:
		conf = "20bit";
		break;
	case SS_24BITS:
		conf = "24bit";
		break;
	default:
		conf = "MAX";
	}
	pos += snprintf(buf + pos, len - pos, "audio sample size: %s\n", conf);

	if (log_buf)
		sprintf(log_buf, "%s", buf);
	else
		HDMITX_INFO("%s", buf);

	return pos;
}

static struct rate_map_fs map_fs[] = {
	{0, FS_REFER_TO_STREAM},
	{32000, FS_32K},
	{44100, FS_44K1},
	{48000, FS_48K},
	{88200, FS_88K2},
	{96000, FS_96K},
	{176400, FS_176K4},
	{192000, FS_192K},
};

static enum hdmi_audio_fs aud_samp_rate_map(u32 rate)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(map_fs); i++) {
		if (map_fs[i].rate == rate)
			return map_fs[i].fs;
	}
	return FS_MAX;
}

static u8 *aud_type_string[] = {
	"CT_REFER_TO_STREAM",
	"CT_PCM",
	"CT_AC_3",
	"CT_MPEG1",
	"CT_MP3",
	"CT_MPEG2",
	"CT_AAC",
	"CT_DTS",
	"CT_ATRAC",
	"CT_ONE_BIT_AUDIO",
	"CT_DOLBY_D",
	"CT_DTS_HD",
	"CT_MAT",
	"CT_DST",
	"CT_WMA",
	"CT_MAX",
};

static struct size_map aud_size_map_ss[] = {
	{0,	 SS_REFER_TO_STREAM},
	{16,	SS_16BITS},
	{20,	SS_20BITS},
	{24,	SS_24BITS},
	{32,	SS_24BITS}, /* for hdmitx, max is 24bits */
};

static enum hdmi_audio_sampsize aud_size_map(u32 bits)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(aud_size_map_ss); i++) {
		if (bits == aud_size_map_ss[i].sample_bits)
			return aud_size_map_ss[i].ss;
	}
	return SS_MAX;
}

u32 aud_sr_idx_to_val(enum hdmi_audio_fs e_sr_idx)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(map_fs); i++) {
		if (map_fs[i].fs == e_sr_idx)
			return map_fs[i].rate / 1000;
	}
	HDMITX_INFO("wrong idx: %d\n", e_sr_idx);
	return -1;
}

static bool hdmitx_set_i2s_mask(struct aud_para *tx_aud_param, char ch_num, char ch_msk)
{
	unsigned int update_flag;

	if (!(ch_num == 2 || ch_num == 4 ||
	      ch_num == 6 || ch_num == 8)) {
		HDMITX_INFO("err chn setting, must be 2, 4, 6 or 8, Rst as def\n");
		return 0;
	}
	if (ch_msk == 0) {
		HDMITX_INFO("err chn msk, must larger than 0\n");
		return 0;
	}
	update_flag = (ch_num << 4) + ch_msk;
	if (update_flag != tx_aud_param->aud_output_i2s_ch) {
		tx_aud_param->aud_output_i2s_ch = update_flag;
		return 1;
	}
	return 0;
}

void hdmitx_audio_notify_callback(struct hdmitx_common *tx_comm,
	struct hdmitx_hw_common *tx_hw_base,
	struct notifier_block *block,
	unsigned long cmd, void *para)
{
	struct aud_para *tx_aud_param = &tx_comm->cur_audio_param;
	/* front audio module callback parameters */
	struct aud_para *aud_param = (struct aud_para *)para;
	enum hdmi_audio_fs n_rate = aud_samp_rate_map(aud_param->rate);
	enum hdmi_audio_sampsize n_size = aud_size_map(aud_param->size);
	int audio_param_update_flag = 0;

	if (tx_aud_param->prepare) {
		tx_aud_param->prepare = 0;
		hdmitx_hw_cntl_misc(tx_hw_base, MISC_AUDIO_ACR_CTRL, 0);
		hdmitx_hw_cntl_misc(tx_hw_base, MISC_AUDIO_PREPARE, 0);
		tx_aud_param->type = CT_PREPARE;
		HDMITX_INFO("%s[%d] audio prepare\n", __func__, __LINE__);
		return;
	}
	HDMITX_INFO("%s[%d] type:%lu rate:%d size:%d chs:%d i2s_ch_mask:%d aud_src_if:%d\n",
		__func__, __LINE__, cmd, n_rate, n_size, aud_param->chs,
		aud_param->i2s_ch_mask, aud_param->aud_src_if);
	/* check audio parameters changing, if true, update hdmitx audio hw */
	if (hdmitx_set_i2s_mask(tx_aud_param, aud_param->chs, aud_param->i2s_ch_mask))
		audio_param_update_flag = 1;
	if (tx_aud_param->rate != n_rate) {
		tx_aud_param->rate = n_rate;
		audio_param_update_flag = 1;
	}

	if (tx_aud_param->type != cmd) {
		tx_aud_param->type = cmd;
		audio_param_update_flag = 1;
		HDMITX_INFO("aout notify format %s\n",
			aud_type_string[tx_aud_param->type & 0xff]);
	}

	if (tx_aud_param->size != n_size) {
		tx_aud_param->size = n_size;
		audio_param_update_flag = 1;
	}

	if (tx_aud_param->chs != (aud_param->chs - 1)) {
		tx_aud_param->chs = aud_param->chs - 1;
		audio_param_update_flag = 1;
	}
	if (tx_aud_param->aud_src_if != aud_param->aud_src_if) {
		tx_aud_param->aud_src_if = aud_param->aud_src_if;
		audio_param_update_flag = 1;
	}
	memcpy(tx_aud_param->status, aud_param->status, sizeof(aud_param->status));

	if (audio_param_update_flag) {
		/* plug-in & update audio param */
		if (tx_comm->hpd_state == 1) {
			tx_aud_param->aud_notify_update = 1;
			tx_hw_base->setaudmode(tx_hw_base, tx_aud_param);
			tx_aud_param->aud_notify_update = 0;
			HDMITX_INFO("set audio param\n");
		}
	}
	if (aud_param->fifo_rst)
		hdmitx_hw_cntl_misc(tx_hw_base, MISC_AUDIO_RESET, 1);
}

static audio_en_callback cb_set_audio_output_en;
static audio_st_callback cb_get_audio_status;

int hdmitx_audio_register_ctrl_callback(audio_en_callback cb1, audio_st_callback cb2)
{
	if (!cb1 || !cb2)
		return -1;
	cb_set_audio_output_en = cb1;
	cb_get_audio_status = cb2;
	return 0;
}

void hdmitx_ext_set_audio_output(int enable)
{
	cb_set_audio_output_en ? cb_set_audio_output_en(enable) : 0;
}
EXPORT_SYMBOL(hdmitx_ext_set_audio_output);

int hdmitx_ext_get_audio_status(void)
{
	if (cb_get_audio_status)
		return cb_get_audio_status();
	return 0;
}
EXPORT_SYMBOL(hdmitx_ext_get_audio_status);
