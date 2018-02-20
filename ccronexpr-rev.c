/*
 * Copyright 2015, alex at staticlibs.net
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* 
 * File:   CronExprParser.cpp
 * Author: alex
 * 
 * Created on February 24, 2015, 9:35 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <math.h>

#include "ccronexpr.h"
#include "ccronexpr-internal.h"

static unsigned int prev_set_bit(uint8_t* bits, int from_index, int to_index, int* notfound) {
    int i;
    if (!bits) {
        *notfound = 1;
        return 0;
    }
    for (i = from_index; i >= to_index; i--) {
        if (cron_get_bit(bits, i)) return i;
    }
    *notfound = 1;
    return 0;
}

static int last_day_of_month(int month, int year) {
    struct tm cal;
    time_t t;
    memset(&cal,0,sizeof(cal));
    cal.tm_sec=0;
    cal.tm_min=0;
    cal.tm_hour=0;
    cal.tm_mon = month+1;
    cal.tm_mday = 0;
    cal.tm_year=year;
    t=mktime(&cal);
    return gmtime(&t)->tm_mday;
}

/**
 * Reset the calendar setting all the fields provided to zero.
 */
static int reset_max(struct tm* calendar, int field) {
    if (!calendar || -1 == field) {
        return 1;
    }
    switch (field) {
    case CRON_CF_SECOND:
        calendar->tm_sec = 59;
        break;
    case CRON_CF_MINUTE:
        calendar->tm_min = 59;
        break;
    case CRON_CF_HOUR_OF_DAY:
        calendar->tm_hour = 23;
        break;
    case CRON_CF_DAY_OF_WEEK:
        calendar->tm_wday = 6;
        break;
    case CRON_CF_DAY_OF_MONTH:
        calendar->tm_mday = last_day_of_month(calendar->tm_mon, calendar->tm_year);
        break;
    case CRON_CF_MONTH:
        calendar->tm_mon = 11;
        break;
    case CRON_CF_YEAR:
        // I don't think this is supposed to happen ...
        fprintf(stderr, "reset CRON_CF_YEAR\n");
        break;
    default:
        return 1; /* unknown field */
    }
    time_t res = cron_mktime(calendar);
    if (CRON_INVALID_INSTANT == res) {
        return 1;
    }
    return 0;
}

static int reset_all_max(struct tm* calendar, int* fields) {
    int i;
    int res = 0;
    if (!calendar || !fields) {
        return 1;
    }
    for (i = 0; i < CRON_CF_ARR_LEN; i++) {
        if (-1 != fields[i]) {
            res = reset_max(calendar, fields[i]);
            if (0 != res) return res;
        }
    }
    return 0;
}

/**
 * Search the bits provided for the next set bit after the value provided,
 * and reset the calendar.
 */
static unsigned int find_prev(uint8_t* bits, unsigned int max, unsigned int value, struct tm* calendar, unsigned int field, unsigned int nextField, int* lower_orders, int* res_out) {
    int notfound = 0;
    int err = 0;
    unsigned int next_value = prev_set_bit(bits, value, 0, &notfound);
    /* roll under if needed */
    if (notfound) {
        err = add_to_field(calendar, nextField, -1);
        if (err) goto return_error;
        err = reset_max(calendar, field);
        if (err) goto return_error;
        notfound = 0;
        next_value = prev_set_bit(bits, max, value, &notfound);
    }
    if (notfound || next_value != value) {
        err = set_field(calendar, field, next_value);
        if (err) goto return_error;
        err = reset_all_max(calendar, lower_orders);
        if (err) goto return_error;
    }
    return next_value;

    return_error:
    *res_out = 1;
    return 0;
}

static unsigned int find_prev_day(struct tm* calendar, uint8_t* days_of_month, unsigned int day_of_month, uint8_t* days_of_week, unsigned int day_of_week, int* resets, int* res_out) {
    int err;
    unsigned int count = 0;
    unsigned int max = 366;
    while ((!cron_get_bit(days_of_month, day_of_month) || !cron_get_bit(days_of_week, day_of_week)) && count++ < max) {
        err = add_to_field(calendar, CRON_CF_DAY_OF_MONTH, -1);

        if (err) goto return_error;
        day_of_month = calendar->tm_mday;
        day_of_week = calendar->tm_wday;
        reset_all_max(calendar, resets);
    }
    return day_of_month;

    return_error:
    *res_out = 1;
    return 0;
}

static int do_prev(cron_expr* expr, struct tm* calendar, unsigned int dot) {
    int i;
    int res = 0;
    int* resets = NULL;
    int* empty_list = NULL;
    unsigned int second = 0;
    unsigned int update_second = 0;
    unsigned int minute = 0;
    unsigned int update_minute = 0;
    unsigned int hour = 0;
    unsigned int update_hour = 0;
    unsigned int day_of_week = 0;
    unsigned int day_of_month = 0;
    unsigned int update_day_of_month = 0;
    unsigned int month = 0;
    unsigned int update_month = 0;

    resets = (int*) cronMalloc(CRON_CF_ARR_LEN * sizeof(int));
    if (!resets) goto return_result;
    empty_list = (int*) cronMalloc(CRON_CF_ARR_LEN * sizeof(int));
    if (!empty_list) goto return_result;
    for (i = 0; i < CRON_CF_ARR_LEN; i++) {
        resets[i] = -1;
        empty_list[i] = -1;
    }

    second = calendar->tm_sec;
    update_second = find_prev(expr->seconds, CRON_MAX_SECONDS, second, calendar, CRON_CF_SECOND, CRON_CF_MINUTE, empty_list, &res);
    if (0 != res) goto return_result;
    if (second == update_second) {
        push_to_fields_arr(resets, CRON_CF_SECOND);
    }

    minute = calendar->tm_min;
    update_minute = find_prev(expr->minutes, CRON_MAX_MINUTES, minute, calendar, CRON_CF_MINUTE, CRON_CF_HOUR_OF_DAY, resets, &res);
    if (0 != res) goto return_result;
    if (minute == update_minute) {
        push_to_fields_arr(resets, CRON_CF_MINUTE);
    } else {
        res = do_prev(expr, calendar, dot);
        if (0 != res) goto return_result;
    }

    hour = calendar->tm_hour;
    update_hour = find_prev(expr->hours, CRON_MAX_HOURS, hour, calendar, CRON_CF_HOUR_OF_DAY, CRON_CF_DAY_OF_WEEK, resets, &res);
    if (0 != res) goto return_result;
    if (hour == update_hour) {
        push_to_fields_arr(resets, CRON_CF_HOUR_OF_DAY);
    } else {
        res = do_prev(expr, calendar, dot);
        if (0 != res) goto return_result;
    }

    day_of_week = calendar->tm_wday;
    day_of_month = calendar->tm_mday;
    update_day_of_month = find_prev_day(calendar, expr->days_of_month, day_of_month, expr->days_of_week, day_of_week, resets, &res);
    if (0 != res) goto return_result;
    if (day_of_month == update_day_of_month) {
        push_to_fields_arr(resets, CRON_CF_DAY_OF_MONTH);
    } else {
        res = do_prev(expr, calendar, dot);
        if (0 != res) goto return_result;
    }

    month = calendar->tm_mon; /*day already adds one if no day in same month is found*/
    update_month = find_prev(expr->months, CRON_MAX_MONTHS, month, calendar, CRON_CF_MONTH, CRON_CF_YEAR, resets, &res);
    if (0 != res) goto return_result;
    if (month != update_month) {
        if (dot - calendar->tm_year > CRON_MAX_YEARS_DIFF) {
            res = -1;
            goto return_result;
        }
        res = do_prev(expr, calendar, dot);
        if (0 != res) goto return_result;
    }
    goto return_result;

    return_result:
    if (!resets || !empty_list) {
        res = -1;
    }
    if (resets) {
        cronFree(resets);
    }
    if (empty_list) {
        cronFree(empty_list);
    }
    return res;
}

time_t cron_prev(cron_expr* expr, time_t date) {
    /*
     The plan:

     1 Round down to a whole second

     2 If seconds match move on, otherwise find the next match:
     2.1 If next match is in the next minute then roll forwards

     3 If minute matches move on, otherwise find the next match
     3.1 If next match is in the next hour then roll forwards
     3.2 Reset the seconds and go to 2

     4 If hour matches move on, otherwise find the next match
     4.1 If next match is in the next day then roll forwards,
     4.2 Reset the minutes and seconds and go to 2

     ...
     */
    if (!expr) return CRON_INVALID_INSTANT;
    struct tm calval;
    memset(&calval, 0, sizeof(struct tm));
    struct tm* calendar = cron_time(&date, &calval);
    if (!calendar) return CRON_INVALID_INSTANT;
    time_t original = cron_mktime(calendar);
    if (CRON_INVALID_INSTANT == original) return CRON_INVALID_INSTANT;

    // calculate the previous occurrence
    int res = do_prev(expr, calendar, calendar->tm_year);
    if (0 != res) return CRON_INVALID_INSTANT;

    // check for a match, try from the next second if one wasn't found
    time_t calculated = cron_mktime(calendar);
    if (CRON_INVALID_INSTANT == calculated) return CRON_INVALID_INSTANT;
    if (calculated == original) {
        /* We arrived at the original timestamp - round up to the next whole second and try again... */
        res = add_to_field(calendar, CRON_CF_SECOND, -1);
        if (0 != res) return CRON_INVALID_INSTANT;
        res = do_prev(expr, calendar, calendar->tm_year);
        if (0 != res) return CRON_INVALID_INSTANT;
    }

    return cron_mktime(calendar);
}


