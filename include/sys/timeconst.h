/*
 * 5.2.1 (2.11BSD) 1996/11/29
 */

/**
 * @addtogroup time
 * @{
 */

#ifndef TIMECONST_H
#define TIMECONST_H

#define SECS_PER_MIN    60
#define MINS_PER_HOUR   60
#define HOURS_PER_DAY   24
#define DAYS_PER_WEEK   7
#define DAYS_PER_NYEAR  365
#define DAYS_PER_LYEAR  366
#define SECS_PER_HOUR   (SECS_PER_MIN * MINS_PER_HOUR)
#define SECS_PER_DAY    ((long) SECS_PER_HOUR * HOURS_PER_DAY)
#define MONS_PER_YEAR   12

#define TM_SUNDAY       0
#define TM_MONDAY       1
#define TM_TUESDAY      2
#define TM_WEDNESDAY    3
#define TM_THURSDAY     4
#define TM_FRIDAY       5
#define TM_SATURDAY     6

#define TM_JANUARY      0
#define TM_FEBRUARY     1
#define TM_MARCH        2
#define TM_APRIL        3
#define TM_MAY          4
#define TM_JUNE         5
#define TM_JULY         6
#define TM_AUGUST       7
#define TM_SEPTEMBER    8
#define TM_OCTOBER      9
#define TM_NOVEMBER     10
#define TM_DECEMBER     11
#define TM_SUNDAY       0

#define TM_YEAR_BASE    1900

#define EPOCH_YEAR      1970
#define EPOCH_WDAY      TM_THURSDAY

/*
 * Accurate only for the past couple of centuries;
 * that will probably do.
 */
#define isleap(y) ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)

#endif /* TIMECONST_H */

/**
 * @}
 */
