/* 
 * File:   CronExprParser_test.cpp
 * Author: alex
 *
 * Created on February 24, 2015, 9:36 AM
 */

#include <iostream>
#include <cassert>
#include <ctime>

#include "staticlib/cron/CronExprParser.hpp"

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

void test_expr() {
    auto parsed = parse("*/15 * 1-4 * * *");
    std::cout << parsed.to_string() << std::endl;
    std::tm calendar = {};
    char buffer[] = "2012-07-01_09:53:50";
    strptime(buffer, "%Y-%m-%d_%H:%M:%S", &calendar);
    strftime(buffer, 20, "%Y-%m-%d_%H:%M:%S", &calendar);
    std::cout << buffer << std::endl;
    auto next = parsed.next(std::mktime(&calendar));
    auto calres = std::localtime(&next);
    strftime(buffer, 20, "%Y-%m-%d_%H:%M:%S", calres);
    std::cout << buffer << std::endl;
//    assertEquals(new Date(2012, 6, 2, 1, 0),
//            new CronExpr("*/15 * 1-4 * * *").next(new Date(2012, 6, 1, 9, 53, 50)));
}

} // namespace

int main() {
//    split_str_test();
//    replace_ordinals_test();
//    test_parse();
    test_expr();

    return 0;
}

