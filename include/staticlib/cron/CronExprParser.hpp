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



#endif	/* CRONEXPRPARSER_HPP */

