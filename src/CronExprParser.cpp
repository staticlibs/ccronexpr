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

#include "staticlib/cron/CronExprParser.hpp"

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


std::string& to_upper(std::string& str) {
    for (std::string::size_type i = 0; i < str.length(); i++) {
        str[i] = std::toupper(str[i]);
    }
    return str;
}

void replace_all(std::string& str, const std::string& from, const std::string& to) {
    std::string::size_type start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

// not all C++11 compilers support it
template<typename T>
std::string to_string(T t) {
    std::stringstream ss{};
    ss << t;
    return ss.str();
}

// workaround for android
uint32_t parse_uint32(const std::string& str) {
    auto cstr = str.c_str();
    char* endptr;
    errno = 0;
    auto l = strtoll(cstr, &endptr, 0);
    if (errno == ERANGE || cstr + str.length() != endptr) {
        throw CronParseException(std::string("Cannot parse uint32_t from string:[]"));
    }
    if (l < 0 || l > UINT_MAX) {
        throw CronParseException(std::string("Value overflow for uint32_t from string:[]"));
    }
    return static_cast<uint32_t> (l);
}

} // namespace



std::vector<std::string> split_str(std::string str, char del) {
    auto res = std::vector<std::string>{};
    auto cur = std::string{};
    for (auto ch : str) {
        if (del == ch) {
            if (cur.length() > 0) {
                // copy here
                res.push_back(cur);
                cur.clear();
            }
        } else if (!std::isspace(ch)) {
            cur.push_back(ch);
        }
    }
    // tail
    if (cur.length() > 0) {
        // copy here
        res.push_back(cur);
    }
    return res;
}

std::string replace_ordinals(std::string value, const std::vector<std::string>& arr) {
    auto res = value;
    for (std::vector<std::string>::size_type i = 0; i < arr.size(); i++) {
        auto replacement = to_string(i);
        replace_all(res, arr[i], replacement);
    }
    return res;
}

std::pair<uint32_t, uint32_t> get_range(std::string field, uint32_t min, uint32_t max) {
    if ("*" == field) {
        return {min, max -1};
    }
    if (std::string::npos == field.find('-')) {
        auto val = parse_uint32(field);
        return { val, val };
    } else {
        auto parts = split_str(field, '-');
        if (parts.size() > 2) {
            throw CronParseException("Range has more than two fields: '" +
                    field + "' in expression \"" + "this.expression" + "\"");
        }
        return { parse_uint32(parts[0]), parse_uint32(parts[1]) };
    }
    // todo: checks
//    if (result[0] >= max || result[1] >= max) {
//        throw new IllegalArgumentException("Range exceeds maximum (" + max + "): '" +
//                field + "' in expression \"" + "this.expression" + "\"");
//    }
//    if (result[0] < min || result[1] < min) {
//        throw new IllegalArgumentException("Range less than minimum (" + min + "): '" +
//                field + "' in expression \"" + "this.expression" + "\"");
//    }
//    return result;
}

std::vector<bool> set_number_hits(std::string value, uint32_t min, uint32_t max) {
    auto bits = std::vector<bool>{};
    bits.resize(max);
    std::vector<std::string> fields = split_str(value, ',');
    for (auto& field : fields) {
        if (std::string::npos == field.find('/')) {
            // Not an incrementer so it must be a range (possibly empty)
            auto range = get_range(field, min, max);
            for (uint32_t i = range.first; i <= range.second; i++) {
                bits[i] = true;
            }
        } else {
            std::vector<std::string> split = split_str(field, '/');
            if (split.size() > 2) {
                throw CronParseException("Incrementer has more than two fields: '");
            }
            auto range = get_range(split[0], min, max);
            if (std::string::npos == split[0].find("-")) {
                range.second = max - 1;
            }
            auto delta = parse_uint32(split[1]);
            for (uint32_t i = range.first; i <= range.second; i += delta) {
                bits[i] = true;
            }
        }
    }
    return bits;
}

std::vector<bool> set_months(std::string value) {
    uint32_t max = 12;
    auto bits = std::vector<bool>{};
    bits.resize(max);
    to_upper(value);
    std::string replaced = replace_ordinals(value, MONTHS);
    // Months start with 1 in Cron and 0 in Calendar, so push the values first into a longer bit set
    auto months = set_number_hits(replaced, 1, max + 1);
    // ... and then rotate it to the front of the months
    for (uint32_t i = 1; i <= max; i++) {
        if (months[i]) {
            bits[i - 1] = true;
        }
    }
    return bits;
}

std::vector<bool> set_days(std::string field, int max) {
    if ("?" == field) {
        field = "*";
    }
    return set_number_hits(field, 0, max);
}

std::vector<bool> set_days_of_month(std::string field) {
    int max = 31;
    // Days of month start with 1 (in Cron and Calendar) so add one
    auto bits = set_days(field, max + 1);
    // ... and remove it from the front
    bits[0] = false;
    return bits;
}



CronExprBitsets parse(std::string expression) {
    auto fields = split_str(expression, ' ');
    if (fields.size() != 6) {
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
