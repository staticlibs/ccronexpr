#ifndef _CCRONEXPR_INTERNAL_H
#define _CCRONEXPR_INTERNAL_H

#define CRON_MAX_SECONDS 60
#define CRON_MAX_MINUTES 60
#define CRON_MAX_HOURS 24
#define CRON_MAX_DAYS_OF_WEEK 8
#define CRON_MAX_DAYS_OF_MONTH 32
#define CRON_MAX_MONTHS 12
#define CRON_MAX_YEARS_DIFF 4

#define CRON_CF_SECOND 0
#define CRON_CF_MINUTE 1
#define CRON_CF_HOUR_OF_DAY 2
#define CRON_CF_DAY_OF_WEEK 3
#define CRON_CF_DAY_OF_MONTH 4
#define CRON_CF_MONTH 5
#define CRON_CF_YEAR 6

#define CRON_CF_ARR_LEN 7

#define CRON_INVALID_INSTANT ((time_t) -1)

#define CRON_MAX_STR_LEN_TO_SPLIT 256
#define CRON_MAX_NUM_TO_SRING 1000000000
/* computes number of digits in decimal number */
#define CRON_NUM_OF_DIGITS(num) (abs(num) < 10 ? 1 : \
                                (abs(num) < 100 ? 2 : \
                                (abs(num) < 1000 ? 3 : \
                                (abs(num) < 10000 ? 4 : \
                                (abs(num) < 100000 ? 5 : \
                                (abs(num) < 1000000 ? 6 : \
                                (abs(num) < 10000000 ? 7 : \
                                (abs(num) < 100000000 ? 8 : \
                                (abs(num) < 1000000000 ? 9 : 10)))))))))

#ifndef _WIN32
struct tm *gmtime_r(const time_t *timep, struct tm *result);
struct tm *localtime_r(const time_t *timep, struct tm *result);
#endif

#ifndef CRON_TEST_MALLOC
#define cronFree(x) free(x);
#define cronMalloc(x) malloc(x);
#else
void* cronMalloc(size_t n);
void cronFree(void* p);
#endif

void free_splitted(char** splitted, size_t len);
char* strdupl(const char* str, size_t len);
void push_to_fields_arr(int* arr, int fi);
int add_to_field(struct tm* calendar, int field, int val);
int to_upper(char* str);
char* to_string(int num);
char* str_replace(char *orig, const char *rep, const char *with);
unsigned int parse_uint(const char* str, int* errcode);
char** split_str(const char* str, char del, size_t* len_out);
char* replace_ordinals(char* value, const char* const * arr, size_t arr_len);
int has_char(char* str, char ch);
unsigned int* get_range(char* field, unsigned int min, unsigned int max, const char** error);
void set_months(char* value, uint8_t* targ, const char** error);
void set_days(char* field, uint8_t* targ, int max, const char** error);
void set_days_of_month(char* field, uint8_t* targ, const char** error);
int set_field(struct tm* calendar, int field, int val);

uint8_t cron_get_bit(uint8_t* rbyte, int idx);
time_t cron_mktime(struct tm* tm);
struct tm* cron_time(time_t* date, struct tm* out);

#endif

