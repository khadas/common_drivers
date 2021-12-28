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
#include <asm/setup.h>
#include <linux/kernel_read_file.h>
#include <linux/vmalloc.h>
#include <linux/amlogic/gki_module.h>
#include "gki_tool.h"

static char cmdline[8192];

static void parse_option(char *cmdline, const char *option,
			 int (*fn)(char *str))
{
	char *str = cmdline;
	char buf[256];

	memset(buf, 0, sizeof(buf));

	while (*str) {
		if (!strncmp(str, option, strlen(option))) {
			char *p = buf;
			int i;

			str += strlen(option);
			for (i = 0; i < (sizeof(buf) - 1) && !(*str == ' ' || !*str); i++)
				*p++ = *str++;

			if (gki_tool_debug)
				pr_info("found option:%s[%s]\n", option, buf);

			fn(buf);
			return;
		}

		while (!(*str == ' ' || !*str))
			str++;

		if (*str == 0)
			break;

		while (*str == ' ')
			str++;
	}

	if (gki_tool_debug)
		pr_info("option:%s not found\n", option);
}

/* hook func of module_init() */
void __module_init_hook(struct module *m)
{
	const struct kernel_symbol *sym;
	struct gki_module_setup_struct *s;
	int i;

	if (gki_tool_debug)
		pr_info("%s() start! num_syms=%d\n", __func__, m->num_syms);

	for (i = 0; i < m->num_syms; i++) {
		sym = &m->syms[i];
		s = (struct gki_module_setup_struct *)gki_symbol_value(sym);

		if (s->magic1 == GKI_MODULE_SETUP_MAGIC1 &&
		    s->magic2 == GKI_MODULE_SETUP_MAGIC2) {
			if (gki_tool_debug)
				pr_info("setup: %s%pS (early=%d)\n",
					s->str, s->fn, s->early);

			parse_option(cmdline, s->str, s->fn);
		}
	}

	if (gki_tool_debug)
		pr_info("%s() end!\n", __func__);
}
EXPORT_SYMBOL(__module_init_hook);

void gki_module_init(void)
{
	void *data = NULL;
	ssize_t size;
	int ret;

	ret = kernel_read_file_from_path("/tmp/cmdline", 0, &data, INT_MAX, &size, READING_UNKNOWN);
	if (ret < 0) {
		pr_err("%s() err ret=%d\n", __func__, ret);
		return;
	}

	strscpy(cmdline, data, 4096);
	pr_info("/proc/cmdline = [%s]\n", cmdline);
	vfree(data);
}

#endif
