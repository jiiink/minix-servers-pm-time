#include "pm.h"
#include <minix/callnr.h>
#include <minix/com.h>
#include <signal.h>
#include <sys/time.h>
#include "mproc.h"

static int validate_clock_id(int clk_id)
{
  if (clk_id == CLOCK_REALTIME || clk_id == CLOCK_MONOTONIC) {
    return OK;
  }
  return EINVAL;
}

static int get_system_times(clock_t *ticks, clock_t *realtime, time_t *boottime)
{
  if (ticks == NULL) {
    panic("NULL argument passed for ticks pointer.");
  }
  if (realtime == NULL) {
    panic("NULL argument passed for realtime pointer.");
  }
  if (boottime == NULL) {
    panic("NULL argument passed for boottime pointer.");
  }

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

struct TimeValue {
  time_t sec;
  uint32_t nsec;
};

static void set_time_reply(struct TimeValue *target_time_fields, time_t sec, uint32_t nsec)
{
  target_time_fields->sec = sec;
  target_time_fields->nsec = nsec;
}

int do_gettime(int clock_id_param)
{
  clock_t cpu_ticks;
  clock_t real_time_ticks;
  clock_t selected_clock_value;
  time_t boot_time_seconds;
  
  int validation_result = validate_clock_id(clock_id_param);
  if (validation_result != OK) {
    return validation_result;
  }

  get_system_times(&cpu_ticks, &real_time_ticks, &boot_time_seconds);
  selected_clock_value = get_clock_value(clock_id_param, cpu_ticks, real_time_ticks);
  
  time_t seconds = boot_time_seconds + (selected_clock_value / system_hz);
  uint32_t nanoseconds = (uint32_t)((selected_clock_value % system_hz) * 1000000000ULL / system_hz);
  set_time_reply(seconds, nanoseconds);

  return OK;
}

int do_getres(void)
{
  int validation_result = validate_clock_id(m_in.m_lc_pm_time.clk_id);
  if (validation_result != OK) {
    return validation_result;
  }

  if (system_hz == 0) {
    return ERROR_SYSTEM_HZ_ZERO;
  }

  set_time_reply(REPLY_STATUS_SUCCESS, NANO_SECONDS_PER_SECOND / system_hz);
  return OK;
}

int do_settime(void)
{
  if (mp == NULL) {
    return EFAULT;
  }

  if (mp->mp_effuid != SUPER_USER) {
    return EPERM;
  }

  if (m_in.m_lc_pm_time.clk_id == CLOCK_REALTIME) {
    return sys_settime(m_in.m_lc_pm_time.now, m_in.m_lc_pm_time.clk_id,
                      m_in.m_lc_pm_time.sec, m_in.m_lc_pm_time.nsec);
  }
  
  return EINVAL;
}

int do_time(void)
{
  struct timespec tv;

  if (clock_time(&tv) != 0) {
    return ERROR;
  }

  if (set_time_reply(tv.tv_sec, tv.tv_nsec) != 0) {
    return ERROR;
  }

  return OK;
}

int do_stime(void)
{
  if (mp->mp_effuid != SUPER_USER) {
    return EPERM;
  }
  
  if (system_hz == 0) {
    panic("pm: system_hz is zero, cannot calculate boot time.");
  }

  clock_t uptime_ticks;
  clock_t realtime_ticks;
  time_t initial_boottime; 

  get_system_times(&uptime_ticks, &realtime_ticks, &initial_boottime);

  time_t realtime_seconds = (time_t)realtime_ticks / system_hz;
  time_t new_boottime = m_in.m_lc_pm_time.sec - realtime_seconds;

  int result = sys_stime(new_boottime);
  if (result != OK) {
    panic("pm: sys_stime failed: %d", result);
  }

  return OK;
}