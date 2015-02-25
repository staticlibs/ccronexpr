/* 
 * File:   CronExprParser_test.cpp
 * Author: alex
 *
 * Created on February 24, 2015, 9:36 AM
 */

#include <iostream>
#include <cassert>
#include <ctime>
#include <cstdlib>
#include <cstdio>

#include "staticlib/cron/CronExprParser.hpp"

#define DATE_FORMAT "%Y-%m-%d_%H:%M:%S"

namespace { // anonymous

//void split_str_test() {
//    assert(split_str("* * *", ' ') == (std::vector<std::string>{"*", "*", "*"}));
//    assert(split_str("*  / 12/ *", '/') == (std::vector<std::string>{"*", "12", "*"}));
//    assert(split_str("*   12 \tFEB-MAY", ' ') == (std::vector<std::string>{"*", "12", "FEB-MAY"}));
//}
//
//void replace_ordinals_test() {
//    assert(replace_ordinals("0 0 7 ? * MON-FRI", DAYS) == "0 0 7 ? * 1-5");
//    assert(replace_ordinals("* * * * FEB-MAY *", MONTHS) == "* * * * 2-5 *");
//}
//
//void test_parse() {
//    auto parsed = parse("0 30 * 30 1/3 ?");
//    std::cout << parsed.to_string() << std::endl;
//}

void check_next(std::string pattern, std::string initial, std::string expected) {
    auto parsed = parse(pattern);
    std::tm calinit_val = {};
    auto calinit = &calinit_val;
    auto res = strptime(initial.c_str(), DATE_FORMAT, calinit);
    assert(res);
    auto buffer = (char*) malloc(21);
    strftime(buffer, 20, DATE_FORMAT, calinit);
//    puts(buffer); 
    time_t dateinit = timegm(calinit);
    assert(-1 != dateinit);
    auto datenext = parsed.next(dateinit);
    auto calnext = std::gmtime(&datenext);
    assert(calnext);
    strftime(buffer, 20, DATE_FORMAT, calnext);
    if(expected != buffer) {
        puts(expected.c_str());
        puts(buffer);
        assert(false);
    }
    free(buffer);
}

void check_same(std::string expr1, std::string expr2) {
    auto parsed1 = parse(expr1);
    auto parsed2 = parse(expr2);
    assert(crons_equal(&parsed1, &parsed2));
}

void check_calc_invalid() {
    auto parsed = parse("0 0 0 31 6 *");
    std::tm calinit_val = {};
    auto calinit = &calinit_val;
    strptime("2012-07-01_09:53:50", DATE_FORMAT, calinit);
    time_t dateinit = timegm(calinit);
    bool thrown = false;
    try {
        parsed.next(dateinit);
    } catch (const std::exception& e) {
        thrown = true;
    }
    assert(thrown);
}

void check_expr_invalid(std::string expr) {
    bool thrown = false;
    try {
        parse(expr);
//        std::cout << parsed.to_string() << std::endl; 
    } catch (const std::exception& e) {
        thrown = true;
    }
    assert(thrown);
}

void test_expr() {
//    (void) check_next;
    check_next("*/15 * 1-4 * * *", "2012-07-01_09:53:50", "2012-07-02_01:00:00");
    check_next("*/15 * 1-4 * * *", "2012-07-01_09:53:00", "2012-07-02_01:00:00");
    check_next("0 */2 1-4 * * *", "2012-07-01_09:00:00", "2012-07-02_01:00:00");
    check_next("* * * * * *", "2012-07-01_09:00:00", "2012-07-01_09:00:01");
    check_next("* * * * * *", "2012-12-01_09:00:58", "2012-12-01_09:00:59");
    check_next("10 * * * * *", "2012-12-01_09:42:09", "2012-12-01_09:42:10");
    check_next("11 * * * * *", "2012-12-01_09:42:10", "2012-12-01_09:42:11");
    check_next("10 * * * * *", "2012-12-01_09:42:10", "2012-12-01_09:43:10");
    check_next("10-15 * * * * *", "2012-12-01_09:42:09", "2012-12-01_09:42:10");
    check_next("10-15 * * * * *", "2012-12-01_21:42:14", "2012-12-01_21:42:15");
    check_next("0 * * * * *", "2012-12-01_21:10:42", "2012-12-01_21:11:00");
    check_next("0 * * * * *", "2012-12-01_21:11:00", "2012-12-01_21:12:00");
    check_next("0 11 * * * *", "2012-12-01_21:10:42", "2012-12-01_21:11:00");
    check_next("0 10 * * * *", "2012-12-01_21:11:00", "2012-12-01_22:10:00");
    check_next("0 0 * * * *", "2012-09-30_11:01:00", "2012-09-30_12:00:00");
    check_next("0 0 * * * *", "2012-09-30_12:00:00", "2012-09-30_13:00:00");
    check_next("0 0 * * * *", "2012-09-10_23:01:00", "2012-09-11_00:00:00");
    check_next("0 0 * * * *", "2012-09-11_00:00:00", "2012-09-11_01:00:00");
    check_next("0 0 0 * * *", "2012-09-01_14:42:43", "2012-09-02_00:00:00");
    check_next("0 0 0 * * *", "2012-09-02_00:00:00", "2012-09-03_00:00:00");
    check_next("* * * 10 * *", "2012-10-09_15:12:42", "2012-10-10_00:00:00");
    check_next("* * * 10 * *", "2012-10-11_15:12:42", "2012-11-10_00:00:00");
    check_next("0 0 0 * * *", "2012-09-30_15:12:42", "2012-10-01_00:00:00");
    check_next("0 0 0 * * *", "2012-10-01_00:00:00", "2012-10-02_00:00:00");
    check_next("0 0 0 * * *", "2012-08-30_15:12:42", "2012-08-31_00:00:00");
    check_next("0 0 0 * * *", "2012-08-31_00:00:00", "2012-09-01_00:00:00");
    check_next("0 0 0 * * *", "2012-10-30_15:12:42", "2012-10-31_00:00:00");
    check_next("0 0 0 * * *", "2012-10-31_00:00:00", "2012-11-01_00:00:00");
    check_next("0 0 0 1 * *", "2012-10-30_15:12:42", "2012-11-01_00:00:00");
    check_next("0 0 0 1 * *", "2012-11-01_00:00:00", "2012-12-01_00:00:00");
    check_next("0 0 0 1 * *", "2010-12-31_15:12:42", "2011-01-01_00:00:00");
    check_next("0 0 0 1 * *", "2011-01-01_00:00:00", "2011-02-01_00:00:00");
    check_next("0 0 0 31 * *", "2011-10-30_15:12:42", "2011-10-31_00:00:00");
    check_next("0 0 0 1 * *", "2011-10-30_15:12:42", "2011-11-01_00:00:00");
    check_next("* * * * * 2", "2010-10-25_15:12:42", "2010-10-26_00:00:00");
    check_next("* * * * * 2", "2010-10-20_15:12:42", "2010-10-26_00:00:00");
    check_next("* * * * * 2", "2010-10-27_15:12:42", "2010-11-02_00:00:00");
    check_next("55 5 * * * *", "2010-10-27_15:04:54", "2010-10-27_15:05:55");
    check_next("55 5 * * * *", "2010-10-27_15:05:55", "2010-10-27_16:05:55");
    check_next("55 * 10 * * *", "2010-10-27_09:04:54", "2010-10-27_10:00:55");
    check_next("55 * 10 * * *", "2010-10-27_10:00:55", "2010-10-27_10:01:55");
    check_next("* 5 10 * * *", "2010-10-27_09:04:55", "2010-10-27_10:05:00");
    check_next("* 5 10 * * *", "2010-10-27_10:05:00", "2010-10-27_10:05:01");
    check_next("55 * * 3 * *", "2010-10-02_10:05:54", "2010-10-03_00:00:55");
    check_next("55 * * 3 * *", "2010-10-03_00:00:55", "2010-10-03_00:01:55");
    check_next("* * * 3 11 *", "2010-10-02_14:42:55", "2010-11-03_00:00:00");
    check_next("* * * 3 11 *", "2010-11-03_00:00:00", "2010-11-03_00:00:01");
    check_next("0 0 0 29 2 *", "2007-02-10_14:42:55", "2008-02-29_00:00:00");
    check_next("0 0 0 29 2 *", "2008-02-29_00:00:00", "2012-02-29_00:00:00");
    check_next("0 0 7 ? * MON-FRI", "2009-09-26_00:42:55", "2009-09-28_07:00:00");
    check_next("0 0 7 ? * MON-FRI", "2009-09-28_07:00:00", "2009-09-29_07:00:00");
    check_next("0 30 23 30 1/3 ?", "2010-12-30_00:00:00", "2011-01-30_23:30:00");
    check_next("0 30 23 30 1/3 ?", "2011-01-30_23:30:00", "2011-04-30_23:30:00");
    check_next("0 30 23 30 1/3 ?", "2011-04-30_23:30:00", "2011-07-30_23:30:00");
    
    
//    auto buffer = (char*) malloc(21);
//    auto cur = std::time(nullptr);
//    auto calinit = std::localtime(&cur);        
//    strptime("2012-07-01_09:00:00", DATE_FORMAT, calinit);
//    std::cout << calinit->tm_hour << std::endl;
//    auto date = mktime(calinit);
//    calinit = std::gmtime(&date);
//    strftime(buffer, 20, DATE_FORMAT, calinit);
//    std::cout << buffer << std::endl;
//    std::cout << res << std::endl;
    
}

void test_parse() {
    check_same("* * * 2 * *", "* * * 2 * ?");
    check_same("57,59 * * * * *", "57/2 * * * * *");
    check_same("1,3,5 * * * * *", "1-6/2 * * * * *");
    check_same("* * 4,8,12,16,20 * * *", "* * 4/4 * * *");
    check_same("* * * * * 0-6", "* * * * * TUE,WED,THU,FRI,SAT,SUN,MON");
    check_same("* * * * * 0", "* * * * * SUN");
    check_same("* * * * * 0", "* * * * * 7");
    check_same("* * * * 1-12 *", "* * * * FEB,JAN,MAR,APR,MAY,JUN,JUL,AUG,SEP,OCT,NOV,DEC *");
    check_same("* * * * 2 *", "* * * * Feb *");
    check_same("*  *  * *  1 *", "* * * * 1 *");
    
    check_expr_invalid("77 * * * * *");
    check_expr_invalid("44-77 * * * * *");
    check_expr_invalid("* 77 * * * *");
    check_expr_invalid("* 44-77 * * * *");
    check_expr_invalid("* * 27 * * *");
    check_expr_invalid("* * 23-28 * * *");
    check_expr_invalid("* * * 45 * *");
    check_expr_invalid("* * * 28-45 * *");
    check_expr_invalid("0 0 0 25 13 ?");
    check_expr_invalid("0 0 0 25 0 ?");
    check_expr_invalid("0 0 0 32 12 ?");
    check_expr_invalid("* * * * 11-13 *");
}

} // namespace

int main() {
//    split_str_test();
//    replace_ordinals_test();
//    test_parse();
    test_expr();
    test_parse();
    check_calc_invalid();

    return 0;
}

