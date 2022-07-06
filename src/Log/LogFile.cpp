#include "Log/LogFile.h"
#include "Log/FileWriter.h"
#include "Util/ThreadInfo.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <string>
#include <unistd.h> /* gethostname */

LogFile::LogFile(const std::string &basename, std::size_t rollSize, FileWriterType fileWriterType) :
    _basename(basename), _rollSize(rollSize), _fileWriterType(fileWriterType) {
    rollFile();
}

// 仅用于日志后台线程，因此无需加锁
void LogFile::append(const char *msg, size_t len) {
    _file->append(msg, len);
    if (_file->writtenBytes() > _rollSize) {
        rollFile();
    }
}

void LogFile::flush() {
    _file->flush();
}

std::string getHostName() {
    char buff[256];
    if (gethostname(buff, sizeof buff) == 0) {
        buff[sizeof buff - 1] = '\0';
        return buff;
    } else {
        return "UNKNOWN HOST";
    }
}

std::string LogFile::getLogFileName(const std::string &basename) {
    std::string filename;
    filename += basename;

    char timeStr[32];
    struct tm tm_time;
    time_t now = 0;
    time(&now);
    localtime_r(&now, &tm_time);
    strftime(timeStr, sizeof timeStr, "-%Y%m%d-%H%M%S.", &tm_time);
    filename += timeStr;

    filename += getHostName();
    filename += '.' + ThreadInfo::getPidStr();

    filename += ".log";
    return filename;
}

void LogFile::rollFile() {
    std::string newFileName = getLogFileName(_basename);
    if (_fileWriterType == FileWriter_NORMAL) {
        _file = std::make_unique<NormalFileWriter>(newFileName);
    } else {
        fprintf(stderr, "Unknow FileWriterType\n");
        abort();
    }
}