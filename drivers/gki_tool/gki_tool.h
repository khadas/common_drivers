/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __GKI_TOOL_AMLOGIC_H
#define __GKI_TOOL_AMLOGIC_H

#ifdef MODULE

void gki_module_init(void);
void gki_config_init(void);

extern int gki_tool_debug;

extern char gki_config_data[];
extern char gki_config_data_end[];

#endif

#endif //__GKI_TOOL_AMLOGIC_H
