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

