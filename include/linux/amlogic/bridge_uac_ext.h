/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _BRIDGE_UAC_EXT_H
#define _BRIDGE_UAC_EXT_H

typedef int(*callback1)(void);
typedef int (*callback2)(int *chmask, int *srate, int *ssize);
typedef int (*callback3)(char *buf, unsigned int size);
typedef int (*callback4)(int cmd, int value);
struct bridge_uac_function {
	callback1 get_capture_status;
	callback1 get_playback_status;

	callback1 start_capture;
	callback1 start_playback;
	callback1 stop_capture;
	callback1 stop_playback;

	callback1 get_default_volume_capture;
	callback1 get_default_volume_playback;

	callback2 get_capture_hw;
	callback2 get_playback_hw;

	callback3 write_data;
	callback3 read_data;

	callback4 ctl_capture;
	callback4 ctl_playback;

	int setup_capture;
	int setup_playback;
};

extern struct bridge_uac_function bridge_uac_f;

void *get_uac_function_p(void);

#endif /*_BRIDGE_UAC_EXT_H*/
