#include "utils/time16.h"
#include <time.h>

void get_fat16_time_date(uint16_t *res_time, uint16_t *date) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);

    uint16_t hour = (uint16_t)tm_info->tm_hour;
    uint16_t minute = (uint16_t)tm_info->tm_min;
    uint16_t second = (uint16_t)tm_info->tm_sec >> 1;
    
    *res_time = (hour << 11) | (minute << 5) | second;

    uint16_t year = (uint16_t)tm_info->tm_year - 80;
    uint16_t month = (uint16_t)tm_info->tm_mon + 1;
    uint16_t day = (uint16_t)tm_info->tm_mday;

    *date = (year << 9) | (month << 5) | day;
}
