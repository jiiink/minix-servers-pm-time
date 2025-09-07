#include "pm.h"
#include <minix/callnr.h>
#include <minix/com.h>
#include <signal.h>
#include <sys/time.h>
#include <stdint.h>
#include "mproc.h"

#define PM_NSEC_PER_SEC 1000000000ULL

int do_gettime(void)
{
  clock_t ticks, realtime, clk_ticks;
  time_t boottime;
  int s;

  if ((s = getuptime(&ticks, &realtime, &boottime)) != OK)
    panic("do_time couldn't get uptime: %d", s);

  switch (m_in.m_lc_pm_time.clk_id) {
    case CLOCK_REALTIME:
      clk_ticks = realtime;
      break;
    case CLOCK_MONOTONIC:
      clk_ticks = ticks;
      break;
    default:
      return EINVAL;
  }

  mp->mp_reply.m_pm_lc_time.sec = boottime + (time_t)(clk_ticks / system_hz);
  mp->mp_reply.m_pm_lc_time.nsec = (uint32_t)(((uint64_t)(clk_ticks % system_hz) * PM_NSEC_PER_SEC) / (uint64_t)system_hz);

  return(OK);
}

int do_getres(void)
{
  switch (m_in.m_lc_pm_time.clk_id) {
    case CLOCK_REALTIME:
    case CLOCK_MONOTONIC:
      mp->mp_reply.m_pm_lc_time.sec = 0;
      mp->mp_reply.m_pm_lc_time.nsec = (uint32_t)(PM_NSEC_PER_SEC / (uint64_t)system_hz);
      return(OK);
    default:
      return EINVAL;
  }
}

int do_settime(void)
{
  int s;

  if (mp->mp_effuid != SUPER_USER) {
    return(EPERM);
  }

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

int do_time(void)
{
  struct timespec tv;

  (void)clock_time(&tv);

  mp->mp_reply.m_pm_lc_time.sec = tv.tv_sec;
  mp->mp_reply.m_pm_lc_time.nsec = tv.tv_nsec;
  return(OK);
}

int do_stime(void)
{
  clock_t uptime, realtime;
  time_t boottime;
  int s;

  if (mp->mp_effuid != SUPER_USER) {
    return(EPERM);
  }
  if ((s = getuptime(&uptime, &realtime, &boottime)) != OK)
    panic("do_stime couldn't get uptime: %d", s);

  boottime = m_in.m_lc_pm_time.sec - (time_t)(realtime / system_hz);

  s = sys_stime(boottime);
  if (s != OK)
    panic("pm: sys_stime failed: %d", s);

  return(OK);
}