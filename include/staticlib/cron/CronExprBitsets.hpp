/* 
 * File:   CronExprBitsets.hpp
 * Author: alex
 *
 * Created on February 24, 2015, 9:35 AM
 */

#ifndef CRONEXPRBITSETS_HPP
#define	CRONEXPRBITSETS_HPP

#include <ctime>
#include <cstdlib>

#include <exception>


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


struct CronExprBitsets {
    char* seconds;
    char* minutes;
    char* hours;
    char* days_of_week;
    char* days_of_month;
    char* months;
    
public:
    CronExprBitsets(char* seconds, char* minutes, char* hours, 
            char* days_of_week, char* days_of_month, char* months) {
        this->seconds = seconds;
        this->minutes = minutes;
        this->hours = hours;
        this->days_of_week = days_of_week;
        this->days_of_month = days_of_month;
        this->months = months;
    }
    
public:

    time_t next(time_t date) {
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
        auto calendar = gmtime(&date);
        auto original = mkgmtime(calendar);
        
        do_next(calendar, calendar->tm_year);

        if (mkgmtime(calendar) == original) {
            // We arrived at the original timestamp - round up to the next whole second and try again...
            add_to_field(calendar, CF_SECOND, 1);
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
    
    void push_to_fields_arr(int* arr, int fi) {
        for (int i = 0; i < CF_ARR_LEN; i++) {
            if (arr[i] == fi) return;
        }
        for (int i=0; i < CF_ARR_LEN; i++) {
            if (-1 == arr[i]) {
                arr[i] = fi;
                return;
            }
        }
    }
    
    void do_next(tm* calendar, unsigned int dot) {
        int* resets = (int*) malloc(CF_ARR_LEN * sizeof (int));
        int* empty_list = (int*) malloc(CF_ARR_LEN * sizeof(int));
        for (int i = 0; i < CF_ARR_LEN; i++) {
            resets[i] = -1;
            empty_list[i] = -1;
        }

        unsigned int second = calendar->tm_sec;
        unsigned int update_second = find_next(this->seconds, MAX_SECONDS, second, calendar, CF_SECOND, CF_MINUTE, empty_list);
        if (second == update_second) {
            push_to_fields_arr(resets, CF_SECOND);
        }

        unsigned int minute = calendar->tm_min;
        unsigned int update_minute = find_next(this->minutes, MAX_MINUTES, minute, calendar, CF_MINUTE, CF_HOUR_OF_DAY, resets);
        if (minute == update_minute) {
            push_to_fields_arr(resets, CF_MINUTE);
        } else {
            do_next(calendar, dot);
        }

        unsigned int hour = calendar->tm_hour;
        unsigned int update_hour = find_next(this->hours, MAX_HOURS, hour, calendar, CF_HOUR_OF_DAY, CF_DAY_OF_WEEK, resets);
        if (hour == update_hour) {
            push_to_fields_arr(resets, CF_HOUR_OF_DAY);
        } else {
            do_next(calendar, dot);
        }

        unsigned int day_of_week = calendar->tm_wday;
        unsigned int day_of_month = calendar->tm_mday;
        unsigned int update_day_of_month = find_next_day(calendar, this->days_of_month, day_of_month, this->days_of_week, day_of_week, resets);
        if (day_of_month == update_day_of_month) {
            push_to_fields_arr(resets, CF_DAY_OF_MONTH);
        } else {
            do_next(calendar, dot);
        }

        unsigned int month = calendar->tm_mon;
        unsigned int update_month = find_next(this->months, MAX_MONTHS, month, calendar, CF_MONTH, CF_YEAR, resets);
        if (month != update_month) {
            if (calendar->tm_year - dot > 4) {
                throw std::exception();
//                throw "Invalid cron expression \" this.expression\" led to runaway search for next trigger";
            }
            do_next(calendar, dot);
        }

    }

    unsigned int find_next_day(tm* calendar, char* days_of_month, 
            unsigned int day_of_month, char* days_of_week, unsigned int day_of_week,
            int* resets) {
        unsigned int count = 0;
        unsigned int max = 366;
        while ((!days_of_month[day_of_month] || !days_of_week[day_of_week]) && count++ < max) {
            add_to_field(calendar, CF_DAY_OF_MONTH, 1);
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
     * @param bits a {@link charean[]} representing the allowed values of the field
     * @param value the current value of the field
     * @param calendar the calendar to increment as we move through the bits
     * @param field the field to increment in the calendar (@see
     * {@link Calendar} for the static constants defining valid fields)
     * @param lowerOrders the Calendar field ids that should be reset (i.e. the
     * ones of lower significance than the field of interest)
     * @return the value of the calendar field that is next in the sequence
     */
    unsigned int find_next(char* bits, int max, unsigned int value, tm* calendar, int field, 
            int nextField, int* lower_orders) {
        int next_value = next_set_bit(bits, max, value);
        // roll over if needed
        if (next_value == -1) {
            add_to_field(calendar, nextField, 1);
            reset(calendar, field);
            next_value = next_set_bit(bits, max, 0);
        }
        if (-1 == next_value || static_cast<unsigned int>(next_value) != value) {
            set_field(calendar, field, next_value);
            reset(calendar, lower_orders);
        }
        return next_value;
    }

    /**
     * Reset the calendar setting all the fields provided to zero.
     */
    void reset(tm* calendar, int field) {
        switch (field) {
        case CF_SECOND: calendar->tm_sec = 0; break;
        case CF_MINUTE: calendar->tm_min = 0; break;
        case CF_HOUR_OF_DAY: calendar->tm_hour = 0; break;
        case CF_DAY_OF_WEEK: calendar->tm_wday = 0; break;
        case CF_DAY_OF_MONTH: calendar->tm_mday = 1; break;
        case CF_MONTH: calendar->tm_mon = 0; break;
        case CF_YEAR: calendar->tm_year = 0; break;
        }
        mkgmtime(calendar);
    }
    
    void reset(tm* calendar, int* fields) {
        for (int i = 0; i < CF_ARR_LEN; i++) {
            if (-1 != fields[i]) {
                reset(calendar, fields[i]);
            }
        }
    }
    
    void add_to_field(tm* calendar, int field, int val) {
        switch (field) {
        case CF_SECOND: calendar->tm_sec = calendar->tm_sec + val; break;
        case CF_MINUTE: calendar->tm_min = calendar->tm_min + val; break;
        case CF_HOUR_OF_DAY: calendar->tm_hour = calendar->tm_hour + val; break;
        case CF_DAY_OF_WEEK: // mkgmtime ignores this field
        case CF_DAY_OF_MONTH: calendar->tm_mday = calendar->tm_mday + val; break;
        case CF_MONTH: calendar->tm_mon = calendar->tm_mon + val; break;
        case CF_YEAR: calendar->tm_year = calendar->tm_year + val; break;
        }
        mkgmtime(calendar);
    }

    void set_field(tm* calendar, int field, int val) {
        switch (field) {
        case CF_SECOND: calendar->tm_sec = val; break;
        case CF_MINUTE: calendar->tm_min = val; break;
        case CF_HOUR_OF_DAY: calendar->tm_hour = val; break;
        case CF_DAY_OF_WEEK: calendar->tm_wday = val; break;
        case CF_DAY_OF_MONTH: calendar->tm_mday = val; break;
        case CF_MONTH: calendar->tm_mon = val; break;
        case CF_YEAR: calendar->tm_year = val; break;
        }
        mkgmtime(calendar);
    }

    int next_set_bit(char* bits, int max, unsigned int from_index) {
        for (int i = from_index; i < max; i++) {
            if (bits[i]) return i;
        }
        return -1;
    }
};

int crons_equal(CronExprBitsets* cr1, CronExprBitsets* cr2);

#endif	/* CRONEXPRBITSETS_HPP */

