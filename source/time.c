/* This file takes care of those system calls that deal with time.
 *
 * The entry points into this file are
 *   do_getres:		perform the CLOCK_GETRES system call
 *   do_gettime:	perform the CLOCK_GETTIME system call
 *   do_settime:	perform the CLOCK_SETTIME system call
 *   do_time:		perform the GETTIMEOFDAY system call
 *   do_stime:		perform the STIME system call
 */

#include "pm.h"
#include <minix/callnr.h>
#include <minix/com.h>
#include <signal.h>
#include <sys/time.h>
#include "mproc.h"

#define NSEC_PER_SEC 1000000000ULL

static int get_timespec(clockid_t clk_id, struct timespec *tv)
{
	clock_t ticks, realtime, clock;
	time_t boottime;
	int s;

	if ((s = getuptime(&ticks, &realtime, &boottime)) != OK) {
		return s;
	}

	switch (clk_id) {
	case CLOCK_REALTIME:
		clock = realtime;
		break;
	case CLOCK_MONOTONIC:
		clock = ticks;
		break;
	default:
		return EINVAL;
	}

	tv->tv_sec = boottime + (clock / system_hz);
	tv->tv_nsec = (long)((clock % system_hz) * NSEC_PER_SEC / system_hz);

	return OK;
}

int do_gettime(void)
{
	struct timespec ts;
	int r;

	r = get_timespec(m_in.m_lc_pm_time.clk_id, &ts);
	if (r == OK) {
		mp->mp_reply.m_pm_lc_time.sec = ts.tv_sec;
		mp->mp_reply.m_pm_lc_time.nsec = (uint32_t)ts.tv_nsec;
	}
	return r;
}

int do_getres(void)
{
	switch (m_in.m_lc_pm_time.clk_id) {
	case CLOCK_REALTIME:
	case CLOCK_MONOTONIC:
		mp->mp_reply.m_pm_lc_time.sec = 0;
		mp->mp_reply.m_pm_lc_time.nsec = NSEC_PER_SEC / system_hz;
		return OK;
	default:
		return EINVAL;
	}
}

int do_settime(void)
{
	if (mp->mp_effuid != SUPER_USER) {
		return EPERM;
	}

	switch (m_in.m_lc_pm_time.clk_id) {
	case CLOCK_REALTIME:
		return sys_settime(m_in.m_lc_pm_time.now, m_in.m_lc_pm_time.clk_id,
			m_in.m_lc_pm_time.sec, m_in.m_lc_pm_time.nsec);
	case CLOCK_MONOTONIC:
	default:
		return EINVAL;
	}
}

int do_time(void)
{
	struct timespec ts;
	int r;

	r = get_timespec(CLOCK_REALTIME, &ts);
	if (r == OK) {
		mp->mp_reply.m_pm_lc_time.sec = ts.tv_sec;
		mp->mp_reply.m_pm_lc_time.nsec = (uint32_t)ts.tv_nsec;
	}
	return r;
}

int do_stime(void)
{
	clock_t realtime;
	time_t boottime;
	int s;

	if (mp->mp_effuid != SUPER_USER) {
		return EPERM;
	}

	if ((s = getuptime(NULL, &realtime, &boottime)) != OK) {
		return s;
	}

	boottime = m_in.m_lc_pm_time.sec - (realtime / system_hz);

	return sys_stime(boottime);
}