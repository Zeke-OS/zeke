/*
 * Copyright (c) 2014 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1987 Regents of the University of California.
 * This file may be freely redistributed provided that this
 * notice remains attached.
 */

#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <timeconst.h>
#include <kstring.h>

static const int mon_lengths[2][MONS_PER_YEAR] = {
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
};

/**
 * Secs elapsed at the beginning of each month.
 */
static const int mon_secs_elapsed[2][MONS_PER_YEAR] = {
    { 0, 2678400, 5097600, 7776000, 10368000, 13046400, 15638400, 18316800,
      20995200, 23587200, 26265600, 28857600, },
    { 0, 2678400, 5184000, 7862400, 10454400, 13132800, 15724800, 18403200,
      21081600, 23673600, 26352000, 28944000, },
};

static const int year_lengths[2] = {
    DAYS_PER_NYEAR, DAYS_PER_LYEAR
};

static const int year_lengths_sec[2] = {
    DAYS_PER_NYEAR * SECS_PER_DAY, DAYS_PER_LYEAR * SECS_PER_DAY
};

void offtime(struct tm * tm, const time_t * clock, long offset)
{
    long days;
    long rem;
    int y;
    int yleap;
    const int * ip;

    days = *clock / SECS_PER_DAY;
    rem = *clock % SECS_PER_DAY;
    rem += offset;
    while (rem < 0) {
        rem += SECS_PER_DAY;
        --days;
    }
    while (rem >= SECS_PER_DAY) {
        rem -= SECS_PER_DAY;
        ++days;
    }
    tm->tm_hour = (int) (rem / SECS_PER_HOUR);
    rem = rem % SECS_PER_HOUR;
    tm->tm_min = (int) (rem / SECS_PER_MIN);
    tm->tm_sec = (int) (rem % SECS_PER_MIN);
    tm->tm_wday = (int) ((EPOCH_WDAY + days) % DAYS_PER_WEEK);
    if (tm->tm_wday < 0)
        tm->tm_wday += DAYS_PER_WEEK;
    y = EPOCH_YEAR;
    if (days >= 0)
        for ( ; ; ) {
            yleap = isleap(y);
            if (days < (long) year_lengths[yleap])
                break;
            ++y;
            days = days - (long) year_lengths[yleap];
        }
    else do {
        --y;
        yleap = isleap(y);
        days = days + (long) year_lengths[yleap];
    } while (days < 0);
    tm->tm_year = y - TM_YEAR_BASE;
    tm->tm_yday = (int) days;
    ip = mon_lengths[yleap];
    for (tm->tm_mon = 0; days >= (long) ip[tm->tm_mon]; ++(tm->tm_mon)) {
        days = days - (long) ip[tm->tm_mon];
    }
    tm->tm_mday = (int) (days + 1);
    tm->tm_isdst = 0;
}

void gmtime(struct tm * tm, const time_t * clock)
{
    offtime(tm, clock, 0L);
}

void mktimespec(struct timespec * ts, const struct tm * tm)
{
    time_t sec;

    sec = tm->tm_sec;
    sec += tm->tm_min * SECS_PER_MIN;
    sec += tm->tm_hour * SECS_PER_HOUR;
    sec += (tm->tm_mday - 1) * SECS_PER_DAY;

    sec += mon_secs_elapsed[isleap(TM_YEAR_BASE + tm->tm_year)][tm->tm_mon];
    for (int y = TM_YEAR_BASE + tm->tm_year - 1; y >= EPOCH_YEAR; y--) {
        sec += year_lengths_sec[isleap(y)];
    }

    ts->tv_sec = sec;
    ts->tv_nsec = 0;
}

void nsec2timespec(struct timespec * ts, int64_t nsec)
{
    const int64_t sec_nsec = (int64_t)1000000000;
    int64_t mod;

    mod = nsec % sec_nsec;
    ts->tv_sec = (nsec - mod) / sec_nsec;
    ts->tv_nsec = mod;
}

void timespec_add(struct timespec * sum, const struct timespec * left,
                  const struct timespec * right)
{
    struct timespec ts;

    sum->tv_sec = left->tv_sec + right->tv_sec;
    nsec2timespec(&ts, (int64_t)left->tv_nsec + (int64_t)right->tv_nsec);
    sum->tv_sec += ts.tv_sec;
    sum->tv_nsec = ts.tv_nsec;
}

void timespec_sub(struct timespec * diff, const struct timespec * left,
                  const struct timespec * right)
{
    const int64_t sec_nsec = (int64_t)1000000000;
    struct timespec ts;

    diff->tv_sec = left->tv_sec - right->tv_sec;
    nsec2timespec(&ts, (int64_t)left->tv_nsec - (int64_t)right->tv_nsec);
    if (ts.tv_nsec < 0 && diff->tv_sec >= 1) {
        diff->tv_sec -= 1;
        ts.tv_nsec += sec_nsec;
    }
    diff->tv_sec += ts.tv_sec;
    diff->tv_nsec = ts.tv_nsec;
}

void timespec_mul(struct timespec * prod, const struct timespec * left,
                  const struct timespec * right)
{
    struct timespec ts;

    prod->tv_sec = left->tv_sec * right->tv_sec;
    nsec2timespec(&ts, (int64_t)left->tv_nsec * (int64_t)right->tv_nsec);
    prod->tv_sec += ts.tv_sec;
    prod->tv_nsec = ts.tv_nsec;
}

void timespec_div(struct timespec * quot, const struct timespec * left,
                  const struct timespec * right)
{
    quot->tv_sec = left->tv_sec / right->tv_sec;
    quot->tv_nsec = left->tv_nsec / right->tv_nsec;
}

void timespec_mod(struct timespec * rem, const struct timespec * left,
                  const struct timespec * right)
{
    rem->tv_sec = left->tv_sec % right->tv_sec;
    rem->tv_nsec = left->tv_nsec % right->tv_nsec;
}
