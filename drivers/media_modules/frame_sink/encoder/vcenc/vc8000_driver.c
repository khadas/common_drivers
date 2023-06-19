// SPDX-License-Identifier: GPL-2.0
/***************************************************************************
 *
 *    The MIT License (MIT)
 *
 *    COPYRIGHT (C) 2019 VERISILICON ALL RIGHTS RESERVED
 *
 *    Permission is hereby granted, free of charge, to any person obtaining a
 *    copy of this software and associated documentation files (the "Software"),
 *    to deal in the Software without restriction, including without limitation
 *    the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *    and/or sell copies of the Software, and to permit persons to whom the
 *    Software is furnished to do so, subject to the following conditions:
 *
 *    The above copyright notice and this permission notice shall be included in
 *    all copies or substantial portions of the Software.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *    DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************
 *
 *    The GPL License (GPL)
 *
 *    COPYRIGHT (C) 2019 VERISILICON ALL RIGHTS RESERVED
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version 2
 *    of the License, or (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software Foundation,
 *    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *****************************************************************************
 *
 *    Note: This software is released under dual MIT and GPL licenses. A
 *    recipient may use this file under the terms of either the MIT license or
 *    GPL License. If you wish to use only one license not the other, you can
 *    indicate your decision by deleting one of the above license notices in your
 *    version of this file.
 *
 *****************************************************************************
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include "vc8000_driver.h"
#include <linux/of.h>

#include <linux/dma-mapping.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/moduleparam.h>

#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/pm_domain.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/cpu_cooling.h>
#include <linux/pm_runtime.h>
#include <linux/pm_domain.h>
#include <linux/amlogic/power_domain.h>

#define VERSENC_TS_WAIT 5
#define WRAP_RESET_CTL 0xfe002010
#define WRAP_RESET_SEL 0xfe08e000
#define WRAP_CLOCK_CTL 0xfe000150
#define HW_RESET_CTL   0xfe380040
#define HW_VCMD_BASE   0xfe380000

extern int venc_file_open_cnt;

struct meson_versenc_data {
	int id;
	struct device		*dev;
	struct meson_versenc_platform_data *pdata;
	struct mutex lock;/*mutex lock for set versenc reg*/
	struct clk *sys_clk;
	struct clk *core_clk;
	struct clk *aclk;
};

static u32 vcmd_supported;

int hantroenc_normal_init(void);
int hantroenc_vcmd_init(struct platform_device *pf_dev);
void hantroenc_normal_cleanup(void);
void hantroenc_vcmd_cleanup(struct platform_device *pf_dev);

extern void vers_resume_hw(u32 on);

static int vc9000e_vce_probe(struct platform_device *pf_dev);

static void hantroenc_cleanup(struct platform_device *pf_dev)
{
    if (vcmd_supported == 0)
        hantroenc_normal_cleanup();
    else
        hantroenc_vcmd_cleanup(pf_dev);
}

static int vc9000e_vce_remove(struct platform_device *pf_dev)
{
    struct meson_versenc_data *data = NULL;
    unsigned int *kvirt_addr = 0;

    data = platform_get_drvdata(pf_dev);

    pm_runtime_get_sync(&pf_dev->dev);
    mdelay(5);

    //hw init
    kvirt_addr = ioremap(WRAP_RESET_CTL, 4);
    *kvirt_addr = 0x000002c0;
    iounmap(kvirt_addr);
    kvirt_addr = ioremap(WRAP_RESET_SEL, 4);
    *kvirt_addr = 0;
    iounmap(kvirt_addr);
    kvirt_addr = ioremap(HW_RESET_CTL, 4);
    *kvirt_addr = 2;
    iounmap(kvirt_addr);
    kvirt_addr = ioremap(WRAP_CLOCK_CTL, 4);
    *kvirt_addr = 0x04000400;
    *kvirt_addr = 0x05000500;
    iounmap(kvirt_addr);

    kvirt_addr = ioremap(HW_VCMD_BASE, 4);
    pr_info("verstile init success, hw id 0x%x\n", *kvirt_addr);

    hantroenc_cleanup(pf_dev);
    if (!(data->sys_clk == NULL || IS_ERR(data->sys_clk)))
        devm_clk_put(&pf_dev->dev, data->sys_clk);
    devm_kfree(&pf_dev->dev, data);

    return 0;
}

static int meson_versenc_hw_initialize(struct platform_device *pdev)
{
    struct meson_versenc_data *data = platform_get_drvdata(pdev);

    mutex_lock(&data->lock);
    //ret = data->versenc_hw_initialize(pdev);
    mutex_unlock(&data->lock);
    return 0;
}

static int meson_versenc_trips_initialize(struct platform_device *pdev)
{
    struct meson_versenc_data *data = platform_get_drvdata(pdev);
    mutex_lock(&data->lock);
    //ret = data->versenc_trips_initialize(pdev);
    mutex_unlock(&data->lock);
    return 0;
}

static void meson_versenc_control(struct platform_device *pdev, bool on)
{
    unsigned int *kvirt_addr = 0;
    struct meson_versenc_data *data = NULL;

    data = platform_get_drvdata(pdev);

    mutex_lock(&data->lock);
    if (on) {
        pm_runtime_get_sync(&pdev->dev);
        //hw init
        kvirt_addr = ioremap(WRAP_RESET_CTL, 4);
        *kvirt_addr = 0x000002c0;
        iounmap(kvirt_addr);
        kvirt_addr = ioremap(WRAP_RESET_SEL, 4);
        *kvirt_addr = 0;
        iounmap(kvirt_addr);
        kvirt_addr = ioremap(HW_RESET_CTL, 4);
        *kvirt_addr = 3;
        iounmap(kvirt_addr);
        kvirt_addr = ioremap(WRAP_CLOCK_CTL, 4);
        *kvirt_addr = 0x05000500;
        iounmap(kvirt_addr);
    } else {
        kvirt_addr = ioremap(WRAP_CLOCK_CTL, 4);
        *kvirt_addr = 0x04000400;
        iounmap(kvirt_addr);
        pm_runtime_put_sync(&pdev->dev);
    }
    mdelay(5);
    vers_resume_hw(on);
    mutex_unlock(&data->lock);
}

static int meson_versenc_suspend(struct device *dev)
{
    meson_versenc_control(to_platform_device(dev), false);
    return 0;
}

static int meson_versenc_resume(struct device *dev)
{
    meson_versenc_control(to_platform_device(dev), false);
    return 0;
    meson_versenc_hw_initialize(to_platform_device(dev));
    meson_versenc_trips_initialize(to_platform_device(dev));
    meson_versenc_control(to_platform_device(dev), true);
    /*wait versenc work*/
    msleep(VERSENC_TS_WAIT);

    return 0;
}

int meson_versenc_suspend_runtime(struct platform_device *pdev)
{
    meson_versenc_control(pdev, false);

    return 0;
}

int meson_versenc_resume_runtime(struct platform_device *pdev)
{
    meson_versenc_control(pdev, true);
    /*wait versenc work*/
    msleep(VERSENC_TS_WAIT);

    return 0;
}

static const struct of_device_id amlogic_vce_dt_match[] = {{
                                                               .compatible = "vc9000e_rev",
                                                           },
                                                           {}};

static const struct dev_pm_ops meson_versenc_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(meson_versenc_suspend, meson_versenc_resume)
};

static struct platform_driver mbd_vce_driver = {.probe = vc9000e_vce_probe,
                                                .remove = vc9000e_vce_remove,
                                                .driver = {
                                                    .name = "vc9000e_rev",
                                                    .owner = THIS_MODULE,
                                                    .of_match_table = amlogic_vce_dt_match,
                                                    .pm = &meson_versenc_pm_ops,
                                                }};

static int hantroenc_init(struct platform_device *pf_dev)
{
    pr_info("vc8000_vcmd_driver: hantroenc_init\n");
    vcmd_supported = 1;
    venc_file_open_cnt = 0;
    if (vcmd_supported == 0)
        return hantroenc_normal_init();
    else
        return hantroenc_vcmd_init(pf_dev);
}

static int vc9000e_vce_probe(struct platform_device *pf_dev)
{
    struct meson_versenc_data *data;
    int ret;
    unsigned int *kvirt_addr = 0;

    pr_info("meson versenc init\n");
    data = devm_kzalloc(&pf_dev->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;
    data->dev = &pf_dev->dev;
    mutex_init(&data->lock);
    data->sys_clk = devm_clk_get(&pf_dev->dev, "vers_sys_clk");
    platform_set_drvdata(pf_dev, data);
    if (IS_ERR(data->sys_clk)) {
        dev_err(&pf_dev->dev, "Failed to vers_sys_clk\n");
        ret = PTR_ERR(data->sys_clk);
        goto ERROR_PROBE_DEVICE;
    }
    pm_runtime_get_sync(&pf_dev->dev);
    mdelay(5);

    //hw init
    kvirt_addr = ioremap(WRAP_RESET_CTL, 4);
    *kvirt_addr = 0x000002c0;
    iounmap(kvirt_addr);
    kvirt_addr = ioremap(WRAP_RESET_SEL, 4);
    *kvirt_addr = 0;
    iounmap(kvirt_addr);
    kvirt_addr = ioremap(HW_RESET_CTL, 4);
    *kvirt_addr = 2;
    iounmap(kvirt_addr);
    kvirt_addr = ioremap(WRAP_CLOCK_CTL, 4);
    *kvirt_addr = 0x04000400;
    *kvirt_addr = 0x05000500;
    iounmap(kvirt_addr);

    kvirt_addr = ioremap(HW_VCMD_BASE, 4);
    pr_info("verstile init success, hw id 0x%x\n", *kvirt_addr);

    return hantroenc_init(pf_dev);

ERROR_PROBE_DEVICE:
    devm_kfree(&pf_dev->dev, data);
    return ret;
}

int __init enc_mem_init(void)
{
    pr_info("vc8000_vcmd_driver: enc_mem_init\n");
    return platform_driver_register(&mbd_vce_driver);
}

void __exit enc_mem_exit(void)
{
    pr_info("vc8000_vcmd_driver: enc_mem_exit\n");
    platform_driver_unregister(&mbd_vce_driver);
}

module_init(enc_mem_init);
module_exit(enc_mem_exit);

module_param(vcmd_supported, uint, 0);

/* module description */
/*MODULE_LICENSE("Proprietary");*/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amlogic Inc.");
MODULE_DESCRIPTION("VC8000 Vcmd driver");
