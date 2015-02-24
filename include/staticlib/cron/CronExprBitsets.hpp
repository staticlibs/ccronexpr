/* 
 * File:   CronExprBitsets.hpp
 * Author: alex
 *
 * Created on February 24, 2015, 9:35 AM
 */

#ifndef CRONEXPRBITSETS_HPP
#define	CRONEXPRBITSETS_HPP

#include <vector>
#include <string>


class CronExprBitsets {
    std::vector<bool> seconds;
    std::vector<bool> minutes;
    std::vector<bool> hours;
    std::vector<bool> days_of_week;
    std::vector<bool> days_of_month;
    std::vector<bool> months;
    
public:
    CronExprBitsets(std::vector<bool> seconds, std::vector<bool> minutes, std::vector<bool> hours, 
            std::vector<bool> days_of_week, std::vector<bool> days_of_month, std::vector<bool> months) :
    seconds(std::move(seconds)),
    minutes(std::move(minutes)),
    hours(std::move(hours)),
    days_of_week(std::move(days_of_week)),
    days_of_month(std::move(days_of_month)),
    months(std::move(months)) { }
    
    std::string to_string() {
        std::string sb{};
        sb.append(to_bitstring(seconds)).push_back('\n');
        sb.append(to_bitstring(minutes)).push_back('\n');
        sb.append(to_bitstring(hours)).push_back('\n');
        sb.append(to_bitstring(days_of_week)).push_back('\n');
        sb.append(to_bitstring(days_of_month)).push_back('\n');
        sb.append(to_bitstring(months));
        return sb;
    }
    
private:
    std::string to_bitstring(std::vector<bool> bits) {
        auto res = std::string{};
        for (auto b : bits) {
            res.push_back(b ? '1' : '0');
        }
        return res;
    }    
};

#endif	/* CRONEXPRBITSETS_HPP */

