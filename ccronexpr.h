/* 
 * File:   CronExprParser.hpp
 * Author: alex
 *
 * Created on February 24, 2015, 9:35 AM
 */

#ifndef CRONEXPRPARSER_HPP
#define	CRONEXPRPARSER_HPP

#include <time.h>
#include <stdlib.h>

#define MAX_SECONDS 60
#define MAX_MINUTES 60
#define MAX_HOURS 24
#define MAX_DAYS_OF_WEEK 8
#define MAX_DAYS_OF_MONTH 32
#define MAX_MONTHS 12

#define CF_SECOND 0
#define CF_MINUTE 1
#define CF_HOUR_OF_DAY 2
#define CF_DAY_OF_WEEK 3
#define CF_DAY_OF_MONTH 4
#define CF_MONTH 5
#define CF_YEAR 6

#define CF_ARR_LEN 7

#define INVALID_INSTANT ((time_t) -1)

typedef struct {
    char* seconds;
    char* minutes;
    char* hours;
    char* days_of_week;
    char* days_of_month;
    char* months;
} cron_expr;


cron_expr* cron_parse_expr(const char* expression, const char** error);

time_t cron_next(cron_expr*, time_t date);

time_t cron_next_local(cron_expr*, time_t date);



#endif	/* CRONEXPRPARSER_HPP */

