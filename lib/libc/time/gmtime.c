/*
 * Copyright (c) 2014 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1987 Regents of the University of California.
 * This file may be freely redistributed provided that this
 * notice remains attached.
 */

#include <sys/time.h>
#include <time.h>
#include <sys/timeconst.h>

static const int mon_lengths[2][MONS_PER_YEAR] = {
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
};

static const int year_lengths[2] = {
    DAYS_PER_NYEAR, DAYS_PER_LYEAR
};

static void offtime(struct tm * restrict tm, const time_t * restrict clock,
                    long offset)
{
    long days, rem;
    int y, yleap;
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
    tm->tm_hour = (int)(rem / SECS_PER_HOUR);
    rem = rem % SECS_PER_HOUR;
    tm->tm_min = (int)(rem / SECS_PER_MIN);
    tm->tm_sec = (int)(rem % SECS_PER_MIN);
    tm->tm_wday = (int)((EPOCH_WDAY + days) % DAYS_PER_WEEK);
    if (tm->tm_wday < 0)
        tm->tm_wday += DAYS_PER_WEEK;
    y = EPOCH_YEAR;
    if (days >= 0)
        for ( ; ; ) {
            yleap = isleap(y);
            if (days < (long)year_lengths[yleap])
                break;
            ++y;
            days = days - (long)year_lengths[yleap];
        }
    else do {
        --y;
        yleap = isleap(y);
        days = days + (long)year_lengths[yleap];
    } while (days < 0);
    tm->tm_year = y - TM_YEAR_BASE;
    tm->tm_yday = (int)days;
    ip = mon_lengths[yleap];
    for (tm->tm_mon = 0; days >= (long)ip[tm->tm_mon]; ++(tm->tm_mon)) {
        days = days - (long)ip[tm->tm_mon];
    }
    tm->tm_mday = (int)(days + 1);
    tm->tm_isdst = 0;
}

struct tm * gmtime(const time_t * timer)
{
    static struct tm tm;
    offtime(&tm, timer, 0L);
    return &tm;
}

void gmtime_r(const time_t * restrict timer, struct tm * restrict result)
{
    offtime(result, timer, 0L);
}
