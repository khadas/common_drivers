// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifdef MODULE
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/ctype.h>
#include <linux/kallsyms.h>
#include <linux/mm.h>
#include "sound_init.h"

#define call_sub_init(func) { \
	int ret = 0; \
	ret = func(); \
	if (ret) {	\
		pr_err("call %s() ret=%d\n", #func, ret); \
		return ret;		\
	} else \
		pr_debug("call %s() ret=%d\n", #func, ret); \
}

#define call_sub_exit(func) { \
	func(); \
	pr_debug("call %s()\n", #func); \
}

static int __init sound_soc_init(void)
{
	call_sub_init(auge_hdmirx_arc_iomap_init);
	call_sub_init(auge_snd_iomap_init);
	call_sub_init(audio_clocks_init);
	call_sub_init(audio_controller_init);
	call_sub_init(audio_ddr_init);
	call_sub_init(pcpd_monitor_init);
	call_sub_init(earc_init);
	call_sub_init(effect_platform_init);
	call_sub_init(extn_init);
	call_sub_init(audio_locker_init);
	call_sub_init(pdm_init);
	call_sub_init(resample_drv_init);
	call_sub_init(spdif_init);
	call_sub_init(audio_pinctrl_init);
	call_sub_init(sm1_audio_pinctrl_init);
	call_sub_init(tdm_init);
	call_sub_init(loopback_init);
	call_sub_init(vad_drv_init);
	call_sub_init(vad_dev_init);
	call_sub_init(aud_sram_init);
	call_sub_init(aml_card_init);

	call_sub_init(audiodsp_init_module);
	call_sub_init(amaudio_init);
	call_sub_init(audio_data_init);

	return 0;
}

static __exit void sound_soc_exit(void)
{
	call_sub_exit(audio_data_exit);
	call_sub_exit(amaudio_exit);
	call_sub_exit(audiodsp_exit_module);

	call_sub_exit(aml_card_exit);
	call_sub_exit(aud_sram_exit);
	call_sub_exit(vad_dev_exit);
	call_sub_exit(vad_drv_exit);
	call_sub_exit(loopback_exit);
	call_sub_exit(tdm_exit);
	call_sub_exit(audio_pinctrl_exit);
	call_sub_exit(sm1_audio_pinctrl_exit);
	call_sub_exit(spdif_exit);
	call_sub_exit(resample_drv_exit);
	call_sub_exit(pdm_exit);
	call_sub_exit(audio_locker_exit);
	call_sub_exit(extn_exit);
	call_sub_exit(effect_platform_exit);
	call_sub_exit(earc_exit);
	call_sub_exit(audio_ddr_exit);
	call_sub_exit(pcpd_monitor_exit);
	call_sub_exit(audio_controller_exit);
	call_sub_exit(audio_clocks_exit);
	call_sub_exit(auge_snd_iomap_exit);
	call_sub_exit(auge_hdmirx_arc_iomap_exit);
}

module_init(sound_soc_init);
module_exit(sound_soc_exit);
MODULE_LICENSE("GPL v2");
#endif
