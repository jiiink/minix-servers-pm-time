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
    int result = getuptime(ticks, realtime, boottime);
    if (result != OK) {
        return result;
    }
    return OK;
}

static clock_t get_clock_value(int clk_id, clock_t ticks, clock_t realtime)
{
    if (clk_id == CLOCK_REALTIME) {
        return realtime;
    }
    return ticks;
}

static void set_time_reply(time_t sec, uint32_t nsec)
{
    if (mp == NULL) {
        return;
    }
    mp->mp_reply.m_pm_lc_time.sec = sec;
    mp->mp_reply.m_pm_lc_time.nsec = nsec;
}

int do_gettime(void)
{
    clock_t ticks = 0, realtime = 0, clock = 0;
    time_t boottime = 0;

    int clk_id = m_in.m_lc_pm_time.clk_id;

    if (validate_clock_id(clk_id) != OK) {
        return EPERM;
    }

    if (!get_system_times(&ticks, &realtime, &boottime)) {
        return EIO;
    }

    clock = get_clock_value(clk_id, ticks, realtime);

    time_t sec = boottime + (clock / system_hz);
    uint32_t nsec = (uint32_t)((clock % system_hz) * 1000000000ULL / system_hz);

    if (!set_time_reply(sec, nsec)) {
        return EFAULT;
    }

    return OK;
}

int do_getres(void)
{
    if (validate_clock_id(m_in.m_lc_pm_time.clk_id) != OK) {
        return ERR_INVALID_CLOCK_ID;
    }

    set_time_reply(0, 1000000000 / system_hz);
    return OK;
}

int do_settime(void)
{
  if (mp == NULL || mp->mp_effuid != SUPER_USER) {
    return EPERM;
  }

  if (m_in.m_lc_pm_time.clk_id != CLOCK_REALTIME) {
    return EINVAL;
  }

  return sys_settime(m_in.m_lc_pm_time.now,
                     m_in.m_lc_pm_time.clk_id,
                     m_in.m_lc_pm_time.sec,
                     m_in.m_lc_pm_time.nsec);
}

int do_time(void)
{
    struct timespec tv;
    if (clock_time(&tv) != 0) {
        return ERR_TIME_UNAVAILABLE;
    }
    if (set_time_reply(tv.tv_sec, tv.tv_nsec) != 0) {
        return ERR_REPLY_FAILED;
    }
    return OK;
}

int do_stime(void)
{
    clock_t uptime, realtime;
    time_t boottime;

    if (mp->mp_effuid != SUPER_USER) {
        return EPERM;
    }

    if (get_system_times(&uptime, &realtime, &boottime) != OK) {
        return EFAULT;
    }

    boottime = m_in.m_lc_pm_time.sec - (realtime / system_hz);

    int result = sys_stime(boottime);
    if (result != OK) {
        return result;
    }

    return OK;
}