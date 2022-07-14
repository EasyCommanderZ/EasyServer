#include "Logger.h"
#include "LogBuffer.h"
#include "LogConfig.h"
#include "SynLog.h"
#include "../Util/Timestamp.h"
#include "../Thread/ThreadInfo.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <ostream>
#include <sstream>
#include <thread>
#if defined(__linux__) || defined(__linux)
#include <cstdarg>
#endif //__linux__

LogConfig dLogConfig;

// 单例变量定义
Logger *Logger::_logger = nullptr;
std::mutex Logger::_mtx;

Logger::GC gcVariable;

// 线程局部变量，对日期和时间进行缓存，每个线程拥有独立的缓存
thread_local time_t prevSecond;
thread_local char timeStr[64];
thread_local Logger::Buffer buffer;

Logger *Logger::getInstance() {
    // 双锁实现单例模式
    if (_logger == nullptr) {
        std::unique_lock<std::mutex> lck(_mtx);
        if (_logger == nullptr) {
            _logger = new Logger;
        }
    }
    return _logger;
}

// 默认日志等级 INFO
inline LogConfig::LogLevel defaultLogLevel() {
    return dLogConfig.logLevel;
}

const char *LogLevelStr[LogConfig::LogLevel::numOfLevels] = {
    "TRACE",
    "DEBUG",
    "INFO ",
    "WARN ",
    "ERROR",
    "FATAL",
};

// 默认输出路径为 stdout
inline void defaultOuput(const char *msg, size_t len, size_t keyLen = 0) {
    size_t n = fwrite(msg, 1, len, stdout);
    (void)n;
}

inline void defaultFlush() {
    fflush(stdout);
}

LogConfig::LogLevel global_loglevel = defaultLogLevel();
Logger::outputFunc global_outputFunc = defaultOuput;
Logger::flushFunc global_flushFunc = defaultFlush;

void Logger::setOutputFunc(Logger::outputFunc outFunc) {
    global_outputFunc = std::move(outFunc);
}

void Logger::setFlushFunc(Logger::flushFunc fluFunc) {
    global_flushFunc = std::move(fluFunc);
}

LogConfig::LogLevel Logger::getLogLevel() {
    return dLogConfig.logLevel;
}

void Logger::setConfig(const LogConfig &config) {
    dLogConfig = config;
}

inline void Logger::formatTime() {
    Timestamp current = Timestamp::now();
    time_t curSecond = current.getSeconds();
    int milliSec = static_cast<int>(current.getMilliSeconds() % Timestamp::msPerSecond);

    if (curSecond != prevSecond) {
        prevSecond = curSecond;
        struct tm tm_time;
        localtime_r(&curSecond, &tm_time);

        snprintf(timeStr, sizeof(timeStr), "%4d-%02d-%02d %02d:%02d:%02d-",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }
    // 写入行缓冲，精确到秒
    buffer.append(timeStr, 20);

    char msStr[6];
    snprintf(msStr, sizeof(msStr), "%03d ", milliSec);
    buffer.append(msStr, 4);
}

void Logger::append(const char *file, size_t fileLen, const char *line, size_t lineLen, const char *fmt, LogConfig::LogLevel level, ...) {
    formatTime();

    // 线程信息
    buffer.append("tid:", 4);

    // auto currentTID = std::this_thread::get_id();
    // std::ostringstream ss;
    // ss << currentTID;
    // std::string tidStr = ss.str();
    // size_t tidStrLen = static_cast<size_t>(tidStr.size());
    // buffer.append(ss.str().c_str(), tidStrLen);
    
    buffer.append(ThreadInfo::tidString(), ThreadInfo::tidStringLength());

    // sourcefile [line:NUM] - LogLevel : text
    buffer.append(" ", 1);
    buffer.append(file, fileLen);
    buffer.append("[Line:", 6);
    buffer.append(line, lineLen);
    buffer.append("] - [", 5);
    buffer.append(LogLevelStr[level], 5);
    buffer.append("]: ", 3);

    // size_t keyLen = buffer.size();
    /* 正文, 使用va_list、va_arg和va_end等宏来处理, 其原理是
     * cdelc_ 参数从右到左压栈, 第一个参数在栈顶 */
    va_list argPtr;
    va_start(argPtr, fmt);
    int n = vsnprintf(buffer.current(), buffer.avail(), fmt, argPtr);
    va_end(argPtr);
    buffer.addLen(static_cast<size_t>(n));

    /* append到 outPutfunc */
    /* used for BenchMark*/
    if(global_outputFunc == nullptr) {
        buffer.clear(); return ;
    } // for Log generation test;
    size_t keyLen = buffer.size();
    global_outputFunc(buffer.data(), buffer.size(), keyLen);
    assert(buffer.size() < static_cast<size_t>(LogBuffer::lineBufferSize));
    buffer.clear();
    if (level == LogConfig::LogLevel::FATAL) {
        global_flushFunc();
        abort();
    }
}