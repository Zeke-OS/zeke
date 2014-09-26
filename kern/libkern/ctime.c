/*
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1987 Regents of the University of California.
 * This file may be freely redistributed provided that this
 * notice remains attached.
 */

#define KERNEL_INTERNAL
#include <stdbool.h>
#include <timeconst.h>
#include <sys/time.h>

static int mon_lengths[2][MONS_PER_YEAR] = {
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
};

static int year_lengths[2] = {
    DAYS_PER_NYEAR, DAYS_PER_LYEAR
};

static char wday_name[DAYS_PER_WEEK][3] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
static char mon_name[MONS_PER_YEAR][3] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};


void
ctime(char * result, time_t * t)
{
    struct timespec ts;
    struct tm tm;

    nanotime(&ts);
    gmtime(&tm, &ts.tv_sec);
    asctime(result, &tm);
}

void
asctime(char * result, struct tm * timeptr)
{
    ksprintf(result, 26, "%.3s %.3s%3d %02d:%02d:%02d %d\n",
             wday_name[timeptr->tm_wday],
             mon_name[timeptr->tm_mon],
             timeptr->tm_mday, timeptr->tm_hour,
             timeptr->tm_min, timeptr->tm_sec,
             TM_YEAR_BASE + timeptr->tm_year);
}

void
gmtime(struct tm * tm, time_t * clock)
{
    offtime(tm, clock, 0L);
}

void
offtime(struct tm * tm, time_t * clock, long offset)
{
    long       days;
    long       rem;
    int        y;
    int        yleap;
    int *      ip;

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
