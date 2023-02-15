#ifndef _Y_PORT_H_
#define _Y_PORT_H_

#include <linux/time32.h>
#include <linux/blkdev.h>
#if 0
struct timespec {
	__kernel_old_time_t	tv_sec;		/* seconds */
	long			tv_nsec;	/* nanoseconds */
};

#if __bits_per_long == 64

/* timespec64 is defined as timespec here */
static inline struct timespec timespec64_to_timespec(const struct timespec64 ts64)
{
	return *(const struct timespec *)&ts64;
}
#else
static inline struct timespec timespec64_to_timespec(const struct timespec64 ts64)
{
	struct timespec ret;

	ret.tv_sec = (time_t)ts64.tv_sec;
	ret.tv_nsec = ts64.tv_nsec;
	return ret;
}
#endif


static inline struct timespec current_kernel_time(void)
{
	struct timespec64 now;
	ktime_get_real_ts64(&now);

	return timespec64_to_timespec(now);
}
#endif

static inline struct timespec64 current_kernel_time64(void)
{
	struct timespec64 now;
	ktime_get_real_ts64(&now);

	return now;
}

#define current_kernel_time current_kernel_time64
#endif /*_Y_PORT_H_ */
