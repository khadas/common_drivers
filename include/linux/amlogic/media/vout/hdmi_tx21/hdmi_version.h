/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMI21_VERSION_H__
#define __HDMI21_VERSION_H__
/****************************************************************/
/***************** HDMITx Version Description *******************/
/* V[aa].[bb].[cc].[dd].[ee].[ff].[gg] */
/* aa: big version, kernel version, New IP
 *     01: kernel 4.9
 *     02: kernel 4.9-q
 *     03: kernel 5.4
 */
/* bb: phy update */
/* cc: audio update */
/* dd: HDR/DV update */
/* ee: HDCP update */
/* ff: EDID update */
/* gg: bugfix, compatibility, etc */
/****************************************************************/

#define HDMITX21_VERSIONS_LOG \
	"V03.00.00.00.00.01.00 [20220616] [EDID] add MPEG-H edid parse\n" \
	"V03.00.00.00.00.01.01 [20220628] [Compatibility] optimize for hotplug during bootup\n"\
	"V03.00.00.00.00.02.01 [20220707] [EDID] add hdr_priority  = 1 parse hdr10+\n" \
	"V03.00.00.00.01.02.01 [20220708] [HDCP] fix hdcp1.4 repeater cts issues\n" \
	"V03.00.00.00.01.02.02 [20220711] [COMP] add hdcp version protect for drm\n" \
	"V03.00.00.00.01.03.02 [20220711] [EDID] HF1-23 add the ddc delay to 20ms\n" \
	"V03.00.00.00.01.03.03 [20220713] [SYSFS] add phy show sysfs\n" \
	"V03.00.00.00.02.03.03 [20220715] [HDCP] T7 DVI 1080p60 1A-09 test fail\n" \
	"V03.00.00.00.02.03.04 [20220726] [COMP] add avmute interface\n" \
	"V03.00.00.00.03.03.04 [20220805] [HDCP] fix hdcp1.4 repeater cts 3C-II-07\n" \
	"V03.00.00.00.03.03.05 [20220811] [NEWF] hdmitx21 add aspect ratio support\n" \
	"V03.00.00.00.03.03.06 [20220829] [NEWF] support avi content type\n" \
	"V03.00.00.00.03.03.07 [20220913] [BUGFIX] add DDC reset before do EDID transaction\n" \
	"V03.00.00.00.03.03.08 [20220919] [BUG] y422 mapping and Enable the dither\n" \
	"V03.00.00.00.03.03.09 [20220915] [BUG] Don't reset variables when parse a new block\n" \
	"V03.00.00.00.03.03.10 [20221012] [NEW] add hdmi hpd extcon\n" \
	"V03.00.00.00.03.03.11 [20221021] [BUG] not read EDID again if EDID already read done\n" \
	"V03.00.00.00.03.03.12 [20221025] [COM] when set mode 4x3 and 16x9, return valid mode 1\n" \
	"V03.00.00.00.03.04.12 [20221025] [EDID] adjust edid parsing for TV_TS\n" \
	"V03.01.00.00.03.04.12 [20221221] [PHY] test pixel clkmsr and adjust phy for 70hz issue\n" \
	"V03.01.00.00.03.04.13 [20230111] [NEW] add support for 480i/576i\n" \
	"V03.01.00.00.03.04.14 [20230308] [BUG]  fix y422 deep color check" \
	"V03.01.00.00.03.04.15 [20230316] [BUG] 480p&576p colorimetry should be 601" \
	"V03.01.00.00.03.04.16 [20230317] [NEW] tx21 send emds pkt for cuva"

#endif // __HDMI21_VERSION_H__
