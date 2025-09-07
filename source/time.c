#include "pm.h"
#include <minix/callnr.h>
#include <minix/com.h>
#include <signal.h>
#include <sys/time.h>
#include "mproc.h"

#define NS_PER_SEC 1000000000ULL

int
do_gettime(void)
{
  clock_t uptime_ticks;
  clock_t realtime_ticks;
  time_t boottime_secs;
  clock_t selected_clock_ticks;
  int s;

  if ((s = getuptime(&uptime_ticks, &realtime_ticks, &boottime_secs)) != OK) {
  	panic("do_gettime couldn't get uptime: %d", s);
  }

  switch (m_in.m_lc_pm_time.clk_id) {
	case CLOCK_REALTIME:
		selected_clock_ticks = realtime_ticks;
		break;
	case CLOCK_MONOTONIC:
		selected_clock_ticks = uptime_ticks;
		break;
	default:
		return EINVAL;
  }

  mp->mp_reply.m_pm_lc_time.sec = boottime_secs + (selected_clock_ticks / system_hz);
  mp->mp_reply.m_pm_lc_time.nsec =
	(uint32_t) (((uint64_t)(selected_clock_ticks % system_hz) * NS_PER_SEC) / system_hz);

  return OK;
}

int
do_getres(void)
{
  switch (m_in.m_lc_pm_time.clk_id) {
	case CLOCK_REALTIME:
	case CLOCK_MONOTONIC:
		mp->mp_reply.m_pm_lc_time.sec = 0;
		mp->mp_reply.m_pm_lc_time.nsec = NS_PER_SEC / system_hz;
		return OK;
	default:
		return EINVAL;
  }
}

int
do_settime(void)
{
  if (mp->mp_effuid != SUPER_USER) {
      return EPERM;
  }

  int s;

  switch (m_in.m_lc_pm_time.clk_id) {
	case CLOCK_REALTIME:
		s = sys_settime(m_in.m_lc_pm_time.now, m_in.m_lc_pm_time.clk_id,
			m_in.m_lc_pm_time.sec, m_in.m_lc_pm_time.nsec);
		return s;
	case CLOCK_MONOTONIC:
	default:
		return EINVAL;
  }
}

int
do_time(void)
{
  struct timespec tv;

  (void)clock_time(&tv);

  mp->mp_reply.m_pm_lc_time.sec = tv.tv_sec;
  mp->mp_reply.m_pm_lc_time.nsec = tv.tv_nsec;
  return OK;
}

int
do_stime(void)
{
  clock_t uptime_ticks_val;
  clock_t realtime_ticks_val;
  time_t current_boottime_secs;
  int s;

  if (mp->mp_effuid != SUPER_USER) {
      return EPERM;
  }
  if ((s = getuptime(&uptime_ticks_val, &realtime_ticks_val, &current_boottime_secs)) != OK) {
      panic("do_stime couldn't get uptime: %d", s);
  }

  time_t new_boottime_secs = m_in.m_lc_pm_time.sec - (realtime_ticks_val / system_hz);

  s = sys_stime(new_boottime_secs);
  if (s != OK) {
	panic("pm: sys_stime failed: %d", s);
  }

  return OK;
}