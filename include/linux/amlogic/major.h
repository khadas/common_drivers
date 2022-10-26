/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
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
 */

#ifndef _LINUX_MAJOR_E_H
#define _LINUX_MAJOR_E_H
#include <linux/fs.h>

#include <linux/major.h>

/* Amlogic extension */
#define AML_BASE		(270)
#define AMSTREAM_MAJOR		(0 + (AML_BASE))
#define AUDIODSP_MAJOR		(2 + (AML_BASE))
#define FIRMWARE_MAJOR		(3 + (AML_BASE))
#define AMVIDEO_MAJOR		(9 + (AML_BASE))
#define AMAUDIO_MAJOR		(11 + (AML_BASE))
#define AMVIDEO2_MAJOR		(12 + (AML_BASE))
#define AMAUDIO2_MAJOR		(13 + (AML_BASE))
#define VFM_MAJOR		(14 + (AML_BASE))
#define IONVIDEO_MAJOR		(15 + (AML_BASE))
#define VAD_MAJOR		(16 + (AML_BASE))
#define VIDEOSYNC_MAJOR		(17 + (AML_BASE))
#define V4LVIDEO_MAJOR		(18 + (AML_BASE))
#define VIDEO_COMPOSER_MAJOR	(19 + (AML_BASE))
#define TSYNC_MAJOR		(20 + (AML_BASE))
#define VIDEOFRAME_MAJOR	(21 + (AML_BASE))
#define MEDIASYNC_MAJOR		(22 + (AML_BASE))
#define VDETECT_MAJOR		(23 + (AML_BASE))
#define AMSYNC_MAJOR		(24 + (AML_BASE))
#define AMSYNC_SESSION_MAJOR	(25 + (AML_BASE))
#define VIDEOQUEUE_MAJOR	(26 + (AML_BASE))
#define DI_V4L_MAJOR		(27 + (AML_BASE))
#define PTSSERVER_MAJOR		(28 + (AML_BASE))
#define AFD_MAJOR               (29 + (AML_BASE))
#define AMEDIA_INFO_MAJOR       (30 + (AML_BASE))
#define VICP_MAJOR              (31 + (AML_BASE))
#endif
