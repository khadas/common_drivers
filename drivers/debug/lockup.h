/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __DEBUG_LOCKUP_H_
#define __DEBUG_LOCKUP_H_

#define DEBUG_HOOK_MAGIC 0x64656280 //'d', 'e', 'b', '\x80'

enum debug_hook_type {
	DEBUG_HOOK_IRQ_START,
	DEBUG_HOOK_IRQ_STOP,
	DEBUG_HOOK_PSTORE_ATTACH,
	DEBUG_HOOK_MAX_CNT,
};

void pr_lockup_info(int lock_cpu);

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_TEST)
extern int smc_long_debug;
extern int idle_long_debug;
#endif

#endif /*_DEBUG_LOCKUP_H__*/
