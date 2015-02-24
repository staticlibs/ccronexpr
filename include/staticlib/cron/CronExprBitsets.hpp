/* 
 * File:   CronExprBitsets.hpp
 * Author: alex
 *
 * Created on February 24, 2015, 9:35 AM
 */

#ifndef CRONEXPRBITSETS_HPP
#define	CRONEXPRBITSETS_HPP

#include <string>


class CronExprBitsets {
    std::string seconds;
    std::string minutes;
    std::string hours;
    std::string days_of_week;
    std::string days_of_month;
    std::string months;
    
public:
    CronExprBitsets(std::string seconds, std::string minutes, std::string hours, std::string days_of_week,
            std::string days_of_month, std::string months) :
    seconds(std::move(seconds)),
    minutes(std::move(minutes)),
    hours(std::move(hours)),
    days_of_week(std::move(days_of_week)),
    days_of_month(std::move(days_of_month)),
    months(std::move(months)) { }
    
    std::string to_string() {
        std::string sb{};
        sb.append(seconds).push_back('\n');
        sb.append(minutes).push_back('\n');
        sb.append(hours).push_back('\n');
        sb.append(days_of_week).push_back('\n');
        sb.append(days_of_month).push_back('\n');
        sb.append(months);
        return sb;
    }
};

#endif	/* CRONEXPRBITSETS_HPP */

