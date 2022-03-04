/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef AM_COM_H
#define AM_COM_H
#include <linux/time.h>
#include <uapi/linux/time.h>

struct timeval {
	__kernel_old_time_t	tv_sec;		/* seconds */
	__kernel_suseconds_t	tv_usec;	/* microseconds */
};

static inline void do_gettimeofday(struct timespec64 *tv)
{
	ktime_get_real_ts64(tv);
}
#endif /* AM_COM_H */
