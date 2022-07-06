#include "Timestamp.h"
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>
#include <sys/time.h>

// unix 时间戳
Timestamp Timestamp::now() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    std::int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * msPerSecond + tv.tv_usec / msPerSecond);
}

// format timestamp
std::string Timestamp::toString() {
    char buff[64];
    auto second = static_cast<time_t>(_milliSecondsSinceEpoch / msPerSecond);
    struct tm tm_time;              // 把时间信息保存在该结构体中
    localtime_r(&second, &tm_time); // localtime_r 可以保证线程安全
    // 时间戳格式： 年月日 时分秒 毫秒
    int millsecond = static_cast<int>(_milliSecondsSinceEpoch % msPerSecond);
    std::snprintf(buff, sizeof buff, "%4d%02d%02d %02d:%02d:%02d.%03d", tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                  tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, millsecond);

    return buff;
}