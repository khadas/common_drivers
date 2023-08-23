/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HOST_POLL_H__
#define __HOST_POLL_H__

#include "host.h"

void host_health_monitor_start(struct host_module *host);
void host_health_monitor_stop(struct host_module *host);
int host_logbuff_start(struct host_module *host);
void host_logbuff_stop(struct host_module *host);

#endif /*_HOST_POLL_H*/
