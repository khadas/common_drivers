/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _AML_WWE_H_
#define _AML_WWE_H_
struct aml_wwe {
	struct device *dev;
	struct task_struct *wwe_task;
	int hwptr;
	int prepare_times;
	int dum;
};
#endif
