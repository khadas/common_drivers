// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#ifdef MODULE

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

bool parameqn(const char *a, const char *b, size_t n)
{
	size_t i;

	for (i = 0; i < n; i++) {
		if (dash2underscore(a[i]) != dash2underscore(b[i]))
			return false;
	}
	return true;
}

bool parameq(const char *a, const char *b)
{
	return parameqn(a, b, strlen(a) + 1);
}

static bool param_check_unsafe(const struct kernel_param *kp)
{
	if (kp->flags & KERNEL_PARAM_FL_HWPARAM &&
	    security_locked_down(LOCKDOWN_MODULE_PARAMETERS))
		return false;

	if (kp->flags & KERNEL_PARAM_FL_UNSAFE) {
		pr_notice("Setting dangerous option %s - tainting kernel\n",
			  kp->name);
		add_taint(TAINT_USER, LOCKDEP_STILL_OK);
	}

	return true;
}

/* hook func of module_init() */
void __module_init_hook(struct module *m)
{
	const struct kernel_symbol *sym;
	struct gki_module_setup_struct *s;
	int i, j;
	int ret;

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

				if (parameqn(cpv[j].param, s->str, n) &&
				   (s->str[n] == '=' || !s->str[n])) {
					fn(cpv[j].val);
					continue;
				}
			}
		}
	}

	for (i = 0; i < m->num_kp; i++) {
		pr_debug("module_param: %s\n", m->kp[i].name);
		for (j = 0; j < cpv_count; j++) {
			int n = strlen(m->name);

			if ((cpv[j].val || m->kp[i].ops->flags & KERNEL_PARAM_OPS_FL_NOARG) &&
			    parameqn(cpv[j].param, m->name, strlen(m->name)) &&
			    (cpv[j].param[n] == '.') &&
			    parameq(&cpv[j].param[n + 1], m->kp[i].name)) {
				pr_debug("cmd_param_val: %s=%s\n", cpv[j].param, cpv[j].val);
				kernel_param_lock(m);
				if (param_check_unsafe(&m->kp[i]))
					ret = m->kp[i].ops->set(cpv[j].val, &m->kp[i]);
				else
					ret = -EPERM;
				kernel_param_unlock(m);
				if (ret)
					pr_debug("parm set result: %d\n", ret);
				continue;
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
	if (!cmdline) {
		pr_err("couldn't allocate memory for cmdline %dbyte\n", len1 + len2 + 2);
		return;
	}

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

#endif
