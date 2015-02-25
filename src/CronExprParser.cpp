/* 
 * File:   CronExprParser.cpp
 * Author: alex
 * 
 * Created on February 24, 2015, 9:35 AM
 */

#include <string>
#include <vector>
#include <cctype>
#include <sstream>
#include <cerrno>
#include <climits>
#include <cstring>
#include <cmath>
#include <cassert>

#include "staticlib/cron/CronExprParser.hpp"

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

//bool has_spaces(const std::string& st) {
//    for (auto ch : st) {
//        if (std::isspace(ch)) return true;
//    }
//    return false;
//}
//
//std::string trim(const std::string& st) {
//    auto res = std::string{};
//    for (auto ch : st) {
//        if (!std::isspace(ch)) {
//            res.push_back(ch);
//        }
//    }
//    return res;
//}

class CronParseException : public std::exception {
private:
    std::string message;
public:
    CronParseException() = default;

    CronParseException(std::string msg) : message(msg) {}

    virtual const char* what() const noexcept {
        return message.c_str();
    }
};


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
        throw CronParseException(std::string("Cannot parse uint32_t from string:[]"));
    }
    if (l < 0 || l > UINT_MAX) {
        throw CronParseException(std::string("Value overflow for uint32_t from string:[]"));
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


char** split_str(std::string pstr, char del, size_t* len_out) {
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
    
    char* str = &pstr.front();
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

std::string replace_ordinals(std::string value, const std::vector<std::string>& arr) {
    auto res = value;
    for (std::vector<std::string>::size_type i = 0; i < arr.size(); i++) {
        auto replacement = std::string(to_string(i));
        res = str_replace(&res.front(), arr[i].c_str(), replacement.c_str());
    }
    return res;
}

std::pair<uint32_t, uint32_t> get_range(std::string field, uint32_t min, uint32_t max) {
    auto res = std::pair<uint32_t, uint32_t>{};
    if ("*" == field) {
        res.first = min;
        res.second = max - 1;
    } else if (std::string::npos == field.find('-')) {
        auto val = parse_uint32(field.c_str());
        res.first = val;
        res.second = val;
    } else {
        size_t len = 0;
        char** parts = split_str(field, '-', &len);
        if (len > 2) {
            throw CronParseException("Range has more than two fields: '" +
                    field + "' in expression \"" + "this.expression" + "\"");
        }
        res.first = parse_uint32(parts[0]);
        res.second = parse_uint32(parts[1]);
    }
    if (res.first >= max || res.second >= max) {
        throw std::exception();
//        throw new IllegalArgumentException("Range exceeds maximum (" + max + "): '" +
//                field + "' in expression \"" + "this.expression" + "\"");
    }
    if (res.first < min || res.second < min) {
        throw std::exception();
//        throw new IllegalArgumentException("Range less than minimum (" + min + "): '" +
//                field + "' in expression \"" + "this.expression" + "\"");
    }
    return res;
}

int has_chars(char* str, char ch) {
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
        if (str[i] == ch) return 1;
    }
    return 0;
}

char* set_number_hits(std::string value, uint32_t min, uint32_t max) {
    char* bits = (char*) malloc(max);
    memset(bits, 0, max);
    size_t len = 0;
    char** fields = split_str(value, ',', &len);
    for (size_t i = 0; i < len; i++) {
        char* field = fields[i];
        if (!has_chars(field, '/')) {
            // Not an incrementer so it must be a range (possibly empty)
            auto range = get_range(field, min, max);
            for (uint32_t i = range.first; i <= range.second; i++) {
                bits[i] = 1;
            }
        } else {
            size_t len2 = 0;
            char** split = split_str(field, '/', &len2);
            if (len2 > 2) {
                throw CronParseException("Incrementer has more than two fields: '");
            }
            auto range = get_range(split[0], min, max);
            if (!has_chars(split[0], '-')) {
                range.second = max - 1;
            }
            auto delta = parse_uint32(split[1]);
            for (uint32_t i = range.first; i <= range.second; i += delta) {
                bits[i] = 1;
            }
        }
    }
    return bits;
}

char* set_months(std::string value) {
    uint32_t max = 12;
    char* bits = (char*) malloc(MAX_MONTHS);
    memset(bits, 0, MAX_MONTHS);
    to_upper(&value.front());
    std::string replaced = replace_ordinals(value, MONTHS);
    // Months start with 1 in Cron and 0 in Calendar, so push the values first into a longer bit set
    auto months = set_number_hits(replaced, 1, max + 1);
    // ... and then rotate it to the front of the months
    for (uint32_t i = 1; i <= max; i++) {
        if (months[i]) {
            bits[i - 1] = 1;
        }
    }
    return bits;
}

char* set_days(std::string field, int max) {
    if ("?" == field) {
        field = "*";
    }
    return set_number_hits(field, 0, max);
}

char* set_days_of_month(std::string field) {
    // Days of month start with 1 (in Cron and Calendar) so add one
    auto bits = set_days(field, MAX_DAYS_OF_MONTH);
    // ... and remove it from the front
    bits[0] = 0;
    return bits;
}



CronExprBitsets parse(std::string expression) {
    size_t len = 0;
    char** fields = split_str(expression, ' ', &len);
    if (len != 6) {
        throw CronParseException("Cron expression must consist of 6 fields (found %d in \"%s\")");
    }
    auto seconds = set_number_hits(fields[0], 0, 60);
    auto minutes = set_number_hits(fields[1], 0, 60);
    auto hours = set_number_hits(fields[2], 0, 24);
    to_upper(fields[5]);
    auto days_of_week = set_days(replace_ordinals(fields[5], DAYS), 8);
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
