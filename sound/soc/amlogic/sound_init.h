/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __SOUND_INIT_H__
#define __SOUND_INIT_H__

int __init earc_init(void);
int __init audio_clocks_init(void);
int __init auge_snd_iomap_init(void);
int __init auge_hdmirx_arc_iomap_init(void);
int __init audio_controller_init(void);
int __init audio_pinctrl_init(void);
int __init sm1_audio_pinctrl_init(void);
int __init g12a_audio_pinctrl_init(void);
int __init aml_card_init(void);
int __init audio_ddr_init(void);
int __init effect_platform_init(void);
int __init extn_init(void);
int __init audio_locker_init(void);
int __init loopback_init(void);
int __init pdm_init(void);
int __init resample_drv_init(void);
int __init spdif_init(void);
int __init tdm_init(void);
int __init vad_drv_init(void);
int __init vad_dev_init(void);
int __init pcpd_monitor_init(void);
int __init aud_sram_init(void);

void __exit vad_dev_exit(void);
void __exit vad_drv_exit(void);
void __exit tdm_exit(void);
void __exit spdif_exit(void);
void __exit resample_drv_exit(void);
void __exit pdm_exit(void);
void __exit loopback_exit(void);
void __exit audio_locker_exit(void);
void __exit extn_exit(void);
void __exit effect_platform_exit(void);
void __exit audio_ddr_exit(void);
void __exit aml_card_exit(void);
void __exit audio_pinctrl_exit(void);
void __exit sm1_audio_pinctrl_exit(void);
void __exit g12a_audio_pinctrl_exit(void);
void __exit audio_controller_exit(void);
void __exit auge_snd_iomap_exit(void);
void __exit auge_hdmirx_arc_iomap_exit(void);
void __exit audio_clocks_exit(void);
void __exit earc_exit(void);
void __exit pcpd_monitor_exit(void);
void __exit aud_sram_exit(void);

#if IS_ENABLED(CONFIG_AMLOGIC_AUDIO_DSP)
int __init audiodsp_init_module(void);
void __exit audiodsp_exit_module(void);
#else
static inline int audiodsp_init_module(void)
{
	return 0;
}

static inline void audiodsp_exit_module(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_AMAUDIO)
int __init amaudio_init(void);
void __exit amaudio_exit(void);
#else
static inline int amaudio_init(void)
{
	return 0;
}

static inline void amaudio_exit(void)
{
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_AUDIO_INFO)
int __init audio_data_init(void);
void __exit audio_data_exit(void);
#else
static inline int audio_data_init(void)
{
	return 0;
}

static inline void audio_data_exit(void)
{
}
#endif

#endif
