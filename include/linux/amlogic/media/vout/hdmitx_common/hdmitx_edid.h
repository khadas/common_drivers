/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#ifndef __HDMITX_EDID_H_
#define __HDMITX_EDID_H_

#include <linux/types.h>
#include <linux/amlogic/media/vout/vinfo.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_mode.h>
#include <linux/amlogic/media/vout/hdmi_tx_ext.h>

#define EDID_MAX_BLOCK		8
#define VESA_MAX_TIMING		64
#define AUD_MAX_NUM			60
#define MAX_RAW_LEN			64

enum frl_rate_enum {
	FRL_NONE = 0,
	FRL_3G3L = 1,
	FRL_6G3L = 2,
	FRL_6G4L = 3,
	FRL_8G4L = 4,
	FRL_10G4L = 5,
	FRL_12G4L = 6,
	FRL_INVALID = 7,
};

enum flt_tx_states {
	FLT_TX_LTS_L,	/* legacy mode */
	FLT_TX_LTS_1,	/* read edid */
	FLT_TX_LTS_2,	/* prepare for frl */
	FLT_TX_LTS_3,	/* training in progress */
	FLT_TX_LTS_4,	/* update frl_rate */
	/* frl training passed, start frl w/o video */
	FLT_TX_LTS_P1,
	/* start frl w/ video */
	FLT_TX_LTS_P2,
	/* maintain frl video */
	FLT_TX_LTS_P3,
};

enum ffe_levels {
	FFE_LEVEL0 = 0,
	FFE_LEVEL1 = 1,
	FFE_LEVEL2 = 2,
	FFE_LEVEL3 = 3,
};

enum ltp_patterns {
	/* No Link Training Pattern requested */
	LTP_PATTERN_NO_LTP = 0,
	/* All 1's pattern */
	LTP_PATTERN_LTP1 = 1,
	/* All 0's pattern */
	LTP_PATTERN_LTP2 = 2,
	/* Nyquist clock pattern */
	LTP_PATTERN_LTP3 = 3,
	/* Source TxFFE Compliance Test Pattern */
	LTP_PATTERN_LTP4 = 4,
	/* LFSR 0 */
	LTP_PATTERN_LTP5 = 5,
	/* LFSR 1 */
	LTP_PATTERN_LTP6 = 6,
	/* LFSR 2 */
	LTP_PATTERN_LTP7 = 7,
	/* LFSR 3 */
	LTP_PATTERN_LTP8 = 8,
	/* reserved 9 ~ 0xd */
	LTP_PATTERN_RSVD = 9,
	/* Sink requests Source to update FFE */
	LTP_PATTERN_UPDATE_FFE = 0xE,
	/* Sink requests Source to change link mode(rate) to a lower value */
	LTP_PATTERN_CHG_LINK_MODE = 0x0F,
};

struct raw_block {
	int len;
	char raw[MAX_RAW_LEN];
};

struct rx_audiocap {
	u8 audio_format_code;
	u8 channel_num_max;
	u8 freq_cc;
	u8 cc3;
};

struct dolby_vsadb_cap {
	unsigned char rawdata[7 + 1]; // padding extra 1 byte
	unsigned int ieeeoui;
	unsigned char length;
	unsigned char dolby_vsadb_ver;
	unsigned char spk_center:1;
	unsigned char spk_surround:1;
	unsigned char spk_height:1;
	unsigned char headphone_only:1;
	unsigned char mat_48k_pcm_only:1;
};

struct rx_cap {
	u32 native_Mode;
	/*video*/
	u32 VIC[VIC_MAX_NUM];
	u32 y420_vic[Y420_VIC_MAX_NUM];
	u32 VIC_count;
	u32 native_vic;
	u32 native_vic2; /* some Rx has two native mode, normally only one */

	/*hdmi_vic have different define for tx20&tx21,use u32 instead here.*/
	u32 vesa_timing[VESA_MAX_TIMING]; /* Max 64 */

	/*audio*/
	struct rx_audiocap RxAudioCap[AUD_MAX_NUM];
	u8 AUD_count;
	u8 RxSpeakerAllocation;
	struct dolby_vsadb_cap dolby_vsadb_cap;
	/*vendor*/
	u32 ieeeoui;
	u8 Max_TMDS_Clock1; /* HDMI1.4b TMDS_CLK */
	u32 hf_ieeeoui;	/* For HDMI Forum */
	u32 Max_TMDS_Clock2; /* HDMI2.0 TMDS_CLK */
	/* CEA861-F, Table 56, Colorimetry Data Block */
	u32 colorimetry_data;
	u32 colorimetry_data2;
	u32 scdc_present:1;
	u32 scdc_rr_capable:1; /* SCDC read request */
	u32 lte_340mcsc_scramble:1;
	u32 dc_y444:1;
	u32 dc_30bit:1;
	u32 dc_36bit:1;
	u32 dc_48bit:1;
	u32 dc_30bit_420:1;
	u32 dc_36bit_420:1;
	u32 dc_48bit_420:1;
	enum frl_rate_enum max_frl_rate:4;
	u32 cnc0:1; /* Graphics */
	u32 cnc1:1; /* Photo */
	u32 cnc2:1; /* Cinema */
	u32 cnc3:1; /* Game */
	u32 qms_tfr_max:1;
	u32 qms:1;
	u32 mdelta:1;
	u32 qms_tfr_min:1;
	u32 neg_mvrr:1;
	u32 fva:1;
	u32 allm:1;
	u32 fapa_start_loc:1;
	u32 fapa_end_extended:1;
	u32 cinemavrr:1;
	u32 vrr_max;
	u32 vrr_min;
	struct hdr_info hdr_info;
	struct dv_info dv_info;
	/* When hdr_priority is 1, then dv_info will be all 0;
	 * when hdr_priority is 2, then dv_info/hdr_info will be all 0
	 * App won't get real dv_cap/hdr_cap, but can get real dv_cap2/hdr_cap2
	 */
	struct hdr_info hdr_info2;
	struct dv_info dv_info2;
	u8 IDManufacturerName[4];
	u8 IDProductCode[2];
	u8 IDSerialNumber[4];
	u8 ReceiverProductName[16];
	u8 manufacture_week;
	u8 manufacture_year;
	u16 physical_width;
	u16 physical_height;
	u8 edid_version;
	u8 edid_revision;
	u8 ColorDeepSupport;
	u32 vLatency;
	u32 aLatency;
	u32 i_vLatency;
	u32 i_aLatency;
	u32 threeD_present;
	u32 threeD_Multi_present;
	u32 hdmi_vic_LEN;
	u32 HDMI_3D_LEN;
	u32 threeD_Structure_ALL_15_0;
	u32 threeD_MASK_15_0;
	struct {
		u8 frame_packing;
		u8 top_and_bottom;
		u8 side_by_side;
	} support_3d_format[VIC_MAX_NUM];

	/*hdmi_vic have different define for tx20&tx21,use u32 instead here.*/
	//enum hdmi_vic preferred_mode;
	u32 preferred_mode;

	struct dtd dtd[16];
	u8 dtd_idx;
	u8 flag_vfpdb;
	u8 number_of_dtd;
	struct raw_block asd;
	struct raw_block vsd;
	/* for DV cts */
	bool ifdb_present;
	/* IFDB, currently only use below node */
	u8 additional_vsif_num;
	/*blk0 check sum*/
	u8 blk0_chksum;
	u8 chksum[10];
	u8 head_err;
	u8 chksum_err;
};

/* VSIF: Vendor Specific InfoFrame
 * It has multiple purposes:
 * 1. HDMI1.4 4K, HDMI_VIC=1/2/3/4, 2160p30/25/24hz, smpte24hz, AVI.VIC=0
 *    In CTA-861-G, matched with AVI.VIC=95/94/93/98
 * 2. 3D application, TB/SS/FP
 * 3. DolbyVision, with Len=0x18
 * 4. HDR10plus
 * 5. HDMI20 3D OSD disparity / 3D dual-view / 3D independent view / ALLM
 * Some functions are exclusive, but some may compound.
 * Consider various state transitions carefully, such as play 3D under HDMI14
 * 4K, exit 3D under 4K, play DV under 4K, enable ALLM under 3D dual-view
 */
enum vsif_type {
	/* Below 4 functions are exclusive */
	VT_HDMI14_4K = 1,
	VT_T3D_VIDEO,
	VT_DOLBYVISION,
	VT_HDR10PLUS,
	/* Maybe compound 3D dualview + ALLM */
	VT_T3D_OSD_DISPARITY = 0x10,
	VT_T3D_DUALVIEW,
	VT_T3D_INDEPENDVEW,
	VT_ALLM,
	/* default: if non-HDMI4K, no any vsif; if HDMI4k, = VT_HDMI14_4K */
	VT_DEFAULT,
	VT_MAX,
};

/* Refer to http://standards-oui.ieee.org/oui/oui.txt */
#define DOVI_IEEEOUI		0x00D046
#define HDR10PLUS_IEEEOUI	0x90848B
#define CUVA_IEEEOUI		0x047503
#define HF_IEEEOUI		0xC45DD8

#define GET_OUI_BYTE0(oui)	((oui) & 0xff) /* Little Endian */
#define GET_OUI_BYTE1(oui)	(((oui) >> 8) & 0xff)
#define GET_OUI_BYTE2(oui)	(((oui) >> 16) & 0xff)

/*edid apis*/
u32 hdmitx_edid_get_hdmi14_4k_vic(u32 vic);
void phy_addr_clear(struct vsdb_phyaddr *vsdb_phy_addr);

/*edid is good return 0, otherwise return < 0.*/
int hdmitx_edid_validate(unsigned char *rawedid);
bool hdmitx_edid_is_all_zeros(unsigned char *rawedid);
int _check_base_structure(unsigned char *buf);
int _check_edid_blk_chksum(unsigned char *block);
int check_dvi_hdmi_edid_valid(unsigned char *buf);

#endif
