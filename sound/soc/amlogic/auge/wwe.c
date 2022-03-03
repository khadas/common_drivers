// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/**
 * platform -> {component, DAI}
 *
 * Difference of our system with reference
 * - DSP's firmware loading & booting is NOT included
 * - mailbox communication is uni-direction, only from ARM to DSP
 *
 * flow risk:
 * - platform probe
 *   WARNING: DSP may haven't boot here
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/clk.h>
#include <linux/pinctrl/consumer.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/control.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <linux/kthread.h>

#include <linux/amlogic/scpi_protocol.h>

#ifdef __KERNEL_419_AUDIO__
#include <linux/amlogic/clk_measure.h>
#endif
#include <linux/amlogic/cpu_version.h>

#include <linux/amlogic/media/sound/aout_notify.h>

#include "dsp/rpc_dev.h"
#include "dsp/rpc_client_shm.h"
#include "dsp/rpc_client_aipc.h"
#include "dsp/hifi4rpc_awe.h"
#include "wwe.h"

#define DRV_NAME "snd_wwe"

/** DAI */
static struct snd_pcm_hardware aml_wwe_hardware = {
	.info = SNDRV_PCM_INFO_INTERLEAVED |
		SNDRV_PCM_INFO_NO_PERIOD_WAKEUP,
	.formats = SNDRV_PCM_FMTBIT_S16_LE,
	.rate_min = 16000,
	.rate_max = 16000,
	.channels_min = 1,
	.channels_max = 1,
	.buffer_bytes_max = 512 * 1024,
	.period_bytes_max	=	1024,
	.period_bytes_min	=	1024,
	.periods_min		=	16,
	.periods_max		=	16,
	.fifo_size		=	0,
};

static int aml_dai_wwe_probe(struct snd_soc_dai *cpu_dai)
{
	pr_debug("WWE: dai probe\n");
	return 0;
}

static const struct snd_soc_dai_ops aml_dai_wwe_ops = {
	/** prepare, trigger, hw_params, hw_free, set_fmt, set_sysclk, set_... */
};

static struct snd_soc_dai_driver aml_wwe_dai[] = {
	{
		/** fix to 1ch, 16kHz, 16bit */
		.name = "WWE",
		/* .id = 0, */
		.capture = {
			.channels_min	= 1,
			.channels_max	= 1,
			.rates		= SNDRV_PCM_RATE_16000,
			.formats	= SNDRV_PCM_FMTBIT_S16_LE,
		},
		.probe = aml_dai_wwe_probe,
		.ops = &aml_dai_wwe_ops,
	}
};

/** COMPONENT */
int aml_wwe_notify(void *data)
{
	struct snd_pcm_substream *substream = data;

	snd_pcm_period_elapsed(substream);
	return 0;
}

static int aml_wwe_open(struct snd_soc_component *component,
			struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd;
	struct aml_wwe *p_wwe;
	struct device *dev;
	int ret = 0;

	pr_debug("wwe: substream=%p runtime=%p\n", substream, runtime);
	rtd = substream->private_data;
	pr_debug("wwe: rtd=%p\n", rtd);
	p_wwe = (struct aml_wwe *)snd_soc_dai_get_drvdata(asoc_rtd_to_cpu(rtd, 0));
	pr_debug("wwe: wwe=%p\n", p_wwe);
	dev = p_wwe->dev;

	RPC_kernel_init(dev);
	shm_kernel_init(dev);

	/* constraint depend on here, otherwise pcm_open fail with EINVAL */
	snd_soc_set_runtime_hwparams(substream, &aml_wwe_hardware);
	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	runtime->private_data = p_wwe;
	p_wwe->prepare_times = 0;

	return ret;
}

static int aml_wwe_close(struct snd_soc_component *component,
			 struct snd_pcm_substream *substream)
{
	/* struct snd_pcm_runtime *runtime = substream->runtime; */
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct aml_wwe *p_wwe = (struct aml_wwe *)snd_soc_dai_get_drvdata(asoc_rtd_to_cpu(rtd, 0));

	if (p_wwe->wwe_task) {
		kthread_stop(p_wwe->wwe_task);
		p_wwe->wwe_task = NULL;
	}

	if (p_wwe->prepare_times) {
		awe_close();
		p_wwe->prepare_times = 0;
	}
	return 0;
}

static int aml_wwe_hw_params(struct snd_soc_component *component,
			     struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params)
{
	pr_debug("unimplemented func=%s\n", __func__);
	return 0;
}

static int aml_wwe_hw_free(struct snd_soc_component *component,
			   struct snd_pcm_substream *substream)
{
	pr_debug("unimplemented func=%s\n", __func__);
	return 0;
}

static int aml_wwe_prepare(struct snd_soc_component *component,
			   struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct aml_wwe *p_wwe = runtime->private_data;
	int ret = 0;

	/** Why call awe_open when first preparing (instead of wwe_open)?
	 * awe_open need correct, configured runtime periods, period_size
	 */
	if (!p_wwe->prepare_times) {
		pr_debug("WWE: wakeup engine start period=%ld*%d\n",
			 runtime->period_size, runtime->periods);
		ret = awe_open(runtime->period_size * sizeof(int16_t),
			       runtime->periods, aml_wwe_notify, substream);
		pr_debug("WWE: awe_open ret=%d\n", ret);
	}
	p_wwe->prepare_times++;
	return ret;
}

static int aml_wwe_trigger(struct snd_soc_component *component,
			   struct snd_pcm_substream *substream, int cmd)
{
	pr_debug("unimplemented func=%s\n", __func__);
	return 0;
}

static snd_pcm_uframes_t aml_wwe_pointer(struct snd_soc_component *component,
					 struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	/* struct aml_wwe *p_wwe = runtime->private_data; */
	snd_pcm_uframes_t r;

	r = bytes_to_frames(runtime, awe_pointer());
	return r;
}

static int aml_wwe_mmap(struct snd_soc_component *component,
			struct snd_pcm_substream *substream,
			struct vm_area_struct *vma)
{
	pr_debug("unimplemented func=%s\n", __func__);
	return 0;
}

/*
 *static int aml_wwe_silence(struct snd_soc_component *component,
 *			   struct snd_pcm_substream *substream,
 *			   int channel, snd_pcm_uframes_t pos,
 *			   snd_pcm_uframes_t count)
 *{
 *	pr_debug("unimplemented func=%s\n", __func__);
 *	return 0;
 *}
 */

static int aml_wwe_copy_user(struct snd_soc_component *component,
			     struct snd_pcm_substream *substream,
			     int channel, unsigned long pos, void __user *buf,
			     unsigned long bytes)
{
	/* struct snd_pcm_runtime *runtime = substream->runtime; */
	/* struct device *pdev = substream->pcm->card->dev; */

	/* pr_info("copy to user str=%lx ch=%d pos=%lu buf=%lx bytes=%ld\n", */
	/* (unsigned long)substream, channel, pos, (unsigned long)buf, bytes); */
	awe_copy_user(buf, bytes);
	return 0;
}

static int aml_wwe_new(struct snd_soc_component *component,
		       struct snd_soc_pcm_runtime *rtd)
{
	int ret = 0;
	struct snd_pcm *pcm = rtd->pcm;
	struct snd_pcm_substream *substream;

	substream = pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream;
	return ret;
}

static void aml_wwe_free(struct snd_soc_component *component, struct snd_pcm *pcm)
{
	pr_debug("unimplemented func=%s\n", __func__);
}

static const struct snd_soc_component_driver aml_wwe_component = {
	.name		= DRV_NAME,

	.pcm_construct	= aml_wwe_new,
	.pcm_destruct	= aml_wwe_free,

	/** calling sequence
	 * - open
	 * - hw_params
	 * - prepare
	 * - trigger
	 * - pointer
	 * - trigger
	 * - hw_free
	 * - close
	 */
	.open = aml_wwe_open,
	.close = aml_wwe_close,
	/* .ioctl = aml_wwe_ioctl, */
	.hw_params = aml_wwe_hw_params,
	.hw_free = aml_wwe_hw_free,
	.prepare = aml_wwe_prepare,
	.trigger = aml_wwe_trigger,
	.pointer = aml_wwe_pointer,
	// .fill_silence = aml_wwe_silence,
	.copy_user = aml_wwe_copy_user,
	.mmap = aml_wwe_mmap,
};

/** Platform */

static int aml_wwe_platform_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct aml_wwe *p_wwe;
	struct device *dev = &pdev->dev;

	pr_info("WWE: register soc platform start\n");
	p_wwe = devm_kzalloc(&pdev->dev, sizeof(struct aml_wwe), GFP_KERNEL);
	if (!p_wwe)
		return -ENOMEM;

	p_wwe->hwptr = 0;
	p_wwe->wwe_task = NULL;
	p_wwe->dum = 0;
	p_wwe->dev = dev;
	p_wwe->prepare_times = 0;
	dev_set_drvdata(&pdev->dev, p_wwe);
	pr_info("WWE: set driver data p_wwe=%p\n", p_wwe);

	ret = devm_snd_soc_register_component(dev, &aml_wwe_component,
					      aml_wwe_dai, ARRAY_SIZE(aml_wwe_dai));
	if (ret) {
		pr_info("WWE: fail to register component\n");
		return ret;
	}
	pr_info("WWE: register soc platform\n");
	return 0;
}

static int aml_wwe_platform_suspend(struct platform_device *pdev,
				    pm_message_t		state)
{
	pr_debug("unimplemented func=%s\n", __func__);
	return 0;
}

static int aml_wwe_platform_resume(struct platform_device *pdev)
{
	pr_debug("unimplemented func=%s\n", __func__);
	return 0;
}

static void aml_wwe_platform_shutdown(struct platform_device *pdev)
{
	pr_debug("unimplemented func=%s\n", __func__);
}

static const struct of_device_id aml_wwe_device_id[] = {
	{
		.compatible = "amlogic, wwe",
	},
	{}
};

struct platform_driver aml_wwe_driver = {
	.driver		= {
		.name	= DRV_NAME,
		.of_match_table = aml_wwe_device_id,
	},
	.probe		= aml_wwe_platform_probe,
	.suspend	= aml_wwe_platform_suspend,
	.resume		= aml_wwe_platform_resume,
	.shutdown	= aml_wwe_platform_shutdown,
};

#ifdef MODULE
int __init aml_wwe_init(void)
{
	return platform_driver_register(&aml_wwe_driver);
}

void __exit aml_wwe_exit(void)
{
	platform_driver_unregister(&aml_wwe_driver);
}
#else
module_platform_driver(aml_wwe_driver);

MODULE_AUTHOR("Amlogic, Inc.");
MODULE_DESCRIPTION("Amlogic WWE ASoc driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);
#endif
