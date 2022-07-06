#ifndef __SRC_LOG_LOGGER_H_
#define __SRC_LOG_LOGGER_H_

/* compile-time 计算字符串长度
ref-link:
https://stackoverflow.com/questions/15858141/conveniently-declaring-compile-time-strings-in-c
*/
#include "Log/LogBuffer.h"
#include "Log/LogBuffer.h"
#include "Log/LogConfig.h"
#include "Util/noncopyable.h"
#include <cstddef>
#include <functional>
#include <mutex>

/* compile-time 计算字符串长度
ref-link: https://stackoverflow.com/questions/15858141/conveniently-declaring-compile-time-strings-in-c
*/
class str_const { // constexpr string
private:
    const char *p_;
    std::size_t sz_;

public:
    template <std::size_t N>
    constexpr str_const(const char (&a)[N]) :
        p_(a), sz_(N - 1) {
        const char *idx = strrchr(p_, '/'); // builtin function
        if (idx) {
            p_ = idx + 1;
            sz_ -= static_cast<size_t>(p_ - a);
        }
    }

    constexpr char operator[](std::size_t n) { // []
        return n < sz_ ? p_[n] : throw std::out_of_range("");
    }

    constexpr std::size_t size() {
        return sz_;
    }
    constexpr const char *data() {
        return p_;
    }
};

/* compile-time 将x转换为字符串 */
#define strify_(x) val_(x)
#define val_(x) #x

/* 编译期获取字符串长度宏定义 */
#define getStrLen_(x) str_const(x).size()
/* 编译期获取字符串的宏定义 */
#define getStr_(x) str_const(x).data()

class Logger : noncopyable {
public:
    // 日志行缓冲
    using Buffer = LogBuffer::FixedBuffer<LogBuffer::lineBufferSize>;

    using outputFunc = std::function<void(const char *, size_t, size_t)>;
    using flushFunc = std::function<void()>;

    // 日志输出 & flush 函数
    static void setOutputFunc(outputFunc);
    static void setFlushFunc(flushFunc);

    // 日志等级
    static LogConfig::LogLevel getLogLevel();
    static void setLogLevel(LogConfig::LogLevel level);

    // 获取 Logger 单例对象
    static Logger *getInstance();

    // 设置 LogConfig
    static void setConfig(const LogConfig &);

    // 设置日志时间
    static void formatTime();

    // 供 LOG 宏使用的接口，将一条日志记录到行缓冲区
    void append(const char *file, size_t fileLen, const char *line, size_t lineLen, const char *fmt, LogConfig::LogLevel level, ...);

    // 内部类，用户回收单例 Logger 资源
    struct GC {
        GC() = default;
        ~GC() {
            if (_logger) delete _logger;
        }
    };

private:
    static Logger *_logger;
    static std::mutex _mtx;
    Logger() = default;
    ~Logger() = default;

    static GC gcVariable;
};

/* LOG_* 相关的宏定义, 向用户提供接口 */
#define LOG_TRACE(fmt, args...)                                             \
    do {                                                                    \
        if (Logger::getLogLevel() <= LogConfig::TRACE) {                       \
            Logger::getInstance()->append(                                  \
                getStr_(__FILE__), getStrLen_(__FILE__), strify_(__LINE__), \
                getStrLen_(strify_(__LINE__)), fmt, LogConfig::TRACE, args);   \
        }                                                                   \
    } while (0)

#define LOG_DEBUG(fmt, args...)                                             \
    do {                                                                    \
        if (Logger::getLogLevel() <= LogConfig::DEBUG) {                       \
            Logger::getInstance()->append(                                  \
                getStr_(__FILE__), getStrLen_(__FILE__), strify_(__LINE__), \
                getStrLen_(strify_(__LINE__)), fmt, LogConfig::DEBUG, args);   \
        }                                                                   \
    } while (0)

#define LOG_INFO(fmt, args...)                                      \
    Logger::getInstance()->append(                                  \
        getStr_(__FILE__), getStrLen_(__FILE__), strify_(__LINE__), \
        getStrLen_(strify_(__LINE__)), fmt, LogConfig::INFO, args)
#define LOG_WARN(fmt, args...)                                      \
    Logger::getInstance()->append(                                  \
        getStr_(__FILE__), getStrLen_(__FILE__), strify_(__LINE__), \
        getStrLen_(strify_(__LINE__)), fmt, LogConfig::WARN, args)
#define LOG_ERROR(fmt, args...)                                     \
    Logger::getInstance()->append(                                  \
        getStr_(__FILE__), getStrLen_(__FILE__), strify_(__LINE__), \
        getStrLen_(strify_(__LINE__)), fmt, LogConfig::ERROR, args)
#define LOG_FATAL(fmt, args...)                                     \
    Logger::getInstance()->append(                                  \
        getStr_(__FILE__), getStrLen_(__FILE__), strify_(__LINE__), \
        getStrLen_(strify_(__LINE__)), fmt, LogConfig::FATAL, args)

#endif /* __SRC_LOG_LOGGER_H_ */
