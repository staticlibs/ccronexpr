/* 
 * File:   CronExprParser_test.cpp
 * Author: alex
 *
 * Created on February 24, 2015, 9:36 AM
 */

#include <iostream>
#include <cassert>

#include "staticlib/cron/CronExprParser.hpp"

namespace { // anonymous

void split_str_test() {
    assert(split_str("* * *", ' ') == (std::vector<std::string>{"*", "*", "*"}));
    assert(split_str("*  / 12/ *", '/') == (std::vector<std::string>{"*", "12", "*"}));
    assert(split_str("*   12 \tFEB-MAY", ' ') == (std::vector<std::string>{"*", "12", "FEB-MAY"}));
}

void replace_ordinals_test() {
    assert(replace_ordinals("0 0 7 ? * MON-FRI", DAYS) == "0 0 7 ? * 1-5");
    assert(replace_ordinals("* * * * FEB-MAY *", MONTHS) == "* * * * 2-5 *");
}

} // namespace

int main() {
    split_str_test();
    replace_ordinals_test();

    return 0;
}

