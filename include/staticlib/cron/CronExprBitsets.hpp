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
#include <ctime>


enum CalendarField {
    SECOND,
    MINUTE,
    HOUR_OF_DAY,
    DAY_OF_WEEK,
    DAY_OF_MONTH,
    MONTH,
    YEAR
};

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
    
public:

    std::time_t next(std::time_t date) {
        /*
        The plan:

        1 Round up to the next whole second

        2 If seconds match move on, otherwise find the next match:
        2.1 If next match is in the next minute then roll forwards

        3 If minute matches move on, otherwise find the next match
        3.1 If next match is in the next hour then roll forwards
        3.2 Reset the seconds and go to 2

        4 If hour matches move on, otherwise find the next match
        4.1 If next match is in the next day then roll forwards,
        4.2 Reset the minutes and seconds and go to 2

        ...
         */
        auto calendar = std::gmtime(&date);
        auto original = mkgmtime(calendar);
        
        do_next(calendar, calendar->tm_year);

        if (mkgmtime(calendar) == original) {
            // We arrived at the original timestamp - round up to the next whole second and try again...
            add_to_field(calendar, CalendarField::SECOND, 1);
            do_next(calendar, calendar->tm_year);
        }

        return mkgmtime(calendar);
    }

private:
// http://stackoverflow.com/a/22557778    
    time_t mkgmtime(struct tm* tm) {
#if defined(_WIN32)
        return _mkgmtime(tm);
#else
        return timegm(tm);
#endif
    }
    
    std::string to_bitstring(std::vector<bool> bits) {
        auto res = std::string{};
        for (auto b : bits) {
            res.push_back(b ? '1' : '0');
        }
        return res;
    }


    void do_next(std::tm* calendar, uint32_t dot) {
        auto resets = std::vector<CalendarField>{};
        auto empty_list = std::vector<CalendarField>{};

        uint32_t second = calendar->tm_sec;
        uint32_t update_second = find_next(this->seconds, second, calendar, CalendarField::SECOND, CalendarField::MINUTE, empty_list);
        if (second == update_second) {
            resets.push_back(CalendarField::SECOND);
        }

        uint32_t minute = calendar->tm_min;
        uint32_t update_minute = find_next(this->minutes, minute, calendar, CalendarField::MINUTE, CalendarField::HOUR_OF_DAY, resets);
        if (minute == update_minute) {
            resets.push_back(CalendarField::MINUTE);
        } else {
            do_next(calendar, dot);
        }

        uint32_t hour = calendar->tm_hour;
        uint32_t update_hour = find_next(this->hours, hour, calendar, CalendarField::HOUR_OF_DAY, CalendarField::DAY_OF_WEEK, resets);
        if (hour == update_hour) {
            resets.push_back(CalendarField::HOUR_OF_DAY);
        } else {
            do_next(calendar, dot);
        }

        uint32_t day_of_week = calendar->tm_wday;
        uint32_t day_of_month = calendar->tm_mday;
        uint32_t update_day_of_month = find_next_day(calendar, this->days_of_month, day_of_month, this->days_of_week, day_of_week, resets);
        if (day_of_month == update_day_of_month) {
            resets.push_back(CalendarField::DAY_OF_MONTH);
        } else {
            do_next(calendar, dot);
        }

        uint32_t month = calendar->tm_mon;
        uint32_t update_month = find_next(this->months, month, calendar, CalendarField::MONTH, CalendarField::YEAR, resets);
        if (month != update_month) {
            if (calendar->tm_year - dot > 4) {
                throw "Invalid cron expression \" this.expression\" led to runaway search for next trigger";
            }
            do_next(calendar, dot);
        }

    }

    uint32_t find_next_day(std::tm* calendar, std::vector<bool> days_of_month, 
            uint32_t day_of_month, std::vector<bool> days_of_week, uint32_t day_of_week,
            std::vector<CalendarField> resets) {
        uint32_t count = 0;
        uint32_t max = 366;
        while ((!days_of_month[day_of_month] || !days_of_week[day_of_week]) && count++ < max) {
            add_to_field(calendar, CalendarField::DAY_OF_MONTH, 1);
            day_of_month = calendar->tm_mday;
            day_of_week = calendar->tm_wday;
            reset(calendar, resets);
        }
        if (count >= max) {
            // todo
            throw "Overflow in day for expression \"this.expression \"";
        }
        return day_of_month;
    }

    /**
     * Search the bits provided for the next set bit after the value provided,
     * and reset the calendar.
     * @param bits a {@link boolean[]} representing the allowed values of the field
     * @param value the current value of the field
     * @param calendar the calendar to increment as we move through the bits
     * @param field the field to increment in the calendar (@see
     * {@link Calendar} for the static constants defining valid fields)
     * @param lowerOrders the Calendar field ids that should be reset (i.e. the
     * ones of lower significance than the field of interest)
     * @return the value of the calendar field that is next in the sequence
     */
    uint32_t find_next(std::vector<bool> bits, uint32_t value, std::tm* calendar, CalendarField field, 
            CalendarField nextField, std::vector<CalendarField> lower_orders) {
        int32_t next_value = next_set_bit(bits, value);
        // roll over if needed
        if (next_value == -1) {
            add_to_field(calendar, nextField, 1);
            reset(calendar, field);
            next_value = next_set_bit(bits, 0);
        }
        if (-1 == next_value || static_cast<uint32_t>(next_value) != value) {
            set_field(calendar, field, next_value);
            reset(calendar, lower_orders);
        }
        return next_value;
    }

    /**
     * Reset the calendar setting all the fields provided to zero.
     */
    void reset(std::tm* calendar, CalendarField field) {
        switch (field) {
        case CalendarField::SECOND: calendar->tm_sec = 0; break;
        case CalendarField::MINUTE: calendar->tm_min = 0; break;
        case CalendarField::HOUR_OF_DAY: calendar->tm_hour = 0; break;
        case CalendarField::DAY_OF_WEEK: calendar->tm_wday = 0; break;
        case CalendarField::DAY_OF_MONTH: calendar->tm_mday = 1; break;
        case CalendarField::MONTH: calendar->tm_mon = 0; break;
        case CalendarField::YEAR: calendar->tm_year = 0; break;
        }
        mkgmtime(calendar);
    }
    
    void reset(std::tm* calendar, const std::vector<CalendarField>& fields) {
        for (auto fi : fields) {
            reset(calendar, fi);
        }
    }
    
    void add_to_field(std::tm* calendar, CalendarField field, int32_t val) {
        switch (field) {
        case CalendarField::SECOND: calendar->tm_sec = calendar->tm_sec + val; break;
        case CalendarField::MINUTE: calendar->tm_min = calendar->tm_min + val; break;
        case CalendarField::HOUR_OF_DAY: calendar->tm_hour = calendar->tm_hour + val; break;
        case CalendarField::DAY_OF_WEEK: // mkgmtime ignores this field
        case CalendarField::DAY_OF_MONTH: calendar->tm_mday = calendar->tm_mday + val; break;
        case CalendarField::MONTH: calendar->tm_mon = calendar->tm_mon + val; break;
        case CalendarField::YEAR: calendar->tm_year = calendar->tm_year + val; break;
        }
        mkgmtime(calendar);
    }

    void set_field(std::tm* calendar, CalendarField field, int32_t val) {
        switch (field) {
        case CalendarField::SECOND: calendar->tm_sec = val; break;
        case CalendarField::MINUTE: calendar->tm_min = val; break;
        case CalendarField::HOUR_OF_DAY: calendar->tm_hour = val; break;
        case CalendarField::DAY_OF_WEEK: calendar->tm_wday = val; break;
        case CalendarField::DAY_OF_MONTH: calendar->tm_mday = val; break;
        case CalendarField::MONTH: calendar->tm_mon = val; break;
        case CalendarField::YEAR: calendar->tm_year = val; break;
        }
        mkgmtime(calendar);
    }

    int32_t next_set_bit(const std::vector<bool>& bits, uint32_t from_index) {
        for (std::vector<bool>::size_type i = from_index; i < bits.size(); i++) {
            if (bits[i]) return i;
        }
        return -1;
    }
};

#endif	/* CRONEXPRBITSETS_HPP */

