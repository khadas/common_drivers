// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/export.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/amlogic/iomap.h>
#include <linux/amlogic/secmon.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/amlogic/cpu_info.h>
#include <linux/arm-smccc.h>
#include <linux/amlogic/cpu_version.h>
#include <linux/seq_file.h>
#include <trace/hooks/cpuinfo.h>

static unsigned char cpuinfo_chip_id[CHIPID_LEN];

void cpuinfo_get_chipid(unsigned char *cid, unsigned int size)
{
	memcpy(&cid[0], cpuinfo_chip_id, size);
}

void show_cpu_chipid(void *data, struct seq_file *m)
{
	unsigned char chipid[CHIPID_LEN];
	int i;

	cpuinfo_get_chipid(chipid, CHIPID_LEN);
	seq_puts(m, "Serial\t\t: ");
	for (i = 0; i < 16; i++)
		seq_printf(m, "%02x", chipid[i]);
	seq_puts(m, "\n");
	seq_printf(m, "Hardware\t: %s\n\n", "Amlogic");
}

static noinline int fn_smc(u64 function_id,
			   u64 arg0,
			   u64 arg1,
			   u64 arg2)
{
	struct arm_smccc_res res;

	arm_smccc_smc((unsigned long)function_id,
		      (unsigned long)arg0,
		      (unsigned long)arg1,
		      (unsigned long)arg2,
		      0, 0, 0, 0, &res);
	return res.a0;
}

static int cpuinfo_probe(struct platform_device *pdev)
{
	void __iomem *shm_out;
	struct device_node *np = pdev->dev.of_node;
	int cmd, ret, i;

	if (of_property_read_u32(np, "cpuinfo_cmd", &cmd))
		return -EINVAL;

	shm_out = get_meson_sm_output_base();
	if (!shm_out) {
		pr_err("failed to allocate shared memory\n");
		return -ENOMEM;
	}

	meson_sm_mutex_lock();
	ret = fn_smc(cmd, 2, 0, 0);
	if (ret == 0) {
		int version = *((unsigned int *)shm_out);

		if (version == 2) {
			memcpy((void *)&cpuinfo_chip_id[0],
			       (void *)shm_out + 4,
			       CHIPID_LEN);
		} else {
			pr_err("failed to get version\n");
			ret = -EINVAL;
		}
	} else {
		ret = -EPROTO;
	}
	meson_sm_mutex_unlock();

	if (ret == 0) {
		pr_info("serial = ");
		for (i = 0; i < CHIPID_LEN; i++)
			pr_cont("%02x", cpuinfo_chip_id[i]);
		pr_cont("\n");
	}

	ret = register_trace_android_vh_show_cpu_chipid(show_cpu_chipid, NULL);
	if (ret)
		pr_err("register_trace_android_vh_show_cpu_chipid fail ret=%d\n", ret);

	return ret;
}

static const struct of_device_id cpuinfo_dt_match[] = {
	{ .compatible = "amlogic, cpuinfo" },
	{ /* sentinel */ }
};

static  struct platform_driver cpuinfo_platform_driver = {
	.probe		= cpuinfo_probe,
	.driver		= {
		.owner		= THIS_MODULE,
		.name		= "cpuinfo",
		.of_match_table	= cpuinfo_dt_match,
	},
};

static int __init meson_cpuinfo_init(void)
{
	meson_cpu_version_init();

	return  platform_driver_register(&cpuinfo_platform_driver);
}

#ifdef MODULE
module_init(meson_cpuinfo_init);
MODULE_LICENSE("GPL v2");
#else
subsys_initcall(meson_cpuinfo_init);
#endif
