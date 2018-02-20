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

static const char* DAYS_ARR[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
#define CRON_DAYS_ARR_LEN 7
static const char* MONTHS_ARR[] = { "FOO", "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
#define CRON_MONTHS_ARR_LEN 13

/* Defining 'cron_mktime' to use use UTC (default) or local time */
#ifndef CRON_USE_LOCAL_TIME

/* http://stackoverflow.com/a/22557778 */
#ifdef _WIN32
time_t cron_mktime(struct tm* tm) {
    return _mkgmtime(tm);
}
#else /* _WIN32 */
#ifndef ANDROID
/* can be hidden in time.h */
time_t timegm(struct tm* __tp);
#endif /* ANDROID */
time_t cron_mktime(struct tm* tm) {
#ifndef ANDROID
    return timegm(tm);
#else /* ANDROID */
    /* https://github.com/adobe/chromium/blob/cfe5bf0b51b1f6b9fe239c2a3c2f2364da9967d7/base/os_compat_android.cc#L20 */
    static const time_t kTimeMax = ~(1L << (sizeof (time_t) * CHAR_BIT - 1));
    static const time_t kTimeMin = (1L << (sizeof (time_t) * CHAR_BIT - 1));
    time64_t result = timegm64(tm);
    if (result < kTimeMin || result > kTimeMax) return -1;
    return result;
#endif /* ANDROID */
}
#endif /* _WIN32 */

struct tm* cron_time(time_t* date, struct tm* out) {
#ifdef __MINGW32__
    (void)(out); /* To avoid unused warning */
    return gmtime(date);
#else /* __MINGW32__ */
#ifdef _WIN32
    errno_t err = gmtime_s(out, date);
    return 0 == err ? out : NULL;
#else /* _WIN32 */
    return gmtime_r(date, out);
#endif /* _WIN32 */
#endif /* __MINGW32__ */
}

#else /* CRON_USE_LOCAL_TIME */

time_t cron_mktime(struct tm* tm) {
    return mktime(tm);
}

struct tm* cron_time(time_t* date, struct tm* out) {
#ifdef _WIN32
    errno_t err = localtime_s(out, date);
    return 0 == err ? out : NULL;
#else /* _WIN32 */    
    return localtime_r(date, out);
#endif /* _WIN32 */    
}

#endif /* CRON_USE_LOCAL_TIME */

void cron_set_bit(uint8_t* rbyte, int idx) {
    uint8_t j = (uint8_t) (idx / 8);
    uint8_t k = (uint8_t) (idx % 8);

    rbyte[j] |= (1 << k);
}

void cron_del_bit(uint8_t* rbyte, int idx) {
    uint8_t j = (uint8_t) (idx / 8);
    uint8_t k = (uint8_t) (idx % 8);

    rbyte[j] &= ~(1 << k);
}

uint8_t cron_get_bit(uint8_t* rbyte, int idx) {
    uint8_t j = (uint8_t) (idx / 8);
    uint8_t k = (uint8_t) (idx % 8);

    if (rbyte[j] & (1 << k)) {
        return 1;
    } else {
        return 0;
    }
}

void free_splitted(char** splitted, size_t len) {
    size_t i;
    if (!splitted) return;
    for (i = 0; i < len; i++) {
        if (splitted[i]) {
            cronFree(splitted[i]);
        }
    }
    cronFree(splitted);
}

char* strdupl(const char* str, size_t len) {
    if (!str) return NULL;
    char* res = (char*) cronMalloc(len + 1);
    if (!res) return NULL;
    memset(res, 0, len + 1);
    memcpy(res, str, len);
    return res;
}

void push_to_fields_arr(int* arr, int fi) {
    int i;
    if (!arr || -1 == fi) {
        return;
    }
    for (i = 0; i < CRON_CF_ARR_LEN; i++) {
        if (arr[i] == fi) return;
    }
    for (i = 0; i < CRON_CF_ARR_LEN; i++) {
        if (-1 == arr[i]) {
            arr[i] = fi;
            return;
        }
    }
}

int add_to_field(struct tm* calendar, int field, int val) {
    if (!calendar || -1 == field) {
        return 1;
    }
    switch (field) {
    case CRON_CF_SECOND:
        calendar->tm_sec = calendar->tm_sec + val;
        break;
    case CRON_CF_MINUTE:
        calendar->tm_min = calendar->tm_min + val;
        break;
    case CRON_CF_HOUR_OF_DAY:
        calendar->tm_hour = calendar->tm_hour + val;
        break;
    case CRON_CF_DAY_OF_WEEK: /* mkgmtime ignores this field */
    case CRON_CF_DAY_OF_MONTH:
        calendar->tm_mday = calendar->tm_mday + val;
        break;
    case CRON_CF_MONTH:
        calendar->tm_mon = calendar->tm_mon + val;
        break;
    case CRON_CF_YEAR:
        calendar->tm_year = calendar->tm_year + val;
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

int to_upper(char* str) {
    if (!str) return 1;
    int i;
    for (i = 0; '\0' != str[i]; i++) {
        str[i] = (char) toupper(str[i]);
    }
    return 0;
}

char* to_string(int num) {
    if (abs(num) >= CRON_MAX_NUM_TO_SRING) return NULL;
    char* str = (char*) cronMalloc(CRON_NUM_OF_DIGITS(num) + 1);
    if (!str) return NULL;
    int res = sprintf(str, "%d", num);
    if (res < 0) return NULL;
    return str;
}

char* str_replace(char *orig, const char *rep, const char *with) {
    char *result; /* the return string */
    char *ins; /* the next insert point */
    char *tmp; /* varies */
    size_t len_rep; /* length of rep */
    size_t len_with; /* length of with */
    size_t len_front; /* distance between rep and end of last rep */
    int count; /* number of replacements */
    if (!orig) return NULL;
    if (!rep) rep = "";
    if (!with) with = "";
    len_rep = strlen(rep);
    len_with = strlen(with);

    ins = orig;
    for (count = 0; NULL != (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    /* first time through the loop, all the variable are set correctly
     from here on,
     tmp points to the end of the result string
     ins points to the next occurrence of rep in orig
     orig points to the remainder of orig after "end of rep"
     */
    tmp = result = (char*) cronMalloc(strlen(orig) + (len_with - len_rep) * count + 1);
    if (!result) return NULL;

    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; /* move to next "end of rep" */
    }
    strcpy(tmp, orig);
    return result;
}

unsigned int parse_uint(const char* str, int* errcode) {
    char* endptr;
    errno = 0;
    long int l = strtol(str, &endptr, 0);
    if (errno == ERANGE || *endptr != '\0' || l < 0 || l > INT_MAX) {
        *errcode = 1;
        return 0;
    } else {
        *errcode = 0;
        return (unsigned int) l;
    }
}

char** split_str(const char* str, char del, size_t* len_out) {
    size_t i;
    size_t stlen = 0;
    size_t len = 0;
    int accum = 0;
    char* buf = NULL;
    char** res = NULL;
    size_t bi = 0;
    size_t ri = 0;
    char* tmp;

    if (!str) goto return_error;
    for (i = 0; '\0' != str[i]; i++) {
        stlen += 1;
        if (stlen >= CRON_MAX_STR_LEN_TO_SPLIT) goto return_error;
    }

    for (i = 0; i < stlen; i++) {
        if (del == str[i]) {
            if (accum > 0) {
                len += 1;
                accum = 0;
            }
        } else if (!isspace(str[i])) {
            accum += 1;
        }
    }
    /* tail */
    if (accum > 0) {
        len += 1;
    }
    if (0 == len) return NULL;

    buf = (char*) cronMalloc(stlen + 1);
    if (!buf) goto return_error;
    memset(buf, 0, stlen + 1);
    res = (char**) cronMalloc(len * sizeof(char*));
    if (!res) goto return_error;
    memset(res, 0, len * sizeof(char*));

    for (i = 0; i < stlen; i++) {
        if (del == str[i]) {
            if (bi > 0) {
                tmp = strdupl(buf, bi);
                if (!tmp) goto return_error;
                res[ri++] = tmp;
                memset(buf, 0, stlen + 1);
                bi = 0;
            }
        } else if (!isspace(str[i])) {
            buf[bi++] = str[i];
        }
    }
    /* tail */
    if (bi > 0) {
        tmp = strdupl(buf, bi);
        if (!tmp) goto return_error;
        res[ri++] = tmp;
    }
    cronFree(buf);
    *len_out = len;
    return res;

    return_error:
    if (buf) {
        cronFree(buf);
    }
    free_splitted(res, len);
    *len_out = 0;
    return NULL;
}

char* replace_ordinals(char* value, const char** arr, size_t arr_len) {
    size_t i;
    char* cur = value;
    char* res = NULL;
    int first = 1;
    for (i = 0; i < arr_len; i++) {
        char* strnum = to_string((int) i);
        if (!strnum) {
            if (!first) {
                cronFree(cur);
            }
            return NULL;
        }
        res = str_replace(cur, arr[i], strnum);
        cronFree(strnum);
        if (!first) {
            cronFree(cur);
        }
        if (!res) {
            return NULL;
        }
        cur = res;
        if (first) {
            first = 0;
        }
    }
    return res;
}

int has_char(char* str, char ch) {
    size_t i;
    size_t len = 0;
    if (!str) return 0;
    len = strlen(str);
    for (i = 0; i < len; i++) {
        if (str[i] == ch) return 1;
    }
    return 0;
}

unsigned int* get_range(char* field, unsigned int min, unsigned int max, const char** error) {

    char** parts = NULL;
    size_t len = 0;
    unsigned int* res = (unsigned int*) cronMalloc(2 * sizeof(unsigned int));
    if (!res) goto return_error;

    res[0] = 0;
    res[1] = 0;
    if (1 == strlen(field) && '*' == field[0]) {
        res[0] = min;
        res[1] = max - 1;
    } else if (!has_char(field, '-')) {
        int err = 0;
        unsigned int val = parse_uint(field, &err);
        if (err) {
            *error = "Unsigned integer parse error 1";
            goto return_error;
        }

        res[0] = val;
        res[1] = val;
    } else {
        parts = split_str(field, '-', &len);
        if (2 != len) {
            *error = "Specified range requires two fields";
            goto return_error;
        }
        int err = 0;
        res[0] = parse_uint(parts[0], &err);
        if (err) {
            *error = "Unsigned integer parse error 2";
            goto return_error;
        }
        res[1] = parse_uint(parts[1], &err);
        if (err) {
            *error = "Unsigned integer parse error 3";
            goto return_error;
        }
    }
    if (res[0] >= max || res[1] >= max) {
        *error = "Specified range exceeds maximum";
        goto return_error;
    }
    if (res[0] < min || res[1] < min) {
        *error = "Specified range is less than minimum";
        goto return_error;
    }
    if (res[0] > res[1]) {
        *error = "Specified range start exceeds range end";
        goto return_error;
    }

    free_splitted(parts, len);
    *error = NULL;
    return res;

    return_error:
    free_splitted(parts, len);
    if (res) {
        cronFree(res);
    }

    return NULL;
}

void set_number_hits(const char* value, uint8_t* target, unsigned int min, unsigned int max, const char** error) {
    size_t i;
    unsigned int i1;
    size_t len = 0;

    char** fields = split_str(value, ',', &len);
    if (!fields) {
        *error = "Comma split error";
        goto return_result;
    }

    for (i = 0; i < len; i++) {
        if (!has_char(fields[i], '/')) {
            /* Not an incrementer so it must be a range (possibly empty) */

            unsigned int* range = get_range(fields[i], min, max, error);

            if (*error) {
                if (range) {
                    cronFree(range);
                }
                goto return_result;

            }

            for (i1 = range[0]; i1 <= range[1]; i1++) {
                cron_set_bit(target, i1);

            }
            cronFree(range);

        } else {
            size_t len2 = 0;
            char** split = split_str(fields[i], '/', &len2);
            if (2 != len2) {
                *error = "Incrementer must have two fields";
                free_splitted(split, len2);
                goto return_result;
            }
            unsigned int* range = get_range(split[0], min, max, error);
            if (*error) {
                if (range) {
                    cronFree(range);
                }
                free_splitted(split, len2);
                goto return_result;
            }
            if (!has_char(split[0], '-')) {
                range[1] = max - 1;
            }
            int err = 0;
            unsigned int delta = parse_uint(split[1], &err);
            if (err) {
                *error = "Unsigned integer parse error 4";
                cronFree(range);
                free_splitted(split, len2);
                goto return_result;
            }
            if (0 == delta) {
                *error = "Incrementer may not be zero";
                cronFree(range);
                free_splitted(split, len2);
                goto return_result;
            }
            for (i1 = range[0]; i1 <= range[1]; i1 += delta) {
                cron_set_bit(target, i1);
            }
            free_splitted(split, len2);
            cronFree(range);

        }
    }
    goto return_result;

    return_result:
    free_splitted(fields, len);

}

void set_months(char* value, uint8_t* targ, const char** error) {
    int err;
    unsigned int i;
    unsigned int max = 12;

    char* replaced = NULL;

    err = to_upper(value);
    if (err) return;
    replaced = replace_ordinals(value, MONTHS_ARR, CRON_MONTHS_ARR_LEN);
    if (!replaced) return;

    set_number_hits(replaced, targ, 1, max + 1, error);
    cronFree(replaced);

    /* ... and then rotate it to the front of the months */
    for (i = 1; i <= max; i++) {
        if (cron_get_bit(targ, i)) {
            cron_set_bit(targ, i - 1);
            cron_del_bit(targ, i);
        }
    }
}

void set_days(char* field, uint8_t* targ, int max, const char** error) {
    if (1 == strlen(field) && '?' == field[0]) {
        field[0] = '*';
    }
    set_number_hits(field, targ, 0, max, error);
}

void set_days_of_month(char* field, uint8_t* targ, const char** error) {
    /* Days of month start with 1 (in Cron and Calendar) so add one */
    if (1 == strlen(field) && '?' == field[0]) {
        field[0] = '*';
    }
    set_number_hits(field, targ, 1, CRON_MAX_DAYS_OF_MONTH, error);
}

void cron_parse_expr(const char* expression, cron_expr* target, const char** error) {
    const char* err_local;
    size_t len = 0;
    char** fields = NULL;
    char* days_replaced = NULL;
    if (!error) {
        error = &err_local;
    }
    *error = NULL;
    if (!expression) {
        *error = "Invalid NULL expression";
        goto return_res;
    }

    fields = split_str(expression, ' ', &len);
    if (len != 6) {
        *error = "Invalid number of fields, expression must consist of 6 fields";
        goto return_res;
    }
    set_number_hits(fields[0], target->seconds, 0, 60, error);
    if (*error) goto return_res;
    set_number_hits(fields[1], target->minutes, 0, 60, error);
    if (*error) goto return_res;
    set_number_hits(fields[2], target->hours, 0, 24, error);
    if (*error) goto return_res;
    to_upper(fields[5]);
    days_replaced = replace_ordinals(fields[5], DAYS_ARR, CRON_DAYS_ARR_LEN);
    set_days(days_replaced, target->days_of_week, 8, error);
    cronFree(days_replaced);
    if (*error) goto return_res;
    if (cron_get_bit(target->days_of_week, 7)) {
        /* Sunday can be represented as 0 or 7*/
        cron_set_bit(target->days_of_week, 0);
        cron_del_bit(target->days_of_week, 7);
    }
    set_days_of_month(fields[3], target->days_of_month, error);
    if (*error) goto return_res;
    set_months(fields[4], target->months, error);
    if (*error) goto return_res;

    goto return_res;

    return_res: 
    free_splitted(fields, len);
}

int set_field(struct tm* calendar, int field, int val) {
    if (!calendar || -1 == field) {
        return 1;
    }
    switch (field) {
    case CRON_CF_SECOND:
        calendar->tm_sec = val;
        break;
    case CRON_CF_MINUTE:
        calendar->tm_min = val;
        break;
    case CRON_CF_HOUR_OF_DAY:
        calendar->tm_hour = val;
        break;
    case CRON_CF_DAY_OF_WEEK:
        calendar->tm_wday = val;
        break;
    case CRON_CF_DAY_OF_MONTH:
        calendar->tm_mday = val;
        break;
    case CRON_CF_MONTH:
        calendar->tm_mon = val;
        break;
    case CRON_CF_YEAR:
        calendar->tm_year = val;
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

