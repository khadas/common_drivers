/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef AM_COM_H
#define AM_COM_H
#include <linux/time.h>
#include <uapi/linux/time.h>

int flashlight_init(void);

int vm_init_module(void);

int gc2145_i2c_driver_init(void);

int gc2145_mipi_i2c_driver_init(void);

int ov5640_i2c_driver_init(void);

void flashlight_exit(void);

void vm_remove_module(void);

struct timeval {
	__kernel_old_time_t	tv_sec;		/* seconds */
	__kernel_suseconds_t	tv_usec;	/* microseconds */
};

static inline void do_gettimeofday(struct timeval *tv)
{
	struct timespec64 now;

	ktime_get_real_ts64(&now);
	tv->tv_sec = now.tv_sec;
	tv->tv_usec = now.tv_nsec / 1000;
}
#endif /* AM_COM_H */
