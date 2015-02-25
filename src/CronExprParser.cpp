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

#include "staticlib/cron/CronExprParser.hpp"

const char* DAYS_ARR[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
#define DAYS_ARR_LEN 7
const char* MONTHS_ARR[] = {"FOO", "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
#define MONTHS_ARR_LEN 13

int crons_equal(CronExprBitsets* cr1, CronExprBitsets* cr2) {
    for (int i = 0; i < MAX_SECONDS; i++) {
        if (cr1->seconds[i] != cr2->seconds[i]) {
            return 0;
        }
    }
    for (int i = 0; i < MAX_MINUTES; i++) {
        if (cr1->minutes[i] != cr2->minutes[i]) {
            return 0;
        }
    }
    for (int i = 0; i < MAX_HOURS; i++) {
        if (cr1->hours[i] != cr2->hours[i]) {
            return 0;
        }
    }
    for (int i = 0; i < MAX_DAYS_OF_WEEK; i++) {
        if (cr1->days_of_week[i] != cr2->days_of_week[i]) {
            return 0;
        }
    }
    for (int i = 0; i < MAX_DAYS_OF_MONTH; i++) {
        if (cr1->days_of_month[i] != cr2->days_of_month[i]) {
            return 0;
        }
    }
    for (int i = 0; i < MAX_MONTHS; i++) {
        if (cr1->months[i] != cr2->months[i]) {
            return 0;
        }
    }
    return 1;
}

namespace { // anonymous


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
//    std::stringstream ss{};
//    ss << t;
//    return ss.str();
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

} // namespace

//int is_blank(char* st) {
//    size_t len = strlen(st);
//    for(size_t i = 0; i < len; i++) {
//        if(!isspace(st[i])) return 0;
//    }
//    return 1;
//}

//char* trim(char* str) {
//    size_t len = strlen(str);
//    char* out = (char*) malloc(len+1);
//    memset(out, 0, len+1);
//    size_t j = 0;
//    for(size_t i = 0; i < len; i++) {
//        if(!isspace(str[i])) {
//            out[j++] = str[i];
//        }
//    }
//    return out;
//}


char** split_str(const char* str, char del, size_t* len_out) {
//    size_t len = 0;
//    char** splitted = str_split(&str.front(), del, &len);
//    auto res = std::vector<std::string>{};
//    res.reserve(len);
//    for (size_t i = 0; splitted[i]; i++) {
//        res.push_back(splitted[i]);
//    }
//    return res;
  
    
//    int accum = 0;
//    size_t len = 0;
//    char* str = &pstr.front();
//    size_t stlen = strlen(str);
//    for(size_t i = 0; i < stlen; i++) {
//        if (del == str[i]) {
//            if (accum > 0) {
//                len += 1;
//                accum = 0;
//            }
//        } else if (!isspace(str[i])) {
//            accum += 1;
//        }
//    }
//    if (accum > 0) {
//        len += 1;
//    }
//    char** res = (char**) malloc(len*sizeof(char*));
//    char* buf = (char*) malloc(stlen + 1);
//    memset(buf, 0, stlen + 1);
//    size_t ri = 0;
//    size_t bi = 0;
//    for (size_t i = 0; i < stlen; i++) {
//        if (del == str[i]) {
//            if (bi > 0) {
//                res[ri++] = trim(buf);
//                memset(buf, 0, stlen + 1);
//                bi = 0;
//            }
//        } else if (!isspace(str[i])) {
//            buf[bi++] = str[i];
//            bi += 1;
//        }
//    }
//    if (bi > 0) {
//        res[ri++] = trim(buf);
//    }
//    
//    
//    auto vec = std::vector<std::string>{};
//    vec.reserve(len);
//    for (size_t i = 0; i < len; i++) {
//        vec.push_back(res[i]);
//    }
//    return vec;
    
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
        // copy here
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
                // copy here
//                res.push_back(std::string(buf));
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
        // copy here
//        res.push_back(std::string(buf));
        res[ri++] = strdup(buf);
    }
    *len_out = len;
    return res;
//    assert(len == res.size());
    
//    auto vec = std::vector<std::string>{};
//    vec.reserve(len);
//    for (size_t i = 0; i < len; i++) {
//        vec.push_back(res[i]);
//    }
//    return vec;
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



CronExprBitsets parse(const char* expression) {
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

    return CronExprBitsets(seconds, minutes, hours, 
            days_of_week, days_of_month, months);
}
