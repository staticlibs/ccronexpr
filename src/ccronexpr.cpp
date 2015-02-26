/* 
 * File:   CronExprParser.cpp
 * Author: alex
 * 
 * Created on February 24, 2015, 9:35 AM
 */

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstring>
#include <cmath>
#include <cassert>
#include <cstdio>

#include <exception>

#include "staticlib/cron/ccronexpr.hpp"

const char* DAYS_ARR[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
#define DAYS_ARR_LEN 7
const char* MONTHS_ARR[] = {"FOO", "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
#define MONTHS_ARR_LEN 13

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

static void add_to_field(tm* calendar, int field, int val) {
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
static void reset(tm* calendar, int field) {
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

static void reset(tm* calendar, int* fields) {
    for (int i = 0; i < CF_ARR_LEN; i++) {
        if (-1 != fields[i]) {
            reset(calendar, fields[i]);
        }
    }
}

static void set_field(tm* calendar, int field, int val) {
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
static unsigned int find_next(char* bits, int max, unsigned int value, tm* calendar, int field,
        int nextField, int* lower_orders) {
    int next_value = next_set_bit(bits, max, value);
    // roll over if needed
    if (next_value == -1) {
        add_to_field(calendar, nextField, 1);
        reset(calendar, field);
        next_value = next_set_bit(bits, max, 0);
    }
    if (-1 == next_value || static_cast<unsigned int> (next_value) != value) {
        set_field(calendar, field, next_value);
        reset(calendar, lower_orders);
    }
    return next_value;
}

static unsigned int find_next_day(tm* calendar, char* days_of_month,
        unsigned int day_of_month, char* days_of_week, unsigned int day_of_week,
        int* resets) {
    unsigned int count = 0;
    unsigned int max = 366;
    while ((!days_of_month[day_of_month] || !days_of_week[day_of_week]) && count++ < max) {
        add_to_field(calendar, CF_DAY_OF_MONTH, 1);
        day_of_month = calendar->tm_mday;
        day_of_week = calendar->tm_wday;
        reset(calendar, resets);
    }
    if (count >= max) {
        // todo
        throw "Overflow in day for expression \"this.expression \"";
    }
    return day_of_month;
}

static int do_next(cron_expr* expr, tm* calendar, unsigned int dot) {
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
        int res = do_next(expr, calendar, dot);
        if (res < 0) return res;
    }

    unsigned int hour = calendar->tm_hour;
    unsigned int update_hour = find_next(expr->hours, MAX_HOURS, hour, calendar, CF_HOUR_OF_DAY, CF_DAY_OF_WEEK, resets);
    if (hour == update_hour) {
        push_to_fields_arr(resets, CF_HOUR_OF_DAY);
    } else {
        int res = do_next(expr, calendar, dot);
        if (res < 0) return res;
    }

    unsigned int day_of_week = calendar->tm_wday;
    unsigned int day_of_month = calendar->tm_mday;
    unsigned int update_day_of_month = find_next_day(calendar, expr->days_of_month, day_of_month, expr->days_of_week, day_of_week, resets);
    if (day_of_month == update_day_of_month) {
        push_to_fields_arr(resets, CF_DAY_OF_MONTH);
    } else {
        int res = do_next(expr, calendar, dot);
        if (res < 0) return res;
    }

    unsigned int month = calendar->tm_mon;
    unsigned int update_month = find_next(expr->months, MAX_MONTHS, month, calendar, CF_MONTH, CF_YEAR, resets);
    if (month != update_month) {
        if (calendar->tm_year - dot > 4) {
            return -1;
            //                throw std::exception();
            //                throw "Invalid cron expression \" this.expression\" led to runaway search for next trigger";
        }
        int res = do_next(expr, calendar, dot);
        if (res < 0) return res;
    }
    return 0;
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
unsigned int parse_uint32(const char* str) {
    char* endptr;
    errno = 0;
    auto l = strtoll(str, &endptr, 0);
    if (errno == ERANGE || *endptr != '\0') {
        throw std::exception();//(std::string("Cannot parse unsigned int from string:[]"));
    }
    if (l < 0 || l > UINT_MAX) {
        throw std::exception();//(std::string("Value overflow for unsigned int from string:[]"));
    }
    return (unsigned int) l;
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
        } else if (!std::isspace(str[i])) {
            buf[bi++] = str[i];            
        }
    }
    // tail
    if (bi > 0) {
        res[ri++] = strdup(buf);
    }
    *len_out = len;
    return res;
}

char* replace_ordinals(char* value, const char** arr, size_t arr_len) {
    auto res = value;
    for (size_t i = 0; i < arr_len; i++) {
        res = str_replace(res, arr[i], to_string(i));
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

unsigned int* get_range(char* field, unsigned int min, unsigned int max) {
    unsigned int* res = (unsigned int*) malloc(2*sizeof (unsigned int));
    if (1 == strlen(field) && '*' == field[0]) {
        res[0] = min;
        res[1] = max - 1;
    } else if (!has_char(field, '-')) {
        auto val = parse_uint32(field);
        res[0] = val;
        res[1] = val;
    } else {
        size_t len = 0;
        char** parts = split_str(field, '-', &len);
        if (len > 2) {
            throw std::exception();//CronParseException("Range has more than two fields: '" +
//                    field + "' in expression \"" + "this.expression" + "\"");
        }
        res[0] = parse_uint32(parts[0]);
        res[1] = parse_uint32(parts[1]);
    }
    if (res[0] >= max || res[1] >= max) {
        throw std::exception();
//        throw new IllegalArgumentException("Range exceeds maximum (" + max + "): '" +
//                field + "' in expression \"" + "this.expression" + "\"");
    }
    if (res[0] < min || res[1] < min) {
        throw std::exception();
//        throw new IllegalArgumentException("Range less than minimum (" + min + "): '" +
//                field + "' in expression \"" + "this.expression" + "\"");
    }
    return res;
}

char* set_number_hits(char* value, unsigned int min, unsigned int max) {
    char* bits = (char*) malloc(max);
    memset(bits, 0, max);
    size_t len = 0;
    char** fields = split_str(value, ',', &len);
    for (size_t i = 0; i < len; i++) {
        char* field = fields[i];
        if (!has_char(field, '/')) {
            // Not an incrementer so it must be a range (possibly empty)
            auto range = get_range(field, min, max);
            for (unsigned int i = range[0]; i <= range[1]; i++) {
                bits[i] = 1;
            }
        } else {
            size_t len2 = 0;
            char** split = split_str(field, '/', &len2);
            if (len2 > 2) {
                throw std::exception();//("Incrementer has more than two fields: '");
            }
            auto range = get_range(split[0], min, max);
            if (!has_char(split[0], '-')) {
                range[1] = max - 1;
            }
            auto delta = parse_uint32(split[1]);
            for (unsigned int i = range[0]; i <= range[1]; i += delta) {
                bits[i] = 1;
            }
        }
    }
    return bits;
}

char* set_months(char* value) {
    unsigned int max = 12;
    char* bits = (char*) malloc(MAX_MONTHS);
    memset(bits, 0, MAX_MONTHS);
    to_upper(value);
    char* replaced = replace_ordinals(value, MONTHS_ARR, MONTHS_ARR_LEN);
    // Months start with 1 in Cron and 0 in Calendar, so push the values first into a longer bit set
    auto months = set_number_hits(replaced, 1, max + 1);
    // ... and then rotate it to the front of the months
    for (unsigned int i = 1; i <= max; i++) {
        if (months[i]) {
            bits[i - 1] = 1;
        }
    }
    return bits;
}

char* set_days(char* field, int max) {
    if (1 == strlen(field) && '?' == field[0]) {
        field[0] = '*';
    }
    return set_number_hits(field, 0, max);
}

char* set_days_of_month(char* field) {
    // Days of month start with 1 (in Cron and Calendar) so add one
    auto bits = set_days(field, MAX_DAYS_OF_MONTH);
    // ... and remove it from the front
    bits[0] = 0;
    return bits;
}



cron_expr* cron_parse_expr(const char* expression, char** error) {
    size_t len = 0;
    char** fields = split_str(expression, ' ', &len);
    if (len != 6) {
        throw std::exception();//("Cron expression must consist of 6 fields (found %d in \"%s\")");
    }
    auto seconds = set_number_hits(fields[0], 0, 60);
    auto minutes = set_number_hits(fields[1], 0, 60);
    auto hours = set_number_hits(fields[2], 0, 24);
    to_upper(fields[5]);
    auto days_of_week = set_days(replace_ordinals(fields[5], DAYS_ARR, DAYS_ARR_LEN), 8);
    if (days_of_week[7]) {
        // Sunday can be represented as 0 or 7
        days_of_week[0] = true;
        days_of_week[7] = false;
    }
    auto days_of_month = set_days_of_month(fields[3]);
    auto months = set_months(fields[4]);

    cron_expr* res = (cron_expr*) malloc(sizeof (cron_expr));
    res->seconds = seconds;
    res->minutes = minutes;
    res->hours = hours;
    res->days_of_week = days_of_week;
    res->days_of_month = days_of_month;
    res->months = months;
    if (error) {
        *error = NULL;
    }
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
    tm* calendar = gmtime(&date);
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


















