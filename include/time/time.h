#pragma once

#include <stdint.h>

#define SECS_PER_HOUR (60 * 60)
#define SECS_PER_DAY (SECS_PER_HOUR * 24)

#define MSEC_PER_SEC    1000L
#define USEC_PER_SEC    1000000L
#define USEC_PER_MSEC   1000L
#define NSEC_PER_SEC    1000000000L
#define NSEC_PER_MSEC   1000000L
#define NSEC_PER_USEC   1000L

#define SEC_TO_MS(_s) ((_s) * MSEC_PER_SEC)
#define SEC_TO_US(_s) ((_s) * USEC_PER_SEC)
#define SEC_TO_NS(_s) ((_s) * NSEC_PER_SEC)

#define MS_TO_SEC(_ms) ((_ms) / MSEC_PER_SEC)
#define MS_TO_US(_ms) ((_ms) * USEC_PER_MSEC)
#define MS_TO_NS(_ms) ((_ms) * NSEC_PER_MSEC)

#define US_TO_MS(_us) ((_us) / USEC_PER_MSEC)
#define US_TO_NS(_us) ((_us) * NSEC_PER_USEC)

#define NS_TO_SEC(_ns) ((_ns) / NSEC_PER_SEC)
#define NS_TO_MS(_ns) ((_ns) / NSEC_PER_MSEC)
#define NS_TO_US(_ns) ((_ns) / NSEC_PER_USEC)


typedef struct {
    uint64_t secs;
    uint32_t ns;
} time_t;

// Imported from linux source tree
typedef struct {
    /*
     * the number of seconds after the minute, normally in the range
     * 0 to 59, but can be up to 60 to allow for leap seconds
     */
    int sec;
    /* the number of minutes after the hour, in the range 0 to 59*/
    int min;
    /* the number of hours past midnight, in the range 0 to 23 */
    int hour;
    /* the day of the month, in the range 1 to 31 */
    int mday;
    /* the number of months since January, in the range 0 to 11 */
    int mon;
    /* the number of years since 1900 */
    long year;
    /* the number of days since Sunday, in the range 0 to 6 */
    int wday;
    /* the number of days since January 1, in the range 0 to 365 */
    int yday;
} calendar_t;

int rtc_gettime(time_t *t);

/**
 * Converts the calendar time to local broken-down time
 *
 * Note: This function was imported from Linux and adapted to laritos
 *
 * @param secs: the number of seconds elapsed since 00:00:00 on January 1, 1970,
 *              Coordinated Universal Time (UTC).
 * @return: 0 on success, <0 on error
 */
int epoch_to_calendar(const uint64_t secs, calendar_t *c);