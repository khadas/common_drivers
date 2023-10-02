/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_VERSION_H__
#define __HDMITX_VERSION_H__

/*arch version*/
#define HDMITX_MAJOR_VERSION		4
/*sub module version*/
#define HDMITX_COMMON_VERSION		01
#define HDMITX_HW_COMM_VERSION		01
#define HDMITX_EDID_VERSION			01
#define HDMITX_HDR_VERSION			00
#define HDMITX_AUDIO_VERSION		00
/*tx main version*/
#define HDMITX_TX20_VERSION			02
#define HDMITX_TX21_VERSION			01
/*change history*/
#define CHANGE_VERSION				20231002

#define _TO_STRING(x) #x
#define HDMITX_VER_STR(x) _TO_STRING(x)

#define GET_HDMITX_VER(ver_name) \
{ \
	const char *ver = HDMITX_VER_STR(HDMITX_MAJOR_VERSION) "." \
		HDMITX_VER_STR(HDMITX_COMMON_VERSION) "-" \
		HDMITX_VER_STR(HDMITX_HW_COMM_VERSION) "-" \
		HDMITX_VER_STR(HDMITX_EDID_VERSION) "-" \
		HDMITX_VER_STR(HDMITX_HDR_VERSION) "-" \
		HDMITX_VER_STR(HDMITX_AUDIO_VERSION) "." \
		HDMITX_VER_STR(HDMITX_TX20_VERSION) "-" \
		HDMITX_VER_STR(HDMITX_TX21_VERSION) "." \
		HDMITX_VER_STR(CHANGE_VERSION); \
	ver_name = ver;\
} \

#endif
