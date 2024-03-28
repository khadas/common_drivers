// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/pm.h>
#include <linux/suspend.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/psci.h>
#include <linux/errno.h>
#include <asm/suspend.h>
#include <linux/of_address.h>
#include <linux/input.h>
#include <linux/cpuidle.h>
#include <asm/cpuidle.h>
#include <uapi/linux/psci.h>
#include <linux/arm-smccc.h>
#include <linux/amlogic/pm.h>
#include <linux/kobject.h>
#include <../kernel/power/power.h>
#include <linux/amlogic/power_domain.h>
#include <linux/syscore_ops.h>
#include <linux/init.h>
#include <linux/amlogic/gki_module.h>

bool is_clr_resume_reason;

#if IS_ENABLED(CONFIG_AMLOGIC_LEGACY_EARLY_SUSPEND)

static DEFINE_MUTEX(early_suspend_lock);
static DEFINE_MUTEX(sysfs_trigger_lock);
static LIST_HEAD(early_suspend_handlers);

/* In order to handle legacy early_suspend driver,
 * here we export sysfs interface
 * for user space to write /sys/power/early_suspend_trigger to trigger
 * early_suspend/late resume call back. If user space do not trigger
 * early_suspend/late_resume, this op will be done
 * by PM_SUSPEND_PREPARE notify.
 */
unsigned int sysfs_trigger;
unsigned int early_suspend_state;
unsigned int suspend_debug_flag;
/*
 * Avoid run early_suspend/late_resume repeatedly.
 */
unsigned int already_early_suspend;
/* early suspend debug flag */
unsigned int early_suspend_debug;

void register_early_suspend(struct early_suspend *handler)
{
	struct list_head *pos;

	mutex_lock(&early_suspend_lock);
	list_for_each(pos, &early_suspend_handlers) {
		struct early_suspend *e;

		e = list_entry(pos, struct early_suspend, link);
		if (e->level > handler->level)
			break;
	}
	list_add_tail(&handler->link, pos);
	mutex_unlock(&early_suspend_lock);
}
EXPORT_SYMBOL(register_early_suspend);

void unregister_early_suspend(struct early_suspend *handler)
{
	mutex_lock(&early_suspend_lock);
	list_del(&handler->link);
	mutex_unlock(&early_suspend_lock);
}
EXPORT_SYMBOL(unregister_early_suspend);

static int suspend_get_pm_env(char *buf)
{
	if (!buf)
		return -EINVAL;

	if (kstrtoint(buf, 0, &early_suspend_debug)) {
		pr_err("early_suspend_debug error: %s\n", buf);
		return -EINVAL;
	}

	return 0;
}
__setup("initcall_debug=", suspend_get_pm_env);

static inline void early_suspend(void)
{
	struct early_suspend *pos;

	mutex_lock(&early_suspend_lock);

	if (!already_early_suspend)
		already_early_suspend = 1;
	else
		goto end_early_suspend;

	if (early_suspend_debug)
		pr_info("%s: call handlers\n", __func__);
	list_for_each_entry(pos, &early_suspend_handlers, link)
		if (pos->suspend) {
			if (early_suspend_debug)
				pr_info("%s: %ps\n", __func__, pos->suspend);
			pos->suspend(pos);
		}

	if (early_suspend_debug)
		pr_info("%s: done\n", __func__);

end_early_suspend:
	mutex_unlock(&early_suspend_lock);
}

static inline void late_resume(void)
{
	struct early_suspend *pos;

	mutex_lock(&early_suspend_lock);

	if (already_early_suspend)
		already_early_suspend = 0;
	else
		goto end_late_resume;

	if (early_suspend_debug)
		pr_info("%s: call handlers\n", __func__);
	list_for_each_entry_reverse(pos, &early_suspend_handlers, link)
		if (pos->resume) {
			if (early_suspend_debug)
				pr_info("%s: %ps\n", __func__, pos->resume);
			pos->resume(pos);
		}
	if (early_suspend_debug)
		pr_info("%s: done\n", __func__);

end_late_resume:
	mutex_unlock(&early_suspend_lock);
}

static ssize_t early_suspend_trigger_show(struct class *class,
					  struct class_attribute *attr,
					  char *buf)
{
	unsigned int len;

	len = sprintf(buf, "%d\n", early_suspend_state);

	return len;
}

static ssize_t early_suspend_trigger_store(struct class *class,
					   struct class_attribute *attr,
					   const char *buf, size_t count)
{
	int ret;

	ret = kstrtouint(buf, 0, &early_suspend_state);
	pr_info("early_suspend_state=%d\n", early_suspend_state);

	if (ret)
		return -EINVAL;

	mutex_lock(&sysfs_trigger_lock);
	sysfs_trigger = 1;

	if (early_suspend_state == 0)
		late_resume();
	else if (early_suspend_state == 1)
		early_suspend();
	mutex_unlock(&sysfs_trigger_lock);

	return count;
}

static CLASS_ATTR_RW(early_suspend_trigger);

#define SUSPEND_DEBUG_LOGLEVEL (0xf << (0))
#define SUSPEND_DEBUG_INITCALL_DEBUG (1U << (4))
static unsigned long __invoke_psci_fn_smc(unsigned long function_id,
					  unsigned long arg0,
					  unsigned long arg1,
					  unsigned long arg2);

static const char *suspend_debug_help_str = {
	"Usage:\n"
	"echo 0xXXXXXXXX > /sys/class/meson_pm/suspend_debug  //enable suspend debug mode\n\n"
	"Suspend Debug State List:\n\n"
	"--------------------------------------------------------------------------------------------------\n"
};

#define MAX_ENTRIES 32

struct Suspend_Debug_Mode {
	const char *description;
	int end_bit;
	int start_bit;
	int state;
};

struct Suspend_Debug_Mode suspend_debug_mode[MAX_ENTRIES] = {
	{"[BL30] clk/pll status check and show clk gate setting", 31, 31, 0},
	{"[BL30] mask RTC Wakeup source", 30, 30, 0},
	{"[BL30] mask SARADC Wakeup source", 29, 29, 0},
	{"[BL30] mask IR Wakeup source", 28, 28, 0},
	{"[BL30] show pwm state and voltage", 27, 27, 0},
	{"[BL30] function call log", 26, 26, 0},
	{"[BL30] open dmc monitor log", 25, 25, 0},
	{"[BL30] bl30 dump boot cpu's fsm and dsu's fsm", 24, 24, 0},
	{"[BL30] aocpu do ddr access(reserved)", 23, 23, 0},
	{"[BL30] skip power switch(vddee vddcpu vcc5v vcc3.3V etc)", 22, 22, 0},
	{"[BL30] skip ddr suspend in bl30", 21, 21, 0},
	{"[BL30] start debug task", 20, 20, 0},

	{"[BL31] reserved", 19, 17, 0},
	{"[BL31] skip clk gate switch", 16, 16, 0},
	{"[BL31] dump nonboot cpu's fsm state", 15, 15, 0},
	{"[BL31] skip ddr suspend in tlpm", 14, 14, 0},
	{"[BL31] power domain clk/pll status check and show clk gate setting", 13, 13, 0},
	{"[BL31] do clk reg check and rate check", 12, 12, 0},
	{"[BL31] do bl31 ddr checksum in tlpm", 11, 11, 0},
	{"[BL31] open dmc monitor log", 10, 10, 0},
	{"[BL31] bl31 debug loglevel(0:without log;1: normal;2: function call log)", 9, 8, 0},

	{"[KERNEL] reserved for dmc monitor", 7, 4, 0},
	{"[KERNEL] set kernel printk loglevel to 0~9", 3, 0, 0},
};

static void check_suspend_debug_mode(void)
{
	int i;

	for (i = 0; i < MAX_ENTRIES; i++) {
		int mask = (1 << (suspend_debug_mode[i].end_bit + 1 -
				  suspend_debug_mode[i].start_bit)) - 1;
		int value = (suspend_debug_flag >> suspend_debug_mode[i].start_bit) & mask;

		suspend_debug_mode[i].state = value;
	}
}

void set_suspend_debug_flag(int suspend_flag)
{
	__invoke_psci_fn_smc(0x820000F3, 0, suspend_debug_flag &
		(~(SUSPEND_DEBUG_LOGLEVEL | SUSPEND_DEBUG_INITCALL_DEBUG)),
		  0);
}

static ssize_t suspend_debug_show(struct class *class,
					  struct class_attribute *attr,
					  char *buf)
{
	unsigned int len;
	int i;

	len = sprintf(&buf[0], "suspend_debug_flag = 0x%x\n\n", suspend_debug_flag);

	len += sprintf(&buf[len], "%s\n", suspend_debug_help_str);

	check_suspend_debug_mode();

	for (i = 0; i < MAX_ENTRIES; i++) {
		if (suspend_debug_mode[i].description) {
			if (suspend_debug_mode[i].start_bit == suspend_debug_mode[i].end_bit)
				len += sprintf(&buf[len], "%-80s bit[%d] \t%d\n",
					  suspend_debug_mode[i].description,
					  suspend_debug_mode[i].start_bit,
					  suspend_debug_mode[i].state);
			else
				len += sprintf(&buf[len], "%-80s bit[%d:%d] \t%d\n",
					  suspend_debug_mode[i].description,
					  suspend_debug_mode[i].end_bit,
					  suspend_debug_mode[i].start_bit,
					  suspend_debug_mode[i].state);
		}
	}

	return len;
}

static ssize_t suspend_debug_store(struct class *class,
					   struct class_attribute *attr,
					   const char *buf, size_t count)
{
	int ret;

	ret = kstrtouint(buf, 0, &suspend_debug_flag);
	pr_info("suspend_debug_flag = 0x%x\n", suspend_debug_flag);

	if (ret)
		return -EINVAL;

	set_suspend_debug_flag(suspend_debug_flag);

	return count;
}

static CLASS_ATTR_RW(suspend_debug);
static int suspend_get_debug_env(char *buf)
{
	if (!buf)
		return -EINVAL;

	if (kstrtoint(buf, 0, &suspend_debug_flag)) {
		pr_err("suspend_debug_flag error: %s\n", buf);
		return -EINVAL;
	}

	return 0;
}

__setup("suspend_debug=", suspend_get_debug_env);

void lgcy_early_suspend(void)
{
	mutex_lock(&sysfs_trigger_lock);

	if (!sysfs_trigger)
		early_suspend();

	mutex_unlock(&sysfs_trigger_lock);
}

void lgcy_late_resume(void)
{
	mutex_lock(&sysfs_trigger_lock);

	if (!sysfs_trigger)
		late_resume();

	mutex_unlock(&sysfs_trigger_lock);
}

static int lgcy_early_suspend_notify(struct notifier_block *nb,
				     unsigned long event, void *dummy)
{
	if (event == PM_SUSPEND_PREPARE)
		lgcy_early_suspend();

	if (event == PM_POST_SUSPEND)
		lgcy_late_resume();

	return NOTIFY_OK;
}

static struct notifier_block lgcy_early_suspend_notifier = {
	.notifier_call = lgcy_early_suspend_notify,
};

unsigned int lgcy_early_suspend_exit(struct platform_device *pdev)
{
	int ret;

	ret = unregister_pm_notifier(&lgcy_early_suspend_notifier);
	return ret;
}

#endif /*CONFIG_AMLOGIC_LEGACY_EARLY_SUSPEND*/

typedef unsigned long (psci_fn)(unsigned long, unsigned long,
				unsigned long, unsigned long);

static unsigned long __invoke_psci_fn_smc(unsigned long function_id,
					  unsigned long arg0,
					  unsigned long arg1,
					  unsigned long arg2)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, arg0, arg1, arg2, 0, 0, 0, 0, &res);
	return res.a0;
}

static void __iomem *debug_reg;
static void __iomem *exit_reg;
static suspend_state_t pm_state;
static unsigned int resume_reason;
static unsigned int suspend_reason;
static bool is_extd_resume_reason;

/*
 * get_resume_value return the register value that stores
 * resume reason.
 */
static uint32_t get_resume_value(void)
{
	u32 val = 0;

	if (exit_reg) {
		/* resume reason extension support for new soc such as s4/t3/sc2/s5... */
		if (is_extd_resume_reason)
			val = readl_relaxed(exit_reg) & 0xff;
		/* other soc such as tm2/axg do not support resume reason extension */
		else
			val = (readl_relaxed(exit_reg) >> 28) & 0xf;
	}

	return val;
}

/*
 * get_resume_reason always return last resume reason.
 */
unsigned int get_resume_reason(void)
{
	unsigned int val = 0;
	unsigned int reason;

	val = get_resume_value();
	if (is_extd_resume_reason)
		reason = val & 0x7f;
	else
		reason = val;

	return reason;
}
EXPORT_SYMBOL_GPL(get_resume_reason);

/*
 * get_resume_method return last resume reason.
 * It can be cleared by clr_resume_method().
 */
unsigned int get_resume_method(void)
{
	return resume_reason;
}
EXPORT_SYMBOL_GPL(get_resume_method);

static void set_resume_method(unsigned int val)
{
	resume_reason = val;
}

static int clr_suspend_notify(struct notifier_block *nb,
				     unsigned long event, void *dummy)
{
	if (event == PM_SUSPEND_PREPARE)
		set_resume_method(UNDEFINED_WAKEUP);

	return NOTIFY_OK;
}

static struct notifier_block clr_suspend_notifier = {
	.notifier_call = clr_suspend_notify,
};

unsigned int is_pm_s2idle_mode(void)
{
	if (pm_state == PM_SUSPEND_TO_IDLE)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL_GPL(is_pm_s2idle_mode);

/*Call it as suspend_reason because of historical reasons. */
/*Actually, we should call it wakeup_reason.               */
ssize_t suspend_reason_show(struct class *class,
			    struct class_attribute *attr,
			    char *buf)
{
	unsigned int len;
	unsigned int val;

	suspend_reason = get_resume_reason();
	val = get_resume_value();
	len = sprintf(buf, "%d\nreg val:0x%x\n", suspend_reason, val);

	return len;
}

ssize_t suspend_reason_store(struct class *class,
			     struct class_attribute *attr,
			     const char *buf, size_t count)
{
	int ret;

	ret = kstrtouint(buf, 0, &suspend_reason);

	if (ret)
		return -EINVAL;

	return count;
}

static CLASS_ATTR_RW(suspend_reason);

static unsigned int suspend_mode;

ssize_t suspend_mode_show(struct class *class,
			    struct class_attribute *attr,
			    char *buf)
{
	unsigned int len;

	len = sprintf(buf, "%d\n", suspend_mode);

	return len;
}

ssize_t suspend_mode_store(struct class *class,
			     struct class_attribute *attr,
			     const char *buf, size_t count)
{
	int ret;

	ret = kstrtouint(buf, 0, &suspend_mode);

	if (ret)
		return ret;

	__invoke_psci_fn_smc(0x82000042, suspend_mode, 0, 0);

	return count;
}

static CLASS_ATTR_RW(suspend_mode);

ssize_t time_out_show(struct class *class, struct class_attribute *attr,
		      char *buf)
{
	unsigned int val = 0, len;

	val = readl_relaxed(debug_reg);
	len = sprintf(buf, "%d\n", val);

	return len;
}

static int sys_time_out;
ssize_t time_out_store(struct class *class, struct class_attribute *attr,
		       const char *buf, size_t count)
{
	unsigned int time_out;
	int ret;

	ret = kstrtouint(buf, 10, &time_out);
	switch (ret) {
	case 0:
		sys_time_out = time_out;
		writel_relaxed(time_out, debug_reg);
		break;
	default:
		return -EINVAL;
	}

	return count;
}

static CLASS_ATTR_RW(time_out);

static struct attribute *meson_pm_attrs[] = {
	&class_attr_suspend_reason.attr,
	&class_attr_suspend_mode.attr,
	&class_attr_time_out.attr,
#if IS_ENABLED(CONFIG_AMLOGIC_LEGACY_EARLY_SUSPEND)
	&class_attr_early_suspend_trigger.attr,
#endif
	&class_attr_suspend_debug.attr,
	NULL,
};

ATTRIBUTE_GROUPS(meson_pm);

static struct class meson_pm_class = {
	.name		= "meson_pm",
	.owner		= THIS_MODULE,
	.class_groups = meson_pm_groups,
};

int gx_pm_syscore_suspend(void)
{
	if (sys_time_out)
		writel_relaxed(sys_time_out, debug_reg);
	return 0;
}

void gx_pm_syscore_resume(void)
{
	sys_time_out = 0;
	set_resume_method(get_resume_reason());
}

static struct syscore_ops gx_pm_syscore_ops = {
	.suspend = gx_pm_syscore_suspend,
	.resume	= gx_pm_syscore_resume,
};

static int __init gx_pm_init_ops(void)
{
	register_syscore_ops(&gx_pm_syscore_ops);
	return 0;
}

static int meson_pm_probe(struct platform_device *pdev)
{
	unsigned int irq_pwrctrl;
	int err;

	if (!of_property_read_u32(pdev->dev.of_node,
				  "irq_pwrctrl", &irq_pwrctrl)) {
		pwr_ctrl_irq_set(irq_pwrctrl, 1, 0);
	}

	if (of_property_read_bool(pdev->dev.of_node, "extend_resume_reason"))
		is_extd_resume_reason = true;
	else
		is_extd_resume_reason = false;

	debug_reg = of_iomap(pdev->dev.of_node, 0);
	if (!debug_reg)
		return -ENOMEM;
	exit_reg = of_iomap(pdev->dev.of_node, 1);
	if (!exit_reg)
		return -ENOMEM;

	err = class_register(&meson_pm_class);
	if (unlikely(err))
		return err;

	gx_pm_init_ops();

#if IS_ENABLED(CONFIG_AMLOGIC_LEGACY_EARLY_SUSPEND)
	err = register_pm_notifier(&lgcy_early_suspend_notifier);
	if (unlikely(err))
		return err;
#endif
	if (of_property_read_bool(pdev->dev.of_node, "clr_resume_reason"))
		is_clr_resume_reason = true;
	else
		is_clr_resume_reason = false;

	err = register_pm_notifier(&clr_suspend_notifier);
	if (unlikely(err))
		return err;

	if (suspend_debug_flag)
		set_suspend_debug_flag(suspend_debug_flag);

	return 0;
}

static int __exit meson_pm_remove(struct platform_device *pdev)
{
	if (debug_reg)
		iounmap(debug_reg);
	if (exit_reg)
		iounmap(exit_reg);

	class_unregister(&meson_pm_class);

#if IS_ENABLED(CONFIG_AMLOGIC_LEGACY_EARLY_SUSPEND)
	lgcy_early_suspend_exit(pdev);
#endif
	return 0;
}

static const struct of_device_id amlogic_pm_dt_match[] = {
	{.compatible = "amlogic, pm",
	},
	{}
};

static void meson_pm_shutdown(struct platform_device *pdev)
{
	u32 val;

	if (exit_reg && is_clr_resume_reason &&
			is_extd_resume_reason) {
		val = readl_relaxed(exit_reg);
		val &= (~0x7f);
		writel_relaxed(val, exit_reg);
	}
}

static struct platform_driver meson_pm_driver = {
	.probe = meson_pm_probe,
	.driver = {
		   .name = "pm-meson",
		   .owner = THIS_MODULE,
		   .of_match_table = amlogic_pm_dt_match,
		   },
	.remove = __exit_p(meson_pm_remove),
	.shutdown = meson_pm_shutdown,
};

int __init pm_init(void)
{
	return platform_driver_register(&meson_pm_driver);
}

void __exit pm_exit(void)
{
	platform_driver_unregister(&meson_pm_driver);
}
