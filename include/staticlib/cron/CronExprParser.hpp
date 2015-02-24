/* 
 * File:   CronExprParser.hpp
 * Author: alex
 *
 * Created on February 24, 2015, 9:35 AM
 */

#ifndef CRONEXPRPARSER_HPP
#define	CRONEXPRPARSER_HPP

#include <string>
#include <vector>

#include "staticlib/cron/CronExprBitsets.hpp"

const std::vector<std::string> DAYS{"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
const std::vector<std::string> MONTHS{"FOO", "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

CronExprBitsets parse(std::string expression);

std::string replace_ordinals(std::string value, const std::vector<std::string>& arr);

std::vector<std::string> split_str(std::string str, char delimiter);

std::pair<uint32_t, uint32_t> get_range(std::string field, uint32_t min, uint32_t max);

std::vector<bool> set_number_hits(std::string value, uint32_t min, uint32_t max);

#endif	/* CRONEXPRPARSER_HPP */

