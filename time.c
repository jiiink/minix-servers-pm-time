#include "pm.h"
#include <minix/callnr.h>
#include <minix/com.h>
#include <signal.h>
#include <sys/time.h>
#include "mproc.h"

static int validate_clock_id(int clk_id)
{
  return (clk_id == CLOCK_REALTIME || clk_id == CLOCK_MONOTONIC) ? OK : EINVAL;
}

static int get_system_times(clock_t *ticks, clock_t *realtime, time_t *boottime)
{
  int result = getuptime(ticks, realtime, boottime);
  if (result != OK) {
    panic("Failed to get uptime: %d", result);
  }
  return result;
}

static clock_t get_clock_value(int clk_id, clock_t ticks, clock_t realtime)
{
  return (clk_id == CLOCK_REALTIME) ? realtime : ticks;
}

static void set_time_reply(time_t sec, uint32_t nsec)
{
  mp->mp_reply.m_pm_lc_time.sec = sec;
  mp->mp_reply.m_pm_lc_time.nsec = nsec;
}

int do_gettime(void)
{
    clock_t ticks, realtime, clock;
    time_t boottime;
    
    int validation_result = validate_clock_id(m_in.m_lc_pm_time.clk_id);
    if (validation_result != OK) {
        return validation_result;
    }

    get_system_times(&ticks, &realtime, &boottime);
    clock = get_clock_value(m_in.m_lc_pm_time.clk_id, ticks, realtime);
    
    time_t sec = calculate_seconds(boottime, clock);
    uint32_t nsec = calculate_nanoseconds(clock);
    set_time_reply(sec, nsec);

    return OK;
}

static time_t calculate_seconds(time_t boottime, clock_t clock)
{
    return boottime + (clock / system_hz);
}

static uint32_t calculate_nanoseconds(clock_t clock)
{
    const uint64_t NANOSECONDS_PER_SECOND = 1000000000ULL;
    return (uint32_t)((clock % system_hz) * NANOSECONDS_PER_SECOND / system_hz);
}

int do_getres(void)
{
  int validation_result = validate_clock_id(m_in.m_lc_pm_time.clk_id);
  if (validation_result != OK) {
    return validation_result;
  }

  const long NANOSECONDS_PER_SECOND = 1000000000L;
  set_time_reply(0, NANOSECONDS_PER_SECOND / system_hz);
  return OK;
}

int do_settime(void)
{
  if (mp->mp_effuid != SUPER_USER) {
    return EPERM;
  }

  if (m_in.m_lc_pm_time.clk_id != CLOCK_REALTIME) {
    return EINVAL;
  }

  return sys_settime(m_in.m_lc_pm_time.now, m_in.m_lc_pm_time.clk_id,
                    m_in.m_lc_pm_time.sec, m_in.m_lc_pm_time.nsec);
}

int do_time(void)
{
  struct timespec tv;
  clock_time(&tv);
  set_time_reply(tv.tv_sec, tv.tv_nsec);
  return OK;
}

int do_stime(void)
{
  if (mp->mp_effuid != SUPER_USER) {
    return EPERM;
  }
  
  clock_t uptime, realtime;
  time_t boottime;
  
  get_system_times(&uptime, &realtime, &boottime);
  boottime = m_in.m_lc_pm_time.sec - (realtime / system_hz);

  int result = sys_stime(boottime);
  if (result != OK) {
    panic("pm: sys_stime failed: %d", result);
  }

  return OK;
}