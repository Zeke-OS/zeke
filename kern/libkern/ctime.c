/*
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1987 Regents of the University of California.
 * This file may be freely redistributed provided that this
 * notice remains attached.
 */

#define KERNEL_INTERNAL
#include <stdbool.h>
#include <sys/time.h>
#include <tzfile.h>

static int  mon_lengths[2][MONS_PER_YEAR] = {
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

const char * tzname[2] = {
    "GMT",
    "GMT"
};


#if 0
char *
ctime(time_t * t)
{
    struct tm   *localtime();
    char    *asctime();

    return asctime(localtime(t));
}

char *
asctime(timeptr)
struct tm *    timeptr;
{
    static char result[26];

    (void) sprintf(result, "%.3s %.3s%3d %02d:%02d:%02d %d\n",
        wday_name[timeptr->tm_wday],
        mon_name[timeptr->tm_mon],
        timeptr->tm_mday, timeptr->tm_hour,
        timeptr->tm_min, timeptr->tm_sec,
        TM_YEAR_BASE + timeptr->tm_year);
    return result;
}
#endif

#if 0
struct ttinfo {         /* time type information */
    long    tt_gmtoff;  /* GMT offset in seconds */
    int     tt_isdst;   /* used to set tm_isdst */
    int     tt_abbrind; /* abbreviation list index */
};

struct state {
    int     timecnt;
    int     typecnt;
    int     charcnt;
    time_t      ats[TZ_MAX_TIMES];
    unsigned char   types[TZ_MAX_TIMES];
    struct ttinfo   ttis[TZ_MAX_TYPES];
    char        chars[TZ_MAX_CHARS + 1];
};

static struct state s;

static int      tz_is_set;

#ifdef USG_COMPAT
time_t  timezone;
int     daylight;
#endif /* USG_COMPAT */

static long
detzcode(codep)
char *  codep;
{
    long   result;
    int    i;

    result = 0;
    for (i = 0; i < 4; ++i) {
        result = (result << 8) | (codep[i] & 0xff);
    }

    return result;
}

static int
tzload(name)
char * name;
{
    int    i;
    int    fid;

    if (name == 0 && (name = _PATH_LOCALTIME) == 0)
        return -1;
    {
        char * p;
        int    doaccess;
        char          * fullname;

        doaccess = name[0] == '/';
        if (!doaccess) {
            if ((p = _PATH_ZONEINFO) == 0)
                return -1;
            if ((strlen(p) + strlen(name) + 1) >= MAXPATHLEN)
                return -1;
                        fullname = alloca(MAXPATHLEN);
            (void) strcpy(fullname, p);
            (void) strcat(fullname, "/");
            (void) strcat(fullname, name);
            /*
            ** Set doaccess if '.' (as in "../") shows up in name.
            */
            while (*name != '\0')
                if (*name++ == '.')
                    doaccess = true;
            name = fullname;
        }
        if (doaccess && access(name, 4) != 0)
            return -1;
        if ((fid = open(name, 0)) == -1)
            return -1;
    }
    {
        char *         p;
        struct tzhead *    tzhp;
        char *              buf;

        buf = alloca(sizeof(s));
        i = read(fid, buf, sizeof(s));
        if (close(fid) != 0 || i < sizeof(*tzhp))
            return -1;
        tzhp = (struct tzhead *) buf;
        s.timecnt = (int) detzcode(tzhp->tzh_timecnt);
        s.typecnt = (int) detzcode(tzhp->tzh_typecnt);
        s.charcnt = (int) detzcode(tzhp->tzh_charcnt);
        if (s.timecnt > TZ_MAX_TIMES ||
            s.typecnt == 0 ||
            s.typecnt > TZ_MAX_TYPES ||
            s.charcnt > TZ_MAX_CHARS)
                return -1;
        if (i < sizeof(*tzhp) +
            s.timecnt * (4 + sizeof(char)) +
            s.typecnt * (4 + 2 * sizeof(char)) +
            s.charcnt * sizeof(char))
                return -1;
        p = buf + sizeof(*tzhp);
        for (i = 0; i < s.timecnt; ++i) {
            s.ats[i] = detzcode(p);
            p += 4;
        }
        for (i = 0; i < s.timecnt; ++i) {
            s.types[i] = (unsigned char) *p++;
        }
        for (i = 0; i < s.typecnt; ++i) {
            struct ttinfo *    ttisp;

            ttisp = &s.ttis[i];
            ttisp->tt_gmtoff = detzcode(p);
            p += 4;
            ttisp->tt_isdst = (unsigned char) *p++;
            ttisp->tt_abbrind = (unsigned char) *p++;
        }
        for (i = 0; i < s.charcnt; ++i) {
            s.chars[i] = *p++;
        }
        s.chars[i] = '\0';  /* ensure '\0' at end */
    }
    /*
    ** Check that all the local time type indices are valid.
    */
    for (i = 0; i < s.timecnt; ++i)
        if (s.types[i] >= s.typecnt)
            return -1;
    /*
    ** Check that all abbreviation indices are valid.
    */
    for (i = 0; i < s.typecnt; ++i)
        if (s.ttis[i].tt_abbrind >= s.charcnt)
            return -1;
    /*
    ** Set tzname elements to initial values.
    */
    tzname[0] = tzname[1] = &s.chars[0];
#ifdef USG_COMPAT
    timezone = s.ttis[0].tt_gmtoff;
    daylight = 0;
#endif /* USG_COMPAT */
    for (i = 1; i < s.typecnt; ++i) {
        struct ttinfo *    ttisp;

        ttisp = &s.ttis[i];
        if (ttisp->tt_isdst) {
            tzname[1] = &s.chars[ttisp->tt_abbrind];
#ifdef USG_COMPAT
            daylight = 1;
#endif /* USG_COMPAT */
        } else {
            tzname[0] = &s.chars[ttisp->tt_abbrind];
#ifdef USG_COMPAT
            timezone = ttisp->tt_gmtoff;
#endif /* USG_COMPAT */
        }
    }
    return 0;
}

static int
tzsetkernel()
{
    struct timeval  tv;
    struct timezone tz;

    if (gettimeofday(&tv, &tz))
        return -1;
    s.timecnt = 0;      /* UNIX counts *west* of Greenwich */
    s.ttis[0].tt_gmtoff = tz.tz_minuteswest * -SECS_PER_MIN;
    s.ttis[0].tt_abbrind = 0;
    (void)strcpy(s.chars, tztab(tz.tz_minuteswest, 0));
    tzname[0] = tzname[1] = s.chars;
#ifdef USG_COMPAT
    timezone = tz.tz_minuteswest * 60;
    daylight = tz.tz_dsttime;
#endif /* USG_COMPAT */
    return 0;
}

static void
tzsetgmt()
{
    s.timecnt = 0;
    s.ttis[0].tt_gmtoff = 0;
    s.ttis[0].tt_abbrind = 0;
    (void) strcpy(s.chars, "GMT");
    tzname[0] = tzname[1] = s.chars;
#ifdef USG_COMPAT
    timezone = 0;
    daylight = 0;
#endif /* USG_COMPAT */
}

void
tzset()
{
    char * name;

    tz_is_set = true;
    name = getenv("TZ");
    if (!name || *name) {           /* did not request GMT */
        if (name && !tzload(name))  /* requested name worked */
            return;
        if (!tzload((char *)0))     /* default name worked */
            return;
        if (!tzsetkernel())     /* kernel guess worked */
            return;
    }
    tzsetgmt();             /* GMT is default */
}

struct tm *
localtime(timep)
time_t *    timep;
{
    struct ttinfo *    ttisp;
    struct tm *        tmp;
    int            i;
    time_t              t;

    if (!tz_is_set)
        (void) tzset();
    t = *timep;
    if (s.timecnt == 0 || t < s.ats[0]) {
        i = 0;
        while (s.ttis[i].tt_isdst)
            if (++i >= s.timecnt) {
                i = 0;
                break;
            }
    } else {
        for (i = 1; i < s.timecnt; ++i)
            if (t < s.ats[i])
                break;
        i = s.types[i - 1];
    }
    ttisp = &s.ttis[i];
    /*
    ** To get (wrong) behavior that's compatible with System V Release 2.0
    ** you'd replace the statement below with
    **  tmp = offtime((time_t) (t + ttisp->tt_gmtoff), 0L);
    */
    tmp = offtime(&t, ttisp->tt_gmtoff);
    tmp->tm_isdst = ttisp->tt_isdst;
    tzname[tmp->tm_isdst] = &s.chars[ttisp->tt_abbrind];
    tmp->tm_zone = &s.chars[ttisp->tt_abbrind];
    return tmp;
}
#endif

void
gmtime(struct tm * tm, time_t * clock)
{
    offtime(tm, clock, 0L);
    tzname[0] = "GMT";
    /* Could be useful? */
#if 0
    tm->tm_zone = "GMT";       /* UCT ? */
#endif
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
    /* TODO */
#if 0
    tm->tm_zone = "";
    tm->tm_gmtoff = offset;
#endif
}
