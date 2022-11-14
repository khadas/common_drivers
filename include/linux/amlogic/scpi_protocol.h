/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _SCPI_PROTOCOL_H_
#define _SCPI_PROTOCOL_H_

#define MBOX_NEW_VERSION

/* Add this to avoid build error*/

static inline u32 scpi_set_ethernet_wol(u32 flag)
{
	return flag;
}
#endif
