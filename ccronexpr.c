/* 
 * File:   CronExprParser.cpp
 * Author: alex
 * 
 * Created on February 24, 2015, 9:35 AM
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>

#include "ccronexpr.h"

const char* DAYS_ARR[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
#define DAYS_ARR_LEN 7
const char* MONTHS_ARR[] = {"FOO", "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
#define MONTHS_ARR_LEN 13

void cron_expr_free(cron_expr* expr) {
    if(!expr) return;
    if (expr->seconds) {
        free(expr->seconds);
    }
    if (expr->minutes) {
        free(expr->minutes);
    }
    if (expr->hours) {
        free(expr->hours);
    }
    if (expr->days_of_week) {
        free(expr->days_of_week);
    }
    if (expr->days_of_month) {
        free(expr->days_of_month);
    }
    if (expr->months) {
        free(expr->months);
    }
    free(expr);
}

void free_splitted(char** splitted, size_t len) {
    if(!splitted) return;
    for(size_t i = 0; i < len; i++) {
        if (splitted[i]) {
            free(splitted[i]);
        }
    }  
    free(splitted);
}

char* strdup(const char *s) {
    char *d = (char*) malloc(strlen(s) + 1); // Space for length plus nul
    if (d == NULL) return NULL; // No memory
    strcpy(d, s); // Copy the characters
    return d; // Return the new string
}

// http://stackoverflow.com/a/22557778    
static time_t mkgmtime(struct tm* tm) {
#if defined(_WIN32)
    return _mkgmtime(tm);
#else
    return timegm(tm);
#endif
}

static int next_set_bit(char* bits, int max, unsigned int from_index) {
    for (int i = from_index; i < max; i++) {
        if (bits[i]) return i;
    }
    return -1;
}

static void push_to_fields_arr(int* arr, int fi) {
    for (int i = 0; i < CF_ARR_LEN; i++) {
        if (arr[i] == fi) return;
    }
    for (int i = 0; i < CF_ARR_LEN; i++) {
        if (-1 == arr[i]) {
            arr[i] = fi;
            return;
        }
    }
}

static void add_to_field(struct tm* calendar, int field, int val) {
    switch (field) {
    case CF_SECOND: calendar->tm_sec = calendar->tm_sec + val; break;
    case CF_MINUTE: calendar->tm_min = calendar->tm_min + val; break;
    case CF_HOUR_OF_DAY: calendar->tm_hour = calendar->tm_hour + val; break;
    case CF_DAY_OF_WEEK: // mkgmtime ignores this field
    case CF_DAY_OF_MONTH: calendar->tm_mday = calendar->tm_mday + val; break;
    case CF_MONTH: calendar->tm_mon = calendar->tm_mon + val; break;
    case CF_YEAR: calendar->tm_year = calendar->tm_year + val; break;
    }
    mkgmtime(calendar);
}

/**
 * Reset the calendar setting all the fields provided to zero.
 */
static void reset(struct tm* calendar, int field) {
    switch (field) {
    case CF_SECOND: calendar->tm_sec = 0; break;
    case CF_MINUTE: calendar->tm_min = 0; break;
    case CF_HOUR_OF_DAY: calendar->tm_hour = 0; break;
    case CF_DAY_OF_WEEK: calendar->tm_wday = 0; break;
    case CF_DAY_OF_MONTH: calendar->tm_mday = 1; break;
    case CF_MONTH: calendar->tm_mon = 0; break;
    case CF_YEAR: calendar->tm_year = 0; break;
    }
    mkgmtime(calendar);
}

static void reset_all(struct tm* calendar, int* fields) {
    for (int i = 0; i < CF_ARR_LEN; i++) {
        if (-1 != fields[i]) {
            reset(calendar, fields[i]);
        }
    }
}

static void set_field(struct tm* calendar, int field, int val) {
    switch (field) {
    case CF_SECOND: calendar->tm_sec = val; break;
    case CF_MINUTE: calendar->tm_min = val; break;
    case CF_HOUR_OF_DAY: calendar->tm_hour = val; break;
    case CF_DAY_OF_WEEK: calendar->tm_wday = val; break;
    case CF_DAY_OF_MONTH: calendar->tm_mday = val; break;
    case CF_MONTH: calendar->tm_mon = val; break;
    case CF_YEAR: calendar->tm_year = val; break;
    }
    mkgmtime(calendar);
}


/**
 * Search the bits provided for the next set bit after the value provided,
 * and reset the calendar.
 * @param bits a {@link charean[]} representing the allowed values of the field
 * @param value the current value of the field
 * @param calendar the calendar to increment as we move through the bits
 * @param field the field to increment in the calendar (@see
 * {@link Calendar} for the static constants defining valid fields)
 * @param lowerOrders the Calendar field ids that should be reset (i.e. the
 * ones of lower significance than the field of interest)
 * @return the value of the calendar field that is next in the sequence
 */
static unsigned int find_next(char* bits, int max, unsigned int value, struct tm* calendar, int field,
        int nextField, int* lower_orders) {
    int next_value = next_set_bit(bits, max, value);
    // roll over if needed
    if (next_value == -1) {
        add_to_field(calendar, nextField, 1);
        reset(calendar, field);
        next_value = next_set_bit(bits, max, 0);
    }
    if (-1 == next_value || (unsigned int) (next_value) != value) {
        set_field(calendar, field, next_value);
        reset_all(calendar, lower_orders);
    }
    return next_value;
}

static unsigned int find_next_day(struct tm* calendar, char* days_of_month,
        unsigned int day_of_month, char* days_of_week, unsigned int day_of_week,
        int* resets) {
    unsigned int count = 0;
    unsigned int max = 366;
    while ((!days_of_month[day_of_month] || !days_of_week[day_of_week]) && count++ < max) {
        add_to_field(calendar, CF_DAY_OF_MONTH, 1);
        day_of_month = calendar->tm_mday;
        day_of_week = calendar->tm_wday;
        reset_all(calendar, resets);
    }
    // todo: check if needed
//    if (count >= max) {
//        throw "Overflow in day for expression \"this.expression \"";
//    }
    return day_of_month;
}

static int do_next(cron_expr* expr, struct tm* calendar, unsigned int dot) {
    int res = 0;
    int* resets = (int*) malloc(CF_ARR_LEN * sizeof (int));
    int* empty_list = (int*) malloc(CF_ARR_LEN * sizeof (int));
    for (int i = 0; i < CF_ARR_LEN; i++) {
        resets[i] = -1;
        empty_list[i] = -1;
    }

    unsigned int second = calendar->tm_sec;
    unsigned int update_second = find_next(expr->seconds, MAX_SECONDS, second, calendar, CF_SECOND, CF_MINUTE, empty_list);
    if (second == update_second) {
        push_to_fields_arr(resets, CF_SECOND);
    }

    unsigned int minute = calendar->tm_min;
    unsigned int update_minute = find_next(expr->minutes, MAX_MINUTES, minute, calendar, CF_MINUTE, CF_HOUR_OF_DAY, resets);
    if (minute == update_minute) {
        push_to_fields_arr(resets, CF_MINUTE);
    } else {
        res = do_next(expr, calendar, dot);
        if (res < 0) goto return_result;
    }

    unsigned int hour = calendar->tm_hour;
    unsigned int update_hour = find_next(expr->hours, MAX_HOURS, hour, calendar, CF_HOUR_OF_DAY, CF_DAY_OF_WEEK, resets);
    if (hour == update_hour) {
        push_to_fields_arr(resets, CF_HOUR_OF_DAY);
    } else {
        res = do_next(expr, calendar, dot);
        if (res < 0) goto return_result;
    }

    unsigned int day_of_week = calendar->tm_wday;
    unsigned int day_of_month = calendar->tm_mday;
    unsigned int update_day_of_month = find_next_day(calendar, expr->days_of_month, day_of_month, expr->days_of_week, day_of_week, resets);
    if (day_of_month == update_day_of_month) {
        push_to_fields_arr(resets, CF_DAY_OF_MONTH);
    } else {
        res = do_next(expr, calendar, dot);
        if (res < 0) goto return_result;
    }

    unsigned int month = calendar->tm_mon;
    unsigned int update_month = find_next(expr->months, MAX_MONTHS, month, calendar, CF_MONTH, CF_YEAR, resets);
    if (month != update_month) {
        if (calendar->tm_year - dot > 4) {
            res = -1;
            goto return_result;
        }
        res = do_next(expr, calendar, dot);
        if (res < 0) goto return_result;
    }
    goto return_result;
    
    return_result:
        free(resets);
        free(empty_list);
        return res;
}

void to_upper(char* str) {
    for (int i = 0; '\0' != str[i]; i++) {
        str[i] = toupper(str[i]);
    }
}

// You must free the result if result is non-NULL.
char* str_replace(char *orig, const char *rep, const char *with) {
    char *result; // the return string
    char *ins; // the next insert point
    char *tmp; // varies
    int len_rep; // length of rep
    int len_with; // length of with
    int len_front; // distance between rep and end of last rep
    int count; // number of replacements

//    if (!orig)
//        return NULL;
//    if (!rep)
//        rep = "";
    len_rep = strlen(rep);
//    if (!with)
//        with = "";
    len_with = strlen(with);

    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    tmp = result = (char*) malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

char* to_string(int num) {
//    int len = (int) ((ceil(log10(num)) + 2) * sizeof (char));
    // todo
    char* str = (char*) malloc(10);
    sprintf(str, "%d", num);
    return str;
}

// workaround for android
unsigned int parse_uint32(const char* str, int* errcode) {
    char* endptr;
    errno = 0;
    long int l = strtol(str, &endptr, 0);
    if (errno == ERANGE || *endptr != '\0' || l < 0 || l > UINT_MAX) {
        *errcode = 1;
        return 0;
    } else {
        *errcode = 0;
        return (unsigned int) l;
    }
}

char** split_str(const char* str, char del, size_t* len_out) {
    size_t stlen = strlen(str);
    int accum = 0;
    size_t len = 0;
    for (size_t i = 0; i < stlen; i++) {
        if (del == str[i]) {
            if (accum > 0) {
                len += 1;
                accum = 0;
            }
        } else if (!isspace(str[i])) {
            accum += 1;
        }
    }
    // tail
    if (accum > 0) {
        len += 1;
    }

    char* buf = (char*) malloc(stlen + 1);
    memset(buf, 0, stlen + 1);
    char** res = (char**) malloc(len * sizeof(char*));
    size_t bi = 0;
    size_t ri = 0;
    for (size_t i = 0; i < stlen; i++) {
        if (del == str[i]) {
            if (bi > 0) {
                res[ri++] = strdup(buf);
                memset(buf, 0, stlen + 1);
                bi = 0;
            }
        } else if (!isspace(str[i])) {
            buf[bi++] = str[i];            
        }
    }
    // tail
    if (bi > 0) {
        res[ri++] = strdup(buf);
    }
    free(buf);
    *len_out = len;
    return res;
}

char* replace_ordinals(char* value, const char** arr, size_t arr_len) {
    char* cur = value;
    char* res = NULL;
    int first = 1;
    for (size_t i = 0; i < arr_len; i++) {
        char* strnum = to_string(i);
        // todo: check strnum and res
        res = str_replace(cur, arr[i], strnum);
        free(strnum);
        if (!first) {
            free(cur);
        }        
        cur = res;
        if (first) {
            first = 0;
        }
    }
    return res;
}

int has_char(char* str, char ch) {
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
        if (str[i] == ch) return 1;
    }
    return 0;
}

static unsigned int* get_range(char* field, unsigned int min, unsigned int max, const char** error) {
    unsigned int* res = (unsigned int*) malloc(2*sizeof (unsigned int));
    res[0] = 0;
    res[1] = 0;
    if (1 == strlen(field) && '*' == field[0]) {
        res[0] = min;
        res[1] = max - 1;
    } else if (!has_char(field, '-')) {
        int err = 0;
        unsigned int val = parse_uint32(field, &err);
        if (err) {
            *error = "Unsigned integer parse error 1";
            return res;
        }
        res[0] = val;
        res[1] = val;
    } else {
        size_t len = 0;
        char** parts = split_str(field, '-', &len);
        if (len > 2) {
            *error = "Specified range has more than two fields";
            free_splitted(parts, len);
            return res;
        }
        int err = 0;
        res[0] = parse_uint32(parts[0], &err);
        if (err) {
            *error = "Unsigned integer parse error 2";
            free_splitted(parts, len);
            return res;
        }
        res[1] = parse_uint32(parts[1], &err);
        if (err) {
            *error = "Unsigned integer parse error 3";
            free_splitted(parts, len);
            return res;
        }
        free_splitted(parts, len);
    }
    if (res[0] >= max || res[1] >= max) {
        *error = "Specified range exceeds maximum";
        return res;
    }
    if (res[0] < min || res[1] < min) {
        *error = "Specified range is less than maximum";
        return res;
    }
    *error = NULL;
    return res;
}

static char* set_number_hits(char* value, unsigned int min, unsigned int max, const char** error) {
    char* bits = (char*) malloc(max);
    memset(bits, 0, max);
    size_t len = 0;
    char** fields = split_str(value, ',', &len);
    for (size_t i = 0; i < len; i++) {
        if (!has_char(fields[i], '/')) {
            // Not an incrementer so it must be a range (possibly empty)
            unsigned int* range = get_range(fields[i], min, max, error);
            if (*error) {
                if (range) {
                    free(range);
                }
                goto return_result;
            }
            for (unsigned int i = range[0]; i <= range[1]; i++) {
                bits[i] = 1;
            }
            free(range);
        } else {
            size_t len2 = 0;
            char** split = split_str(fields[i], '/', &len2);
            if (len2 > 2) {
                *error = "Incrementer has more than two fields";
                free_splitted(split, len2);
                goto return_result;
            }
            unsigned int* range = get_range(split[0], min, max, error);
            if (*error) {
                free(range);
                free_splitted(split, len2);
                goto return_result;
            }
            if (!has_char(split[0], '-')) {
                range[1] = max - 1;
            }
            int err = 0;
            unsigned int delta = parse_uint32(split[1], &err);
            if (err) {
                *error = "Unsigned integer parse error 4";
                free(range);
                free_splitted(split, len2);
                goto return_result;
            }
            for (unsigned int i = range[0]; i <= range[1]; i += delta) {
                bits[i] = 1;
            }
            free_splitted(split, len2);
            free(range);
        }
    }
    goto return_result;
    
    return_result:
        free_splitted(fields, len);
        return bits;
}

char* set_months(char* value, const char** error) {
    unsigned int max = 12;
    char* bits = (char*) malloc(MAX_MONTHS);
    memset(bits, 0, MAX_MONTHS);
    to_upper(value);
    char* replaced = replace_ordinals(value, MONTHS_ARR, MONTHS_ARR_LEN);
    // Months start with 1 in Cron and 0 in Calendar, so push the values first into a longer bit set
    char* months = set_number_hits(replaced, 1, max + 1, error);
    free(replaced);
    if (*error) {
        if (months) {
            free(months);
        }
        return bits;
    }
    // ... and then rotate it to the front of the months
    for (unsigned int i = 1; i <= max; i++) {
        if (months[i]) {
            bits[i - 1] = 1;
        }
    }
    free(months);
    return bits;
}

char* set_days(char* field, int max, const char** error) {
    if (1 == strlen(field) && '?' == field[0]) {
        field[0] = '*';
    }
    return set_number_hits(field, 0, max, error);
}

char* set_days_of_month(char* field, const char** error) {
    // Days of month start with 1 (in Cron and Calendar) so add one
    char* bits = set_days(field, MAX_DAYS_OF_MONTH, error);
    // ... and remove it from the front
    bits[0] = 0;
    return bits;
}

cron_expr* cron_parse_expr(const char* expression, const char** error) {
    const char* err_local;
    if (!error) {
        error = &err_local;
    }
    *error = NULL;
    char* seconds = NULL;
    char* minutes = NULL;
    char* hours = NULL;
    char* days_of_week = NULL;
    char* days_of_month = NULL;
    char* months = NULL;
    size_t len = 0;
    char** fields = split_str(expression, ' ', &len);
    if (len != 6) {
        *error = "Invalid number of fields, expression must consist of 6 fields";
        goto return_res;
    }
    seconds = set_number_hits(fields[0], 0, 60, error);
    if (*error) goto return_res;
    minutes = set_number_hits(fields[1], 0, 60, error);
    if (*error) goto return_res;
    hours = set_number_hits(fields[2], 0, 24, error);
    if (*error) goto return_res;
    to_upper(fields[5]);
    char* days_replaced = replace_ordinals(fields[5], DAYS_ARR, DAYS_ARR_LEN);
    days_of_week = set_days(days_replaced, 8, error);
    free(days_replaced);
    if (*error) goto return_res;
    if (days_of_week[7]) {
        // Sunday can be represented as 0 or 7
        days_of_week[0] = 1;
        days_of_week[7] = 0;
    }
    days_of_month = set_days_of_month(fields[3], error);
    if (*error) goto return_res;
    months = set_months(fields[4], error);
    if (*error) goto return_res;

    goto return_res;
    
    return_res: 
    free_splitted(fields, len);
    if(*error) {
        if(seconds) free(seconds);
        if(minutes) free(minutes);
        if(hours) free(hours);
        if(days_of_week) free(days_of_week);
        if(days_of_month) free(days_of_month);
        if(months) free(months);
        return NULL;
    }
    cron_expr* res = (cron_expr*) malloc(sizeof (cron_expr));
    res->seconds = seconds;
    res->minutes = minutes;
    res->hours = hours;
    res->days_of_week = days_of_week;
    res->days_of_month = days_of_month;
    res->months = months;
    return res;

}

// todo
time_t cron_next_local(cron_expr* expr, time_t date) {
    return cron_next(expr, date);
}

time_t cron_next(cron_expr* expr, time_t date) {
    /*
    The plan:

    1 Round up to the next whole second

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
    struct tm* calendar = gmtime(&date);
    time_t original = mkgmtime(calendar);

    int res = do_next(expr, calendar, calendar->tm_year);
    if (res < 0) return INVALID_INSTANT;

    if (mkgmtime(calendar) == original) {
        // We arrived at the original timestamp - round up to the next whole second and try again...
        add_to_field(calendar, CF_SECOND, 1);
        int res = do_next(expr, calendar, calendar->tm_year);
        if (res < 0) return INVALID_INSTANT;
    }

    return mkgmtime(calendar);
}


















