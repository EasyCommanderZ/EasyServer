#ifndef __SRC_LOG_LOGCONFIG_H_
#define __SRC_LOG_LOGCONFIG_H_

#include "Log/FileWriter.h"
#include <cstddef>
#include <string>

// 日志的配置类
class LogConfig {
public:
    enum LogLevel {
        TRACE = 0,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        numOfLevels,
    };

    LogConfig::LogLevel logLevel = LogLevel::INFO;

    struct LogFileOptions {
        std::string baseName = "MUST_NOT_NULL";
        // 日志滚动大小和 flush 间隔，用于AsynLog
        size_t rollSize = static_cast<size_t>(50 * 1024 * 1024); // default 50Mb
        int flushInterval = 3;
        int bufferNums = 4; // 默认buffer个数
        int maxBuffToWrite = 16;

        FileWriterType fileWriterType = FileWriter_NORMAL;
    };

    LogFileOptions logFileOptions;
};

extern LogConfig dLogConfig;

#endif /* __SRC_LOG_LOGCONFIG_H_ */
