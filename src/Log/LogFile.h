#ifndef __SRC_LOG_LOGFILE_H_
#define __SRC_LOG_LOGFILE_H_

#include "FileWriter.h"
#include "../Util/noncopyable.h"
#include <cstddef>
#include <memory>
#include <string>

// 日志文件名格式 ： basename - time . hostname . PID . log 
class LogFile : noncopyable {
private:
    const std::string _basename;
    const std::size_t _rollSize;
    FileWriterType _fileWriterType;

    std::unique_ptr<FileWriter> _file; // 日志文件的独占指针

public:
    LogFile(const std::string &basename, size_t rollsize, FileWriterType fileWriterType);
    ~LogFile() = default;

    void append(const char *msg, size_t len);
    void flush();
    void rollFile();

    // 获取日志文件名称
    static std::string getLogFileName(const std::string &basename);
};

#endif /* __SRC_LOG_LOGFILE_H_ */
