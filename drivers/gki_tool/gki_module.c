// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
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
#include <asm/setup.h>
#include <linux/kernel_read_file.h>
#include <linux/vmalloc.h>
#include <linux/amlogic/gki_module.h>
#include <linux/kconfig.h>
#include <linux/security.h>
#include <linux/kprobes.h>
#include "gki_tool.h"

#if defined(CONFIG_CMDLINE_FORCE)
static const int overwrite_incoming_cmdline = 1;
static const int read_dt_cmdline;
static const int concat_cmdline;
#elif defined(CONFIG_CMDLINE_EXTEND)
static const int overwrite_incoming_cmdline;
static const int read_dt_cmdline = 1;
static const int concat_cmdline = 1;
#else /* CMDLINE_FROM_BOOTLOADER */
static const int overwrite_incoming_cmdline;
static const int read_dt_cmdline = 1;
static const int concat_cmdline;
#endif

#ifdef CONFIG_CMDLINE
static const char *config_cmdline = CONFIG_CMDLINE;
#else
static const char *config_cmdline = "";
#endif

static char *cmdline;
struct cmd_param_val *cpv;
int cpv_count;

static char dash2underscore(char c)
{
	if (c == '-')
		return '_';
	return c;
}

bool gki_tool_parameqn(const char *a, const char *b, size_t n)
{
	size_t i;

	for (i = 0; i < n; i++) {
		if (dash2underscore(a[i]) != dash2underscore(b[i]))
			return false;
	}
	return true;
}

/* hook func of module_init() */
void __module_init_hook(struct module *m)
{
	const struct kernel_symbol *sym;
	struct gki_module_setup_struct *s;
	int i, j;
	static int initialized;

	if (!initialized) {
		gki_module_init();
		initialized = 1;
	}

	for (i = 0; i < m->num_syms; i++) {
		sym = &m->syms[i];
		s = (struct gki_module_setup_struct *)gki_symbol_value(sym);

		if (s->magic1 == GKI_MODULE_SETUP_MAGIC1 &&
		    s->magic2 == GKI_MODULE_SETUP_MAGIC2) {
			pr_debug("setup: %s, %ps (early=%d)\n",
				s->str, s->fn, s->early);
			for (j = 0; j < cpv_count; j++) {
				int n = strlen(cpv[j].param);
				int (*fn)(char *str) = s->fn;

				if (gki_tool_parameqn(cpv[j].param, s->str, n) &&
				   (s->str[n] == '=' || !s->str[n])) {
					fn(cpv[j].val);
					continue;
				}
			}
		}
	}
}
EXPORT_SYMBOL(__module_init_hook);

int cmdline_parse_args(char *args)
{
	char *param, *val, *args1, *args0;
	int i;

	args1 = kstrdup(args, GFP_KERNEL);
	args0 = args1;
	args1 = skip_spaces(args1);
	cpv_count = 0;
	while (*args1) {
		args1 = next_arg(args1, &param, &val);
		if (!val && strcmp(param, "--") == 0)
			break;
		cpv_count++;
	}
	kfree(args0);

	args = skip_spaces(args);
	i = 0;
	cpv = kmalloc_array(cpv_count, sizeof(struct cmd_param_val), GFP_KERNEL);
	while (*args) {
		args = next_arg(args, &param, &val);
		if (!val && strcmp(param, "--") == 0)
			break;
		cpv[i].param = param;
		cpv[i].val = val;
		i++;
		if (i == cpv_count)
			break;
	}

	cpv_count = i;
	for (i = 0; i < cpv_count; i++)
		pr_debug("[%02d] %s=%s\n", i, cpv[i].param, cpv[i].val);

	return 0;
}

void gki_module_init(void)
{
	struct device_node *of_chosen;
	int len1 = 0;
	int len2 = 0;
	const char *bootargs = NULL;

	//if (overwrite_incoming_cmdline || !cmdline[0])
	if (config_cmdline)
		len1 = strlen(config_cmdline);

	if (read_dt_cmdline) {
		of_chosen = of_find_node_by_path("/chosen");
		if (!of_chosen)
			of_chosen = of_find_node_by_path("/chosen@0");
		if (of_chosen)
			if (of_property_read_string(of_chosen, "bootargs", &bootargs) == 0)
				len2 = strlen(bootargs);
	}

	cmdline = kmalloc(len1 + len2 + 2, GFP_KERNEL);
	if (!cmdline)
		return;

	if (len1) {
		strcpy(cmdline, config_cmdline);
		strcat(cmdline, " ");
	}

	if (len2) {
		if (concat_cmdline)
			strcat(cmdline, bootargs);
		else
			strcpy(cmdline, bootargs);
	}

	pr_debug("cmdline: %lx, %s\n", (unsigned long)cmdline, cmdline);
	cmdline_parse_args(cmdline);
}

static int ramoops_io_en;

static int ramoops_io_en_gki_setup(char *buf)
{
	if (!buf)
		return -EINVAL;

	if (kstrtoint(buf, 0, &ramoops_io_en)) {
		pr_err("ramoops_io_en error: %s\n", buf);
		return -EINVAL;
	}

	return 0;
}
__setup("ramoops_io_en=", ramoops_io_en_gki_setup);

static struct kprobe do_free_init_kp = {
	.symbol_name = "do_free_init",
};

static struct work_struct *w;

static struct delayed_work mod_free_work;

static int do_free_init_hook(struct kprobe *p, struct pt_regs *regs)
{
#ifdef CONFIG_ARM64
	w = (struct work_struct *)regs->regs[0];
	instruction_pointer_set(regs, regs->regs[30]);
#elif CONFIG_ARM
	w = (struct work_struct *)regs->ARM_r0;
	instruction_pointer_set(regs, regs->ARM_lr);
#endif

	// return to lr, skip do_free_init function
	return 1;
}

static void mod_free_func(struct work_struct *work)
{
	unregister_kprobe(&do_free_init_kp);

	// free module init_layout memory, w->func = do_free_init
	schedule_work(w);
}

int module_debug_init(void)
{
	int ret;

	if (ramoops_io_en) {
		do_free_init_kp.pre_handler = do_free_init_hook;
		ret = register_kprobe(&do_free_init_kp);

		if (ret < 0) {
			pr_err("register_kprobe failed, returned %d\n", ret);
			return ret;
		}

		INIT_DELAYED_WORK(&mod_free_work, mod_free_func);

		// trigger free module init_layout memory work
		queue_delayed_work(system_unbound_wq, &mod_free_work, 60 * HZ);
	}

	return 0;
}
