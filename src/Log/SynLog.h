#ifndef __SRC_LOG_SYNLOG_H_
#define __SRC_LOG_SYNLOG_H_

#include "FileWriter.h"
#include "LogConfig.h"
#include "LogFile.h"
#include "../Util/noncopyable.h"
#include <cstddef>
#include <mutex>
#include <string>

// extern LogConfig dLogConfig;

class SynLog : noncopyable {
private:
    const std::string &_basename;
    const size_t _rollSize;
    const int _flushInterval;
    FileWriterType _fileWriterType;
    time_t prevSecond;
    std::mutex _mtx;
    LogFile fileWriter;

public:
    SynLog(const std::string &basename = dLogConfig.logFileOptions.baseName,
           size_t rollSize = dLogConfig.logFileOptions.rollSize,
           int flushInterval = dLogConfig.logFileOptions.flushInterval,
           FileWriterType fileWriterType = dLogConfig.logFileOptions.fileWriterType) :
        _basename(basename),
        _rollSize(rollSize), _flushInterval(flushInterval), _fileWriterType(fileWriterType), fileWriter(_basename, _rollSize, _fileWriterType) {
        prevSecond = static_cast<time_t>(0);
    };

    ~SynLog() {
        fileWriter.flush();
    }

    void append(const char *msg, size_t len, size_t kenLen = 0);
};

#endif /* __SRC_LOG_SYNLOG_H_ */
