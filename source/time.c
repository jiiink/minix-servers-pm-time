#include "pm.h"
#include <minix/callnr.h>
#include <minix/com.h>
#include <signal.h>
#include <sys/time.h>
#include "mproc.h"

int do_gettime(void)
{
  clock_t ticks, realtime;
  time_t boottime;
  clock_t clock;

  if (getuptime(&ticks, &realtime, &boottime) != OK)
  	panic("do_gettime couldn't get uptime");

  switch (m_in.m_lc_pm_time.clk_id) {
	case CLOCK_REALTIME:
		clock = realtime;
		break;
	case CLOCK_MONOTONIC:
		clock = ticks;
		break;
	default:
		return EINVAL;
  }

  mp->mp_reply.m_pm_lc_time.sec = boottime + (clock / system_hz);
  mp->mp_reply.m_pm_lc_time.nsec = (uint32_t) ((clock % system_hz) * 1000000000ULL / system_hz);

  return OK;
}

int do_getres(void)
{
  switch (m_in.m_lc_pm_time.clk_id) {
	case CLOCK_REALTIME:
	case CLOCK_MONOTONIC:
		mp->mp_reply.m_pm_lc_time.sec = 0;
		mp->mp_reply.m_pm_lc_time.nsec = 1000000000 / system_hz;
		return OK;
	default:
		return EINVAL;
  }
}

int do_settime(void)
{
  if (mp->mp_effuid != SUPER_USER)
      return EPERM;

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
  struct timespec tv;

  clock_time(&tv);

  mp->mp_reply.m_pm_lc_time.sec = tv.tv_sec;
  mp->mp_reply.m_pm_lc_time.nsec = tv.tv_nsec;
  return OK;
}

int do_stime(void)
{
  clock_t realtime;
  time_t boottime;

  if (mp->mp_effuid != SUPER_USER)
      return EPERM;

  if (getuptime(NULL, &realtime, &boottime) != OK)
      panic("do_stime couldn't get uptime");

  boottime = m_in.m_lc_pm_time.sec - (realtime / system_hz);

  if (sys_stime(boottime) != OK)
	panic("pm: sys_stime failed");

  return OK;
}
